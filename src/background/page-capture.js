/**
 * @file page-capture.js
 * @brief Reads the active tab's text with scripting.executeScript.
 *
 * Nothing is injected ahead of time: the capture function runs once, on the
 * tab the user opened the popup on (activeTab grant), and returns plain data.
 */
import { PageSnapshot, CupanoError, StatusMessage } from '../shared/dto.js';

/**
 * Runs inside the tab. Must stay self-contained: executeScript serializes the
 * function source, so it cannot reference anything from this module.
 */
function capturePageInTab() {
  const MAX_BODY_CHARS = 60000;
  const MIN_MAIN_CHARS = 400;
  const MAX_SEARCH_CHARS = 8000;
  // host pattern, path pattern, query parameter
  const SEARCH_ENGINES = [
    [/(^|\.)google\.[a-z.]+$/, /^\/search/, 'q'],
    [/(^|\.)bing\.com$/, /^\/search/, 'q'],
    [/(^|\.)duckduckgo\.com$/, /^\/$/, 'q'],
    [/^search\.naver\.com$/, /^\/search/, 'query'],
    [/^search\.daum\.net$/, /^\/search/, 'q'],
    [/^search\.yahoo\.(co\.jp|com)$/, /^\/search/, 'p'],
    [/(^|\.)baidu\.com$/, /^\/s/, 'wd'],
    [/(^|\.)yandex\.[a-z.]+$/, /^\/search/, 'text'],
    [/(^|\.)ecosia\.org$/, /^\/search/, 'q'],
  ];

  const url = new URL(location.href);
  const engine = SEARCH_ENGINES.find(([host, path, param]) =>
    host.test(url.hostname) && path.test(url.pathname) && url.searchParams.get(param));
  const meta = document.querySelector('meta[name="description"], meta[property="og:description"]');

  if (engine) {
    const query = url.searchParams.get(engine[2]).trim();
    const results = document.querySelector('#search, #rso, #b_results, main, [role="main"]') ?? document.body;
    return {
      kind: 'search',
      url: location.href,
      title: query,
      metaDescription: '',
      bodyText: `Search query: ${query}\n${(results?.innerText ?? '').slice(0, MAX_SEARCH_CHARS)}`,
      documentLanguage: document.documentElement.lang,
      searchQuery: query,
    };
  }

  // innerText keeps table cells as tabs and blocks as newlines, which the C++
  // label extractor relies on ("Origin\tEthiopia").
  const main = document.querySelector('main, [role="main"], #MainContent, article');
  const mainText = main?.innerText ?? '';
  const bodyText = mainText.length >= MIN_MAIN_CHARS ? mainText : (document.body?.innerText ?? '');
  const headline = document.querySelector('h1')?.innerText?.trim() ?? '';
  return {
    kind: 'page',
    url: location.href,
    title: document.title || headline,
    metaDescription: meta?.getAttribute('content') ?? '',
    bodyText: (headline && !bodyText.startsWith(headline) ? `${headline}\n${bodyText}` : bodyText)
      .slice(0, MAX_BODY_CHARS),
    documentLanguage: document.documentElement.lang,
    searchQuery: '',
  };
}

/** Captures a tab as a PageSnapshot. */
export class PageCapture {
  #scripting;

  constructor(scriptingApi) {
    this.#scripting = scriptingApi;
  }

  async capture(tabId) {
    let results;
    try {
      results = await this.#scripting.executeScript({ target: { tabId }, func: capturePageInTab });
    } catch (error) {
      throw new CupanoError(new StatusMessage({
        code: 'page_unreadable',
        title: 'Cannot read this page',
        detail: String(error?.message ?? error),
      }));
    }
    const data = results?.[0]?.result;
    if (!data) {
      throw new CupanoError(new StatusMessage({ code: 'page_empty', title: 'Cannot read this page' }));
    }
    return new PageSnapshot(data);
  }
}
