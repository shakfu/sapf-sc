.PHONY: test defs

test:
	uv run pytest

# SynthDefs for the SSP's scsy module, with their sidecars
defs:
	uv run python poc/looper.py build/scsy
