/**
 * @file dom.js
 * @brief DOM helpers shared by the popup views.
 */

/** Creates DOM nodes. Text is always assigned via textContent, never HTML. */
export class DomFactory {
  #document;

  constructor(documentRef) {
    this.#document = documentRef;
  }

  element(tag, className = '', text = '') {
    const node = this.#document.createElement(tag);
    if (className) node.className = className;
    if (text) node.textContent = text;
    return node;
  }

  button(className, text) {
    const node = this.element('button', className, text);
    node.type = 'button';
    return node;
  }

  /** External link; only absolute http(s) URLs become anchors. */
  externalLink(url, text) {
    if (!/^https?:\/\//i.test(url)) return this.element('span', '', text);
    const anchor = this.element('a', '', text);
    anchor.href = url;
    anchor.target = '_blank';
    anchor.rel = 'noopener noreferrer';
    return anchor;
  }
}

/** Resolves ISO country codes to localized names and flags for display. */
export class CountryLabeler {
  #catalog;
  #CountryCode;

  constructor(catalog, CountryCodeType) {
    this.#catalog = catalog;
    this.#CountryCode = CountryCodeType;
  }

  describe(code) {
    return this.#catalog.option(new this.#CountryCode({ code }));
  }
}
