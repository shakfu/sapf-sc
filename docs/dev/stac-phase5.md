# Phase 5 inventory

What the split in `stac.md` (Phases, 5) breaks, counted on 2026-10-15. Words come from `helpall` in `build/stac` without a prelude. Uses are word tokens from `stactool.py`'s lexer: in `examples/sapf.stac` and `prelude.stac`, and in the string literals of `tests/py/*.py`.

## Size

- `examples/sapf.stac` has 4735 word tokens. 777 (16%) are sapf DSP words; 155 more are `play`.
- `prelude.stac` uses sapf DSP words 106 times.
- 122 sapf DSP words exist: `helpall`'s sections SuperCollider UGens, wave tables, dsp operations, random signals and finite random signals, less `play` and `stop`.
- 154 bare UGen words sit in `ug` only. `stac.md` counts 165; the 11 others are skipped or hidden classes.

## Collisions

Binding the bare UGens globally replaces 41 sapf words of the same name. By stack effect:

| Kind | Words | Example uses |
|-|-|-|
| Same inputs | `clip`, `fold`, `wrap`, `hpf`, `lpf`, `rhpf`, `rlpf`, `leakdc`, `lfnoise0`, `lfnoise1`, `decay` | `lpf` 14, `lfnoise1` 23, `decay` 18 |
| Same count, other order | `combc`, `combl`, `combn`, `delayc`, `delayl`, `delayn` (`delay maxdelay` vs `maxdelaytime delaytime`); `line`, `xline` (`dur start end` vs `start end dur`) | `combn` 55, `xline` 10 |
| Same count, other parameter | `bpf` (bandwidth vs rq), `resonz` (rq vs bwr), `ringz`, `formlet`, `decay2`, `lag`, `lag2`, `lagud` | `decay2` 35, `lag` 29 |
| Fewer inputs | `sinosc`, `saw`, `lfsaw`, `lftri`, `impulse` (no phase); `dust`, `dust2` (no amp); `pulse`, `lfpulse`, `sinoscfb` | `sinosc` 95, `impulse` 42, `lfpulse` 31 |
| Other shape | `klank` (lists vs spec array), `osc` (wavetable vs bufnum), `pan2` (two outputs vs one) | `klank` 35, `pan2` 11 |
| Not a UGen in sapf | `ramp`, a list word; `stac.md` moves it to `sapf.ramp` | 0 |

The second and third rows change results without an error: the program runs with arguments in other slots or units. Whether "same inputs" also means same units is not verified per word; `ugens.md` holds the fidelity figures.

The other 81 sapf DSP words have no bare replacement, among them `white`, `pink`, `brown`, `tri`, `square`, `adsr`, `seq`, `steps`, `lines`, `wavefill`, `sf>`, `playbuf`, and the `randz` family.

## Removed words

| Word | Examples | Tests |
|-|-|-|
| `>sd` | 0 | `conftest.py`'s `compile_sapf` fixture, used by `test_spec_ugens.py`, `test_scgf_parity.py` and others; `test_errors.py`, `test_samples.py`, `test_seq.py` |
| `>sf`, `>sfo` | `>sfo` 6 | `test_sf.py` |
| `>nrt` | 0 | `conftest.py`, `test_samples.py`; the conformance runner renders `nrt` cases with it |
| `kr` | 0 | `test_kr_smoke.py`, `test_rates.py`, `test_scgf_parity.py` |
| `ctl` | 0 | `test_errors.py`, `test_samples.py`, `test_scgf_parity.py` |
| `N`, `T` | `N` 24, prelude `N` 18 | `test_sf.py`, `test_play.py`, `cases.py` |

`N` and `T` on lists stay; only their use on a `Sig` goes. The counts above do not separate the two.

## Tests

- `test_conformance.py` runs 108 cases from `cases.py` against original sapf renders. They use sapf DSP words 213 times. `stac.md` says they test `sapf.stac`.
- 9 test files exercise sapf words or removed words: `test_kr_smoke.py`, `test_rates.py`, `test_scgf_parity.py`, `test_sf.py`, `test_seq.py`, `test_samples.py`, `test_wavetables.py`, `test_errors.py`, `test_play_embedded.py`.
- `compile_sapf` builds every fixture def with `>sd`. It needs a replacement, such as `synthdef` and `.write`, before `>sd` goes.

## The decision this raises

`stac.md` sketches `sapf.stac` as stac source over the bare UGens: `\freq phase [{:freq freq :phase phase 2pi *} sinosc] = sinosc`. Two readings:

1. **Move, keep the C++.** The 122 words become the `sapf` namespace as they are. `sapf.stac` binds them globally. Conformance tests and examples need only the loader. The `ZList` inputs survive inside `sapf` words only; bare UGens reject them.
2. **Rewrite in stac.** Each sapf word becomes stac source over bare UGens. Words built on interpreter machinery have no stac form yet: `ola`'s voice scheduler, the rate inference behind `kr`, `seq`'s reading of lazy streams, `wavefill`'s table loading, the end gates behind `N`, `T` and `>sf`'s length. They would stay C++ or be dropped.

Reading 1 changes no DSP and is testable by the existing 108 cases. Reading 2 tests stac's expressiveness but rewrites 122 words to keep the same output.

Decided 2026-10-15: reading 1.

## What the core kept

- `clip`, `wrap`, `fold`, `iwrap`, `ifold` and `*+` work on reals, so they stay core words, decided 2026-10-15. On a `Sig`, `clip`, `wrap` and `fold` build the same def as `ug.clip`, `ug.wrap` and `ug.fold`. `ug.clip` on reals builds a `Clip` UGen, as `Clip.ar` does in sclang.
- So the prelude's range mapping words, `unilinc` to `expexpc`, and `walk` and `iwalk` stay in `prelude.stac`. 48 of its 290 statements moved to `sapf.stac`.
- `N`, `keep`, `NZ` and `T` stay for lists. On a `Sig` the core's versions fail and name `sapf.N` and the rest, which end it with an `EnvGen` as before.
- `play` stays and pushes a `Node`. `sapf.play` drops it, and keeps the player of an event stream.

## ola

Decided 2026-10-15: `ola` stays in `sapf` as C++, not rewritten on `pbind`. `ola` does three things `pbind play` does not:

1. It returns a signal: an `In` on a private bus, which the rest of the graph can process.
2. Its voices start when the main synth plays and stop when it ends.
3. `>nrt` writes its voices into the score.

A `pbind` version would keep only the first. It would also break the 4 `nrt` conformance cases in `cases.py`, `test_ola_stops_with_its_main_synth` and the `ola` case in `test_sf.py`. The C++ stays either way, for `sapf.ola`, `oltx` and `xftx`.

The core sends a pattern to an effect with `:out` and `bus.audio`, as SC's `Pbind(\out, bus)` does. If a program needs a pattern as a signal tied to a graph, generalise `ola`'s scheduler to event streams, as SC's `Ndef(\x, Pbind(...))` does. That would also give `ndef` its private bus.

## Before phase 5

Phase 1, the rename to `stac`, is done.
