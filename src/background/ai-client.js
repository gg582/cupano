/**
 * @file ai-client.js
 * @brief Sends a PromptRequest to Gemini or OpenAI straight from the extension.
 */
import { AiTextResponse, CupanoError, HttpRequestSpec, StatusMessage } from '../shared/dto.js';

/** Google Gemini generateContent with Google Search grounding. */
export class GeminiTransport {
  get provider() {
    return 'gemini';
  }

  buildRequest(prompt, credentials) {
    const model = encodeURIComponent(credentials.model);
    return new HttpRequestSpec({
      endpoint: `https://generativelanguage.googleapis.com/v1beta/models/${model}:generateContent`,
      headers: { 'Content-Type': 'application/json', 'x-goog-api-key': credentials.apiKey },
      body: JSON.stringify({
        systemInstruction: { parts: [{ text: prompt.systemInstruction }] },
        contents: [{ role: 'user', parts: [{ text: prompt.userPrompt }] }],
        tools: [{ googleSearch: {} }],
        generationConfig: { temperature: 0.2 },
      }),
    });
  }

  parseResponse(json) {
    const blockReason = json?.promptFeedback?.blockReason;
    if (blockReason) throw new CupanoError(new StatusMessage({ code: 'blocked', title: 'Gemini blocked the request', detail: blockReason }));
    const parts = json?.candidates?.[0]?.content?.parts ?? [];
    const text = parts.map((part) => part?.text ?? '').join('');
    return new AiTextResponse({ provider: this.provider, text });
  }

  readError(json) {
    return json?.error?.message ?? '';
  }
}

/** OpenAI Responses API with the hosted web_search tool. */
export class OpenAiTransport {
  get provider() {
    return 'openai';
  }

  buildRequest(prompt, credentials) {
    return new HttpRequestSpec({
      endpoint: 'https://api.openai.com/v1/responses',
      headers: { 'Content-Type': 'application/json', Authorization: `Bearer ${credentials.apiKey}` },
      body: JSON.stringify({
        model: credentials.model,
        instructions: prompt.systemInstruction,
        input: prompt.userPrompt,
        tools: [{ type: 'web_search' }],
      }),
    });
  }

  parseResponse(json) {
    const text = (json?.output ?? [])
      .filter((item) => item?.type === 'message')
      .flatMap((item) => item.content ?? [])
      .filter((content) => content?.type === 'output_text')
      .map((content) => content.text ?? '')
      .join('');
    return new AiTextResponse({ provider: this.provider, text });
  }

  readError(json) {
    return json?.error?.message ?? '';
  }
}

/** Dispatches a PromptRequest to the configured provider via fetch(). */
export class AiClient {
  static #TIMEOUT_MS = 180000;

  #fetch;
  #transports;

  constructor(fetchFn, transports) {
    this.#fetch = fetchFn;
    this.#transports = new Map(transports.map((transport) => [transport.provider, transport]));
  }

  async generate(prompt, credentials) {
    const transport = this.#selectTransport(credentials);
    const request = transport.buildRequest(prompt, credentials);
    const json = await this.#post(request, transport);
    const response = transport.parseResponse(json);
    if (!response.text.trim()) {
      throw new CupanoError(new StatusMessage({ code: 'empty_response', title: 'Empty response' }));
    }
    return response;
  }

  #selectTransport(credentials) {
    const transport = this.#transports.get(credentials.provider);
    if (!transport) {
      throw new CupanoError(new StatusMessage({ code: 'unknown_provider', title: 'Unknown provider', detail: credentials.provider }));
    }
    return transport;
  }

  async #post(request, transport) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), AiClient.#TIMEOUT_MS);
    try {
      const response = await this.#fetch(request.endpoint, {
        method: 'POST',
        headers: request.headers,
        body: request.body,
        signal: controller.signal,
        credentials: 'omit',
        referrerPolicy: 'no-referrer',
      });
      const json = await response.json().catch(() => ({}));
      if (!response.ok) {
        throw new CupanoError(new StatusMessage({
          code: 'api_error',
          params: { provider: transport.provider, status: response.status },
          title: `${transport.provider} API error (${response.status})`,
          detail: transport.readError(json) || response.statusText,
        }));
      }
      return json;
    } catch (error) {
      if (error instanceof CupanoError) throw error;
      if (error?.name === 'AbortError') {
        throw new CupanoError(new StatusMessage({ code: 'timeout', title: 'Request timed out' }));
      }
      throw new CupanoError(new StatusMessage({
        code: 'network_error', title: 'Network error', detail: String(error?.message ?? error),
      }));
    } finally {
      clearTimeout(timer);
    }
  }
}
