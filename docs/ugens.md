# sapf UGen words

Each word keeps sapf's arguments and units and converts them for the SC UGen. Fidelity is measured by `tests/py/test_conformance.py` against original sapf at 48 kHz:

- **exact**: every sample within 1e-4 (1e-3 for feedback and envelope UGens), apart from the exceptions in Notes.

- **approx**: SC's algorithm differs from sapf's. The figure is the RMS of the difference over the RMS of sapf's output.

All UGens run at audio rate.

## Oscillators

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `sinosc` | `SinOsc` | exact | phase in cycles, times 2pi |
| `lfsaw` | `LFSaw` | exact | phase in cycles, times 2; one frame can land on the other side of the wrap |
| `lftri` | `LFTri` | exact | phase in cycles, times 2 |
| `lfpulse` | `LFPulse` | exact | |
| `lfpulseb` | `LFPulse + duty - 1` | exact | high is `duty`, low `duty - 1` |
| `lfsquare` | `LFPulse * 2 - 1` | exact | |
| `impulse` | `Impulse` | exact | |

`lfsaw`, `lftri`, `lfpulse`, `lfpulseb` and `lfsquare` take a constant phase, as in sapf.

## Noise

| Word | SC |
|-|-|
| `white` `pink` `brown` | `WhiteNoise` `PinkNoise` `BrownNoise`, times amp |
| `dust` `dust2` | `Dust` `Dust2`, times amp |
| `lfnoise0` `lfnoise1` | `LFNoise0` `LFNoise1` |

Random output is not compared with sapf.

## Filters

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `hpf` | `HPF` | exact | |
| `leakdc` | `LeakDC` | exact | SC starts from the first input, sapf from zero |
| `lpf` | `LPF` | approx 0.066 | |
| `rlpf` `rhpf` | `RLPF` `RHPF` | approx 0.002 | |
| `bpf` `bsf` | `BPF` `BRF` | approx 0.016, 0.006 | bandwidth in octaves to 1/Q: `2^(bw/2) - 2^(-bw/2)` |
| `resonz` | `Resonz / (1 - R^2)` | approx 0.040 | sapf does not normalize gain; `R = 1 - w0*rq/2` |
| `ringz` | `Ringz` | approx 0.010 | SC counts a nonzero first input sample twice |
| `formlet` | `Formlet * 2` | approx 0.012 | as `ringz` |
| `decay` `decay2` | `Decay` `Decay2` | approx 0.005, 0.014 | |
| `lag` `lagud` | `Lag` `LagUD` | approx 0.009, 0.010 | |

## Delays

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `delayn` `delayl` `delayc` | `DelayN` `DelayL` `DelayC` | exact | `delayc` differs on 2 frames at the start |
| `combn` `combl` `combc` | `CombN` ... `* feedback` | exact | sapf's output includes the feedback gain `0.001^(delay/decay)` |
| `alpasn` `alpasl` `alpasc` | `AllpassN` `AllpassL` `AllpassC` | exact | `AllpassC` outputs zero until its delay line fills |

A `maxdelay` of 0 means the delay time, which must then be a constant.

## Panning and misc

| Word | SC | Fidelity |
|-|-|-|
| `pan2` `bal2` `rot2` | `Pan2` `Balance2` `Rotate2` | exact within 2e-3: SC pans through a sine table |
| `fade2` | `XFade2` | exact within 1e-3 |
| `sah` | `Latch` | exact |
| `clip` `wrap` `fold` | `Clip` `Wrap` `Fold` | exact; reals compute in the interpreter |
| `fadein` | `EnvGen`, exponential | exact |
| `fadeout` | `EnvGen`, exponential, frees the synth | exact |

## Operators

Unary and binary math on a `Sig` maps to SC operators, directly or through a few UGens (`ohz`, `biuni`, `sigm`). All mapped operators are exact, with these exceptions:

- Below zero, sapf returns NaN for `sqrt`, `rsqrt` and `ampdb`, and for `^` with a fractional exponent. SC keeps the sign or uses the magnitude.

- `round`, `roundUp` and `trunc` differ on a few frames where float32 and double fall on different sides of a step.

- `ratiosemi` returns semitones, `12 * log2(r)`. Original sapf returns `log2(r) / 12`; see [dev/upstream-sapf-bugs.md](dev/upstream-sapf-bugs.md).

Operators with no SC equivalent (`erf`, `tgamma`, the character tests and others) fail on a `Sig`.

## Not mapped

sapf words with no SC UGen of the same algorithm are not defined: `saw`, `square`, `tri`, `pulse`, `blip`, `osc` and the other wavetable oscillators, `dsf1`, `dsf3`, `vosim`, `smoothsaw`, `klank`, `adsr` and the other envelopes, `lpf1`, `hpf1`, `apf`, `lsf`, `hsf`, `peq`, `hilbert`. `ugen` reaches any SC UGen directly.
