# cupano — Wasm core build.
#
#   make          Build wasm/cupano_core.{js,wasm} and vendor the polyfill.
#   make wasm     Build only the Wasm core.
#   make vendor   Copy webextension-polyfill from node_modules (run `npm install` first).
#   make package  Store packages for Chrome and Firefox in publish/ (runs ./publish.sh).
#   make clean    Remove build outputs.
#
# Flags beyond the required minimal set, and why they matter for MV3:
#   DYNAMIC_EXECUTION=0  Embind must not use eval()/new Function(); MV3 CSP forbids them.
#   ENVIRONMENT=web,worker  Drop Node.js shims; runs in Chrome's service worker and Firefox's event page.
#   FILESYSTEM=0         No virtual FS is needed, which keeps the glue small.
#   ALLOW_MEMORY_GROWTH  Page bodies of varying size are copied into the heap.

EMCC      ?= emcc
CPP_DIR   := src/cpp
SRC       := $(shell find $(CPP_DIR) -name '*.cpp' | sort)
HDR       := $(shell find $(CPP_DIR) -name '*.hpp' | sort)
OUT_DIR   := wasm
OUT_JS    := $(OUT_DIR)/cupano_core.js
POLYFILL  := vendor/browser-polyfill.min.js

CXXFLAGS  := -std=c++17 -O3 -Wall -Wextra -I$(CPP_DIR)
EMFLAGS   := -s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=createCupanoCore \
             -s ENVIRONMENT=web,worker -s DYNAMIC_EXECUTION=0 -s FILESYSTEM=0 -s ALLOW_MEMORY_GROWTH=1 \
             -lembind

.PHONY: all wasm vendor package clean

all: wasm vendor

wasm: $(OUT_JS)

$(OUT_JS): $(SRC) $(HDR) Makefile
	@mkdir -p $(OUT_DIR)
	$(EMCC) $(CXXFLAGS) $(EMFLAGS) $(SRC) -o $(OUT_JS)

vendor: $(POLYFILL)

$(POLYFILL): node_modules/webextension-polyfill/dist/browser-polyfill.min.js
	@mkdir -p vendor
	cp $< $@

node_modules/webextension-polyfill/dist/browser-polyfill.min.js:
	@echo "webextension-polyfill missing: run 'npm install' first." && exit 1

package:
	./publish.sh

clean:
	rm -rf $(OUT_DIR) vendor dist publish
