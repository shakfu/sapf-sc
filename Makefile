.PHONY: all build test grammar reference server editor clean

all: build

build:
	@cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Wno-dev > /dev/null
	@cmake --build build

# Language and unit tests (ctest), then SCgf parity and scsynth conformance (pytest, needs uv)
test: build
	@cd build && ctest --output-on-failure
	@uv run pytest -q

# Regenerates tree-sitter-stac/src from grammar.js and runs its corpus; needs the tree-sitter CLI
grammar:
	@cd tree-sitter-stac && tree-sitter generate && tree-sitter test

# Regenerates tests/fixtures/ref with original sapf: make reference SAPF_ORIG=/path/to/sapf
reference:
	@SAPF_ORIG=$(SAPF_ORIG) uv run python scripts/make_reference.py

# scsynth on 127.0.0.1:57110 for play and stop, from nanosynth: make server [PORT=57110]
PORT ?= 57110
server:
	@uv run python scripts/server.py --port $(PORT)

# The CLAP plugin's editor in a minimal host, with audio to the default output
editor: build
	@./build/examples/clap/stac_dev_host

clean:
	@rm -rf build
