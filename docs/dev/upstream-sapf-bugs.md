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
| `{}` crashes | fixed |
| `try` rethrows | fixed in `try_` |
| `throw` drops its value | fixed in `throw_` |
| `op^i` takes one argument | fixed in `DEFNa2` |
| `sortf` and `gradef` cannot run | fixed in `sortf_`, `gradef_` |
| `permzwr` crashes | fixed in `PermsWithRepeatedItemsZ` |
| `evmerge` crashes on an event without `:dt` | fixed in `MergeEvents` |
| `evrest` takes three arguments | fixed |
| Names with equal hashes share a binding | fixed in `GTable` |

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

## `{}` crashes

`{} pr cr` segfaults. Braces with no keys evaluate to `vm._ee`, built as `new Form(0, NULL)` (`src/engine/VM.cpp`): a Form with no table. `Form::print` and `Form::get` dereference the table. sapf-sc gives `_ee` an empty table, and `TableMap::getIndex` returns not found for an empty map, whose index array is null. `{}` is the keyword call that takes every default, as in `{} ug.whitenoise`.

## `try` rethrows

`try` applied its handler and then rethrew the exception, as `protect` does, so no error was ever caught. Its help says "if an exception is thrown, function B is applied". `try_` (`src/engine/CoreOps.cpp`) copied `protect_`'s `throw;`. Found writing the dictionary entry.

## `throw` drops its value

`throw`'s help reads `(a -->)`, but it took no argument and threw the integer -1, which prints as `error: -1`. It now throws `a`, which prints as `error: a`.

## `op^i` takes one argument

`+^i` and the other pairwise forms with an initial value pop a list and an initial value, but `DEFNa2` (`src/engine/MathOps.cpp`) registered them as taking one, so `[1 2 3] 10 -^i` underflowed. The help also listed `op/i`, a reducing form with an initial value that sapf never defined; the help line is removed.

## `sortf` and `gradef` cannot run

Both popped their compare function with `popList`, which accepts only a list, so every call failed. Found writing the dictionary.

## `permzwr` crashes

`#[1 1 2] permzwr` ended stac with a segmentation fault. `PermsWithRepeatedItemsZ` declared a list of values but filled it through `fulfillz`, as floats, and built each permutation as a list of values holding floats. It now builds a list of signals, as `permz` does.

## `evmerge` and `evrest`

`evmerge` stopped its list partway through a block when an event had no `:dt`, which failed the assertion in `List::end` and ended stac. It now reports the event. `evrest` was registered with the argument mask `aak`, so `1 evrest` underflowed; it takes one value.

## Names with equal hashes share a binding

`GTable::get`, `getInner` and `putImpure` (`src/engine/Object.cpp`) matched a key by its 32-bit hash alone. Two names with the same hash read each other's bindings, and the second could not be bound. `TreeNode::putPure` compared the key, so the tree held both. Lookups now compare the key, by text for strings, since a string literal is not interned. Found reading the code. stac's 1467 built-in names, with and without their namespace, have no collision.

## Help lines that misstate behaviour

Found writing the dictionary; each help line now says what the word does.

- `wrapAt`, `foldAt` and `clipAt`: the three descriptions were rotated, so `wrapAt` described clipping, `foldAt` wrapping and `clipAt` folding.
- `irand2`, `irand2s`, `irand2z`, `nirand2s` and `nirand2z` return integers; their help said reals.
- `atan2` computes the arctangent of x/y, as C's `atan2(x, y)`; its help said y/x.
- `distort` computes x/(1 + |x|); its help gave `sigm`'s x/sqrt(1 + x^2).
- `expm1` computes exp(x) - 1; its help said exp(x - 1). `isspace` described graphic characters.
- `to` and `toz` said they count down "if a < b". `nbyz` and `ngrowz` left `n` out of their stack effects. `degkey` and `keydeg` had no outputs in their stack effects and did not say that a scale's last item is its cycle width.
