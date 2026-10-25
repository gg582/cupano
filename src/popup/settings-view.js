/**
 * @file settings-view.js
 * @brief Settings screen: language and country pinning up top, and behind the
 *        orange Advanced button the AI provider/key/model plus taste preferences.
 *        Purely local UI behaviour (show/hide, enable/disable) stays inside;
 *        persistence and network work belong to the SettingsController.
 */

/** Everything the settings screen displays. */
export class SettingsViewModel {
  constructor({ settings, preferences, countries, processOptions, providers, defaultModels, languages }) {
    this.settings = settings;
    this.preferences = preferences;
    this.countries = Object.freeze([...(countries ?? [])]);
    this.processOptions = Object.freeze([...(processOptions ?? [])]);
    this.providers = Object.freeze([...(providers ?? [])]);
    this.defaultModels = Object.freeze({ ...defaultModels });
    this.languages = Object.freeze([...(languages ?? [])]);
    Object.freeze(this);
  }
}

/** Interactive elements of the settings screen the controller wires. */
export class SettingsHandles {
  constructor({ saveButton, advancedButton, loadModelsButton }) {
    this.saveButton = saveButton;
    this.advancedButton = advancedButton;
    this.loadModelsButton = loadModelsButton;
    Object.freeze(this);
  }
}

/** Raw values read back from the settings form. */
export class SettingsFormValues {
  constructor({ language, pinCountry, pinnedCountry, activeProvider, providers, preferredProcesses, tastePreference }) {
    this.language = String(language ?? 'auto');
    this.pinCountry = Boolean(pinCountry);
    this.pinnedCountry = String(pinnedCountry ?? '');
    this.activeProvider = String(activeProvider ?? '');
    this.providers = Object.freeze({ ...providers });
    this.preferredProcesses = Object.freeze([...(preferredProcesses ?? [])]);
    this.tastePreference = String(tastePreference ?? '');
    Object.freeze(this);
  }
}

/** Short inline notice inside the settings screen, optionally with a link to follow. */
export class SettingsNotice {
  constructor({ text, tone, linkUrl, linkText }) {
    this.text = String(text ?? '');
    this.tone = tone === 'warn' ? 'warn' : 'ok';
    this.linkUrl = String(linkUrl ?? '');
    this.linkText = String(linkText ?? '');
    Object.freeze(this);
  }
}

/** Builds and reads the settings form. */
export class SettingsView {
  #dom;
  #i18n;
  #refs = null;

  constructor(dom, translator) {
    this.#dom = dom;
    this.#i18n = translator;
  }

  build(viewModel) {
    const form = this.#dom.element('form', 'settings');
    form.addEventListener('submit', (event) => event.preventDefault());
    this.#refs = {};
    const language = this.#buildLanguageSection(viewModel);
    const country = this.#buildCountrySection(viewModel);
    const advanced = this.#buildAdvancedSection(viewModel);
    const footer = this.#buildFooter();
    form.append(language, country, advanced.section, footer.node);
    this.#refs = { ...this.#refs, advanced: advanced.section, notice: footer.notice };
    footer.advancedButton.addEventListener('click', () => this.#toggleAdvanced(footer.advancedButton));
    return {
      nodes: [form],
      handles: new SettingsHandles({
        saveButton: footer.saveButton,
        advancedButton: footer.advancedButton,
        loadModelsButton: advanced.loadModelsButton,
      }),
    };
  }

  read() {
    const refs = this.#refs;
    const activeProvider = refs.providerRadios.find((radio) => radio.checked)?.value ?? '';
    const providers = Object.fromEntries(Object.entries(refs.providerFields).map(([provider, fields]) => [
      provider, { apiKey: fields.key.value, model: fields.model.value },
    ]));
    return new SettingsFormValues({
      language: refs.languageSelect.value,
      pinCountry: refs.pinToggle.checked,
      pinnedCountry: refs.countrySelect.value,
      activeProvider,
      providers,
      preferredProcesses: refs.processBoxes.filter((box) => box.checked).map((box) => box.value),
      tastePreference: refs.taste.value,
    });
  }

  showDetectedCountry(resolved) {
    const refs = this.#refs;
    if (!refs) return;
    if (resolved.source === 'geoip') {
      refs.detected.textContent = this.#i18n.t('settings.geoipDetected', { name: resolved.name, code: resolved.code });
      if (!refs.pinToggle.checked) refs.countrySelect.value = resolved.code;
    } else {
      refs.detected.textContent = this.#i18n.t('settings.geoipFailed');
    }
  }

  renderModelOptions(catalog) {
    const list = this.#refs?.modelLists[catalog.provider];
    if (!list) return;
    list.replaceChildren(...catalog.models.map((model) => {
      const option = this.#dom.element('option');
      option.value = model.id;
      if (model.label && model.label !== model.id) option.label = model.label;
      return option;
    }));
  }

  setModelsLoading(loading) {
    const button = this.#refs?.loadModelsButton;
    if (!button) return;
    button.disabled = loading;
    button.textContent = this.#i18n.t(loading ? 'settings.loading' : 'settings.loadModels');
  }

  showModelStatus(notice) {
    if (!this.#refs) return;
    const status = this.#refs.modelStatus;
    status.className = `small ${notice.tone}`;
    status.replaceChildren(this.#dom.element('span', '', notice.text));
    if (notice.linkUrl) {
      const link = this.#dom.externalLink(notice.linkUrl, notice.linkText);
      link.className = 'key-link';
      status.append(link);
    }
  }

  showNotice(notice) {
    if (!this.#refs) return;
    this.#refs.notice.textContent = notice.text;
    this.#refs.notice.className = `notice ${notice.tone}`;
  }

  #buildLanguageSection(viewModel) {
    const section = this.#dom.element('fieldset');
    section.append(this.#dom.element('legend', '', this.#i18n.t('settings.language')));
    const select = this.#dom.element('select');
    select.append(...viewModel.languages.map((language) => {
      const option = this.#dom.element('option', '', language.code === 'auto' ? this.#i18n.t('lang.auto') : language.label);
      option.value = language.code;
      return option;
    }));
    select.value = viewModel.preferences.language;
    section.append(select, this.#dom.element('p', 'muted small', this.#i18n.t('settings.languageHint')));
    this.#refs = { ...this.#refs, languageSelect: select };
    return section;
  }

  #buildCountrySection(viewModel) {
    const t = (key) => this.#i18n.t(key);
    const section = this.#dom.element('fieldset');
    section.append(this.#dom.element('legend', '', t('settings.country')));
    const pinned = viewModel.preferences.isCountryPinned;
    const detected = this.#dom.element('p', 'muted small', t(pinned ? 'settings.geoipPinned' : 'settings.geoipDetecting'));

    const pinLabel = this.#dom.element('label', 'check');
    const pinToggle = this.#dom.element('input');
    pinToggle.type = 'checkbox';
    pinToggle.checked = pinned;
    pinLabel.append(pinToggle, this.#dom.element('span', '', t('settings.pinCountry')));

    const countrySelect = this.#dom.element('select');
    countrySelect.disabled = !pinned;
    countrySelect.append(...viewModel.countries.map((country) => {
      const option = this.#dom.element('option', '', `${country.flag} ${country.name}`);
      option.value = country.code;
      return option;
    }));
    if (pinned) countrySelect.value = viewModel.preferences.pinnedCountry;
    pinToggle.addEventListener('change', () => { countrySelect.disabled = !pinToggle.checked; });

    section.append(detected, pinLabel, countrySelect, this.#dom.element('p', 'muted small', t('settings.countryHint')));
    this.#refs = { ...this.#refs, detected, pinToggle, countrySelect };
    return section;
  }

  #buildAdvancedSection(viewModel) {
    const section = this.#dom.element('div', 'advanced');
    section.hidden = true;

    const providerSet = this.#dom.element('fieldset');
    providerSet.append(this.#dom.element('legend', '', this.#i18n.t('settings.aiModel')));
    const radioRow = this.#dom.element('div', 'chips');
    const providerRadios = viewModel.providers.map((provider) => {
      const label = this.#dom.element('label', 'check');
      const radio = this.#dom.element('input');
      radio.type = 'radio';
      radio.name = 'cupano-provider';
      radio.value = provider;
      radio.checked = provider === viewModel.settings.activeProvider;
      label.append(radio, this.#dom.element('span', '', this.#i18n.t(`settings.provider.${provider}`)));
      radioRow.append(label);
      return radio;
    });
    providerSet.append(radioRow);

    const providerFields = {};
    const modelLists = {};
    const providerBlocks = {};
    for (const provider of viewModel.providers) {
      const block = this.#buildProviderFields(provider, viewModel);
      providerFields[provider] = block.fields;
      modelLists[provider] = block.datalist;
      providerBlocks[provider] = block.node;
      providerSet.append(block.node);
    }
    const loadModelsButton = this.#dom.button('secondary', this.#i18n.t('settings.loadModels'));
    const modelStatus = this.#dom.element('p', 'small');
    providerSet.append(loadModelsButton, modelStatus);

    const showActiveProvider = () => {
      const active = providerRadios.find((radio) => radio.checked)?.value;
      for (const [provider, node] of Object.entries(providerBlocks)) node.hidden = provider !== active;
    };
    providerRadios.forEach((radio) => radio.addEventListener('change', showActiveProvider));
    showActiveProvider();

    const tasteSet = this.#buildTasteFields(viewModel);
    section.append(providerSet, tasteSet.node);
    this.#refs = {
      ...this.#refs, providerRadios, providerFields, modelLists, modelStatus, loadModelsButton,
      processBoxes: tasteSet.processBoxes, taste: tasteSet.taste,
    };
    return { section, loadModelsButton };
  }

  #buildProviderFields(provider, viewModel) {
    const stored = viewModel.settings.providers[provider];
    const node = this.#dom.element('div', 'provider');

    const keyRow = this.#dom.element('div', 'inline');
    const key = this.#dom.element('input');
    key.type = 'password';
    key.spellcheck = false;
    key.autocomplete = 'off';
    key.placeholder = provider === 'gemini' ? 'AIza…' : 'sk-…';
    key.value = stored.apiKey;
    const reveal = this.#dom.button('secondary', this.#i18n.t('settings.show'));
    reveal.addEventListener('click', () => {
      const show = key.type === 'password';
      key.type = show ? 'text' : 'password';
      reveal.textContent = this.#i18n.t(show ? 'settings.hide' : 'settings.show');
    });
    keyRow.append(key, reveal);

    const datalist = this.#dom.element('datalist');
    datalist.id = `cupano-models-${provider}`;
    const model = this.#dom.element('input');
    model.type = 'text';
    model.spellcheck = false;
    model.placeholder = viewModel.defaultModels[provider] ?? '';
    model.value = stored.model;
    model.setAttribute('list', datalist.id);

    node.append(this.#dom.element('label', 'field', this.#i18n.t('settings.apiKey')), keyRow,
      this.#dom.element('label', 'field', this.#i18n.t('settings.model')), model, datalist);
    return { node, datalist, fields: { key, model } };
  }

  #buildTasteFields(viewModel) {
    const node = this.#dom.element('fieldset');
    node.append(this.#dom.element('legend', '', this.#i18n.t('settings.yourTaste')));
    node.append(this.#dom.element('label', 'field', this.#i18n.t('settings.processes')));
    const chips = this.#dom.element('div', 'chips');
    const processBoxes = viewModel.processOptions.map((process) => {
      const label = this.#dom.element('label', 'check chip');
      const box = this.#dom.element('input');
      box.type = 'checkbox';
      box.value = process;
      box.checked = viewModel.preferences.preferredProcesses.includes(process);
      label.append(box, this.#dom.element('span', '', this.#i18n.t(`process.${process}`)));
      chips.append(label);
      return box;
    });
    const taste = this.#dom.element('textarea');
    taste.rows = 3;
    taste.maxLength = 600;
    taste.placeholder = this.#i18n.t('settings.tastePlaceholder');
    taste.value = viewModel.preferences.tastePreference;
    node.append(chips, this.#dom.element('label', 'field', this.#i18n.t('settings.tastePrompt')), taste);
    return { node, processBoxes, taste };
  }

  #buildFooter() {
    const node = this.#dom.element('div', 'settings-footer');
    const saveButton = this.#dom.button('primary', this.#i18n.t('settings.save'));
    const notice = this.#dom.element('span', 'notice');
    const advancedButton = this.#dom.button('advanced-toggle', `${this.#i18n.t('settings.advanced')} ▾`);
    advancedButton.setAttribute('aria-expanded', 'false');
    const left = this.#dom.element('div', 'inline');
    left.append(saveButton, notice);
    node.append(left, advancedButton);
    return { node, saveButton, notice, advancedButton };
  }

  #toggleAdvanced(button) {
    const open = this.#refs.advanced.hidden;
    this.#refs.advanced.hidden = !open;
    button.textContent = `${this.#i18n.t('settings.advanced')} ${open ? '▴' : '▾'}`;
    button.setAttribute('aria-expanded', String(open));
  }
}
