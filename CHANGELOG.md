# Changelog

## [Unreleased]

### Fixed

- `skipWhile` with a predicate function returned wrong values on `Z` lists when the cutoff fell mid-chunk. The tail copy offset the source by `i * sizeof(V)` bytes instead of `i * sizeof(Z)`, so it copied the wrong elements and could read past the end of the source buffer.
- The `sapf` executable linked `libsapf.a` twice, which made Apple ld warn `ignoring duplicate libraries`. It now links only `sapf_server` when present, and gets the engine through it.

### Removed

- Dead `kOneThird` and `NCubicLinez` in `StreamOps.cpp`, left over when `line` and `xline` moved to scsynth UGens.
