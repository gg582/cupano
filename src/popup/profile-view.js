/**
 * @file profile-view.js
 * @brief Builds the analysis result: roast, availability, taste match,
 *        flavor wheel, notes, summary and discrepancies.
 */

/** Builds the profile body from a CupanoProfileView. */
export class ProfileView {
  static #REGION_LABELS = Object.freeze({ KO: 'KO', JA: 'JA', 'ZH-TW': 'TW', EN: 'EN', NO: 'NO', DE: 'DE' });

  #dom;
  #countries;
  #wheelView;
  #i18n;

  constructor(dom, countryLabeler, wheelView, translator) {
    this.#dom = dom;
    this.#countries = countryLabeler;
    this.#wheelView = wheelView;
    this.#i18n = translator;
  }

  build(profile) {
    const t = (key, params) => this.#i18n.t(key, params);
    const nodes = [
      this.#dom.element('p', 'meta', this.#describeBean(profile.bean)),
      this.#buildRoast(profile),
      this.#buildAvailability(profile.availability),
    ];
    if (profile.tasteMatch.present) nodes.push(this.#buildTasteMatch(profile.tasteMatch));
    if (profile.summary) nodes.push(this.#dom.element('p', 'summary', profile.summary));
    const wheel = this.#wheelView.build(profile.flavorWheel);
    if (wheel) nodes.push(wheel);
    nodes.push(this.#dom.element('p', 'section', t('profile.topNotes')));
    nodes.push(...profile.topNotes.map((note) => this.#buildNote(note)));
    if (profile.discrepancies.length) {
      nodes.push(this.#dom.element('p', 'section', t('profile.discrepancies')));
      nodes.push(this.#buildList(profile.discrepancies));
    }
    nodes.push(this.#dom.element('p', 'footer', t('profile.footer', { count: profile.reportingRegions })));
    return nodes;
  }

  #buildRoast(profile) {
    const block = this.#dom.element('div', 'roast');
    const head = this.#dom.element('div', 'row');
    const label = this.#dom.element('span', '', `${this.#i18n.t('profile.roast')} `);
    const level = this.#i18n.has(`roast.${profile.roastLevel}`) ? this.#i18n.t(`roast.${profile.roastLevel}`) : profile.roastLevel;
    label.append(this.#dom.element('b', '', level));
    head.append(label, this.#dom.element('span', 'muted',
      this.#i18n.t('profile.confidence', { percent: Math.round(profile.roastConfidence * 100) })));
    block.append(head);
    if (profile.roastAgtron) {
      block.append(this.#dom.element('div', 'muted small', `${profile.roastStage} · Agtron ${profile.roastAgtron}`));
    }
    return block;
  }

  #buildAvailability(availability) {
    const block = this.#dom.element('div', 'availability');
    block.append(this.#dom.element('p', 'section', this.#i18n.t('profile.availability')));
    block.append(this.#availabilityRow('🌐', this.#i18n.t('profile.worldwide'), availability.globalStatus,
      availability.sellersInStock, availability.sellersChecked, ''));
    if (availability.userCountry) {
      const country = this.#countries.describe(availability.userCountry);
      block.append(this.#availabilityRow(country.flag, country.name, availability.localStatus,
        availability.localSellersInStock, availability.localSellersChecked, country.name));
    }
    if (availability.sellers.length) block.append(this.#buildSellers(availability.sellers));
    return block;
  }

  #availabilityRow(icon, label, status, inStock, checked, countryName) {
    const row = this.#dom.element('div', 'row');
    row.append(this.#dom.element('span', '', `${icon} ${label}`));
    row.append(this.#dom.element('span', `pill ${status.toLowerCase()}`,
      this.#statusText(status, inStock, checked, countryName)));
    return row;
  }

  #statusText(status, inStock, checked, countryName) {
    if (status === 'AVAILABLE') return this.#i18n.t('profile.inStock', { inStock, checked });
    if (status === 'SOLD_OUT') {
      return countryName
        ? this.#i18n.t('profile.soldOutIn', { country: countryName, checked })
        : this.#i18n.t('profile.soldOutEverywhere', { checked });
    }
    return this.#i18n.t('profile.noStock');
  }

  #buildSellers(sellers) {
    const details = this.#dom.element('details', 'sellers');
    details.append(this.#dom.element('summary', '', this.#i18n.t('profile.listings', { count: sellers.length })));
    const list = this.#dom.element('ul');
    for (const seller of sellers) {
      const item = this.#dom.element('li');
      item.append(
        this.#dom.externalLink(seller.url, seller.name),
        this.#dom.element('span', 'muted', ` · ${seller.market} · ${this.#i18n.t(`profile.stock.${seller.status}`)}`),
      );
      list.append(item);
    }
    details.append(list);
    return details;
  }

  #buildTasteMatch(match) {
    const block = this.#dom.element('div', 'taste');
    const head = this.#dom.element('div', 'row');
    head.append(this.#dom.element('b', '', this.#i18n.t('profile.tasteMatch')),
      this.#dom.element('b', 'score', `${match.score}/100`));
    block.append(head);
    if (match.reason) block.append(this.#dom.element('p', 'small', match.reason));
    return block;
  }

  #buildNote(note) {
    const wrapper = this.#dom.element('div', 'note');
    const head = this.#dom.element('div', 'row');
    head.append(
      this.#dom.element('span', 'descriptor', note.descriptor),
      this.#dom.element('span', 'muted', `${this.#count(note.regionCount, 'region')} · ${this.#count(note.mentionCount, 'mention')}`),
    );
    // Bar length = share of reporting regions that named this note.
    const bar = this.#dom.element('div', 'bar');
    const fill = this.#dom.element('span');
    fill.style.width = `${Math.min(100, note.consensusScore * 100)}%`;
    bar.append(fill);
    const chips = this.#dom.element('div', 'chips');
    if (note.category) chips.append(this.#dom.element('span', 'muted', note.category));
    chips.append(...note.sourceRegions.map((code) => this.#dom.element('span', 'chip', ProfileView.#REGION_LABELS[code] ?? code)));
    wrapper.append(head, bar, chips);
    if (note.nativeTerms.length) wrapper.append(this.#dom.element('p', 'native', note.nativeTerms.join(' · ')));
    return wrapper;
  }

  #buildList(items) {
    const list = this.#dom.element('ul', 'discrepancies');
    list.append(...items.map((item) => this.#dom.element('li', '', item)));
    return list;
  }

  #describeBean(bean) {
    return [bean.roaster, bean.originCountry, bean.region, bean.variety, bean.process].filter(Boolean).join(' · ');
  }

  #count(count, noun) {
    return count === 1 ? this.#i18n.t(`profile.${noun}`) : this.#i18n.t(`profile.${noun}s`, { count });
  }
}
