/**
 * @file run-support.js
 * @brief Small helpers the analysis service drives: keep-alive and badge.
 */

/**
 * Keeps an MV3 background context alive during a long model call. Chrome may
 * stop an idle service worker after ~30 s; any extension API call resets that
 * timer, so a cheap call every 20 s covers a multi-minute swarm request.
 */
export class KeepAlive {
  static #INTERVAL_MS = 20000;

  #runtime;
  #timers;

  constructor(runtimeApi, timers) {
    this.#runtime = runtimeApi;
    this.#timers = timers;
  }

  /** Starts pinging; returns the function that stops it. */
  hold() {
    const id = this.#timers.setInterval(() => { this.#runtime.getPlatformInfo().catch(() => {}); }, KeepAlive.#INTERVAL_MS);
    return () => this.#timers.clearInterval(id);
  }
}

/** Reflects a tab's analysis phase on the toolbar icon badge. */
export class BadgePresenter {
  static #STYLES = Object.freeze({
    reading: { text: '…', color: '#8a4b2a' },
    consulting: { text: '…', color: '#8a4b2a' },
    done: { text: '✓', color: '#2e7d32' },
    not_coffee: { text: '', color: '#8a4b2a' },
    error: { text: '!', color: '#b3261e' },
  });

  #action;

  constructor(actionApi) {
    this.#action = actionApi;
  }

  async show(tabId, phase) {
    const style = BadgePresenter.#STYLES[phase] ?? BadgePresenter.#STYLES.error;
    try {
      await this.#action.setBadgeBackgroundColor({ tabId, color: style.color });
      await this.#action.setBadgeText({ tabId, text: style.text });
    } catch {
      // The tab may have closed meanwhile; the badge is cosmetic.
    }
  }
}
