/**
 * @file dto.js
 * @brief Value objects shared by the background analysis service and the popup.
 *
 * Every object is frozen and made of plain fields, so it survives a round trip
 * through browser.storage.session (structured clone / JSON) and can be
 * rehydrated with the matching fromRecord().
 */

/** Page text captured from the active tab, or a search-results page. */
export class PageSnapshot {
  static KINDS = Object.freeze(['page', 'search']);

  constructor({ kind, url, title, metaDescription, bodyText, documentLanguage, searchQuery }) {
    this.kind = PageSnapshot.KINDS.includes(kind) ? kind : 'page';
    this.url = String(url ?? '');
    this.title = String(title ?? '');
    this.metaDescription = String(metaDescription ?? '');
    this.bodyText = String(bodyText ?? '');
    this.documentLanguage = String(documentLanguage ?? '');
    this.searchQuery = String(searchQuery ?? '');
    Object.freeze(this);
  }
}

/** Prompt sizing and output language. */
export class PromptOptions {
  constructor({ outputLocale, notesPerAgent, sourcesPerAgent }) {
    this.outputLocale = String(outputLocale ?? 'en');
    this.notesPerAgent = Number.isInteger(notesPerAgent) ? notesPerAgent : 5;
    this.sourcesPerAgent = Number.isInteger(sourcesPerAgent) ? sourcesPerAgent : 3;
    Object.freeze(this);
  }
}

/** The user as seen by the Wasm core: market country and taste. */
export class UserContextView {
  constructor({ countryCode, countryName, preferredProcesses, tastePreference }) {
    this.countryCode = String(countryCode ?? '');
    this.countryName = String(countryName ?? '');
    this.preferredProcesses = Object.freeze([...(preferredProcesses ?? [])]);
    this.tastePreference = String(tastePreference ?? '');
    Object.freeze(this);
  }
}

/** System + user prompt pair produced by the Wasm prompt builder. */
export class PromptRequest {
  constructor({ systemInstruction, userPrompt }) {
    this.systemInstruction = String(systemInstruction ?? '');
    this.userPrompt = String(userPrompt ?? '');
    Object.freeze(this);
  }
}

/** Fully described HTTP request prepared by a provider transport. */
export class HttpRequestSpec {
  constructor({ endpoint, headers, body }) {
    this.endpoint = String(endpoint);
    this.headers = Object.freeze({ ...headers });
    this.body = String(body);
    Object.freeze(this);
  }
}

/** Plain text emitted by the model. */
export class AiTextResponse {
  constructor({ provider, text }) {
    this.provider = String(provider ?? '');
    this.text = String(text ?? '');
    Object.freeze(this);
  }
}

/** JS-side copy of the Wasm BeanMetadata. */
export class BeanView {
  constructor(fields) {
    this.productName = String(fields?.productName ?? '');
    this.roaster = String(fields?.roaster ?? '');
    this.originCountry = String(fields?.originCountry ?? '');
    this.region = String(fields?.region ?? '');
    this.producer = String(fields?.producer ?? '');
    this.variety = String(fields?.variety ?? '');
    this.process = String(fields?.process ?? '');
    this.altitude = String(fields?.altitude ?? '');
    this.declaredRoastLevel = String(fields?.declaredRoastLevel ?? '');
    this.sourceUrl = String(fields?.sourceUrl ?? '');
    this.isCoffeeProduct = Boolean(fields?.isCoffeeProduct);
    Object.freeze(this);
  }
}

/** JS-side copy of one frequency-ranked SensoryNote. */
export class SensoryNoteView {
  constructor({ descriptor, category, regionCount, mentionCount, consensusScore, sourceRegions, nativeTerms }) {
    this.descriptor = String(descriptor ?? '');
    this.category = String(category ?? '');
    this.regionCount = Number(regionCount) || 0;
    this.mentionCount = Number(mentionCount) || 0;
    this.consensusScore = Number(consensusScore) || 0;
    this.sourceRegions = Object.freeze([...(sourceRegions ?? [])]);
    this.nativeTerms = Object.freeze([...(nativeTerms ?? [])]);
    Object.freeze(this);
  }
}

/** One store listing checked for stock. */
export class SellerView {
  constructor({ name, market, status, url }) {
    this.name = String(name ?? '');
    this.market = String(market ?? '');
    this.status = String(status ?? 'unknown');
    this.url = String(url ?? '');
    Object.freeze(this);
  }
}

/** Sold-out verdicts computed by the Wasm AvailabilityAnalyzer. */
export class AvailabilityView {
  constructor({ userCountry, globalStatus, localStatus, sellersChecked, sellersInStock, localSellersChecked,
    localSellersInStock, sellers }) {
    this.userCountry = String(userCountry ?? '');
    this.globalStatus = String(globalStatus ?? 'UNKNOWN');
    this.localStatus = String(localStatus ?? 'UNKNOWN');
    this.sellersChecked = Number(sellersChecked) || 0;
    this.sellersInStock = Number(sellersInStock) || 0;
    this.localSellersChecked = Number(localSellersChecked) || 0;
    this.localSellersInStock = Number(localSellersInStock) || 0;
    this.sellers = Object.freeze((sellers ?? []).map((seller) => new SellerView(seller)));
    Object.freeze(this);
  }
}

/** One flavor-wheel segment with its regional tally (see WheelSegment in C++). */
export class WheelSegmentView {
  constructor({ name, path, parentPath, tier, regionCount, mentionCount, consensusScore, contested, sourceRegions }) {
    this.name = String(name ?? '');
    this.path = String(path ?? '');
    this.parentPath = String(parentPath ?? '');
    this.tier = Number(tier) || 1;
    this.regionCount = Number(regionCount) || 0;
    this.mentionCount = Number(mentionCount) || 0;
    this.consensusScore = Number(consensusScore) || 0;
    this.contested = Boolean(contested);
    this.sourceRegions = Object.freeze([...(sourceRegions ?? [])].map(String));
    Object.freeze(this);
  }
}

/** LLM judgment of fit with the user's taste prompt. */
export class TasteMatchView {
  constructor({ present, score, reason }) {
    this.present = Boolean(present);
    this.score = Number(score) || 0;
    this.reason = String(reason ?? '');
    Object.freeze(this);
  }
}

/** JS-side copy of the Wasm FinalCupanoProfile, ready for rendering. */
export class CupanoProfileView {
  constructor({ success, errorMessage, bean, topNotes, roastLevel, roastStage, roastAgtron, roastConfidence,
    reportingRegions, discrepancies, flavorWheel, summary, availability, tasteMatch }) {
    this.success = Boolean(success);
    this.errorMessage = String(errorMessage ?? '');
    this.bean = new BeanView(bean);
    this.topNotes = Object.freeze((topNotes ?? []).map((note) => new SensoryNoteView(note)));
    this.roastLevel = String(roastLevel ?? 'Unknown');
    this.roastStage = String(roastStage ?? '');
    this.roastAgtron = String(roastAgtron ?? '');
    this.roastConfidence = Number(roastConfidence) || 0;
    this.reportingRegions = Number(reportingRegions) || 0;
    this.discrepancies = Object.freeze([...(discrepancies ?? [])].map(String));
    this.flavorWheel = Object.freeze((flavorWheel ?? []).map((segment) => new WheelSegmentView(segment)));
    this.summary = String(summary ?? '');
    this.availability = new AvailabilityView(availability ?? {});
    this.tasteMatch = new TasteMatchView(tasteMatch ?? {});
    Object.freeze(this);
  }
}

/**
 * Status shown while the pipeline runs or when it fails. `code` selects the
 * translated title/detail in the popup ("status.<code>.title"); `params` fill
 * its placeholders; `detail` carries raw, untranslatable text such as an API
 * error message. `title` is an English fallback.
 */
export class StatusMessage {
  constructor({ code, params, title, detail }) {
    this.code = String(code ?? 'unexpected');
    this.params = Object.freeze(Object.fromEntries(Object.entries(params ?? {}).map(([k, v]) => [k, String(v)])));
    this.title = String(title ?? '');
    this.detail = String(detail ?? '');
    Object.freeze(this);
  }
}

/** Error carrying a user-presentable StatusMessage and the phase to report. */
export class CupanoError extends Error {
  constructor(status, phase = 'error') {
    super(status.detail || status.title);
    this.name = 'CupanoError';
    this.status = status;
    this.phase = phase;
  }
}

/**
 * Progress of one analysis for one tab, written by the background service and
 * read by the popup. Phases:
 *   reading     capturing the page and extracting metadata in Wasm
 *   consulting  waiting for the six-agent swarm
 *   done        profile available
 *   not_coffee  the page does not look like coffee (the user may force it)
 *   error       status explains what went wrong
 */
export class TabAnalysisState {
  static PHASES = Object.freeze(['reading', 'consulting', 'done', 'not_coffee', 'error']);

  constructor({ tabId, url, phase, status, bean, profile, startedAt, updatedAt }) {
    this.tabId = Number(tabId) || 0;
    this.url = String(url ?? '');
    this.phase = TabAnalysisState.PHASES.includes(phase) ? phase : 'error';
    this.status = new StatusMessage(status ?? {});
    this.bean = bean ? new BeanView(bean) : null;
    this.profile = profile ? new CupanoProfileView(profile) : null;
    this.startedAt = Number(startedAt) || 0;
    this.updatedAt = Number(updatedAt) || 0;
    Object.freeze(this);
  }

  static #STALE_MS = 4 * 60 * 1000;

  get isRunning() {
    return this.phase === 'reading' || this.phase === 'consulting';
  }

  /** A run whose background context died before finishing (no update for minutes). */
  isInterrupted(now) {
    return this.isRunning && now - this.updatedAt > TabAnalysisState.#STALE_MS;
  }

  /** Rebuilds the state from what storage returned. */
  static fromRecord(record) {
    return record ? new TabAnalysisState(record) : null;
  }

  /** Plain object for storage; getters are not persisted. */
  toRecord() {
    return JSON.parse(JSON.stringify(this));
  }
}

/** The single message the popup sends to the background. */
export class AnalyzeTabCommand {
  static TYPE = 'cupano/analyze-tab';

  constructor({ tabId, force }) {
    this.type = AnalyzeTabCommand.TYPE;
    this.tabId = Number(tabId) || 0;
    this.force = Boolean(force);
    Object.freeze(this);
  }

  static isCommand(message) {
    return message?.type === AnalyzeTabCommand.TYPE && Number.isInteger(message.tabId) && message.tabId > 0;
  }
}
