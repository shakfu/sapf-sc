# sapf UGen words

These are the words of the namespace `sapf`, such as `sapf.sinosc`, which `sapf.stac` imports unqualified. stac's own UGens, with SC's arguments, are in `sc`; see the [dictionary](dictionary/sc.md).

Each word keeps sapf's arguments and units and converts them for the SC UGen. Fidelity is measured by `tests/py/test_conformance.py` against original sapf at 48 kHz:

- **exact**: every sample within 1e-4 (1e-3 for feedback and envelope UGens), apart from the exceptions in Notes.

- **approx**: SC's algorithm differs from sapf's. The figure is the RMS of the difference over the RMS of sapf's output.

UGens run at audio rate unless `kr` lowers them; see the README. Fidelity is measured at audio rate. A 30 Hz `sinosc kr` matches sapf within 0.6% RMS once delayed one block; the error grows with frequency.

## Oscillators

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `sinosc` | `SinOsc` | exact | phase in cycles, times 2pi |
| `sinoscm` | `SinOsc * mul + add` | exact | |
| `sinoscfb` | `SinOscFB` | exact to feedback .8 | phase must be 0; above feedback 1 it is chaotic, and float32 and double runs part |
| `lfsaw` | `LFSaw` | exact | phase in cycles, times 2; one frame can land on the other side of the wrap |
| `lftri` | `LFTri` | exact | sapf's phase moves along the rising segment only: `.5` starts at -1 |
| `lfpulse` | `LFPulse` | exact | |
| `lfpulseb` | `LFPulse + duty - 1` | exact | high is `duty`, low `duty - 1` |
| `lfsquare` | `LFPulse * 2 - 1` | exact | |
| `impulse` | `Impulse` | exact | |
| `saw` `tri` `square` | `LFSaw` `LFTri` `LFPulse * 2 - 1` | see below | phase in cycles |
| `pulse` | `(LFSaw >= 1 - 2 duty) - duty` | see below | -duty to 1 - duty; duty may vary within a cycle |

`lfsaw`, `lftri`, `lfpulse`, `lfpulseb` and `lfsquare` take a constant phase, as in sapf. So do `saw`, `tri`, `square` and `pulse`; sapf's accept a signal.

`saw`, `tri`, `square` and `pulse` are band limited in sapf. Original sapf's tables for them are broken ([dev/upstream-sapf-bugs.md](dev/upstream-sapf-bugs.md)), so they are tested against the waveforms the tables describe, built from sapf's `lf` words, and match exactly. They are not band limited and alias at high frequencies.

An `LFTri` that starts in its last quarter cycle outputs one wrong frame first.

## Noise

| Word | SC |
|-|-|
| `white` `pink` `brown` | `WhiteNoise` `PinkNoise` `BrownNoise`, times amp |
| `dust` `dust2` | `Dust` `Dust2`, times amp |
| `lfnoise0` `lfnoise1` | `LFNoise0` `LFNoise1` |
| `lfnoise3` | `LFDNoise3 * .8`; both interpolate a Catmull-Rom spline |

Random output is not compared with sapf.

## Filters

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `hpf` | `HPF` | exact | |
| `lpf1` `hpf1` | `FOS` | exact | sapf's first-order coefficients, computed in the graph |
| `lpf2` `hpf2` `rlpf2` | two `SOS` | exact; 0.004 when freq moves | `SOS` is direct form II, sapf direct form I |
| `klank` | sum of `Ringz * amp` | as `ringz` | sapf's klank is a bank of its ringz filters |
| `leakdc` | `LeakDC` | exact | SC starts from the first input, sapf from zero |
| `lpf` | `LPF` | exact | original sapf's lpf is wrong at a constant freq; see [dev/upstream-sapf-bugs.md](dev/upstream-sapf-bugs.md) |
| `rlpf` `rhpf` | `RLPF` `RHPF` | approx 0.002 | |
| `bpf` `bsf` | `BPF` `BRF` | approx 0.016, 0.006 | bandwidth in octaves to 1/Q: `2^(bw/2) - 2^(-bw/2)` |
| `resonz` | `Resonz / (1 - R^2)` | approx 0.040 | sapf does not normalize gain; `R = 1 - w0*rq/2` |
| `ringz` | `Ringz` | approx 0.010 at 1 kHz, 0.032 at 300 Hz | SC counts a nonzero first input sample twice |
| `formlet` | `Formlet * 2` | approx 0.012 | as `ringz` |
| `decay` `decay2` | `Decay` `Decay2` | approx 0.005, 0.014 | |
| `lag` `lagud` `lag2` | `Lag` `LagUD` `Lag2` | approx 0.009, 0.010, 0.013 | |

## Delays

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `delayn` `delayl` `delayc` | `DelayN` `DelayL` `DelayC` | exact | `delayc` differs on 2 frames at the start |
| `combn` `combl` `combc` | `CombN` ... `* feedback` | exact | sapf's output includes the feedback gain `0.001^(delay/decay)` |
| `alpasn` `alpasl` `alpasc` | `AllpassN` `AllpassL` `AllpassC` | exact | `AllpassC` outputs zero until its delay line fills |
| `flange` `flangep` | `DelayC` minus or plus `DelayN` | within 1e-3 RMS | sapf interpolates Lagrange, `DelayC` Hermite |

A `maxdelay` of 0 means the delay time, which must then be a constant.

## Panning and misc

| Word | SC | Fidelity |
|-|-|-|
| `itd` | two `DelayC` | within 1e-3 RMS: sapf interpolates Lagrange, `DelayC` Hermite |
| `pan2` `bal2` `rot2` | `Pan2` `Balance2` `Rotate2` | exact within 2e-3: SC pans through a sine table |
| `fade2` | `XFade2` | exact within 1e-3 |
| `sah` | `Latch` | exact |
| `clip` `wrap` `fold` | `Clip` `Wrap` `Fold` | exact; reals compute in the interpreter |
| `line` | `Line`, then ends | exact |
| `xline` | `XLine`, then ends; across zero, a cubed `Line` between cube roots, as sapf | exact |
| `iwrap` `ifold` | none; reals only | exact |
| `adsr` | `EnvGen` with a gate that closes at note off | 0.003 RMS: sapf starts each segment a frame late |
| `fadein` | `EnvGen`, exponential | exact |
| `fadeout` | `EnvGen`, exponential, frees the synth | exact |

## Sequencing

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `seq` | `Demand`, `Dseq` | exact | |
| `iseq` | `Demand * (trig > 0)` | exact | |
| `steps` | `Duty`, `Dseq` | exact, apart from drift | rate must be a real |
| `imps` | `TDuty`, `Dseq` | exact, apart from drift | rate must be a real |

- Values and durations are reals or lists of reals; the README gives how streams are stored.

- sapf fires on every sample above zero; SC fires when the trigger rises above zero. Single-sample triggers such as `impulse` behave the same.

- sapf sums `rate / SR` per frame in double precision, so a step can start one frame late: frame 481 for .01 s at 48 kHz. SC counts frames and starts it at 480.

- sapf integrates a signal rate within each step. `Duty` reads its duration once per step, so `steps` and `imps` reject a UGen rate.

## Envelopes

| Word | SC | Fidelity | Notes |
|-|-|-|-|
| `lines` `xlines` `curves` | `DemandEnvGen` over `Dseq`s | exact, apart from drift | rate must be a real |

Values, durations and curvatures are read as for `steps`. At a duration of zero or less sapf jumps to the value; `DemandEnvGen` takes one frame. `DemandEnvGen` starts a frame ahead and outputs each segment's first step rather than its start, so a two-frame segment holding the first value comes first.

## Wave tables

| Word | SC | Fidelity |
|-|-|-|
| `osc` | `VOsc` over 30 buffers | within 1e-3 RMS of tables computed in numpy |

`osc` takes a constant phase; sapf's accepts a signal. `VOsc` crossfades linearly between neighbouring tables, sapf along an S curve.

## Polyphony

`ola` is in the README. Voices match sapf apart from sapf's drift and block-aligned starts.

## Operators

Unary and binary math on a `Sig` maps to SC operators, directly or through a few UGens (`ohz`, `biuni`, `sigm`). All mapped operators are exact, with these exceptions:

- Below zero, sapf returns NaN for `sqrt`, `rsqrt` and `ampdb`, and for `^` with a fractional exponent. SC keeps the sign or uses the magnitude.

- `round`, `roundUp` and `trunc` differ on a few frames where float32 and double fall on different sides of a step.

- `ratiosemi` returns semitones, `12 * log2(r)`. Original sapf returns `log2(r) / 12`; see [dev/upstream-sapf-bugs.md](dev/upstream-sapf-bugs.md).

Operators with no SC equivalent (`erf`, `tgamma`, the character tests and others) fail on a `Sig`.

## Not mapped

sapf words with no SC UGen of the same algorithm are not defined: `blip`, `oscp`, `sosc` and the other wavetable oscillators, `dsf1`, `dsf3`, `vosim`, `smoothsaw`, `dadsr`, `dahdsr` and the other envelopes, `apf`, `lsf`, `hsf`, `peq`, `hilbert`. `lpcombc` filters inside its feedback loop, which needs single-sample feedback that no SC core UGen gives. `ugen` reaches any SC UGen directly.
