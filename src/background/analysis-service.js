/**
 * @file analysis-service.js
 * @brief Runs one analysis for one tab: capture -> Wasm -> swarm LLM -> Wasm.
 *
 * Progress is published only by writing TabAnalysisState to the TabStateStore;
 * the popup observes storage. The service never calls back into its caller.
 */
import { CupanoError, PromptOptions, StatusMessage, TabAnalysisState, UserContextView } from '../shared/dto.js';
import { resolveLanguage } from '../shared/i18n.js';

export class AnalysisService {
  #capture;
  #engine;
  #aiClient;
  #settingsStore;
  #preferencesStore;
  #countryResolver;
  #stateStore;
  #badge;
  #keepAlive;
  #promptOptions;
  #now;
  #running = new Set();

  constructor({ capture, engine, aiClient, settingsStore, preferencesStore, countryResolver, stateStore, badge,
    keepAlive, promptOptions, now }) {
    this.#capture = capture;
    this.#engine = engine;
    this.#aiClient = aiClient;
    this.#settingsStore = settingsStore;
    this.#preferencesStore = preferencesStore;
    this.#countryResolver = countryResolver;
    this.#stateStore = stateStore;
    this.#badge = badge;
    this.#keepAlive = keepAlive;
    this.#promptOptions = promptOptions;
    this.#now = now;
  }

  /** Starts an analysis unless one is already running for the tab. */
  async analyze(command) {
    const { tabId } = command;
    if (this.#running.has(tabId)) return;
    this.#running.add(tabId);
    const release = this.#keepAlive.hold();
    const run = { tabId, url: '', bean: null, startedAt: this.#now() };
    try {
      await this.#publish(run, 'reading', { code: 'reading', title: 'Reading page' });
      const credentials = await this.#loadCredentials();
      const snapshot = await this.#capture.capture(tabId);
      run.url = snapshot.url;

      const session = await this.#engine.openSession(snapshot);
      run.bean = session.beanView;
      if (!session.beanView.isCoffeeProduct && !command.force) {
        await this.#publish(run, 'not_coffee', { code: 'not_coffee', title: 'No coffee detected' });
        return;
      }

      const preferences = await this.#preferencesStore.load();
      const user = await this.#loadUserContext(preferences);
      const prompt = await this.#engine.buildPrompt(session, this.#promptOptionsFor(preferences), user);
      await this.#publish(run, 'consulting', { code: 'consulting', title: 'Consulting the swarm' });
      const response = await this.#aiClient.generate(prompt, credentials);
      const profile = await this.#engine.normalize(session, response, user);
      if (!profile.success) {
        throw new CupanoError(new StatusMessage({
          code: 'swarm_unreadable', title: 'Could not read the swarm answer', detail: profile.errorMessage,
        }));
      }
      await this.#publish(run, 'done', { code: 'done', title: 'Done' }, profile);
    } catch (error) {
      await this.#publish(run, 'error', this.#toStatus(error));
    } finally {
      release();
      this.#running.delete(tabId);
    }
  }

  /** Drops state for a closed tab. */
  async forget(tabId) {
    await this.#stateStore.remove(tabId);
  }

  async #publish(run, phase, status, profile = null) {
    await this.#stateStore.write(new TabAnalysisState({
      tabId: run.tabId,
      url: run.url,
      phase,
      status,
      bean: run.bean,
      profile,
      startedAt: run.startedAt,
      updatedAt: this.#now(),
    }));
    await this.#badge.show(run.tabId, phase);
  }

  async #loadCredentials() {
    const credentials = (await this.#settingsStore.load()).activeCredentials;
    if (!credentials.isConfigured) {
      throw new CupanoError(new StatusMessage({
        code: 'key_required', params: { provider: credentials.provider }, title: 'API key required',
      }));
    }
    return credentials;
  }

  /** The swarm writes its prose in the user's chosen language. */
  #promptOptionsFor(preferences) {
    return new PromptOptions({
      outputLocale: resolveLanguage(preferences.language, this.#promptOptions.outputLocale),
      notesPerAgent: this.#promptOptions.notesPerAgent,
      sourcesPerAgent: this.#promptOptions.sourcesPerAgent,
    });
  }

  async #loadUserContext(preferences) {
    const country = await this.#countryResolver.resolve(preferences);
    return new UserContextView({
      countryCode: country.code,
      countryName: country.name,
      preferredProcesses: preferences.preferredProcesses,
      tastePreference: preferences.tastePreference,
    });
  }

  #toStatus(error) {
    if (error instanceof CupanoError) return error.status;
    return new StatusMessage({ code: 'unexpected', title: 'Unexpected error', detail: String(error?.message ?? error) });
  }
}
