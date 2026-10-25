/**
 * @file controllers.js
 * @brief Popup controllers. PopupController is the root; it owns the home
 *        screen and hands the screen area to SettingsController on ⚙.
 */
import { AnalyzeTabCommand } from '../shared/dto.js';
import { LANGUAGE_OPTIONS, resolveLanguage } from '../shared/i18n.js';
import { TabStateStore } from '../shared/tab-state-store.js';
import { HomeView, HomeViewModel, TabInfo } from './home-view.js';
import { SettingsNotice, SettingsViewModel } from './settings-view.js';

/** Renders, reads and persists the settings screen. */
export class SettingsController {
  #settingsStore;
  #preferencesStore;
  #countryResolver;
  #countryCatalog;
  #catalogClient;
  #view;
  #types;
  #i18n;
  #browserLanguage;
  #container = null;
  #modelsLoaded = false;
  #advancedOpen = false;

  constructor({ settingsStore, preferencesStore, countryResolver, countryCatalog, catalogClient, view, types,
    translator, browserLanguage }) {
    this.#settingsStore = settingsStore;
    this.#preferencesStore = preferencesStore;
    this.#countryResolver = countryResolver;
    this.#countryCatalog = countryCatalog;
    this.#catalogClient = catalogClient;
    this.#view = view;
    this.#types = types;
    this.#i18n = translator;
    this.#browserLanguage = browserLanguage;
  }

  async open(container, { expandAdvanced = false } = {}) {
    this.#container = container;
    this.#advancedOpen = false;
    const [settings, preferences] = await Promise.all([this.#settingsStore.load(), this.#preferencesStore.load()]);
    const { CupanoSettings, UserPreferences } = this.#types;
    const { nodes, handles } = this.#view.build(new SettingsViewModel({
      settings,
      preferences,
      countries: this.#countryCatalog.options(),
      processOptions: UserPreferences.PROCESS_OPTIONS,
      providers: CupanoSettings.PROVIDERS,
      defaultModels: CupanoSettings.DEFAULT_MODELS,
      languages: LANGUAGE_OPTIONS,
    }));
    container.replaceChildren(...nodes);
    this.#modelsLoaded = false;
    handles.saveButton.addEventListener('click', () => this.#save());
    handles.loadModelsButton.addEventListener('click', () => this.#loadModels());
    handles.advancedButton.addEventListener('click', () => {
      this.#advancedOpen = !this.#advancedOpen;
      if (!this.#modelsLoaded) this.#loadModels();
    });
    if (expandAdvanced) handles.advancedButton.click();
    // GeoIP only runs when the user has not pinned a country.
    if (!preferences.isCountryPinned) this.#view.showDetectedCountry(await this.#countryResolver.detect());
  }

  async #save() {
    const { CupanoSettings, UserPreferences } = this.#types;
    try {
      const values = this.#view.read();
      await this.#settingsStore.save(new CupanoSettings({
        activeProvider: values.activeProvider,
        providers: values.providers,
      }));
      const languageChanged = (await this.#preferencesStore.load()).language !== values.language;
      await this.#preferencesStore.save(new UserPreferences({
        language: values.language,
        countryMode: values.pinCountry ? 'pinned' : 'auto',
        pinnedCountry: values.pinnedCountry,
        preferredProcesses: values.preferredProcesses,
        tastePreference: values.tastePreference,
      }));
      if (languageChanged) {
        // Re-render this screen in the new language; the top bar follows on the way back.
        this.#i18n.use(resolveLanguage(values.language, this.#browserLanguage));
        await this.open(this.#container, { expandAdvanced: this.#advancedOpen });
      }
      this.#view.showNotice(new SettingsNotice({ text: this.#i18n.t('settings.saved'), tone: 'ok' }));
    } catch (error) {
      this.#view.showNotice(new SettingsNotice({
        text: this.#i18n.t('settings.saveFailed', { error: error.message }), tone: 'warn',
      }));
    }
  }

  async #loadModels() {
    const values = this.#view.read();
    const provider = values.activeProvider;
    const apiKey = values.providers[provider]?.apiKey ?? '';
    if (!apiKey.trim()) {
      this.#view.showModelStatus(this.#keyNotice(provider, this.#i18n.t('settings.enterKey')));
      return;
    }
    this.#view.setModelsLoading(true);
    try {
      const catalog = await this.#catalogClient.list(new this.#types.ModelCatalogRequest({ provider, apiKey }));
      this.#modelsLoaded = true;
      this.#view.renderModelOptions(catalog);
      this.#view.showModelStatus(new SettingsNotice({
        text: this.#i18n.t('settings.modelsLoaded', { count: catalog.models.length }),
        tone: 'ok',
      }));
    } catch (error) {
      // A failed model listing almost always means a missing or invalid key: point straight to where keys are issued.
      this.#view.showModelStatus(this.#keyNotice(provider, this.#i18n.t('settings.modelsFailed', { error: error.message })));
    } finally {
      this.#view.setModelsLoading(false);
    }
  }

  #keyNotice(provider, text) {
    const gemini = provider === 'gemini';
    return new SettingsNotice({
      text,
      tone: 'warn',
      linkUrl: gemini ? HomeView.GEMINI_KEY_URL : '',
      linkText: gemini ? this.#i18n.t('settings.getGeminiKey') : '',
    });
  }
}

/** Root of the popup: active tab, home screen, live state, settings switch. */
export class PopupController {
  static #PROVIDER_LABELS = Object.freeze({ gemini: 'Gemini', openai: 'OpenAI' });

  #api;
  #settingsStore;
  #preferencesStore;
  #countryResolver;
  #countryCatalog;
  #stateStore;
  #shell;
  #homeView;
  #settingsController;
  #i18n;
  #browserLanguage;
  #now;
  #handles = null;
  #tab = null;
  #state = null;
  #settings = null;
  #screen = 'home';

  constructor({ api, settingsStore, preferencesStore, countryResolver, countryCatalog, stateStore, shell, homeView,
    settingsController, translator, browserLanguage, now }) {
    this.#api = api;
    this.#settingsStore = settingsStore;
    this.#preferencesStore = preferencesStore;
    this.#countryResolver = countryResolver;
    this.#countryCatalog = countryCatalog;
    this.#stateStore = stateStore;
    this.#shell = shell;
    this.#homeView = homeView;
    this.#settingsController = settingsController;
    this.#i18n = translator;
    this.#browserLanguage = browserLanguage;
    this.#now = now;
  }

  async start() {
    this.#handles = this.#shell.mount();
    this.#handles.gearButton.addEventListener('click', () => this.#openSettings(false));
    this.#handles.countryChip.addEventListener('click', () => this.#openSettings(false));
    this.#handles.backButton.addEventListener('click', () => this.#showHome());

    const [tab] = await this.#api.tabs.query({ active: true, currentWindow: true });
    this.#tab = new TabInfo(tab ?? {});
    this.#api.storage.onChanged.addListener((changes, area) => this.#onStorageChanged(changes, area));
    await this.#showHome();
  }

  async #showHome() {
    this.#screen = 'home';
    this.#shell.showHomeBar();
    const [settings, preferences, state] = await Promise.all([
      this.#settingsStore.load(),
      this.#preferencesStore.load(),
      this.#tab.id ? this.#stateStore.read(this.#tab.id) : null,
    ]);
    this.#settings = settings;
    this.#state = this.#relevant(state);
    this.#i18n.use(resolveLanguage(preferences.language, this.#browserLanguage));
    this.#shell.applyLanguage(this.#i18n);
    this.#renderHome();
    this.#showCountry(preferences);
  }

  async #showCountry(preferences) {
    const resolved = await this.#countryResolver.resolve(preferences);
    const known = resolved.code ? this.#countryCatalog.option(resolved) : null;
    this.#shell.showCountry(known, preferences.isCountryPinned, this.#i18n);
  }

  async #openSettings(expandAdvanced) {
    this.#screen = 'settings';
    this.#shell.showSettingsBar();
    await this.#settingsController.open(this.#handles.screen, { expandAdvanced });
  }

  #renderHome() {
    const credentials = this.#settings.activeCredentials;
    const handles = this.#homeView.render(this.#handles.screen, new HomeViewModel({
      tab: this.#tab,
      state: this.#state,
      keyConfigured: credentials.isConfigured,
      provider: credentials.provider,
      providerLabel: PopupController.#PROVIDER_LABELS[credentials.provider] ?? credentials.provider,
      modelLabel: credentials.model,
      now: this.#now(),
    }));
    handles.analyzeButton?.addEventListener('click', () => this.#analyze(false));
    handles.forceButton?.addEventListener('click', () => this.#analyze(true));
    handles.keyButton?.addEventListener('click', () => this.#openSettings(true));
  }

  async #analyze(force) {
    if (!this.#tab.id) return;
    await this.#api.runtime.sendMessage(new AnalyzeTabCommand({ tabId: this.#tab.id, force }));
  }

  #onStorageChanged(changes, area) {
    if (area === 'local' && changes.cupanoPreferences) {
      // A saved language switches the top bar at once, whichever screen is open.
      const language = changes.cupanoPreferences.newValue?.language;
      this.#i18n.use(resolveLanguage(language, this.#browserLanguage));
      this.#shell.applyLanguage(this.#i18n);
      return;
    }
    if (area !== 'session' || !this.#tab?.id) return;
    const change = TabStateStore.changeFor(this.#tab.id, changes);
    if (!change.present) return;
    this.#state = this.#relevant(change.state);
    if (this.#screen === 'home') this.#renderHome();
  }

  /** Ignores state left over from another URL in the same tab, and marks dead runs. */
  #relevant(state) {
    if (!state) return null;
    if (state.url && state.url !== this.#tab.url) return null;
    if (!state.isInterrupted(this.#now())) return state;
    return new state.constructor({
      ...state.toRecord(),
      phase: 'error',
      status: { code: 'interrupted', title: 'Analysis interrupted' },
    });
  }
}
