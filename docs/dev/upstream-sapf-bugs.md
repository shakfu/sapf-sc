# Bugs found in original sapf

Found while generating `tests/fixtures/ref` from `../sapf` (macOS arm64, Accelerate). `tests/py/cases.py` works around each with `ref_expr` or per-channel rendering.

## Signal minus signal is reversed

`#[1 2 3] #[10 20 30] -` gives `#[9 18 27]`; `[1 2 3] [10 20 30] -` gives `[-9 -18 -27]`.

`BinaryOp_minus::loopz` (`src/engine/MathOps.cpp`) calls `vDSP_vsubD(aa, astride, bb, bstride, ...)` when both strides are nonzero. vDSP computes its second operand minus its first, so the result is `b - a`. The scalar branches are correct.

## `ratiosemi` divides where it should multiply

`sc_ratiosemi` (`include/MathFuns.hpp`) returns `log2(ratio) / 12`. `semiratio` is `2^(semitones / 12)`, so its inverse is `12 * log2(ratio)`. `ratiocents` is correct.

## Multichannel `>sf` drops frames

`[a b] @ 4800 N "x" >sf` writes 2400 frames, and frames from 512 onward are wrong. 512 is the block size. Single-channel writes are correct.
