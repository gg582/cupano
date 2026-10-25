/**
 * @file popup.js
 * @brief Composition root of the toolbar popup.
 *
 *   PopupController
 *     ├─ PopupShell          top bar + screen area (static markup)
 *     ├─ HomeView            Analyze orb, swarm progress, result
 *     │    └─ ProfileView
 *     │         └─ FlavorWheelView  SVG sunburst of the wheel frequency analysis
 *     ├─ SettingsController  ⚙ screen
 *     │    └─ SettingsView
 *     ├─ TabStateStore       read + observe what the background writes
 *     └─ Translator          UI language (shared by every view, switched by the controllers)
 */
import { CupanoSettings, PreferencesStore, SettingsStore, UserPreferences } from '../shared/settings.js';
import { CountryCatalog, CountryCode, CountryResolver, GeoCache, GeoIpLocator } from '../shared/geo.js';
import {
  GeminiModelSource, ModelCatalogClient, ModelCatalogRequest, OpenAiModelSource,
} from '../shared/model-catalog.js';
import { TabStateStore } from '../shared/tab-state-store.js';
import { Translator } from '../shared/i18n.js';
import { CountryLabeler, DomFactory } from './dom.js';
import { ProfileView } from './profile-view.js';
import { FlavorWheelView } from './flavor-wheel-view.js';
import { SettingsView } from './settings-view.js';
import { HomeView } from './home-view.js';
import { PopupShell } from './popup-shell.js';
import { PopupController, SettingsController } from './controllers.js';

function buildPopup(api, documentRef) {
  const fetchFn = globalThis.fetch.bind(globalThis);
  const local = api.storage.local;
  const countryCatalog = new CountryCatalog(navigator.language);
  const settingsStore = new SettingsStore(local);
  const preferencesStore = new PreferencesStore(local);
  const countryResolver = new CountryResolver({
    locator: new GeoIpLocator(fetchFn),
    cache: new GeoCache(local, () => Date.now()),
    catalog: countryCatalog,
  });
  const dom = new DomFactory(documentRef);
  const translator = new Translator();
  const profileView = new ProfileView(dom, new CountryLabeler(countryCatalog, CountryCode),
    new FlavorWheelView(documentRef, dom, translator), translator);

  return new PopupController({
    api,
    settingsStore,
    preferencesStore,
    countryResolver,
    countryCatalog,
    stateStore: new TabStateStore(api.storage.session),
    shell: new PopupShell(documentRef),
    homeView: new HomeView(dom, profileView, globalThis, translator),
    settingsController: new SettingsController({
      settingsStore,
      preferencesStore,
      countryResolver,
      countryCatalog,
      catalogClient: new ModelCatalogClient(fetchFn, [new GeminiModelSource(), new OpenAiModelSource()]),
      view: new SettingsView(dom, translator),
      types: { CupanoSettings, UserPreferences, ModelCatalogRequest },
      translator,
      browserLanguage: navigator.language,
    }),
    translator,
    browserLanguage: navigator.language,
    now: () => Date.now(),
  });
}

buildPopup(globalThis.browser, document).start()
  .catch((error) => console.error('[cupano] popup failed to start:', error));
