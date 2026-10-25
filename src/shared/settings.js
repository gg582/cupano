/**
 * @file settings.js
 * @brief Persisted settings shared by the options page and the in-page panel.
 *
 * Two storage records live in browser.storage.local:
 *   cupanoSettings     AI provider, API keys and models
 *   cupanoPreferences  country pinning, preferred processes, taste prompt
 *
 * Pure ES module: exports classes only, holds no module-level state.
 */

/** Credentials resolved for one request (model already defaulted). */
export class AiCredentials {
  constructor({ provider, apiKey, model }) {
    this.provider = String(provider ?? '');
    this.apiKey = String(apiKey ?? '');
    this.model = String(model ?? '');
    Object.freeze(this);
  }

  get isConfigured() {
    return this.apiKey.length > 0 && this.model.length > 0;
  }
}

/** Stored key and model for a single provider. */
export class ProviderSettings {
  constructor({ provider, apiKey, model }) {
    this.provider = String(provider ?? '');
    this.apiKey = String(apiKey ?? '').trim();
    this.model = String(model ?? '').trim();
    Object.freeze(this);
  }
}

/** Complete AI settings document. */
export class CupanoSettings {
  static PROVIDERS = Object.freeze(['gemini', 'openai']);
  static DEFAULT_MODELS = Object.freeze({ gemini: 'gemini-2.5-flash', openai: 'gpt-5-mini' });

  constructor({ activeProvider, providers }) {
    this.activeProvider = CupanoSettings.PROVIDERS.includes(activeProvider) ? activeProvider : 'gemini';
    this.providers = Object.freeze(Object.fromEntries(CupanoSettings.PROVIDERS.map((provider) => [
      provider,
      new ProviderSettings({ ...(providers?.[provider] ?? {}), provider }),
    ])));
    Object.freeze(this);
  }

  /** Credentials for the active provider, falling back to its default model. */
  get activeCredentials() {
    const entry = this.providers[this.activeProvider];
    return new AiCredentials({
      provider: this.activeProvider,
      apiKey: entry.apiKey,
      model: entry.model || CupanoSettings.DEFAULT_MODELS[this.activeProvider],
    });
  }

  /** Plain, structured-clone-safe representation for storage. */
  toStorageRecord() {
    return {
      activeProvider: this.activeProvider,
      providers: Object.fromEntries(CupanoSettings.PROVIDERS.map((provider) => [
        provider,
        { apiKey: this.providers[provider].apiKey, model: this.providers[provider].model },
      ])),
    };
  }
}

/** User-level preferences: language, country handling and taste profile. */
export class UserPreferences {
  static PROCESS_OPTIONS = Object.freeze([
    'Washed', 'Natural', 'Honey', 'Anaerobic', 'Carbonic Maceration', 'Wet-hulled', 'Co-fermented',
  ]);
  static MAX_TASTE_LENGTH = 600;
  static LANGUAGES = Object.freeze(['auto', 'ko', 'en', 'ja', 'zh-TW', 'nb', 'de']);

  constructor({ language, countryMode, pinnedCountry, preferredProcesses, tastePreference }) {
    this.language = UserPreferences.LANGUAGES.includes(language) ? language : 'auto';
    this.countryMode = countryMode === 'pinned' ? 'pinned' : 'auto';
    const code = String(pinnedCountry ?? '').trim().toUpperCase();
    this.pinnedCountry = /^[A-Z]{2}$/.test(code) ? code : '';
    this.preferredProcesses = Object.freeze(
      (preferredProcesses ?? []).filter((process) => UserPreferences.PROCESS_OPTIONS.includes(process)),
    );
    this.tastePreference = String(tastePreference ?? '').trim().slice(0, UserPreferences.MAX_TASTE_LENGTH);
    Object.freeze(this);
  }

  get isCountryPinned() {
    return this.countryMode === 'pinned' && this.pinnedCountry.length === 2;
  }

  toStorageRecord() {
    return {
      language: this.language,
      countryMode: this.countryMode,
      pinnedCountry: this.pinnedCountry,
      preferredProcesses: [...this.preferredProcesses],
      tastePreference: this.tastePreference,
    };
  }
}

/** Reads and writes CupanoSettings. */
export class SettingsStore {
  static #STORAGE_KEY = 'cupanoSettings';

  #storageArea;

  constructor(storageArea) {
    this.#storageArea = storageArea;
  }

  async load() {
    const stored = await this.#storageArea.get(SettingsStore.#STORAGE_KEY);
    return new CupanoSettings(stored?.[SettingsStore.#STORAGE_KEY] ?? {});
  }

  async save(settings) {
    await this.#storageArea.set({ [SettingsStore.#STORAGE_KEY]: settings.toStorageRecord() });
  }

  async clear() {
    await this.#storageArea.remove(SettingsStore.#STORAGE_KEY);
  }
}

/** Reads and writes UserPreferences. */
export class PreferencesStore {
  static #STORAGE_KEY = 'cupanoPreferences';

  #storageArea;

  constructor(storageArea) {
    this.#storageArea = storageArea;
  }

  async load() {
    const stored = await this.#storageArea.get(PreferencesStore.#STORAGE_KEY);
    return new UserPreferences(stored?.[PreferencesStore.#STORAGE_KEY] ?? {});
  }

  async save(preferences) {
    await this.#storageArea.set({ [PreferencesStore.#STORAGE_KEY]: preferences.toStorageRecord() });
  }
}
