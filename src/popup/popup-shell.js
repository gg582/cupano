/**
 * @file popup-shell.js
 * @brief The fixed popup frame: top bar (brand, country chip, gear/back) and
 *        the screen area the controllers render into.
 */

/** Top-bar controls and the screen container, for the controller to wire. */
export class ShellHandles {
  constructor({ screen, countryChip, gearButton, backButton }) {
    this.screen = screen;
    this.countryChip = countryChip;
    this.gearButton = gearButton;
    this.backButton = backButton;
    Object.freeze(this);
  }
}

export class PopupShell {
  #document;
  #refs = null;

  constructor(documentRef) {
    this.#document = documentRef;
  }

  /** Binds to the static markup in popup.html. */
  mount() {
    const byId = (id) => this.#document.getElementById(id);
    this.#refs = {
      screen: byId('screen'),
      countryChip: byId('country-chip'),
      gearButton: byId('gear-button'),
      backButton: byId('back-button'),
      title: byId('topbar-title'),
      brand: byId('brand'),
    };
    return new ShellHandles(this.#refs);
  }

  showHomeBar() {
    this.#refs.backButton.hidden = true;
    this.#refs.title.hidden = true;
    this.#refs.brand.hidden = false;
    this.#refs.gearButton.hidden = false;
    this.#refs.countryChip.hidden = false;
  }

  showSettingsBar() {
    this.#refs.backButton.hidden = false;
    this.#refs.title.hidden = false;
    this.#refs.brand.hidden = true;
    this.#refs.gearButton.hidden = true;
    this.#refs.countryChip.hidden = true;
  }

  /** Re-labels the static top bar in the translator's language. */
  applyLanguage(translator) {
    this.#document.documentElement.lang = translator.language;
    this.#refs.title.textContent = translator.t('shell.settings');
    this.#refs.backButton.setAttribute('aria-label', translator.t('shell.back'));
    this.#refs.gearButton.setAttribute('aria-label', translator.t('shell.gear'));
    this.#refs.gearButton.title = translator.t('shell.gear');
  }

  /** country: CountryOption or null when unknown. */
  showCountry(country, pinned, translator) {
    const chip = this.#refs.countryChip;
    chip.textContent = country ? `${country.flag} ${country.code}` : '🌐';
    chip.title = country
      ? translator.t(pinned ? 'shell.country.pinned' : 'shell.country.detected', { name: country.name })
      : translator.t('shell.country.unknown');
    chip.classList.toggle('pinned', Boolean(pinned));
  }
}
