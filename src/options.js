/**
 * @file options.js
 * @brief cupano options page: manage and persist Gemini/OpenAI API keys.
 *
 * Call graph (top-down only):
 *   OptionsController
 *     ├─ SettingsStore       browser.storage.local <-> CupanoSettings
 *     ├─ KeyFormatChecker    CupanoSettings -> ValidationReport
 *     ├─ ModelCatalogClient  fetch() provider model lists -> ModelCatalog
 *     │    ├─ GeminiModelSource
 *     │    └─ OpenAiModelSource
 *     └─ OptionsFormView     DOM <-> CupanoSettings, status rendering
 *
 * Loaded as an ES module, so nothing here is attached to the global scope.
 * Storage schema and model listing live in src/shared/ and are reused by the
 * in-page settings panel.
 */

import { CupanoSettings, SettingsStore } from './shared/settings.js';
import {
  GeminiModelSource, ModelCatalog, ModelCatalogClient, ModelCatalogRequest, OpenAiModelSource,
} from './shared/model-catalog.js';

// ============================================================================
// DTOs
// ============================================================================

/** A message shown next to the action buttons. */
class StatusNotice {
  constructor({ text, tone }) {
    this.text = String(text ?? '');
    this.tone = tone === 'warn' ? 'warn' : 'ok';
    Object.freeze(this);
  }
}

/** Outcome of the lightweight key-format check. */
class ValidationReport {
  constructor({ warnings }) {
    this.warnings = Object.freeze([...(warnings ?? [])]);
    Object.freeze(this);
  }
}

// ============================================================================
// Services
// ============================================================================

/** Flags keys that do not look like the provider's documented format. */
class KeyFormatChecker {
  static #PATTERNS = Object.freeze({
    gemini: { label: 'Gemini', pattern: /^AIza[0-9A-Za-z_-]{20,}$/ },
    openai: { label: 'OpenAI', pattern: /^sk-[0-9A-Za-z_-]{20,}$/ },
  });

  check(settings) {
    const warnings = [];
    for (const [provider, rule] of Object.entries(KeyFormatChecker.#PATTERNS)) {
      const key = settings.providers[provider].apiKey;
      if (key && !rule.pattern.test(key)) warnings.push(`${rule.label} key format looks unusual`);
    }
    if (!settings.providers[settings.activeProvider].apiKey) {
      warnings.push('the active provider has no key');
    }
    return new ValidationReport({ warnings });
  }
}

/** Owns every DOM interaction on the options page. */
class OptionsFormView {
  #form;
  #status;

  constructor(documentRef) {
    this.#form = documentRef.getElementById('settings-form');
    this.#status = documentRef.getElementById('status');
  }

  /** Returns the form element so the controller can subscribe to its events. */
  get form() {
    return this.#form;
  }

  get clearButton() {
    return this.#form.querySelector('#clear-button');
  }

  get modelRefreshButtons() {
    return [...this.#form.querySelectorAll('button.refresh-models')];
  }

  apiKeyInput(provider) {
    return this.#form.elements.namedItem(`${provider}.apiKey`);
  }

  readModelCatalogRequest(provider) {
    return new ModelCatalogRequest({ provider, apiKey: this.apiKeyInput(provider).value });
  }

  /** Fills the datalist; the input keeps its placeholder and still accepts free text. */
  renderModelCatalog(catalog) {
    const list = this.#form.ownerDocument.getElementById(`${catalog.provider}-models`);
    list.replaceChildren(...catalog.models.map((model) => {
      const option = this.#form.ownerDocument.createElement('option');
      option.value = model.id;
      if (model.label && model.label !== model.id) option.label = model.label;
      return option;
    }));
  }

  setModelLoading(provider, loading) {
    const button = this.#form.querySelector(`button.refresh-models[data-provider="${provider}"]`);
    button.disabled = loading;
    button.textContent = loading ? 'Loading…' : 'Load models';
  }

  showModelStatus(provider, notice) {
    const status = this.#form.querySelector(`.model-status[data-provider="${provider}"]`);
    status.textContent = notice.text;
    status.className = `model-status ${notice.text ? notice.tone : ''}`.trim();
  }

  read() {
    const data = new FormData(this.#form);
    return new CupanoSettings({
      activeProvider: data.get('activeProvider'),
      providers: {
        gemini: { apiKey: data.get('gemini.apiKey'), model: data.get('gemini.model') },
        openai: { apiKey: data.get('openai.apiKey'), model: data.get('openai.model') },
      },
    });
  }

  write(settings) {
    for (const radio of this.#form.elements.namedItem('activeProvider')) {
      radio.checked = radio.value === settings.activeProvider;
    }
    for (const provider of CupanoSettings.PROVIDERS) {
      this.#form.elements.namedItem(`${provider}.apiKey`).value = settings.providers[provider].apiKey;
      this.#form.elements.namedItem(`${provider}.model`).value = settings.providers[provider].model;
    }
    this.highlightActiveProvider();
  }

  highlightActiveProvider() {
    const active = new FormData(this.#form).get('activeProvider');
    for (const card of this.#form.querySelectorAll('.card[data-provider]')) {
      card.classList.toggle('inactive', card.dataset.provider !== active);
    }
  }

  /** Self-contained UI behaviour: toggles masking on the key inputs. */
  bindRevealButtons() {
    for (const button of this.#form.querySelectorAll('button.reveal')) {
      button.addEventListener('click', () => {
        const input = this.#form.querySelector(`#${button.dataset.target}`);
        const reveal = input.type === 'password';
        input.type = reveal ? 'text' : 'password';
        button.textContent = reveal ? 'Hide' : 'Show';
        button.setAttribute('aria-pressed', String(reveal));
      });
    }
  }

  showStatus(notice) {
    this.#status.textContent = notice.text;
    this.#status.className = notice.tone;
  }
}

// ============================================================================
// Controller
// ============================================================================

/** Wires the view to the store. Sole owner of the page's event handlers. */
class OptionsController {
  #store;
  #checker;
  #catalogClient;
  #view;
  #modelRequestSerial = new Map();

  constructor({ store, checker, catalogClient, view }) {
    this.#store = store;
    this.#checker = checker;
    this.#catalogClient = catalogClient;
    this.#view = view;
  }

  async start() {
    this.#view.bindRevealButtons();
    this.#view.form.addEventListener('submit', (event) => {
      event.preventDefault();
      this.#save();
    });
    this.#view.form.addEventListener('change', () => this.#view.highlightActiveProvider());
    this.#view.clearButton.addEventListener('click', () => this.#clear());
    for (const button of this.#view.modelRefreshButtons) {
      button.addEventListener('click', () => this.#refreshModels(button.dataset.provider));
    }
    for (const provider of CupanoSettings.PROVIDERS) {
      this.#view.apiKeyInput(provider).addEventListener('change', () => this.#refreshModels(provider));
    }
    await this.#load();
    for (const provider of CupanoSettings.PROVIDERS) this.#refreshModels(provider);
  }

  async #refreshModels(provider) {
    const request = this.#view.readModelCatalogRequest(provider);
    // A newer request (e.g. the key was edited again) supersedes this one.
    const serial = (this.#modelRequestSerial.get(provider) ?? 0) + 1;
    this.#modelRequestSerial.set(provider, serial);
    if (!request.apiKey) {
      this.#view.renderModelCatalog(new ModelCatalog({ provider, models: [] }));
      this.#view.showModelStatus(provider, new StatusNotice({ text: '', tone: 'ok' }));
      return;
    }
    this.#view.setModelLoading(provider, true);
    try {
      const catalog = await this.#catalogClient.list(request);
      if (this.#modelRequestSerial.get(provider) !== serial) return;
      this.#view.renderModelCatalog(catalog);
      this.#view.showModelStatus(provider, new StatusNotice({
        text: `${catalog.models.length} models found. Pick one in the Model field.`,
        tone: 'ok',
      }));
    } catch (error) {
      if (this.#modelRequestSerial.get(provider) !== serial) return;
      this.#view.showModelStatus(provider, new StatusNotice({
        text: `Could not load models: ${error.message}`,
        tone: 'warn',
      }));
    } finally {
      if (this.#modelRequestSerial.get(provider) === serial) this.#view.setModelLoading(provider, false);
    }
  }

  async #load() {
    try {
      this.#view.write(await this.#store.load());
    } catch (error) {
      this.#view.showStatus(new StatusNotice({ text: `Could not load settings: ${error.message}`, tone: 'warn' }));
    }
  }

  async #save() {
    try {
      const settings = this.#view.read();
      await this.#store.save(settings);
      const report = this.#checker.check(settings);
      this.#view.showStatus(report.warnings.length
        ? new StatusNotice({ text: `Saved, but ${report.warnings.join('; ')}.`, tone: 'warn' })
        : new StatusNotice({ text: 'Saved.', tone: 'ok' }));
    } catch (error) {
      this.#view.showStatus(new StatusNotice({ text: `Save failed: ${error.message}`, tone: 'warn' }));
    }
  }

  async #clear() {
    try {
      await this.#store.clear();
      this.#view.write(new CupanoSettings({}));
      for (const provider of CupanoSettings.PROVIDERS) this.#refreshModels(provider);
      this.#view.showStatus(new StatusNotice({ text: 'All keys removed.', tone: 'ok' }));
    } catch (error) {
      this.#view.showStatus(new StatusNotice({ text: `Clear failed: ${error.message}`, tone: 'warn' }));
    }
  }
}

// ============================================================================
// Composition root
// ============================================================================

new OptionsController({
  store: new SettingsStore(browser.storage.local),
  checker: new KeyFormatChecker(),
  catalogClient: new ModelCatalogClient(globalThis.fetch.bind(globalThis), [
    new GeminiModelSource(),
    new OpenAiModelSource(),
  ]),
  view: new OptionsFormView(document),
}).start();
