# tree-sitter-stac

A [tree-sitter](https://tree-sitter.github.io) grammar for stac. Its tokens follow `src/engine/Parser.cpp`.

- `grammar.js`: the grammar. `src/scanner.c` ends an `import` at its line's end.
- `queries/highlights.scm`: highlights, in nvim-treesitter's capture names. The REPL compiles it in (`src/engine/Highlight.cpp`).
- `src/parser.c` and the rest of `src/`, except `scanner.c`, are generated and committed, so building stac needs no tree-sitter CLI.
- `test/corpus/`: `tree-sitter test` cases.

```sh
make grammar        # from the repository root: tree-sitter generate, then tree-sitter test
```

`tests/py/test_grammar.py` fails when `src/` is stale. `tests/unit/test_highlight.cpp` parses every stac file and documentation example and fails on an `ERROR` node.
