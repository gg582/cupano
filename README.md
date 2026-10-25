# cupano

cupano is a browser extension for Chrome and Firefox that compares reviews of the same coffee bean across six coffee cultures: Korea, Japan, Taiwan, the English-speaking world, Norway and Germany. It shows which tasting notes are shared across regions and which are regional or personal. Few people read six languages, so coffee reviews tend to stay isolated by region. cupano reads across them for you.

[한국어 README](README.ko.md)

## How it works

Open a coffee bean's product page or search for it on Google, Naver or another search engine, click the cupano icon and press Analyze. cupano reads the page, then asks your AI provider to run six regional agents, each searching in its own language. The answers come back to a WebAssembly core written in C++, which counts how often each note appears across regions, places the notes on the SCA / WCR Coffee Taster's Flavor Wheel, maps every roaster's roast wording onto one standard scale, and checks whether the bean is sold out worldwide or in your country. If you describe what you like, it also scores how well the bean fits your taste.

The analysis runs in the background, so you can close the popup and come back to the result later. The interface is available in English, Korean, Japanese, Traditional Chinese, Norwegian and German.

## API key

cupano uses your own Google Gemini or OpenAI API key. A Gemini key is free and can be created at https://aistudio.google.com/api-keys. Add it in the popup under ⚙ and then Advanced.

## Privacy

cupano has no servers of its own. Page text goes straight from your browser to the AI provider you chose, and an IP-based lookup detects your country unless you pin one. Your key stays in your browser. The full policy is in [PRIVACY.md](PRIVACY.md).

## Building from source

You need Emscripten 5.0.0 or later, Node.js with npm, GNU Make and zip.

```sh
source /path/to/emsdk/emsdk_env.sh
npm ci
make            # builds wasm/ from src/cpp and copies the polyfill to vendor/
```

Load the project folder as an unpacked extension in `chrome://extensions` with Developer mode on, or through `about:debugging` in Firefox.

To produce store packages, run `./publish.sh`. It writes the Chrome upload to `publish/chrome/` and the Firefox upload, plus the source archive AMO reviewers ask for, to `publish/firefox/`.

## Project layout

`src/cpp` holds the C++ core, compiled to WebAssembly with embind: page text sanitizing, the swarm prompt, response parsing, note and flavor wheel frequency analysis, roast consensus and stock checks. `src/background` runs the analysis in the extension's background context. `src/popup` is the toolbar popup, `src/shared` holds code used by both, and `src/options.*` is the standalone options page.

## Credits

The flavor wheel follows the Coffee Taster's Flavor Wheel (2016) by the Specialty Coffee Association and World Coffee Research. Only its taxonomy of names is used. cupano is an independent project and is not affiliated with SCA, WCR, Google or OpenAI.

## License

GPL v2. See [LICENSE](LICENSE).
