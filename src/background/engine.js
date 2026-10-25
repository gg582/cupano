/**
 * @file engine.js
 * @brief Owns the Wasm NormalizerFacade and exposes DTO-only operations.
 */
import { BeanView, CupanoProfileView, PromptRequest } from '../shared/dto.js';

/** One extraction-to-normalization run: the Wasm bean value plus its view. */
export class AnalysisSession {
  constructor({ nativeBean, beanView }) {
    this.nativeBean = nativeBean;
    this.beanView = beanView;
    Object.freeze(this);
  }
}

/** Converts between JS DTOs and embind values, freeing every native handle. */
export class WasmMarshaller {
  toRawPageData(snapshot) {
    return {
      // A search engine URL says nothing about the bean (and would be read as the roaster).
      url: snapshot.kind === 'search' ? '' : snapshot.url,
      title: snapshot.title,
      metaDescription: snapshot.metaDescription,
      bodyText: snapshot.bodyText,
      documentLanguage: snapshot.documentLanguage,
    };
  }

  /** Builds an embind UserContext; release it with releaseUser() after the call. */
  toNativeUser(core, user) {
    const preferredProcesses = new core.VectorString();
    for (const process of user.preferredProcesses) preferredProcesses.push_back(process);
    return {
      countryCode: user.countryCode,
      countryName: user.countryName,
      preferredProcesses,
      tastePreference: user.tastePreference,
    };
  }

  releaseUser(nativeUser) {
    nativeUser.preferredProcesses.delete();
  }

  /** Consumes a FinalCupanoProfile value object, releasing every handle it owns. */
  takeProfile(nativeProfile) {
    const topNotes = this.#takeVector(nativeProfile.topNotes, (note) => ({
      ...note,
      sourceRegions: this.#takeVector(note.sourceRegions, (code) => code),
      nativeTerms: this.#takeVector(note.nativeTerms, (term) => term),
    }));
    return new CupanoProfileView({
      ...nativeProfile,
      topNotes,
      discrepancies: this.#takeVector(nativeProfile.discrepancies, (item) => item),
      flavorWheel: this.#takeVector(nativeProfile.flavorWheel, (segment) => ({
        ...segment,
        sourceRegions: this.#takeVector(segment.sourceRegions, (code) => code),
      })),
      availability: {
        ...nativeProfile.availability,
        sellers: this.#takeVector(nativeProfile.availability.sellers, (seller) => seller),
      },
    });
  }

  #takeVector(vector, mapItem) {
    const values = [];
    for (let i = 0; i < vector.size(); i += 1) values.push(mapItem(vector.get(i)));
    vector.delete();
    return values;
  }
}

/** Lazily instantiates the Emscripten module and runs the three core calls. */
export class CupanoEngine {
  #createCore;
  #marshaller;
  #corePromise = null;

  constructor(createCore, marshaller) {
    this.#createCore = createCore;
    this.#marshaller = marshaller;
  }

  async openSession(snapshot) {
    const { facade } = await this.#core();
    const nativeBean = facade.extractMetadata(this.#marshaller.toRawPageData(snapshot));
    return new AnalysisSession({ nativeBean, beanView: new BeanView(nativeBean) });
  }

  async buildPrompt(session, options, user) {
    const { core, facade } = await this.#core();
    const nativeUser = this.#marshaller.toNativeUser(core, user);
    try {
      return new PromptRequest(facade.buildSwarmPrompt({
        bean: session.nativeBean,
        user: nativeUser,
        notesPerAgent: options.notesPerAgent,
        sourcesPerAgent: options.sourcesPerAgent,
        outputLocale: options.outputLocale,
      }));
    } finally {
      this.#marshaller.releaseUser(nativeUser);
    }
  }

  async normalize(session, response, user) {
    const { core, facade } = await this.#core();
    const nativeUser = this.#marshaller.toNativeUser(core, user);
    try {
      return this.#marshaller.takeProfile(facade.normalizeResponse({
        bean: session.nativeBean,
        user: nativeUser,
        rawResponseText: response.text,
      }));
    } finally {
      this.#marshaller.releaseUser(nativeUser);
    }
  }

  #core() {
    // Instantiated once per background lifetime, on the first analysis.
    this.#corePromise ??= this.#createCore().then((core) => ({ core, facade: new core.NormalizerFacade() }));
    return this.#corePromise;
  }
}
