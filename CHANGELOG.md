# Changelog

## [Unreleased]

### Added

- `scripts/stactool.py`, a formatter and linter for sapf source. Its lexer ports `Parser.cpp`'s token rules and keeps comments, so formatting changes only indentation and trailing whitespace, and it checks that the token stream is unchanged. The linter resolves names in source order, as the compiler does, so it reports words used before they are bound. It reads built-in names from `helpall` in `build/sapf`, so its word list cannot drift from the binary.

  ```
  uv run python scripts/stactool.py fmt [--check] [--indent N|tab] FILE...
  uv run python scripts/stactool.py lint [--prelude sapf-prelude.txt] FILE...
  ```

### Fixed

- `skipWhile` with a predicate function returned wrong values on `Z` lists when the cutoff fell mid-chunk. The tail copy offset the source by `i * sizeof(V)` bytes instead of `i * sizeof(Z)`, so it copied the wrong elements and could read past the end of the source buffer.
- The `sapf` executable linked `libsapf.a` twice, which made Apple ld warn `ignoring duplicate libraries`. It now links only `sapf_server` when present, and gets the engine through it.

### Removed

- Dead `kOneThird` and `NCubicLinez` in `StreamOps.cpp`, left over when `line` and `xline` moved to scsynth UGens.
