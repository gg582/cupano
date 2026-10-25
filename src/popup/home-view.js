/**
 * @file home-view.js
 * @brief The main popup screen: page card, the big Analyze orb, live swarm
 *        progress, and the finished profile.
 */

/** The active tab as the popup sees it. */
export class TabInfo {
  constructor({ id, url, title }) {
    this.id = Number(id) || 0;
    this.url = String(url ?? '');
    this.title = String(title ?? '');
    Object.freeze(this);
  }

  get host() {
    try {
      return new URL(this.url).hostname.replace(/^www\./, '');
    } catch {
      return '';
    }
  }
}

/** Everything the home screen needs to render once. */
export class HomeViewModel {
  constructor({ tab, state, keyConfigured, provider, providerLabel, modelLabel, now }) {
    this.tab = tab;
    this.state = state;
    this.keyConfigured = Boolean(keyConfigured);
    this.provider = String(provider ?? '');
    this.providerLabel = String(providerLabel ?? '');
    this.modelLabel = String(modelLabel ?? '');
    this.now = Number(now) || 0;
    Object.freeze(this);
  }
}

/** Buttons the controller wires after a render; absent ones are null. */
export class HomeHandles {
  constructor({ analyzeButton = null, forceButton = null, keyButton = null }) {
    this.analyzeButton = analyzeButton;
    this.forceButton = forceButton;
    this.keyButton = keyButton;
    Object.freeze(this);
  }
}

export class HomeView {
  /** Where Gemini keys are issued; shown whenever a Gemini key is missing or rejected. */
  static GEMINI_KEY_URL = 'https://aistudio.google.com/api-keys';

  static #AGENTS = Object.freeze([
    ['KO', '한국어'], ['JA', '日本語'], ['TW', '繁中'], ['EN', 'English'], ['NO', 'Norsk'], ['DE', 'Deutsch'],
  ]);

  #dom;
  #profileView;
  #timers;
  #i18n;
  #clockId = null;

  constructor(dom, profileView, timers, translator) {
    this.#dom = dom;
    this.#profileView = profileView;
    this.#timers = timers;
    this.#i18n = translator;
  }

  /** Renders into container and returns the buttons to wire. */
  render(container, model) {
    this.#stopClock();
    const state = model.state;
    if (state?.phase === 'done' && state.profile) return this.#renderProfile(container, model);

    const running = Boolean(state?.isRunning);
    const nodes = [this.#pageCard(model)];
    const orb = this.#orb(running, state);
    nodes.push(orb.node);

    if (running) {
      nodes.push(this.#progress(state, model.now));
    } else if (state?.phase === 'not_coffee') {
      nodes.push(this.#notice('info', state.status));
    } else if (state?.phase === 'error') {
      nodes.push(this.#notice('error', state.status));
      if (this.#isGeminiKeyProblem(state.status, model)) nodes.push(this.#geminiKeyLink());
    } else {
      nodes.push(this.#agentStrip(false));
    }

    let forceButton = null;
    if (state?.phase === 'not_coffee') {
      forceButton = this.#dom.button('ghost wide', this.#i18n.t('home.analyzeAnyway'));
      nodes.push(forceButton);
    }
    const provider = this.#providerLine(model);
    nodes.push(provider.node);

    container.replaceChildren(...nodes);
    return new HomeHandles({ analyzeButton: orb.button, forceButton, keyButton: provider.keyButton });
  }

  #renderProfile(container, model) {
    const { profile } = model.state;
    const head = this.#dom.element('div', 'result-head');
    const titles = this.#dom.element('div', 'result-titles');
    titles.append(
      this.#dom.element('p', 'eyebrow', model.tab.host || this.#i18n.t('home.coffeeProfile')),
      this.#dom.element('h1', 'result-title',
        profile.bean.productName || model.tab.title || this.#i18n.t('home.coffeeProfile')),
    );
    const rerun = this.#dom.button('ghost small', this.#i18n.t('home.reanalyze'));
    rerun.title = this.#i18n.t('home.reanalyzeTitle');
    head.append(titles, rerun);
    const body = this.#dom.element('div', 'result');
    body.append(...this.#profileView.build(profile));
    container.replaceChildren(head, body);
    return new HomeHandles({ analyzeButton: rerun });
  }

  #pageCard(model) {
    const card = this.#dom.element('div', 'page-card');
    const bean = model.state?.bean;
    card.append(
      this.#dom.element('p', 'eyebrow', model.tab.host || this.#i18n.t('home.thisTab')),
      this.#dom.element('p', 'page-title', bean?.productName || model.tab.title || this.#i18n.t('home.untitled')),
    );
    return card;
  }

  #orb(running, state) {
    const wrap = this.#dom.element('div', 'orb-wrap');
    const button = this.#dom.button(`orb${running ? ' spinning' : ''}`);
    button.disabled = running;
    const t = (key) => this.#i18n.t(key);
    const label = running ? t('home.analyzing') : t(state?.phase === 'error' ? 'home.tryAgain' : 'home.analyze');
    const sub = running ? t(state.phase === 'consulting' ? 'home.sub.consulting' : 'home.sub.reading') : t('home.sub.idle');
    const ring = this.#dom.element('span', 'orb-ring');
    ring.setAttribute('aria-hidden', 'true');
    const face = this.#dom.element('span', 'orb-face');
    face.append(this.#dom.element('span', 'orb-icon', '☕'), this.#dom.element('span', 'orb-label', label),
      this.#dom.element('span', 'orb-sub', sub));
    button.append(ring, face);
    wrap.append(button);
    return { node: wrap, button };
  }

  #progress(state, now) {
    const block = this.#dom.element('div', 'progress');
    const clock = this.#dom.element('span', 'clock', this.#elapsed(now - state.startedAt));
    const head = this.#dom.element('div', 'row');
    head.append(this.#dom.element('b', '', this.#statusTitle(state.status)), clock);
    block.append(head, this.#dom.element('p', 'muted small', this.#statusDetail(state.status)));
    block.append(this.#agentStrip(state.phase === 'consulting'));
    block.append(this.#dom.element('p', 'hint', this.#i18n.t('home.closeHint')));
    // Purely visual ticking clock; the state itself comes from storage.
    const started = state.startedAt;
    const base = now;
    const t0 = Date.now();
    this.#clockId = this.#timers.setInterval(() => {
      clock.textContent = this.#elapsed(base + (Date.now() - t0) - started);
    }, 1000);
    return block;
  }

  #agentStrip(active) {
    const strip = this.#dom.element('div', `agents${active ? ' active' : ''}`);
    for (const [code, name] of HomeView.#AGENTS) {
      const chip = this.#dom.element('span', 'agent');
      chip.append(this.#dom.element('b', '', code), this.#dom.element('span', '', name));
      strip.append(chip);
    }
    return strip;
  }

  #notice(tone, status) {
    const box = this.#dom.element('div', `notice-box ${tone}`);
    box.append(this.#dom.element('b', '', this.#statusTitle(status)));
    const detail = this.#statusDetail(status);
    if (detail) box.append(this.#dom.element('p', 'small', detail));
    return box;
  }

  /** Translated title for a status code, falling back to the stored English title. */
  #statusTitle(status) {
    const key = `status.${status.code}.title`;
    return this.#i18n.has(key) ? this.#i18n.t(key, status.params) : status.title;
  }

  /** Translated explanation plus any raw detail (API messages are not translatable). */
  #statusDetail(status) {
    const key = `status.${status.code}.detail`;
    const explanation = this.#i18n.has(key) ? this.#i18n.t(key, status.params) : '';
    if (!status.detail) return explanation;
    return explanation ? `${explanation} (${status.detail})` : status.detail;
  }

  #isGeminiKeyProblem(status, model) {
    if (status.code === 'key_required') return model.provider === 'gemini';
    return status.code === 'api_error' && status.params.provider === 'gemini'
      && ['400', '401', '403'].includes(status.params.status);
  }

  #geminiKeyLink() {
    const link = this.#dom.externalLink(HomeView.GEMINI_KEY_URL, `${this.#i18n.t('home.getGeminiKey')} ↗`);
    link.className = 'key-link';
    return link;
  }

  #providerLine(model) {
    const line = this.#dom.element('div', 'provider-line');
    if (!model.keyConfigured) {
      const keyButton = this.#dom.button('key-warning', this.#i18n.t('home.addKey'));
      line.append(keyButton);
      if (model.provider === 'gemini') line.append(this.#geminiKeyLink());
      return { node: line, keyButton };
    }
    line.append(this.#dom.element('span', 'dot'), this.#dom.element('span', '', `${model.providerLabel} · ${model.modelLabel}`));
    return { node: line, keyButton: null };
  }

  #elapsed(ms) {
    const seconds = Math.max(0, Math.floor(ms / 1000));
    return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, '0')}`;
  }

  #stopClock() {
    if (this.#clockId !== null) this.#timers.clearInterval(this.#clockId);
    this.#clockId = null;
  }
}
