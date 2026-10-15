"""Generate src/engine/SpecUGens.inc, stac's UGen table, from spec/sclang-reference.json,
spec/sclang-wire.json (scripts/sclang_wire.py) and spec/stac-ugens.json.

    gen_ugens.py            # write the table
    gen_ugens.py --check    # exit 1 if the committed table is stale
"""

from __future__ import annotations

import json
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / 'spec' / 'sclang-reference.json'
WIRE = ROOT / 'spec' / 'sclang-wire.json'
OVERLAY = ROOT / 'spec' / 'stac-ugens.json'
TRUSTED = {'verified', 'consistent'}
OUT = ROOT / 'src' / 'engine' / 'SpecUGens.inc'

RATES = {'ar': 'rateAR', 'kr': 'rateKR', 'ir': 'rateIR'}
ORDER = ['rateAR', 'rateKR', 'rateIR', 'rateDR']
NEW_RATES = {'audio': 'rateAR', 'control': 'rateKR', 'scalar': 'rateIR', 'demand': 'rateDR'}
PURE = {'PureUGen', 'PureMultiOutUGen'}


class SpecError(Exception):
    pass


def c_str(s: str) -> str:
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def c_num(x: float) -> str:
    return 'INFINITY' if x == math.inf else repr(float(x))


def live_methods(cls: str, spec: dict) -> dict[str, dict]:
    """The class's rate methods, less those sclang refuses (OffsetOut.kr) or that only call another
    method at the same rate (IFFT.new calls IFFT.ar)."""
    methods = spec['ugens'][cls]['methods']
    out = {m: d for m, d in methods.items() if d.get('rate') is not None}
    if 'new' in out and any(RATES.get(m) == NEW_RATES.get(out['new']['rate']) for m in out if m != 'new'):
        del out['new']
    return out


def extracted_wire(cls: str, wires: dict, live) -> tuple[list[str], str | None, list[str], dict] | None:
    """The class's wire order, the argument that sets its output count, the inputs its ar method
    raises to audio rate, and the arguments that fall back to another argument when omitted, from
    sclang-wire.json. None unless every rate method is trusted."""
    methods = {m: r for m, r in wires['ugens'].get(cls, {}).items() if m in live}
    rows = list(methods.values())
    if not rows or any(r['status'] not in TRUSTED for r in rows):
        return None
    for r in rows:
        extra = set(r) - {'wire', 'status', 'from', 'reassigns', 'outputs', 'audio', 'fallback'}
        if extra:
            raise SpecError(f'{cls}: wire needs {sorted(extra)}, which the table does not support yet')
    if len({json.dumps([r['wire'], r.get('outputs'), r.get('fallback')]) for r in rows}) != 1:
        raise SpecError(f'{cls}: rate methods differ in wire order')
    return (rows[0]['wire'], rows[0].get('outputs'), methods.get('ar', {}).get('audio', []),
            rows[0].get('fallback', {}))


def groups(spec: dict, wires: dict, overlay: dict) -> dict[str, list[str]]:
    """Generated group -> its classes: by first help category, or placed by hand."""
    out: dict[str, list[str]] = {g: [] for g in overlay['groups']}
    for cls in sorted(spec['ugens']):
        if cls in overlay['hidden'] or cls in overlay['skip']:
            continue
        cats = wires['categories'].get(cls, [])
        group = overlay['place'].get(cls)
        if group is None and cats:
            group = next((g for g, gc in overlay['groups'].items() if cats[0] in gc), None)
        if group in out:
            out[group].append(cls)
    return out


def ugen(cls: str, spec: dict, wires: dict, overlay: dict) -> dict:
    """One class's table row, checked against the spec."""
    methods = live_methods(cls, spec)
    if not methods:
        raise SpecError(f'{cls}: no rate method sclang can build')
    over = overlay['classes'].get(cls, {})
    rates = {}
    for m, d in methods.items():
        if m in RATES:
            rates[RATES[m]] = d
        elif d.get('rate') in NEW_RATES:
            rates[NEW_RATES[d['rate']]] = d
        else:
            raise SpecError(f'{cls}: rate method {m} at {d.get("rate")} is not handled')
    if len({json.dumps([a['name'] for a in m['args']]) for m in methods.values()}) != 1:
        raise SpecError(f'{cls}: rate methods differ in argument names')
    # A call builds at the highest rate and takes that method's defaults; LeakDC.kr's coef is .9,
    # LeakDC.ar's .995.
    first = rates[min(rates, key=ORDER.index)]
    # stac has no mul and add; * and + replace them. Nor the arguments sclang applies after the UGen.
    dropped = {'mul', 'add', *over.get('unsent', [])}
    args = []
    for a in first['args']:
        d = over.get('defaults', {}).get(a['name'], a.get('default'))
        if a['name'] in dropped:
            continue
        if d is None or (isinstance(d, str) and d.startswith('(')):
            args.append((a['name'], 0., True))
        elif d == 'inf':
            args.append((a['name'], math.inf, False))
        elif isinstance(d, (int, float)) and not isinstance(d, bool):
            args.append((a['name'], float(d), False))
        else:
            raise SpecError(f'{cls}: default {d!r} for {a["name"]} is not a number')
    names = [a[0] for a in args]

    extracted = extracted_wire(cls, wires, methods)
    order, outputs_value, audio, fallback = extracted if extracted else (None, None, [], {})
    # A fallback to a constant, as BeatTrack2's `weightingscheme ? (-2.1)`, is a default.
    for a, b in list(fallback.items()):
        if re.fullmatch(r'-?\d+(\.\d+)?', b):
            i = names.index(a)
            args[i] = (a, float(b), False)
            del fallback[a]
    order = [t for t in order if t.lstrip('*#') not in dropped] if order else order
    if 'wire' in over:
        if order is not None and over['wire'] != order:
            raise SpecError(f'{cls}: overlay wire {over["wire"]} contradicts the extracted {order}')
        order = over['wire']
    if order is None:
        raise SpecError(f'{cls}: no trusted wire order in sclang-wire.json; add one to stac-ugens.json')
    wire, used = [], []
    for tok in order:
        if tok.startswith('#'):
            wire.append(('wireSize', names.index(tok[1:]), 0))
        elif tok.startswith('&&'):
            wire.append(('wireEnvInterp', names.index(tok[2:]), 0))
            used.append(tok[2:])
        elif tok.startswith('&'):
            wire.append(('wireEnv', names.index(tok[1:]), 0))
            used.append(tok[1:])
        elif tok.startswith('%'):
            wire.append(('wireFlop', names.index(tok[1:]), 0))
            used.append(tok[1:])
        elif tok.startswith('*') and '~' in tok:
            a, b = tok[1:].split('~')
            wire.append(('wireSpliceWrap', names.index(a), names.index(b)))
            used.append(a)
        elif tok.startswith('*') and '/' in tok:
            a, b = tok[1:].split('/')
            wire.append(('wireSpliceTo', names.index(a), names.index(b)))
            used.append(a)
        elif tok.startswith('*'):
            wire.append(('wireSplice', names.index(tok[1:]), 0))
            used.append(tok[1:])
        else:
            wire.append(('wireArg', names.index(tok), 0))
            used.append(tok)
    if outputs_value or (over.get('outputs') and over['outputs'][0] not in '#='):
        used.append(outputs_value or over['outputs'])
    if sorted(used) != sorted(names):
        raise SpecError(f'{cls}: wire {over.get("wire")} must send each argument once')

    done = -1
    for i, (kind, arg, _) in enumerate(wire):
        if names[arg] == 'doneAction' and kind == 'wireArg':
            if any(k != 'wireArg' for k, _, _ in wire[:i]):
                raise SpecError(f'{cls}: doneAction follows a splice')
            done = i

    positional = over.get('positional')
    if positional is None:
        positional = len(names)
        while positional and names[positional - 1] in overlay['tail']:
            positional -= 1

    outputs, outputs_arg, outputs_kind = first.get('outputs', 1), -1, 'outputsFixed'
    if 'outputs' in over and over['outputs'][0] in '#=':
        kind = 'outputsSize' if over['outputs'][0] == '#' else 'outputsValue'
        outputs, outputs_arg, outputs_kind = 0, names.index(over['outputs'][1:]), kind
    elif 'outputs' in over:
        outputs, outputs_arg, outputs_kind = 0, names.index(over['outputs']), 'outputsValue'
    elif outputs_value:
        outputs, outputs_arg, outputs_kind = 0, names.index(outputs_value), 'outputsValue'
    elif not isinstance(outputs, int):
        raise SpecError(f'{cls}: variable output count needs an overlay entry')

    keyword = names[positional:]
    help_ = f'({" ".join(names[:positional])} --> out) {cls}.'
    if keyword:
        help_ += f' Keyword only: {" ".join(keyword)}.'
    return {
        'cls': cls, 'word': cls.lower(), 'args': args, 'wire': wire, 'positional': positional,
        'rates': sorted(rates, key=ORDER.index), 'outputs': outputs, 'outputs_arg': outputs_arg,
        'outputs_kind': outputs_kind, 'audio': sum(1 << names.index(a) for a in audio),
        'fallback': {names.index(a): names.index(b) for a, b in fallback.items()},
        'fill': over.get('fill', []),
        'special': first.get('special_index', 0), 'done': done, 'help': help_,
        'pure': bool(PURE & set(spec['ugens'][cls]['superclasses'])),
    }


def generate() -> str:
    spec = json.loads(SPEC.read_text())
    wires = json.loads(WIRE.read_text())
    overlay = json.loads(OVERLAY.read_text())
    lines = [
        '// Generated by scripts/gen_ugens.py from spec/sclang-reference.json',
        f'// (SC {spec["sclang_version"]}), spec/sclang-wire.json and spec/stac-ugens.json. Do not edit.',
        '',
    ]
    rows = []
    for ns, classes in groups(spec, wires, overlay).items():
        for cls in classes:
            u = ugen(cls, spec, wires, overlay)
            args = ', '.join(
                f'{{{c_str(n)}, {c_num(d)}, {str(r and i not in u["fallback"]).lower()}, {u["fallback"].get(i, -1)}}}'
                for i, (n, d, r) in enumerate(u['args']))
            wire = ', '.join(f'{{{k}, {a}, {b}}}' for k, a, b in u['wire'])
            # ISO C++ has no zero-length arrays; a UGen without arguments points nowhere.
            if args:
                lines.append(f'static const SpecArg kArgs{cls}[] = {{{args}}};')
            if wire:
                lines.append(f'static const SpecWire kWire{cls}[] = {{{wire}}};')
            if u['fill']:
                lines.append(f'static const double kFill{cls}[] = {{{", ".join(c_num(x) for x in u["fill"])}}};')
            rate_mask = ' | '.join(f'(1 << {r})' for r in u['rates'])
            rows.append(
                f'\t{{{c_str(ns)}, {c_str(u["word"])}, {c_str(cls)}, {c_str(u["help"])}, {rate_mask}, '
                f'{u["rates"][0]}, {len(u["args"])}, {u["positional"]}, {f"kArgs{cls}" if args else "nullptr"}, '
                f'{len(u["wire"])}, {f"kWire{cls}" if wire else "nullptr"}, {u["outputs"]}, {u["outputs_kind"]}, {u["outputs_arg"]}, {u["special"]}, '
                f'{str(u["pure"]).lower()}, {u["done"]}, {hex(u["audio"])}, {len(u["fill"])}, '
                f'{f"kFill{cls}" if u["fill"] else "nullptr"}}},'
            )
    lines += ['', 'static const SpecUGen kSpecUGens[] = {', *rows, '};', '']
    return '\n'.join(lines)


def main(argv: list[str]) -> int:
    text = generate()
    if '--check' in argv:
        if not OUT.exists() or OUT.read_text() != text:
            print(f'{OUT.relative_to(ROOT)} is stale; run scripts/gen_ugens.py', file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
