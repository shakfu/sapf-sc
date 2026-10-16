/**
 * @file tree-sitter grammar for stac, the language of sapf-sc.
 * @license GPL-3.0-or-later
 *
 * The tokens follow src/engine/Parser.cpp, as scripts/stactool.py's lexer does. A word runs to the
 * next space or one of ;()[]{}.`,:" and a number is a word that parseNumber reads in full, so
 * longest match decides between them: `10ms` is a word, `10m` a number. At equal length the number
 * wins, as the earlier rule: `1/12`, `2pi`, `-3`.
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const FLOAT = /pi|[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d*)?(pi|[Mkhcmu])?/;
const float = FLOAT.source;

module.exports = grammar({
  name: 'stac',

  extras: $ => [/[\s\x00-\x1f\x7f]/, $.comment],

  // An import's items run to the end of its line (parseImport); only the scanner sees newlines.
  externals: $ => [$._import_end],

  word: $ => $.word,

  rules: {
    source_file: $ => repeat($._item),

    _item: $ => choice(
      $.def_statement,
      $.synthdef_statement,
      $.const_statement,
      $.import_statement,
      $.binding,
      $._element,
    ),

    _element: $ => choice(
      $.number,
      $.string,
      $.word,
      prec(-1, alias('synthdef', $.word)),  // synthdef.new: the namespace, not the statement
      $.quote,
      $.backquote,
      $.member,
      $.field,
      $.key,
      $.each,
      $.lambda,
      $.list,
      $.signal,
      $.form,
      $.parens,
    ),

    comment: _ => token(seq(';', /[^\n]*/)),

    // Declared before word: at equal length the earlier rule wins. A precedence would beat length.
    number: _ => token(choice(
      /0x[0-9a-fA-F]*/,
      new RegExp(`(${float})(/(${float}))?`),
    )),

    // "" is a quote; \\ and the next character are an escape (parseString).
    string: _ => token(seq('"', repeat(choice(/[^"\\]/, '""', /\\\\[\s\S]/, '\\')), '"')),

    word: _ => /[^\s\x00-\x1f\x7f;()\[\]{}.`,:"@\\'#][^\s\x00-\x1f\x7f;()\[\]{}.`,:"]*/,

    // 'name pushes a symbol; `name pushes a value unapplied.
    quote: $ => seq("'", field('name', $.word)),
    backquote: $ => seq('`', field('name', $.word)),
    // .name applies a member; ,name reads it; :name starts a key in a form.
    member: $ => seq('.', field('name', $.word)),
    field: $ => seq(',', field('name', $.word)),
    key: $ => seq(':', field('name', $.word)),

    each: _ => token(seq('@', optional(choice(/@+/, /[2-9]/, /[01]+/)))),

    list: $ => seq('[', repeat($._item), ']'),
    signal: $ => seq('#[', repeat($._item), ']'),
    form: $ => seq('{', repeat($._item), '}'),
    parens: $ => seq('(', repeat($._item), ')'),

    binding: $ => seq('=', choice(
      field('name', $.word),
      seq('(', repeat1(field('name', $.word)), ')'),
      seq('[', repeat1(field('name', $.word)), ']'),
    )),

    // \ARG* [HELP] [ BODY ]; an argument may carry a stack effect, TAKES.LEAVES.
    lambda: $ => seq(
      '\\',
      repeat(seq(field('parameter', $.word), optional(field('effect', alias($.number, $.stack_effect))))),
      optional(field('help', $.string)),
      field('body', $.list),
    ),

    // def NAME PARAM* [--> RESULT*] [HELP] [ BODY ]
    def_statement: $ => seq(
      'def',
      field('name', $.word),
      repeat(field('parameter', $.word)),
      optional($.results),
      optional(field('help', $.string)),
      field('body', $.list),
    ),

    results: $ => seq('-->', choice('...', repeat(field('type', $.word)))),

    // synthdef NAME CONTROL* [HELP] [ BODY ]; CONTROL is name or name:LITERAL
    synthdef_statement: $ => seq(
      'synthdef',
      field('name', $.word),
      repeat(field('control', $.control)),
      optional(field('help', $.string)),
      field('body', $.list),
    ),

    control: $ => seq(field('name', $.word), optional(seq(':', field('default', $.number)))),

    // const NAME [HELP] [ BODY ]
    const_statement: $ => seq(
      'const',
      field('name', $.word),
      optional(field('help', $.string)),
      field('body', $.list),
    ),

    // import NS.* NS.name NS ... to the end of the line
    import_statement: $ => seq('import', repeat1($.import_item), $._import_end),

    import_item: $ => seq(
      field('namespace', $.word),
      optional(seq(token.immediate('.'), field('member', alias(token.immediate(/[^\s\x00-\x1f\x7f;()\[\]{}.`,:"]+/), $.word)))),
    ),
  },
});
