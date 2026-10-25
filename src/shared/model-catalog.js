/**
 * @file model-catalog.js
 * @brief Lists the models a provider key can use (Gemini models.list, OpenAI /v1/models).
 *
 * Shared by the options page and the in-page settings panel. Pure ES module:
 * exports classes only, holds no module-level state.
 *
 * Call graph: ModelCatalogClient -> {GeminiModelSource, OpenAiModelSource}
 */

/** Input for a model-list lookup. */
export class ModelCatalogRequest {
  constructor({ provider, apiKey }) {
    this.provider = String(provider ?? '');
    this.apiKey = String(apiKey ?? '').trim();
    Object.freeze(this);
  }
}

/** One selectable model. */
export class ModelOption {
  constructor({ id, label }) {
    this.id = String(id ?? '');
    this.label = String(label ?? '');
    Object.freeze(this);
  }
}

/** Models available to a provider key, newest-looking first. */
export class ModelCatalog {
  constructor({ provider, models }) {
    this.provider = String(provider ?? '');
    this.models = Object.freeze([...(models ?? [])]);
    Object.freeze(this);
  }
}

/** One page of a paginated model listing. */
export class ModelPage {
  constructor({ models, nextPageToken }) {
    this.models = Object.freeze([...(models ?? [])]);
    this.nextPageToken = String(nextPageToken ?? '');
    Object.freeze(this);
  }
}

/** URL + headers for a single GET request. */
export class ModelPageRequest {
  constructor({ url, headers }) {
    this.url = String(url);
    this.headers = Object.freeze({ ...headers });
    Object.freeze(this);
  }
}

/** Gemini models.list, limited to text models that support generateContent. */
export class GeminiModelSource {
  static #EXCLUDED = /embedding|aqa|imagen|veo|tts|image|live|native-audio|learnlm/i;

  get provider() {
    return 'gemini';
  }

  buildPageRequest(request, pageToken) {
    const url = new URL('https://generativelanguage.googleapis.com/v1beta/models');
    url.searchParams.set('pageSize', '1000');
    if (pageToken) url.searchParams.set('pageToken', pageToken);
    return new ModelPageRequest({ url: url.href, headers: { 'x-goog-api-key': request.apiKey } });
  }

  parsePage(json) {
    const models = (json?.models ?? [])
      .filter((model) => (model?.supportedGenerationMethods ?? []).includes('generateContent'))
      .map((model) => new ModelOption({
        id: String(model.name ?? '').replace(/^models\//, ''),
        label: model.displayName ?? '',
      }))
      .filter((model) => model.id.startsWith('gemini-') && !GeminiModelSource.#EXCLUDED.test(model.id));
    return new ModelPage({ models, nextPageToken: json?.nextPageToken });
  }

  readError(json) {
    return json?.error?.message ?? '';
  }
}

/** OpenAI models list, limited to text-generation model families. */
export class OpenAiModelSource {
  static #INCLUDED = /^(gpt-|o\d|chatgpt-)/i;
  static #EXCLUDED = /audio|realtime|transcribe|tts|image|embedding|moderation|instruct|search-preview/i;

  get provider() {
    return 'openai';
  }

  buildPageRequest(request) {
    return new ModelPageRequest({
      url: 'https://api.openai.com/v1/models',
      headers: { Authorization: `Bearer ${request.apiKey}` },
    });
  }

  parsePage(json) {
    const models = (json?.data ?? [])
      .map((model) => new ModelOption({ id: model?.id, label: '' }))
      .filter((model) => OpenAiModelSource.#INCLUDED.test(model.id) && !OpenAiModelSource.#EXCLUDED.test(model.id));
    return new ModelPage({ models, nextPageToken: '' });
  }

  readError(json) {
    return json?.error?.message ?? '';
  }
}

/** Fetches every page of a provider's model listing and returns a ModelCatalog. */
export class ModelCatalogClient {
  static #MAX_PAGES = 5;

  #fetch;
  #sources;

  constructor(fetchFn, sources) {
    this.#fetch = fetchFn;
    this.#sources = new Map(sources.map((source) => [source.provider, source]));
  }

  async list(request) {
    const source = this.#sources.get(request.provider);
    if (!source) throw new Error(`Unknown provider: ${request.provider}`);
    const models = [];
    let pageToken = '';
    for (let page = 0; page < ModelCatalogClient.#MAX_PAGES; page += 1) {
      const result = source.parsePage(await this.#get(source.buildPageRequest(request, pageToken), source));
      models.push(...result.models);
      pageToken = result.nextPageToken;
      if (!pageToken) break;
    }
    return new ModelCatalog({ provider: request.provider, models: this.#dedupeAndSort(models) });
  }

  async #get(pageRequest, source) {
    const response = await this.#fetch(pageRequest.url, {
      method: 'GET',
      headers: pageRequest.headers,
      credentials: 'omit',
      referrerPolicy: 'no-referrer',
    });
    const json = await response.json().catch(() => ({}));
    if (!response.ok) throw new Error(source.readError(json) || `HTTP ${response.status}`);
    return json;
  }

  #dedupeAndSort(models) {
    const unique = new Map(models.map((model) => [model.id, model]));
    // Descending natural order puts higher version numbers first.
    return [...unique.values()].sort((a, b) => b.id.localeCompare(a.id, 'en', { numeric: true }));
  }
}
