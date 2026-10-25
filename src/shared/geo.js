/**
 * @file geo.js
 * @brief Resolves the user's country: a pinned choice wins, otherwise GeoIP.
 *
 * Call graph (top-down only):
 *   CountryResolver
 *     ├─ GeoCache         storage-backed, 12 h TTL (avoids a lookup per page)
 *     ├─ GeoIpLocator     fetch() country.is, falling back to geojs.io
 *     └─ CountryCatalog   ISO 3166-1 codes -> localized names / flags
 *
 * GeoIP sends the caller's IP address to the lookup service; it only runs
 * when the country is not pinned. Pure ES module, no module-level state.
 */

/** A validated ISO 3166-1 alpha-2 code (empty when unknown). */
export class CountryCode {
  constructor({ code }) {
    const upper = String(code ?? '').trim().toUpperCase();
    this.code = /^[A-Z]{2}$/.test(upper) ? upper : '';
    Object.freeze(this);
  }

  get isKnown() {
    return this.code.length === 2;
  }
}

/** The country cupano will use, and where it came from. */
export class ResolvedCountry {
  static SOURCES = Object.freeze(['pinned', 'geoip', 'unknown']);

  constructor({ code, name, source }) {
    this.code = String(code ?? '');
    this.name = String(name ?? '');
    this.source = ResolvedCountry.SOURCES.includes(source) ? source : 'unknown';
    Object.freeze(this);
  }
}

/** One selectable country. */
export class CountryOption {
  constructor({ code, name, flag }) {
    this.code = String(code);
    this.name = String(name);
    this.flag = String(flag);
    Object.freeze(this);
  }
}

/** ISO 3166-1 alpha-2 catalog with names localized through Intl.DisplayNames. */
export class CountryCatalog {
  #codes;
  #displayNames;

  constructor(locale) {
    this.#codes = Object.freeze((
      'AD AE AF AG AI AL AM AO AQ AR AS AT AU AW AX AZ BA BB BD BE BF BG BH BI BJ BL BM BN BO BQ BR BS BT BV BW '
      + 'BY BZ CA CC CD CF CG CH CI CK CL CM CN CO CR CU CV CW CX CY CZ DE DJ DK DM DO DZ EC EE EG EH ER ES ET FI '
      + 'FJ FK FM FO FR GA GB GD GE GF GG GH GI GL GM GN GP GQ GR GS GT GU GW GY HK HM HN HR HT HU ID IE IL IM IN '
      + 'IO IQ IR IS IT JE JM JO JP KE KG KH KI KM KN KP KR KW KY KZ LA LB LC LI LK LR LS LT LU LV LY MA MC MD ME '
      + 'MF MG MH MK ML MM MN MO MP MQ MR MS MT MU MV MW MX MY MZ NA NC NE NF NG NI NL NO NP NR NU NZ OM PA PE PF '
      + 'PG PH PK PL PM PN PR PS PT PW PY QA RE RO RS RU RW SA SB SC SD SE SG SH SI SJ SK SL SM SN SO SR SS ST SV '
      + 'SX SY SZ TC TD TF TG TH TJ TK TL TM TN TO TR TT TV TW TZ UA UG UM US UY UZ VA VC VE VG VI VN VU WF WS YE '
      + 'YT ZA ZM ZW'
    ).split(' '));
    try {
      this.#displayNames = new Intl.DisplayNames([locale, 'en'], { type: 'region' });
    } catch {
      this.#displayNames = null;
    }
  }

  /** All countries, sorted by localized name. */
  options() {
    return this.#codes
      .map((code) => this.option(new CountryCode({ code })))
      .sort((a, b) => a.name.localeCompare(b.name));
  }

  option(countryCode) {
    const code = countryCode.code;
    return new CountryOption({ code, name: this.#nameOf(code), flag: this.#flagOf(code) });
  }

  #nameOf(code) {
    return this.#displayNames?.of(code) ?? code;
  }

  #flagOf(code) {
    // Regional indicator symbols: 'A' (0x41) maps to U+1F1E6.
    return String.fromCodePoint(...[...code].map((letter) => 0x1f1e6 + letter.charCodeAt(0) - 0x41));
  }
}

/** Asks public GeoIP services for the caller's country. */
export class GeoIpLocator {
  static #TIMEOUT_MS = 5000;
  static #PROVIDERS = Object.freeze([
    { url: 'https://api.country.is/', field: 'country' },
    { url: 'https://get.geojs.io/v1/ip/country.json', field: 'country' },
  ]);

  #fetch;

  constructor(fetchFn) {
    this.#fetch = fetchFn;
  }

  /** Returns the first provider's answer, or an unknown CountryCode. */
  async locate() {
    for (const provider of GeoIpLocator.#PROVIDERS) {
      const code = await this.#query(provider);
      if (code.isKnown) return code;
    }
    return new CountryCode({});
  }

  async #query(provider) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), GeoIpLocator.#TIMEOUT_MS);
    try {
      const response = await this.#fetch(provider.url, {
        signal: controller.signal,
        credentials: 'omit',
        referrerPolicy: 'no-referrer',
      });
      if (!response.ok) return new CountryCode({});
      const json = await response.json();
      return new CountryCode({ code: json?.[provider.field] });
    } catch {
      return new CountryCode({});
    } finally {
      clearTimeout(timer);
    }
  }
}

/** Caches the GeoIP answer in storage so it is not looked up on every page. */
export class GeoCache {
  static #STORAGE_KEY = 'cupanoGeoCache';
  static #TTL_MS = 12 * 60 * 60 * 1000;

  #storageArea;
  #now;

  constructor(storageArea, nowFn) {
    this.#storageArea = storageArea;
    this.#now = nowFn;
  }

  async read() {
    const stored = await this.#storageArea.get(GeoCache.#STORAGE_KEY);
    const entry = stored?.[GeoCache.#STORAGE_KEY];
    const fresh = entry && this.#now() - Number(entry.fetchedAt) < GeoCache.#TTL_MS;
    return new CountryCode({ code: fresh ? entry.code : '' });
  }

  async write(countryCode) {
    await this.#storageArea.set({ [GeoCache.#STORAGE_KEY]: { code: countryCode.code, fetchedAt: this.#now() } });
  }
}

/** Decides the effective country from preferences, cache and GeoIP. */
export class CountryResolver {
  #locator;
  #cache;
  #catalog;

  constructor({ locator, cache, catalog }) {
    this.#locator = locator;
    this.#cache = cache;
    this.#catalog = catalog;
  }

  /** Pinned country if set, otherwise the GeoIP country. */
  async resolve(preferences) {
    if (preferences.isCountryPinned) {
      return this.#describe(new CountryCode({ code: preferences.pinnedCountry }), 'pinned');
    }
    return this.detect();
  }

  /** GeoIP country (cached), regardless of pinning; used to show what was detected. */
  async detect() {
    const cached = await this.#cache.read();
    if (cached.isKnown) return this.#describe(cached, 'geoip');
    const located = await this.#locator.locate();
    if (!located.isKnown) return new ResolvedCountry({ source: 'unknown' });
    await this.#cache.write(located);
    return this.#describe(located, 'geoip');
  }

  #describe(countryCode, source) {
    const option = this.#catalog.option(countryCode);
    return new ResolvedCountry({ code: option.code, name: option.name, source });
  }
}
