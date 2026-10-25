/**
 * @file tab-state-store.js
 * @brief Per-tab analysis state in browser.storage.session.
 *
 * The background service is the only writer; the popup only reads and
 * observes. Session storage is in memory and cleared when the browser closes.
 */
import { TabAnalysisState } from './dto.js';

export class TabStateStore {
  static #PREFIX = 'cupanoTab:';

  #area;

  constructor(sessionStorageArea) {
    this.#area = sessionStorageArea;
  }

  static keyFor(tabId) {
    return `${TabStateStore.#PREFIX}${tabId}`;
  }

  async read(tabId) {
    const key = TabStateStore.keyFor(tabId);
    const stored = await this.#area.get(key);
    return TabAnalysisState.fromRecord(stored?.[key]);
  }

  async write(state) {
    await this.#area.set({ [TabStateStore.keyFor(state.tabId)]: state.toRecord() });
  }

  async remove(tabId) {
    await this.#area.remove(TabStateStore.keyFor(tabId));
  }

  /** Extracts this tab's new state from a storage.onChanged payload, if present. */
  static changeFor(tabId, changes) {
    const change = changes?.[TabStateStore.keyFor(tabId)];
    return change ? { present: true, state: TabAnalysisState.fromRecord(change.newValue) } : { present: false, state: null };
  }
}
