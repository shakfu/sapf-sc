.PHONY: all build test reference clean

all: build

build:
	@cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
	@cmake --build build

# Language and unit tests (ctest), then SCgf parity and scsynth conformance (pytest, needs uv)
test: build
	@cd build && ctest --output-on-failure
	@uv run pytest -q

# Regenerates tests/fixtures/ref with original sapf: make reference SAPF_ORIG=/path/to/sapf
reference:
	@SAPF_ORIG=$(SAPF_ORIG) uv run python scripts/make_reference.py

clean:
	@rm -rf build
