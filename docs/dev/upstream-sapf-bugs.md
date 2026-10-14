# Bugs found in original sapf

Found while generating `tests/fixtures/ref` from `../sapf` (macOS arm64, Accelerate). `tests/py/cases.py` works around each with `ref_expr` or per-channel rendering.

Policy: sapf-sc computes what sapf's code and documentation describe, not what upstream outputs. A bug in code sapf-sc compiles is fixed, with a test of the correct result; reference renders keep the bug and `ref_expr` names the correct expression. Running old sapf programs with identical output is not a goal.

| Bug | sapf-sc |
|-|-|
| Signal minus signal is reversed | fixed in `BinaryOp_minus::loopz` |
| `ratiosemi` divides | fixed in `sc_ratiosemi` |
| Multichannel `>sf` drops frames | not compiled: scsynth renders `>sf` |
| `unspell` reads uninitialized memory | fixed |
| Classic wavetables | not compiled: `saw`, `square`, `tri`, `pulse` are SC UGens |
| `lpf` coefficients | not compiled: `lpf` is SC's `LPF` |
| `Z` and `V` call every output finite | fixed |

## Signal minus signal is reversed

`#[1 2 3] #[10 20 30] -` gives `#[9 18 27]`; `[1 2 3] [10 20 30] -` gives `[-9 -18 -27]`.

`BinaryOp_minus::loopz` (`src/engine/MathOps.cpp`) calls `vDSP_vsubD(aa, astride, bb, bstride, ...)` when both strides are nonzero. vDSP computes its second operand minus its first, so the result is `b - a`. The scalar branches are correct.

## `ratiosemi` divides where it should multiply

`sc_ratiosemi` (`include/MathFuns.hpp`) returns `log2(ratio) / 12`. `semiratio` is `2^(semitones / 12)`, so its inverse is `12 * log2(ratio)`. `ratiocents` is correct.

## Multichannel `>sf` drops frames

`[a b] @ 4800 N "x" >sf` writes 2400 frames, and frames from 512 onward are wrong. 512 is the block size. Single-channel writes are correct.

## `unspell` reads uninitialized memory

`[97 98 99] unspell` aborts with SIGTRAP (macOS arm64).

`vlistToString` and `zlistToString` (`src/engine/StreamOps.cpp`) construct the `String` before filling its buffer. That constructor takes the buffer and hashes it at once, so it hashes uninitialized memory with no terminator. sapf-sc fills the buffer first.

## Classic wavetables do not hold their waveforms

`100 0 saw` has a spike at each cycle start and an RMS of .12; a sawtooth has a linear ramp. `square`, `tri` and `pulse` are also wrong; all use the tables from `makeClassicWavetables` (`src/engine/OscilUGens.cpp`). The sine series the code describes differs from the output by about 100% RMS.

Probable cause, not verified: `fillWaveTable` interleaves magnitudes and angles, then calls `vDSP_rectD` with stride 1. Pairs need stride 2, so each angle is read as the next magnitude.

## `lpf` scales its coefficients more than once at a constant freq

`LPF::pull` (`src/engine/FilterUGens.cpp`) has two branches. With a constant freq it divides `b1` by `a0`, then computes `b0` from the scaled `b1` and divides again, and `b2` from the scaled `b0` and divides a third time. The branch for a varying freq divides once. `SAW 1000 lpf` therefore differs from `SAW 0 0 sinosc 1000 + lpf` by 6.6% RMS; the second matches SC's `LPF` exactly.

## `Z` and `V` call every output finite

`[2 0 1] cyc finite` gives 0; `[2 0 1] cyc Z finite` gives 1. `ZGen` and `VGen` (`src/engine/StreamOps.cpp`) pass `true` as the `Gen` finite flag whatever their input. Original sapf rarely asks; sapf-sc's `seq` and `iseq` measured a finite list with `length()`, which forced `pat picks $/ Z` (three examples, `native algorhythms`) until memory ran out. Both now pass `a.isFinite()`, and `readItems` counts values instead of calling `length()`.
