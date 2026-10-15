"""scripts/stactool.py: lexer parity with Parser.cpp, formatter, linter."""

import sys
from pathlib import Path

import pytest

from conftest import ROOT, run_sapf

sys.path.insert(0, str(ROOT / 'scripts'))
import stactool as st  # noqa: E402

SOURCES = sorted([ROOT / 'prelude.stac', ROOT / 'sapf.stac', ROOT / 'examples' / 'sapf.stac', ROOT / 'sapf-bif-examples.txt',
                  ROOT / 'unit-tests.txt', *ROOT.glob('tests/test_*.txt')])


def kinds(src):
    return [(t.kind, t.text) for t in st.lex(src).toks if t.kind not in st.TRIVIA]


@pytest.mark.parametrize('path', SOURCES, ids=lambda p: p.name)
def test_lex_is_lossless_and_clean(path):
    src = path.read_text()
    lx = st.lex(src)
    assert ''.join(t.text for t in lx.toks) == src
    assert lx.errors == []


@pytest.mark.parametrize('src,expect', [
    ('-2', [('num', '-2')]),
    ('- 2', [('word', '-'), ('num', '2')]),
    ('5/4', [('num', '5/4')]),
    ('5 / 4', [('num', '5'), ('word', '/'), ('num', '4')]),
    ('2pi pi 24k 1e3 .5 0xff', [('num', x) for x in ('2pi', 'pi', '24k', '1e3', '.5', '0xff')]),
    ('*.5', [('word', '*'), ('num', '.5')]),
    ('10ms 2X 5/x', [('word', '10ms'), ('word', '2X'), ('word', '5/x')]),
    ('x.free', [('word', 'x'), ('prefix', '.'), ('sym', 'free')]),
    ("'a `b ,c :d", [('prefix', "'"), ('sym', 'a'), ('prefix', '`'), ('sym', 'b'),
                     ('prefix', ','), ('sym', 'c'), ('prefix', ':'), ('sym', 'd')]),
    ('= [a b]', [('bind', '='), ('bopen', '['), ('name', 'a'), ('name', 'b'), ('close', ']')]),
    (r'\a 2.1 b "h" [a]', [('lambda', '\\'), ('param', 'a'), ('effect', '2.1'), ('param', 'b'),
                           ('str', '"h"'), ('lopen', '['), ('word', 'a'), ('close', ']')]),
    (r'"a ""q"" \\n"', [('str', r'"a ""q"" \\n"')]),
    ('@ @@ @2 @101', [('each', '@'), ('each', '@@'), ('each', '@2'), ('each', '@101')]),
    ('#[1] ;c', [('open', '#['), ('num', '1'), ('close', ']')]),
    ('def beep freq:440 t_trig = [freq]', [('def', 'def'), ('defname', 'beep'), ('param', 'freq'), ('prefix', ':'),
                                           ('num', '440'), ('param', 't_trig'), ('bind', '='), ('lopen', '['),
                                           ('word', 'freq'), ('close', ']')]),
])
def test_tokens(src, expect):
    assert kinds(src) == expect


@pytest.mark.parametrize('src,code', [
    ('"open', 'E1'), ('[1', 'E2'), ('1]', 'E3'), ('[1)', 'E3'), ("'", 'E4'), ('#x', 'E5'),
    (r'\a 2 [a]', 'E6'), (r'\a', 'E6'), ('1 =', 'E7'), ('= (a b]', 'E7'), ('@12', 'E8'),
    ('def = [1]', 'E10'), ('def f a:x = [a]', 'E10'), ('def f a', 'E10'), ('def f a = 1', 'E10'),
])
def test_syntax_errors(src, code):
    assert code in [e.code for e in st.lex(src).errors]


def test_format_indents_by_bracket_depth():
    src = '[\n1 [\n2\n]\n  ]   \n'
    assert st.format_source(src) == '[\n    1 [\n        2\n    ]\n]\n'


def test_format_one_level_per_line():
    assert st.format_source('{:a [\n1\n]}\n') == '{:a [\n    1\n]}\n'


def test_format_lambda_header_and_comments():
    src = '\\list\n"help"\n[list\n; note   \nx] = f\n'
    assert st.format_source(src) == '\\list\n    "help"\n    [list\n        ; note\n        x] = f\n'


def test_format_lambda_body_on_own_line():
    src = '\\a b\n"h"\n[\na b +\n] = f\n'
    assert st.format_source(src, '\t') == '\\a b\n\t"h"\n\t[\n\t\ta b +\n\t] = f\n'


def test_format_def_body():
    src = 'def pad freq:220 = [\nfreq ug.saw\n.2 *\n]\n'
    assert st.format_source(src) == 'def pad freq:220 = [\n    freq ug.saw\n    .2 *\n]\n'


def test_format_keeps_inner_spacing_and_strings():
    src = '[\n  a  b   ; c\n"x\n   y"\n]\n'
    assert st.format_source(src) == '[\n    a  b   ; c\n    "x\n   y"\n]\n'


def test_format_tab_indent():
    assert st.format_source('[\n1\n]', '\t') == '[\n\t1\n]\n'


def test_format_refuses_syntax_errors():
    with pytest.raises(st.FormatError, match='E2'):
        st.format_source('[1')


@pytest.mark.parametrize('path', SOURCES, ids=lambda p: p.name)
def test_format_is_idempotent_and_keeps_tokens(path):
    src = path.read_text()
    once = st.format_source(src)
    assert st.format_source(once) == once
    assert st._significant(st.lex(once).toks) == st._significant(st.lex(src).toks)


def test_fmt_check_writes_nothing(tmp_path, capsys):
    f = tmp_path / 'a.txt'
    f.write_text('[\n1\n]\n')
    assert st.main(['fmt', '--check', str(f)]) == 1
    assert f.read_text() == '[\n1\n]\n'
    assert st.main(['fmt', str(f)]) == 0
    assert st.main(['fmt', '--check', str(f)]) == 0


@pytest.fixture(scope='module')
def builtins(sapf_bin):
    return st.builtin_words(sapf_bin)


def codes(src, builtins, known=frozenset()):
    return [d.code for d in st.analyze(src, builtins, known)[0]]


def test_builtin_words(builtins):
    assert {'sinosc', 'sapf.sinosc', '+', '+/', '*\\', 'nyq', 'sapf.>sd', 'ug.clip'} <= builtins
    assert 'white' not in builtins
    assert len(builtins) > 900


@pytest.mark.parametrize('src,expect', [
    ('440 0 sapf.sinosc', []),
    ('440 sinoz', ['W1']),
    ('10ms', ['W1']),
    ('foo = foo', ['W1']),
    ('1 = foo  foo `foo', []),
    ('1 = play', ['W2']),
    (r'\x [x = y  3 = z  y]', ['W3']),
    (r'\x [x]', []),
    (r'\a [\b [a = a] ]', ['E9']),
    (r'\a [\b [a] = g  g]', []),
    ('[1 2] = [a b]  a b', []),
    ('x . free', ['W1', 'W4']),
    ('def beep freq:440 = [freq ug.sinosc]  {} beep', []),
    ('def beep freq = [beep]', ['W1']),
    ('def neg x = [x]', ['W2']),
    ('def f a = [b]', ['W1']),
])
def test_lint(src, expect, builtins):
    assert codes(src, builtins) == expect


@pytest.mark.parametrize('src,hint', [
    ('1 pv_magfreeze', "'pv_magfreeze' is undefined; fft.pv_magfreeze?"),
    ('1 white', "'white' is undefined; sapf.white?"),
])
def test_lint_names_the_namespace(src, hint, builtins, tmp_path):
    """As sapf's own error does."""
    (d,) = st.analyze(src, builtins)[0]
    assert d.msg == hint
    assert hint.replace("'", '"', 2).replace(' undefined', ' an undefined word', 1) in run_sapf(src, tmp_path, lib=[])


def test_lint_prelude_names(builtins):
    _, prelude = st.analyze((ROOT / 'prelude.stac').read_text(), builtins)
    assert 'prall' in prelude
    assert codes('[1 2] prall', builtins) == ['W1']
    assert codes('[1 2] prall', builtins, prelude) == []


@pytest.mark.parametrize('src', ['440 sinoz', '10ms', 'foo = foo', r'\a [\b [a = a] ] = f', '`nope'])
def test_lint_errors_match_sapf(src, builtins, tmp_path):
    """Each snippet the linter rejects also fails to compile in sapf."""
    assert codes(src, builtins)
    assert 'syntax' in run_sapf(src, tmp_path)


@pytest.mark.parametrize('path', SOURCES, ids=lambda p: p.name)
def test_lint_repo_sources_have_no_errors(path, builtins):
    _, prelude = st.analyze((ROOT / 'prelude.stac').read_text(), builtins)
    _, sapf = st.analyze((ROOT / 'sapf.stac').read_text(), builtins, prelude)
    diags, _ = st.analyze(path.read_text(), builtins, prelude | sapf)
    assert [d for d in diags if d.code.startswith('E')] == []
