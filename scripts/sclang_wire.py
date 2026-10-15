"""Extract each UGen rate method's wire order from SC's class library source, and each class's
help categories from HelpSource/Classes/*.schelp, into spec/sclang-wire.json.

    sclang_wire.py [--lib PATH]     # default: build/_deps/supercollider-src/SCClassLibrary

The order is read from the method's multiNew or multiNewList call. Each map is checked against
spec/sclang-reference.json: the probe recorded the inputs sclang built from the defaults, so the
map, filled with the same values, must reproduce them.

Tokens: 'name' sends an argument; '*name' splices its list; '#name' sends its list's size;
'=x' is the constant x; '?expr' is an expression the extractor does not read.
Per method, besides the wire: 'outputs' names the argument that sets the output count and is not
sent; 'audio' lists inputs sclang raises with asAudioRateInput; 'fallback' maps an argument to the
expression sclang uses when it is nil.
Status per method:
  verified      reproduces the probe's inputs, and the probe passed distinct values for the
                arguments that can be told apart only by value
  consistent    reproduces them, but two arguments got the same value, so a swap would pass
  mismatch      does not reproduce them
  unparsed      the method does not end in a multiNew or multiNewList call
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'spec' / 'sclang-reference.json'
OUT = ROOT / 'spec' / 'sclang-wire.json'
LIB = ROOT / 'build' / '_deps' / 'supercollider-src' / 'SCClassLibrary'

# The probe's values for arguments without a default (sclang_reference.scd in nanosynth).
OVERRIDES = {'numChannels': 1, 'numChans': 2, 'label': 'x'}
SKIP = {'mul', 'add'}


def strip_comments(t: str) -> str:
    """Blanks // and nested /* */ comments, leaving strings, symbols and char literals."""
    out, i, n = [], 0, len(t)
    while i < n:
        c = t[i]
        if c == '$' and i + 1 < n:
            out.append(t[i:i + 2])
            i += 2
        elif c in '"\'':
            j = i + 1
            while j < n and t[j] != c:
                j += 2 if t[j] == '\\' else 1
            out.append(t[i:j + 1])
            i = j + 1
        elif t.startswith('//', i):
            j = t.find('\n', i)
            i = n if j < 0 else j
        elif t.startswith('/*', i):
            depth, j = 1, i + 2
            while j < n and depth:
                if t.startswith('/*', j):
                    depth, j = depth + 1, j + 2
                elif t.startswith('*/', j):
                    depth, j = depth - 1, j + 2
                else:
                    j += 1
            out.append(' ')
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def match(t: str, i: int) -> int:
    """Index after the bracket closing the one at t[i], skipping strings and char literals."""
    pairs = {'(': ')', '[': ']', '{': '}'}
    stack, n = [], len(t)
    while i < n:
        c = t[i]
        if c == '$':
            i += 2
            continue
        if c in '"\'':
            j = i + 1
            while j < n and t[j] != c:
                j += 2 if t[j] == '\\' else 1
            i = j + 1
            continue
        if c in pairs:
            stack.append(pairs[c])
        elif stack and c == stack[-1]:
            stack.pop()
            if not stack:
                return i + 1
        i += 1
    raise ValueError('unbalanced')


def split_top(s: str, sep: str) -> list[str]:
    parts, depth, cur, i = [], 0, '', 0
    while i < len(s):
        c = s[i]
        if c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
        if depth == 0 and s.startswith(sep, i):
            parts.append(cur.strip())
            cur, i = '', i + len(sep)
            continue
        cur += c
        i += 1
    parts.append(cur.strip())
    return [p for p in parts if p]


def class_methods(lib: Path) -> dict[str, dict[str, str]]:
    """Class name -> {'*ar': body, 'init': body, ...} for every class and extension."""
    classes: dict[str, dict[str, str]] = {}
    for path in sorted(lib.rglob('*.sc')):
        t = strip_comments(path.read_text(errors='ignore'))
        for m in re.finditer(r'^\s*\+?\s*([A-Z]\w*)\s*(?::\s*\w+\s*)?\{', t, re.M):
            start = t.index('{', m.start())
            try:
                end = match(t, start)
            except ValueError:
                continue
            body = t[start + 1:end - 1]
            methods = classes.setdefault(m.group(1), {})
            for mm in re.finditer(r'(?:^|[;}\s])(\*?\w+)\s*\{', body):
                name = mm.group(1)
                if not re.match(r'\*?[a-z]\w*$', name):
                    continue
                b0 = body.index('{', mm.start(1))
                try:
                    b1 = match(body, b0)
                except ValueError:
                    continue
                if name not in methods:
                    methods[name] = body[b0 + 1:b1 - 1]
    return classes


def find(classes, chain: list[str], method: str) -> tuple[str, str] | None:
    for cls in chain:
        if method in classes.get(cls, {}):
            return cls, classes[cls][method]
    return None


def tokens(body: str, names: list[str]) -> tuple[list[str], dict] | None:
    """The wire tokens of the method's multiNew or multiNewList call and their metadata, or None."""
    m = re.search(r'(?:this|super)\s*\.\s*(multiNewList|multiNew)\s*\(', body)
    if not m:
        return None
    open_ = m.end() - 1
    inner = body[open_ + 1:match(body, open_) - 1]
    if m.group(1) == 'multiNew':
        pieces = [(p, False) for p in split_top(inner, ',')[1:]]
    else:
        pieces = []
        for part in split_top(inner, '++'):
            if part.startswith('['):
                pieces += [(p, False) for p in split_top(part[1:-1], ',')]
            else:
                pieces.append((re.sub(r'\.asArray$', '', part), True))
        pieces = pieces[1:]  # the rate
    out, meta = [], {}
    for p, splice in pieces:
        audio = re.match(r'(\w+)\.asAudioRateInput(\(this\))?$', p)
        if audio:
            p = audio.group(1)
            meta.setdefault('audio', []).append(p)
        fallback = re.fullmatch(r'(\w+)\s*\?\s*\(?(.+?)\)?', p)
        if fallback and fallback.group(1) in names:
            p = fallback.group(1)
            meta.setdefault('fallback', {})[p] = fallback.group(2)
        size = re.fullmatch(r'(\w+)\.size', p)
        if size and size.group(1) in names:
            out.append('#' + size.group(1))
        elif p in names:
            out.append(('*' if splice else '') + p)
        elif re.fullmatch(r'-?\d+(\.\d+)?', p):
            out.append('=' + p)
        else:
            out.append('?' + p)
    return out, meta


def reassigned(body: str, names: list[str]) -> list[str]:
    """Arguments the method assigns before its call, as in `freq = freq.max(0)`."""
    body = re.sub(r'^\s*(arg\b[^;]*;|\|[^|]*\|)', '', body)
    head = body[:body.find('^')] if '^' in body else body
    return [n for n in names if re.search(rf'(?<![\w.]){re.escape(n)}\s*=(?!=)', head)]


def probe_values(method: dict) -> dict[str, object]:
    """The value the probe passed for each argument: a number, 'ugen' or None (unknown)."""
    sub_rate, sub_first = method.get('substitute_rate'), method.get('substitute_first')
    vals = {}
    for i, a in enumerate(method['args']):
        d, n = a.get('default'), a['name']
        literal = isinstance(d, (int, float)) and not isinstance(d, bool)
        if sub_first and i == 0 and n not in SKIP:
            vals[n] = 'ugen' if sub_rate in ('ar', 'kr') else 0.5
        elif literal:
            vals[n] = float(d)
        elif d == 'inf':
            vals[n] = 'inf'
        elif d is None and n in OVERRIDES:
            vals[n] = OVERRIDES[n] if isinstance(OVERRIDES[n], (int, float)) else None
        elif d is None and n not in SKIP:
            vals[n] = 'ugen' if sub_rate in ('ar', 'kr') else 0.5
        else:
            vals[n] = None
    return vals


def check(wire: list[str], meta: dict, method: dict) -> str:
    probed = method.get('inputs')
    if probed is None or any(t.startswith('?') for t in wire):
        return 'unverified'
    vals = probe_values(method)
    for name, expr in meta.get('fallback', {}).items():
        if vals.get(name) is None:
            vals[name] = vals.get(expr, float(expr) if re.fullmatch(r'-?\d+(\.\d+)?', expr) else None)
    for name in meta.get('audio', []):
        if vals.get(name) is not None:
            vals[name] = 'ugen'  # asAudioRateInput makes a constant DC.ar or Silent.ar
    pred = []
    for t in wire:
        if t.startswith('='):
            pred.append(float(t[1:]))
        elif t.startswith('#'):
            pred.append(None)
        else:
            pred.append(vals.get(t.lstrip('*')))
    if len(pred) != len(probed):
        return 'mismatch'
    for p, q in zip(pred, probed):
        if p is None:
            continue
        q = 'ugen' if isinstance(q, str) and q.startswith('ugen:') else q
        if p != q:
            return 'mismatch'
    seen = [vals.get(t.lstrip('*')) for t in wire if t[0] not in '=#']
    numeric = [v for v in seen if v is not None and v != 'ugen']
    return 'verified' if len(numeric) == len(set(numeric)) else 'consistent'


def categories(help_dir: Path, classes) -> dict[str, list[str]]:
    """Class -> its help file's categories, without the leading 'UGens>'."""
    out = {}
    for cls in classes:
        path = help_dir / f'{cls}.schelp'
        if not path.exists():
            continue
        m = re.search(r'^categories::(.*)$', path.read_text(errors='ignore'), re.M)
        if m:
            out[cls] = [re.sub(r'^UGens>', '', c.strip()) for c in m.group(1).split(',') if c.strip()]
    return out


def extract(lib: Path) -> dict:
    spec = json.loads(SPEC.read_text())
    classes = class_methods(lib)
    out = {}
    for cls, entry in sorted(spec['ugens'].items()):
        chain = [cls] + entry['superclasses']
        for sel, method in sorted(entry['methods'].items()):
            names = [a['name'] for a in method['args']]
            hit = find(classes, chain, '*' + sel)
            row: dict[str, object] = {}
            if hit is None:
                row['status'] = 'unparsed'
                row['note'] = 'no source'
            else:
                owner, body = hit
                parsed = tokens(body, names)
                if owner != cls:
                    row['from'] = owner
                if parsed is None:
                    row['status'] = 'unparsed'
                else:
                    wire, meta = parsed
                    status = check(wire, meta, method)
                    # numChannels first, read by init as the output count and not sent
                    if status == 'mismatch' and wire and wire[0] in ('numChannels', 'numChans'):
                        rest = check(wire[1:], meta, method)
                        if rest != 'mismatch':
                            meta['outputs'], wire, status = wire[0], wire[1:], rest
                    row.update(meta)
                    row['wire'] = wire
                    row['status'] = status
                    changed = reassigned(body, names)
                    if changed:
                        row['reassigns'] = changed
            out.setdefault(cls, {})[sel] = row
    return {'sclang_version': spec['sclang_version'], 'ugens': out,
            'categories': categories(lib.parent / 'HelpSource' / 'Classes', sorted(spec['ugens']))}


def main(argv: list[str]) -> int:
    p = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    p.add_argument('--lib', type=Path, default=LIB)
    args = p.parse_args(argv)
    if not args.lib.is_dir():
        print(f'{args.lib} not found; run make build to fetch the SuperCollider source', file=sys.stderr)
        return 1
    data = extract(args.lib)
    OUT.write_text(json.dumps(data, indent=1, sort_keys=True) + '\n')
    counts: dict[str, int] = {}
    for methods in data['ugens'].values():
        for row in methods.values():
            counts[row['status']] = counts.get(row['status'], 0) + 1
    print(f'wrote {OUT.relative_to(ROOT)}: ' + ', '.join(f'{v} {k}' for k, v in sorted(counts.items())))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
