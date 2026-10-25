/**
 * @file background.js
 * @brief Composition root of the background context (Chrome service worker,
 *        Firefox event page). Owns the Wasm core and the swarm request so an
 *        analysis keeps running after the popup closes.
 *
 * Call graph (top-down, acyclic):
 *
 *   runtime.onMessage ─ AnalysisService
 *                         ├─ PageCapture        scripting.executeScript on the active tab
 *                         ├─ CupanoEngine       Wasm NormalizerFacade (+ WasmMarshaller)
 *                         ├─ AiClient           Gemini / OpenAI transports
 *                         ├─ SettingsStore, PreferencesStore, CountryResolver
 *                         ├─ TabStateStore      storage.session, observed by the popup
 *                         ├─ BadgePresenter
 *                         └─ KeepAlive
 */
import '../../vendor/browser-polyfill.min.js';
import createCupanoCore from '../../wasm/cupano_core.js';
import { AnalyzeTabCommand, PromptOptions } from '../shared/dto.js';
import { SettingsStore, PreferencesStore } from '../shared/settings.js';
import { CountryCatalog, CountryResolver, GeoCache, GeoIpLocator } from '../shared/geo.js';
import { TabStateStore } from '../shared/tab-state-store.js';
import { PageCapture } from './page-capture.js';
import { CupanoEngine, WasmMarshaller } from './engine.js';
import { AiClient, GeminiTransport, OpenAiTransport } from './ai-client.js';
import { BadgePresenter, KeepAlive } from './run-support.js';
import { AnalysisService } from './analysis-service.js';

function buildService(api) {
  const fetchFn = globalThis.fetch.bind(globalThis);
  const local = api.storage.local;
  const locale = globalThis.navigator?.language ?? 'en';
  return new AnalysisService({
    capture: new PageCapture(api.scripting),
    engine: new CupanoEngine(() => createCupanoCore(), new WasmMarshaller()),
    aiClient: new AiClient(fetchFn, [new GeminiTransport(), new OpenAiTransport()]),
    settingsStore: new SettingsStore(local),
    preferencesStore: new PreferencesStore(local),
    countryResolver: new CountryResolver({
      locator: new GeoIpLocator(fetchFn),
      cache: new GeoCache(local, () => Date.now()),
      catalog: new CountryCatalog(locale),
    }),
    stateStore: new TabStateStore(api.storage.session),
    badge: new BadgePresenter(api.action),
    keepAlive: new KeepAlive(api.runtime, globalThis),
    promptOptions: new PromptOptions({ outputLocale: locale, notesPerAgent: 5, sourcesPerAgent: 3 }),
    now: () => Date.now(),
  });
}

const service = buildService(globalThis.browser);

// Listeners are registered synchronously at top level, as MV3 requires.
globalThis.browser.runtime.onMessage.addListener((message, sender) => {
  // Only the extension's own pages (the popup) may start an analysis.
  const ownPage = sender?.id === globalThis.browser.runtime.id
    && String(sender?.url ?? '').startsWith(globalThis.browser.runtime.getURL(''));
  if (!ownPage) return undefined;
  if (!AnalyzeTabCommand.isCommand(message)) return undefined;
  const command = new AnalyzeTabCommand(message);
  service.analyze(command);
  return Promise.resolve({ accepted: true });
});

globalThis.browser.tabs.onRemoved.addListener((tabId) => { service.forget(tabId); });
