#!/usr/bin/env bash
# publish.sh: builds cupano and writes store submission packages.
#
#   publish/chrome/   cupano-chrome-<version>.zip          Chrome Web Store upload
#                     cupano-chrome-<version>/             same files, unpacked (for a last local check)
#   publish/firefox/  cupano-firefox-<version>.zip         AMO upload
#                     cupano-firefox-<version>-source.zip  source for AMO review (Wasm + minified polyfill)
#                     cupano-firefox-<version>/            same files, unpacked
#
# Each browser gets its own manifest: Chrome drops the Firefox-only keys
# (browser_specific_settings, background.scripts) and Firefox drops the
# Chrome-only ones (minimum_chrome_version, background.service_worker), so
# neither store reports unknown-key warnings.
#
# Usage: ./publish.sh            build and package
#        EMCC=/path/to/emcc ./publish.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

OUT="$ROOT/publish"
# Files that make up the installable extension (no C++ sources, no tooling).
EXTENSION_FILES=(icons src/background src/popup src/shared src/options.html src/options.js src/options.css wasm vendor)
# Files a reviewer needs to rebuild the extension from source.
SOURCE_FILES=(manifest.json icons src Makefile package.json package-lock.json publish.sh LICENSE README.md README.ko.md PRIVACY.md .gitignore)

step() { printf '\n==> %s\n' "$*"; }
fail() { printf 'publish.sh: %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------
step "Checking tools"
for tool in node npm zip; do
  command -v "$tool" >/dev/null || fail "'$tool' is required but was not found."
done
EMCC="${EMCC:-emcc}"
command -v "$EMCC" >/dev/null || fail "Emscripten ('$EMCC') was not found. Activate emsdk first (source emsdk_env.sh)."

VERSION="$(node -p "require('./manifest.json').version")"
PACKAGE_VERSION="$(node -p "require('./package.json').version")"
[[ "$VERSION" == "$PACKAGE_VERSION" ]] \
  || fail "manifest.json ($VERSION) and package.json ($PACKAGE_VERSION) versions differ. Bump both."
echo "cupano $VERSION"

# ---------------------------------------------------------------------------
step "Building from a clean state"
if [[ ! -d node_modules/webextension-polyfill ]]; then
  npm ci --no-audit --no-fund
fi
make clean >/dev/null
make EMCC="$EMCC" all

# MV3 forbids eval-style code; the Wasm glue must stay free of it.
if grep -qE '\beval\(|new Function\(' wasm/cupano_core.js; then
  fail "wasm/cupano_core.js contains eval()/new Function(); check DYNAMIC_EXECUTION=0 in the Makefile."
fi

# ---------------------------------------------------------------------------
# write_manifest <browser> <destination>: per-browser copy of manifest.json.
write_manifest() {
  node - "$1" "$2" <<'NODE'
const fs = require('fs');
const [browser, destination] = process.argv.slice(2);
const manifest = JSON.parse(fs.readFileSync('manifest.json', 'utf8'));
const background = manifest.background ?? {};
if (browser === 'chrome') {
  delete manifest.browser_specific_settings;
  manifest.background = { service_worker: background.service_worker, type: background.type };
} else {
  delete manifest.minimum_chrome_version;
  manifest.background = { scripts: background.scripts, type: background.type };
}
fs.writeFileSync(destination, `${JSON.stringify(manifest, null, 2)}\n`);
NODE
}

# zip_dir <directory> <zip>: zips a directory's contents (not the directory itself).
zip_dir() {
  rm -f "$2"
  (cd "$1" && zip -r -X -q "$2" . -x '*.DS_Store')
  zip -T -q "$2" >/dev/null || fail "zip integrity check failed for $2"
}

# stage <browser>: copies the extension files and that browser's manifest.
stage() {
  local browser="$1"
  local dir="$OUT/$browser/cupano-$browser-$VERSION"
  rm -rf "$OUT/$browser"
  mkdir -p "$dir"
  for path in "${EXTENSION_FILES[@]}"; do
    mkdir -p "$dir/$(dirname "$path")"
    cp -R "$path" "$dir/$path"
  done
  write_manifest "$browser" "$dir/manifest.json"
  zip_dir "$dir" "$OUT/$browser/cupano-$browser-$VERSION.zip"
}

# ---------------------------------------------------------------------------
step "Packaging for Chrome"
stage chrome

step "Packaging for Firefox"
stage firefox

step "Packaging Firefox review source"
SOURCE_DIR="$(mktemp -d)"
trap 'rm -rf "$SOURCE_DIR"' EXIT
SOURCE_ROOT="$SOURCE_DIR/cupano-$VERSION-source"
mkdir -p "$SOURCE_ROOT"
for path in "${SOURCE_FILES[@]}"; do
  mkdir -p "$SOURCE_ROOT/$(dirname "$path")"
  cp -R "$path" "$SOURCE_ROOT/$path"
done

EMCC_VERSION="$("$EMCC" --version | head -n 1)"
POLYFILL_VERSION="$(node -p "require('./node_modules/webextension-polyfill/package.json').version")"
cat > "$SOURCE_ROOT/BUILD.md" <<EOF
# Building cupano $VERSION from source

The add-on contains two non-handwritten files:

- \`wasm/cupano_core.js\` and \`wasm/cupano_core.wasm\`: compiled with Emscripten from the C++ sources in \`src/cpp/\`.
- \`vendor/browser-polyfill.min.js\`: copied unmodified from the npm package \`webextension-polyfill@$POLYFILL_VERSION\`.

Everything else under \`src/\`, \`icons/\` and \`manifest.json\` is shipped as written.

## Tools used for this build

- $EMCC_VERSION
- Node.js $(node --version), npm $(npm --version)
- GNU Make, zip
- OS: $(uname -sr)

## Steps

\`\`\`sh
# 1. Install Emscripten $( "$EMCC" --version | head -n 1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n 1 ) (https://emscripten.org/docs/getting_started/downloads.html)
#    and activate it:  source /path/to/emsdk/emsdk_env.sh
# 2. Build and package:
./publish.sh
\`\`\`

\`publish.sh\` runs \`npm ci\` (only to fetch webextension-polyfill), \`make\` (compiles \`src/cpp\` to
\`wasm/\` and copies the polyfill to \`vendor/\`), then writes the Firefox package to
\`publish/firefox/cupano-firefox-$VERSION.zip\`. Its \`wasm/\` and \`vendor/\` contents match the submitted add-on.

To build only the Wasm core: \`make wasm\`.
EOF
zip_dir "$SOURCE_DIR" "$OUT/firefox/cupano-firefox-$VERSION-source.zip"

# ---------------------------------------------------------------------------
step "Done"
for file in "$OUT"/chrome/*.zip "$OUT"/firefox/*.zip; do
  printf '  %-60s %8s\n' "${file#"$ROOT"/}" "$(du -h "$file" | cut -f1)"
done
cat <<EOF

Next:
  Chrome   upload publish/chrome/cupano-chrome-$VERSION.zip at https://chrome.google.com/webstore/devconsole
  Firefox  upload publish/firefox/cupano-firefox-$VERSION.zip at https://addons.mozilla.org/developers/
           and attach publish/firefox/cupano-firefox-$VERSION-source.zip when asked for source code.
EOF
