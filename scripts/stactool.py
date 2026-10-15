"""Formatter and linter for sapf-sc source, the syntax stac keeps.

    stactool.py fmt [--check] [--indent N|tab] FILE...
    stactool.py lint [--stac PATH] [--prelude PATH]... FILE...

The lexer ports the token rules of src/engine/Parser.cpp and keeps whitespace
and comments, so source -> tokens -> source is exact, and reads stac's
`def` statement as the parser does.

Errors, each one the compiler also rejects:
    E1 unterminated string         E6 bad lambda header
    E2 unclosed bracket            E7 '=' without a name
    E3 unexpected closer           E8 extra digit after '@'
    E4 prefix without a name       E9 rebinding a free variable in a lambda
    E5 '#' not followed by '['     E10 malformed def statement
Warnings:
    W1 undefined word (bound later, misspelt, or a malformed number like 10ms)
    W2 '=' hides a built-in word
    W3 a lambda binds a name and never uses it
    W4 space between a prefix and its name, as in '. free'
"""

from __future__ import annotations

import argparse
import bisect
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

NONNAME = set(';()[]{}.`,:"\n')
SPACE = set(' \t\n\v\f\r')
DIGITS = set('0123456789')
HEX = set('0123456789abcdefABCDEF')
TRIVIA = {'ws', 'nl', 'comment'}
OPENERS = {'open', 'lopen', 'bopen'}
CLOSER = {'(': ')', '[': ']', '{': '}', '#[': ']'}


@dataclass
class Token:
    kind: str
    text: str
    start: int


@dataclass
class Diag:
    line: int
    col: int
    code: str
    msg: str

    def __str__(self):
        return f'{self.line}:{self.col}: {self.code} {self.msg}'


def _end_of_word(s: str, i: int) -> bool:
    return i >= len(s) or s[i] in SPACE or s[i] in NONNAME


def _at(s: str, i: int) -> str:
    return s[i] if i < len(s) else ''


def _float_end(s: str, i: int) -> int | None:
    """End of a float at i, as parseFloat reads it, or None."""
    if s.startswith('pi', i):
        return i + 2
    if _at(s, i) in ('+', '-'):
        i += 1
    digits, sawdot = 0, False
    while True:
        c = _at(s, i)
        if c and c in DIGITS:
            digits += 1
        elif c == '.' and not sawdot:
            sawdot = True
        else:
            break
        i += 1
    if not digits:
        return None
    if _at(s, i) in ('e', 'E') and _at(s, i):
        i += 1
        if _at(s, i) in ('+', '-') and _at(s, i):
            i += 1
        while _at(s, i) and _at(s, i) in DIGITS:
            i += 1
    if s.startswith('pi', i):
        return i + 2
    if _at(s, i) and _at(s, i) in 'Mkhcmu':
        return i + 1
    return i


def number_end(s: str, i: int) -> int | None:
    """End of a number at i (float or a/b), as parseNumber reads it, or None."""
    j = _float_end(s, i)
    if j is None:
        return None
    if _at(s, j) == '/':
        k = _float_end(s, j + 1)
        return k if k is not None and _end_of_word(s, k) else None
    return j if _end_of_word(s, j) else None


class Lexer:
    """Lossless tokens plus syntax errors (E codes) for one source string."""

    def __init__(self, src: str):
        self.s = src
        self.i = 0
        self.toks: list[Token] = []
        self.errors: list[Diag] = []
        self.stack: list[tuple[str, Token]] = []
        self._lines = [0] + [m.end() for m in re.finditer('\n', src)]
        while True:
            self.trivia()
            if self.i >= len(self.s):
                break
            self.elem()
        for _, tok in self.stack:
            self.err(tok.start, 'E2', f"unclosed '{tok.text}'")

    def pos(self, offset: int) -> tuple[int, int]:
        line = bisect.bisect_right(self._lines, offset)
        return line, offset - self._lines[line - 1] + 1

    def err(self, offset: int, code: str, msg: str):
        self.errors.append(Diag(*self.pos(offset), code, msg))

    def c(self, k: int = 0) -> str:
        return _at(self.s, self.i + k)

    def emit(self, kind: str, end: int) -> Token:
        tok = Token(kind, self.s[self.i:end], self.i)
        self.toks.append(tok)
        self.i = end
        return tok

    def trivia(self):
        s = self.s
        while self.i < len(s):
            c = s[self.i]
            if c == ';':
                j = s.find('\n', self.i)
                self.emit('comment', len(s) if j < 0 else j)
            elif c == '\n':
                self.emit('nl', self.i + 1)
            elif c in SPACE or ord(c) < 32 or ord(c) == 127:
                j = self.i
                while j < len(s) and s[j] != '\n' and (s[j] in SPACE or ord(s[j]) < 32 or ord(s[j]) == 127):
                    j += 1
                self.emit('ws', j)
            else:
                return

    def symbol(self, kind: str) -> Token | None:
        """parseSymbol: skips trivia, then reads to the end of a word."""
        self.trivia()
        j = self.i
        while not _end_of_word(self.s, j):
            j += 1
        return self.emit(kind, j) if j > self.i else None

    def elem(self):
        c = self.c()
        if c in ')]}':
            self.close(c)
        elif c == '@':
            self.each()
        elif c in '([{':
            self.open(c, 'open')
        elif c == '\\':
            self.lam()
        elif c == '"':
            self.string()
        elif c in "'`,:":
            self.prefixed()
        elif c == '#':
            if self.c(1) == '[':
                self.open('#[', 'open')
            else:
                self.err(self.i, 'E5', "'#' must be followed by '['")
                self.word()
        elif c == '0' and self.c(1) == 'x':
            j = self.i + 2
            while _at(self.s, j) and _at(self.s, j) in HEX:
                j += 1
            if _end_of_word(self.s, j):
                self.emit('num', j)
            else:
                self.word()
        elif c in DIGITS or c in '+-.' or self.s.startswith('pi', self.i):
            j = number_end(self.s, self.i)
            if j is not None:
                self.emit('num', j)
            elif c == '.':
                self.prefixed()
            else:
                self.word()
        else:
            self.word()

    def word(self):
        j = self.i + 1
        while not _end_of_word(self.s, j):
            j += 1
        tok = self.emit('word', j)
        if tok.text == '=':
            tok.kind = 'bind'
            self.binding(tok)
        elif tok.text == 'def':
            tok.kind = 'def'
            self.definition(tok)

    def definition(self, kw: Token):
        """def NAME PARAM* = [ BODY ], PARAM being name or name:LITERAL (Parser.cpp, parseDef)."""
        name = self.symbol('defname')
        if not name or name.text == '=':
            self.err(kw.start, 'E10', 'expected a name after def')
            return
        while True:
            param = self.symbol('param')
            if not param:
                self.err(kw.start, 'E10', "expected '=' after the parameters of def")
                return
            if param.text == '=':
                param.kind = 'bind'
                break
            if self.c() == ':':
                colon = self.emit('prefix', self.i + 1)
                j = number_end(self.s, self.i)
                if j is None:
                    self.err(colon.start, 'E10', "expected a number after ':' in def")
                    return
                self.emit('num', j)
        self.trivia()
        if self.c() == '[':
            self.open('[', 'lopen')
        else:
            self.err(kw.start, 'E10', "expected '[' after def's '='")

    def binding(self, eq: Token):
        self.trivia()
        c = self.c()
        if c in ('(', '['):
            opener = self.open(c, 'bopen')
            names = 0
            while self.symbol('name'):
                names += 1
            self.trivia()
            if not names:
                self.err(eq.start, 'E7', f"expected a name after '= {c}'")
            if self.c() == CLOSER[c]:
                self.close(self.c())
            else:
                self.err(opener.start, 'E7', f"expected '{CLOSER[c]}' after '= {c}'")
        elif not self.symbol('name'):
            self.err(eq.start, 'E7', "expected a name after '='")

    def open(self, text: str, kind: str) -> Token:
        tok = self.emit(kind, self.i + len(text))
        self.stack.append((CLOSER[text], tok))
        return tok

    def close(self, c: str):
        tok = self.emit('close', self.i + 1)
        if self.stack and self.stack[-1][0] == c:
            self.stack.pop()
            return
        self.err(tok.start, 'E3', f"unexpected '{c}'")
        if any(closer == c for closer, _ in self.stack):
            while self.stack.pop()[0] != c:
                pass

    def prefixed(self):
        p = self.emit('prefix', self.i + 1)
        if not self.symbol('sym'):
            self.err(p.start, 'E4', f"expected a name after '{p.text}'")

    def each(self):
        s, j = self.s, self.i + 1
        c = _at(s, j)
        if c == '@':
            while _at(s, j) == '@':
                j += 1
        elif c and c in '23456789':
            j += 1
        elif c and c in '01':
            while _at(s, j) and _at(s, j) in '01':
                j += 1
        if _at(s, j) and _at(s, j) in DIGITS:
            self.err(self.i, 'E8', "unexpected extra digit after '@'")
        self.emit('each', j)

    def lam(self):
        lam = self.emit('lambda', self.i + 1)
        while self.symbol('param'):
            self.trivia()
            if self.c() and self.c() in DIGITS:
                j = self.i
                while _at(self.s, j) and _at(self.s, j) in DIGITS:
                    j += 1
                if _at(self.s, j) != '.':
                    self.err(self.i, 'E6', 'stack effect must be TAKES.LEAVES')
                else:
                    j += 1
                    while _at(self.s, j) and _at(self.s, j) in DIGITS:
                        j += 1
                self.emit('effect', j)
        self.trivia()
        if self.c() == '"':
            self.string()
            self.trivia()
        if self.c() == '[':
            self.open('[', 'lopen')
        else:
            self.err(lam.start, 'E6', "expected '[' after argument list")

    def string(self):
        s, j = self.s, self.i + 1
        while True:
            if j >= len(s):
                self.err(self.i, 'E1', 'unterminated string')
                break
            if s[j] == '\\' and _at(s, j + 1) == '\\':
                j += 3
            elif s[j] == '"':
                if _at(s, j + 1) != '"':
                    j += 1
                    break
                j += 2  # "" is a literal quote
            else:
                j += 1
        self.emit('str', min(j, len(s)))


def lex(src: str) -> Lexer:
    return Lexer(src)


# Formatter

class FormatError(Exception):
    pass


def _significant(toks: list[Token]) -> list[tuple[str, str]]:
    return [(t.kind, t.text.rstrip() if t.kind == 'comment' else t.text) for t in toks if t.kind not in ('ws', 'nl')]


def format_source(src: str, indent: str = '    ') -> str:
    """Re-indents by bracket depth and strips trailing whitespace.

    One level per line that opens brackets, however many it opens. A lambda
    header indents its continuation lines. Spacing inside a line is kept.
    """
    lx = lex(src)
    if lx.errors:
        raise FormatError('\n'.join(map(str, lx.errors)))
    groups: list[list[Token]] = [[]]
    for t in lx.toks:
        if t.kind == 'nl':
            groups.append([])
        else:
            groups[-1].append(t)
    stack: list[list[int]] = []  # per open bracket: the lines that opened it
    out = []
    for g, toks in enumerate(groups):
        while toks and toks[0].kind == 'ws':
            toks = toks[1:]
        while toks and toks[-1].kind == 'ws':
            toks = toks[:-1]
        closers = 0
        while closers < len(toks) and toks[closers].kind == 'close':
            closers += 1
        open_lines = {line for lines in stack[:len(stack) - closers] for line in lines}
        for lines in stack[len(stack) - closers:]:  # a closer aligns with its '[', not its lambda header
            open_lines |= set(lines[:-1]) - {lines[-1]}
        level = len(open_lines)
        text = ''.join(t.text.rstrip() if t.kind == 'comment' else t.text for t in toks)
        out.append(indent * level + text if text else '')
        for t in toks:
            if t.kind == 'lopen':
                stack[-1].append(g)  # the lambda or def header's entry
            elif t.kind in OPENERS or t.kind in ('lambda', 'def'):
                stack.append([g])
            elif t.kind == 'close':
                stack.pop()
    result = '\n'.join(out).rstrip('\n')
    result = result + '\n' if result else ''
    if _significant(lex(result).toks) != _significant(lx.toks):
        raise FormatError('internal error: formatting changed the token stream')
    return result


# Linter

@dataclass
class Frame:
    """One lambda scope. locals maps name -> [token, uses, bound by '=']."""
    locals: dict[str, list] = field(default_factory=dict)
    free: set[str] = field(default_factory=set)


def _looks_numeric(name: str) -> bool:
    return bool(re.match(r'[+-]?\.?\d', name))


def analyze(src: str, builtins: set[str], known: set[str] = frozenset()) -> tuple[list[Diag], set[str]]:
    """Lints src in source order, as the compiler resolves names.

    builtins are VM words; known are names bound before src runs (a prelude).
    Returns diagnostics and the workspace names src binds.
    """
    lx = lex(src)
    diags = list(lx.errors)
    workspace: set[str] = set()
    frames: list[Frame] = []
    brackets: list[str] = []
    params: list[str] = []
    defs: list[Token | None] = []  # per lambda frame: the def name it binds when it closes
    pending_def: Token | None = None
    toks = lx.toks

    def warn(tok, code, msg):
        diags.append(Diag(*lx.pos(tok.start), code, msg))

    def lookup(name: str) -> bool:
        for depth in range(len(frames) - 1, -1, -1):
            f = frames[depth]
            if name in f.locals:
                f.locals[name][1] += 1
                for inner in frames[depth + 1:]:
                    inner.free.add(name)
                return True
        return name in workspace or name in known or name in builtins

    def bind(tok: Token):
        name = tok.text
        if name in builtins:
            warn(tok, 'W2', f"'= {name}' hides a built-in word")
        if not frames:
            workspace.add(name)
            return
        f = frames[-1]
        if name in f.free:
            warn(tok, 'E9', f"'{name}' is already used in this lambda as a free variable")
            return
        f.locals.setdefault(name, [tok, 0, True])

    for n, t in enumerate(toks):
        k = t.kind
        if k == 'lambda':
            params, pending_def = [], None
        elif k == 'def':
            params, pending_def = [], None
        elif k == 'defname':
            pending_def = t
        elif k == 'param':
            params.append(t.text)
        elif k == 'lopen':
            frames.append(Frame({p: [t, 0, False] for p in params}))
            brackets.append('lambda')
            defs.append(pending_def)
            pending_def = None
        elif k in ('open', 'bopen'):
            brackets.append(k)
        elif k == 'close' and brackets:
            if brackets.pop() == 'lambda':
                for name, (tok, uses, is_bind) in frames.pop().locals.items():
                    if is_bind and not uses:
                        warn(tok, 'W3', f"'{name}' is bound but never used")
                defname = defs.pop()
                if defname is not None:  # bound after its body compiles, as the compiler does
                    if defname.text in builtins:
                        warn(defname, 'W2', f"def {defname.text} hides a built-in word")
                    if frames:
                        frames[-1].locals.setdefault(defname.text, [defname, 0, True])
                    else:
                        workspace.add(defname.text)
        elif k == 'name':
            bind(t)
        elif k == 'word' or (k == 'sym' and toks[n - 1].kind == 'prefix' and toks[n - 1].text == '`'):
            if not lookup(t.text):
                hint = '; not a number, so read as a word' if _looks_numeric(t.text) else ''
                hint += ''.join(f'; {w}?' for w in namespaced(t.text, builtins))
                warn(t, 'W1', f"'{t.text}' is undefined{hint}")
        elif k == 'prefix' and n + 1 < len(toks) and toks[n + 1].kind in TRIVIA:
            warn(t, 'W4', f"space after '{t.text}'; write '{t.text}name'")
    diags.sort(key=lambda d: (d.line, d.col))
    return diags, workspace


def namespaced(word: str, builtins: set[str]) -> list[str]:
    """NS.word for each namespace holding word; ug, the index of every UGen, only when no other does,
    and sapf, the compatibility library, only when neither does."""
    found = sorted(b for b in builtins if b.endswith('.' + word))
    return ([b for b in found if not b.startswith(('ug.', 'sapf.'))] or [b for b in found if b.startswith('ug.')]
            or found)


def builtin_words(sapf: Path) -> set[str]:
    """Built-in word names, read from `helpall` in a sapf-sc binary without a prelude."""
    with tempfile.TemporaryDirectory() as tmp:
        src = Path(tmp) / 'helpall.txt'
        src.write_text('helpall\n')
        env = dict(os.environ, STAC_PRELUDE='/dev/null', STAC_LOG=str(Path(tmp) / 'log'))
        out = subprocess.run([str(sapf), '-q', str(src)], capture_output=True, text=True, env=env, timeout=30).stdout
    words, binary = set(), False
    for line in out.splitlines():
        if line.startswith('***'):
            binary = line.startswith('*** binary math ops')
        m = re.match(r' (\S+)(?: @[a-z]*)? \(', line)
        if m:
            words.add(m.group(1))
            if binary:
                words.update(m.group(1) + v for v in ('/', '\\', '^', '/i', '\\i', '^i'))
    return words


# CLI

def _cmd_fmt(args) -> int:
    indent = '\t' if args.indent == 'tab' else ' ' * int(args.indent)
    status = 0
    for path in args.files:
        src = path.read_text()
        try:
            new = format_source(src, indent)
        except FormatError as e:
            for line in str(e).splitlines():
                print(f'{path}:{line}', file=sys.stderr)
            status = 2
            continue
        if new == src:
            continue
        if args.check:
            print(f'{path}: would reformat')
            status = max(status, 1)
        else:
            path.write_text(new)
            print(f'{path}: reformatted')
    return status


def _cmd_lint(args) -> int:
    if not args.stac.exists():
        print(f'{args.stac} not found; run make build or pass --stac', file=sys.stderr)
        return 2
    builtins = builtin_words(args.stac)
    known: set[str] = set()
    for prelude in args.prelude or []:
        _, names = analyze(prelude.read_text(), builtins, known)
        known |= names
    status = 0
    for path in args.files:
        diags, _ = analyze(path.read_text(), builtins, known)
        for d in diags:
            print(f'{path}:{d}')
        if diags:
            status = 1
    return status


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(prog='stactool', description=__doc__.split('\n')[0])
    sub = p.add_subparsers(dest='cmd', required=True)
    f = sub.add_parser('fmt', help='re-indent files in place')
    f.add_argument('--check', action='store_true', help='report files that would change; write nothing')
    f.add_argument('--indent', default='4', help="spaces per level, or 'tab' (default 4)")
    f.add_argument('files', nargs='+', type=Path)
    f.set_defaults(run=_cmd_fmt)
    lint = sub.add_parser('lint', help='report errors and warnings')
    lint.add_argument('--stac', type=Path, default=Path(os.environ.get('STAC_BIN', ROOT / 'build' / 'stac')))
    prelude = os.environ.get('STAC_PRELUDE')
    lint.add_argument('--prelude', type=Path, action='append', default=None,
                      help='names it binds count as defined; repeat to read several in order (default $STAC_PRELUDE)')
    lint.add_argument('files', nargs='+', type=Path)
    lint.set_defaults(run=_cmd_lint)
    args = p.parse_args(argv)
    if args.cmd == 'lint' and args.prelude is None and prelude:
        args.prelude = [Path(prelude)]
    return args.run(args)


if __name__ == '__main__':
    sys.exit(main())
