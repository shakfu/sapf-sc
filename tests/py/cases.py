"""Conformance cases: a sapf expression rendered by original sapf (tests/fixtures/ref) and by
scsynth from sapf-sc's SynthDef.

- exact: at most `max_bad` frames differ by more than `tol`, scaled by the sample's magnitude when
  that is above 1.
- rms: the RMS of the difference is at most `tol` times the RMS of the reference. For UGens whose
  SC algorithm differs from sapf's.

`ref_expr` replaces `expr` for the reference where original sapf has a bug. A reference shorter
than `seconds` is a finite signal: the synth must free itself and output zeros after it."""

from dataclasses import dataclass


@dataclass(frozen=True)
class Case:
    name: str
    expr: str
    tol: float = 1e-4
    mode: str = "exact"
    max_bad: int = 0
    channels: int = 1
    seconds: float = 0.1
    ref_expr: str | None = None
    delay: int = 0  # frames the render lags the reference


A = "100 0 sinosc .9 * .0123 +"  # zero crossings fall between samples
B = "130 .2 sinosc .3 * .5 +"  # positive
POS = "100 0 sinosc .4 * .5 +"
SIN = "100 0 sinosc .9 *"
SIN2 = "130 .2 sinosc .7 *"
SAW = "200 0 lfsaw"
IMP = "100 0 impulse"
IMP2 = "100 .5 impulse"

CASES = [
    Case("sinosc", "100 0 sinosc"),
    Case("sinosc_phase", "100 .25 sinosc"),
    Case("sinosc_fm", "3 0 sinosc 100 * 300 + 0 sinosc", 1e-3),
    Case("lfsaw", "100 0 lfsaw"),
    # One frame lands on the wrap point; rounding puts it on opposite sides.
    Case("lfsaw_phase", "100 .3 lfsaw", max_bad=1),
    Case("lftri", "100 0 lftri"),
    Case("lftri_phase", "100 .3 lftri"),
    Case("lfpulse", "100 0 .3 lfpulse"),
    Case("lfpulse_phase", "100 .5 .3 lfpulse"),
    Case("lfpulseb", "100 0 .3 lfpulseb"),
    Case("lfsquare", "100 0 lfsquare"),
    Case("impulse", IMP),
    Case("impulse_phase", "100 .5 impulse"),
    Case("lpf", f"{SAW} 1000 lpf", .1, "rms"),
    Case("hpf", f"{SAW} 1000 hpf"),
    Case("rlpf", f"{SAW} 1000 .3 rlpf", .005, "rms"),
    Case("rhpf", f"{SAW} 1000 .3 rhpf", .005, "rms"),
    Case("bpf", f"{SAW} 1000 1 bpf", .03, "rms"),
    Case("bsf", f"{SAW} 1000 1 bsf", .01, "rms"),
    Case("resonz", f"{SAW} 1000 .2 resonz", .06, "rms"),
    # SC's resonators process their first input sample twice, so the first impulse comes later.
    Case("ringz", f"{IMP2} 1000 .05 ringz", .02, "rms"),
    Case("formlet", f"{IMP2} 1000 .01 .05 formlet", .02, "rms"),
    # SC's LeakDC starts from its first input, sapf's from zero, so the input starts at zero.
    Case("leakdc", f"3 0 sinosc .5 * {SAW} + .995 leakdc", 1e-3),
    Case("decay", f"{IMP} .01 decay", .01, "rms"),
    Case("decay2", f"{IMP} .002 .01 decay2", .02, "rms"),
    Case("lag", "100 0 lfsquare .002 lag", .02, "rms"),
    Case("lagud", "100 0 lfsquare .001 .004 lagud", .02, "rms"),
    Case("delayn", f"{IMP} .003 0 delayn"),
    Case("delayl", f"{SIN} .00313 0 delayl"),
    # SC's cubic interpolation reads zeros before the start differently for two frames.
    Case("delayc", f"{SIN} .00313 0 delayc", max_bad=2),
    Case("combn", f"{IMP} .003 0 .05 combn"),
    Case("combl", f"{SIN} .00313 0 .05 combl", 1e-3),
    Case("combc", f"{SIN} .00313 0 .05 combc", 1e-3, max_bad=2),
    Case("alpasn", f"{IMP} .003 0 .05 alpasn"),
    Case("alpasl", f"{SIN} .00313 0 .05 alpasl", 1e-3),
    # SC's AllpassC outputs zero until its delay line fills; sapf outputs -feedback * in.
    Case("alpasc", f"{SIN} .00313 0 .05 alpasc", 1e-3, max_bad=151),
    # SC pans through a sine table.
    Case("pan2", f"{SIN} .3 pan2", 2e-3, channels=2),
    Case("bal2", f"{SIN} {SIN2} .3 bal2", 2e-3, channels=2),
    Case("rot2", f"{SIN} {SIN2} .3 rot2", 2e-3, channels=2),
    Case("fade2", f"{SIN} {SIN2} .3 fade2", 1e-3),
    Case("sah", f"{SIN} 30 0 impulse sah"),
    Case("clip", f"{SIN} 1.5 * -.5 .7 clip"),
    Case("wrap", f"{SIN} 1.5 * -.5 .7 wrap"),
    Case("fold", f"{SIN} 1.5 * -.5 .7 fold"),
    Case("fadein", "100 0 sinosc .05 fadein"),
    Case("fadeout", "100 0 sinosc .03 .04 fadeout", 1e-3),
    # Control rate computes once per 64-frame block. K2A interpolates from the previous block,
    # so the output lags one block.
    Case("kr", "30 0 sinosc kr", .01, "rms", ref_expr="30 0 sinosc", delay=64),
    Case("line", ".05 -.5 .8 line"),
    Case("line_mod", ".05 200 800 line 0 sinosc", 1e-3),
    Case("xline", ".05 .01 .8 xline"),
    Case("xline_zero", ".05 -.5 .8 xline"),
    # Taking part of a UGen ends the synth.
    Case("N", "100 0 sinosc 2000 N"),
    Case("T", "100 0 sinosc .03 T"),
    Case("NZ", "100 0 sinosc 1000 NZ"),
]

UNARY = [
    "neg", "abs", "ceil", "floor", "frac", "sgn", "^2", "^3", "e^", "sin", "cos", "tan", "asin",
    "acos", "atan", "sinh", "cosh", "tanh", "distort", "softclip", "/2", "*2", "++", "--", "cmpl",
    "biuni", "unibi", "sin1", "cos1", "degrad", "raddeg", "2^", "10^", "sigm", "~", "0=", "0<",
    "0>", "0>=", "0<=", "dbamp", "log", "log2", "log10",
]
# Below zero sapf returns NaN where SC's sqrt and ampdb keep the sign. 1/ near zero magnifies
# SinOsc's float32 error. Pitch conversions get inputs in range.
POSITIVE = ["sqrt", "rsqrt", "ampdb", "1/", "nnhz", "hznn", "ohz", "hzo", "semiratio", "centsratio",
            "ratiocents"]
BINARY = [
    "+", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "&", "|", "atan2", "hypot", "ring1",
    "ring2", "ring3", "ring4", "difsq", "sumsq", "sqsum", "sqdif", "absdif", "thresh", "amclip",
    "scaleneg", "clip2", "excess", "fold2", "wrap2", "avg2",
]
# Quantizers jump where a/b crosses a step, so float32 and double disagree on a few frames.
STEPPED = ["round", "roundUp", "trunc"]

def _id(op: str) -> str:
    return "".join(c if c.isalnum() else f"x{ord(c):02x}" for c in op)


for op in UNARY:
    CASES.append(Case(f"op1_{_id(op)}", f"{A} {op}", 1e-3))
for op in POSITIVE:
    CASES.append(Case(f"op1_{_id(op)}", f"{POS} {op}", 1e-3))
for op in BINARY:
    CASES.append(Case(f"op2_{_id(op)}", f"{A} {B} {op}", 1e-3))
for op in STEPPED:
    CASES.append(Case(f"op2_{_id(op)}", f"{A} {B} {op}", 1e-3, max_bad=5))
CASES += [
    Case("op2_pow", f"{POS} {B} ^", 1e-3),
    # Original sapf computes b - a for two signals.
    Case("op2_minus", f"{A} {B} -", 1e-3, ref_expr=f"{A} {B} neg +"),
    # Original sapf's ratiosemi divides log2 by 12 where semitones need 12 times log2.
    Case("op1_ratiosemi", f"{POS} ratiosemi", 1e-3, ref_expr=f"{POS} log2 12 *"),
]
