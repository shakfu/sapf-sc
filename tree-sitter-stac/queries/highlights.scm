; stac highlights. Capture names follow nvim-treesitter's; the REPL maps them in src/engine/Highlight.cpp.
; Earlier patterns win where captures overlap, as the REPL applies them.

(comment) @comment
(string) @string
(number) @number
(stack_effect) @number

[
  "def"
  "synthdef"
  "const"
  "import"
] @keyword

[
  "="
  "-->"
  "..."
  "\\"
  (each)
] @operator

[
  "["
  "]"
  "#["
  "{"
  "}"
  "("
  ")"
] @punctuation.bracket

[
  "."
  ","
  ":"
  "'"
  "`"
] @punctuation.delimiter

(def_statement name: (word) @function)
(synthdef_statement name: (word) @function)
(const_statement name: (word) @constant)
(def_statement parameter: (word) @variable.parameter)
(lambda parameter: (word) @variable.parameter)
(control name: (word) @variable.parameter)
(results type: (word) @type)
(binding name: (word) @variable)

(import_item namespace: (word) @module)
(import_item member: (word) @function)

(quote name: (word) @string.special.symbol)
(backquote name: (word) @function)
(key name: (word) @property)
(field name: (word) @property)
(member name: (word) @function.method.call)

; A word of operator characters only, such as + */ or +\i.
((word) @operator
  (#match? @operator "^[-+*/%<>=!&|^$~?\\\\]+i?$"))

(word) @function.call
