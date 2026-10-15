# sapf and sapf-sc compared

## Baselines

- **sapf**: [lfnoise/sapf](https://github.com/lfnoise/sapf), James McCartney's interpreter. Reference renders in `tests/fixtures/ref` come from its v0.1.22 macOS release and an earlier build of `sapf (fork)`.

- **sapf (fork)**: [shakfu/sapf](https://github.com/shakfu/sapf), a fork of sapf. It adds Linux and Windows backends (ALSA, RtAudio, RtMidi, libsndfile, FFTW). The word counts and source diffs below are taken against this tree.

- **sapf-sc**: this repository, at `9297625`.

Word counts come from a static scan of `DEF*` macros and `vm.def*` calls in `src/`. Runtime checks used `build/stac`.

## Architecture

| | sapf | sapf-sc |
|-|-|-|
| Signal | lazy stream of doubles (`ZList`), computed by the interpreter | `Sig`: one output of a UGen graph node |
| DSP | sapf's own UGens, in C++ | scsynth's core UGens (SuperCollider 3.14.1), float32 |
| Audio output | CoreAudio, ALSA, RtAudio or Max | scsynth, embedded or over UDP (`STAC_SCSYNTH`) |
| `play` | pulls samples to the device | compiles a SynthDef, sends it, starts a synth |
| File render (`>sf`) | pulls samples to a file | scsynth NRT render |
| Polyphony (`ola`) | mixes streams in the interpreter | one synth per voice on a private bus, scheduled by a thread or written into an NRT score |
| New outputs | none | `.scsyndef` (`>sd`), NRT score `.osc` (`>nrt`), `scsy` sidecar `name.txt` |
| Dependencies | Accelerate or FFTW, RtAudio, RtMidi, libmanta | libscsynth, libsndfile 1.2.2, PortAudio (non-macOS), Accelerate (macOS) |

## Language

### Unchanged

The parser, VM and non-audio words are the same code. Lines differing from `../sapf`:

| File | Lines |
|-|-|
| `Parser.cpp`, `CoreOps.cpp`, `SetOps.cpp`, `MultichannelExpansion.cpp` | 0 |
| `VM.cpp`, `Opcode.cpp` | 1 each |
| `MathOps.cpp` | 2 (the minus fix) |
| `Object.cpp` | 20 |

Syntax, stack effects, lists, forms, automapping (`@`), reductions and scans, and math on reals behave as in sapf. 620 words share a name.

### Two kinds of signal

In sapf, every signal is a `ZList`. `#[1 2 3]`, `440 0 sinosc` and `sf>` output are interchangeable. In sapf-sc they are separate types:

- UGen words return `Sig`. List operations on a `Sig` fail: `at`, `reverse`, `sort`, `skip`, reductions. Example: `440 0 sinosc 3 at` gives `wrong type for at : expected List. got Sig.`

- A `ZList` cannot feed a UGen or `>sf`. `#[100 200] 0 sinosc` gives `expected Real or UGen. got ZList.` Exceptions: wave tables (`wavefill` into `osc`) and value streams read by `seq`, `iseq`, `steps`, `imps`, `lines`, `xlines`, `curves`.

- Math on a `Sig` builds a `BinaryOpUGen` or `UnaryOpUGen`. Math on reals folds in the interpreter.

- A program that inspects or rearranges samples has no sapf-sc equivalent. A SynthDef cannot express it.

### Shared words with changed meaning

| Word | sapf | sapf-sc |
|-|-|-|
| `kr` | `(fun n --> out)`: runs `fun` at `SR / n`, upsamples linearly | `(in --> out)`: control-rate copy; upstream UGens feeding only `kr` inputs become control rate |
| `sf>` | reads the file into streams, not real time | `(filename --> channels)`: `PlayBuf`, once, from scsynth buffers 0..63; WAV or AIFF only |
| `play`, `stop` | device playback | synth on scsynth; `stop` frees every synth on that server and ends `ola` scheduling |
| `>sf`, `>sfo` | writes until the streams end | NRT render; refuses channels that never end or end on a signal condition (e.g. finite `seq`) |
| `N`, `T`, `NZ` on a signal | truncate the stream | `EnvGen` drops to zero and frees the synth |
| `line`, `xline`, `fadeout` | finite stream | UGen, then the synth ends |
| `saw`, `tri`, `square`, `pulse` | band limited (tables broken upstream) | `LFSaw`/`LFTri`/`LFPulse`, not band limited |
| `sinoscfb` | any phase | phase must be 0 (`SinOscFB` has no phase input) |
| `lfsaw`, `lftri`, `lfpulse*`, `saw`..`pulse`, `osc` | phase may be a signal | phase must be a constant |
| `steps`, `imps`, `lines`, `xlines`, `curves` | rate may be a signal | rate must be a real |
| `adsr` | tempo may be a signal | tempo must be a real |
| `seq`, `iseq`, `steps`, `imps` | read the value stream lazily, forever | read it at compile time into a `Dseq`; infinite streams keep 1024 values and loop |
| `itd` | `--> out` | `--> [left right]` |
| `wavefill` | `sin(i*theta + phases[i]) ... cos(pi*i/n)` | `sin(i*theta + 2pi*phases[i]) ... cos(pi/2*i/n)`: the formula sapf's code describes |
| `-r` CLI flag | audio sample rate | rate for list timing ops (`T`); UGen durations use the server's `SampleRate.ir` |

Per-UGen fidelity against sapf is in [ugens.md](ugens.md).

### Bug fixes

sapf-sc computes what sapf's code and docs describe. Details in [dev/upstream-sapf-bugs.md](dev/upstream-sapf-bugs.md).

| Expression | sapf | sapf-sc |
|-|-|-|
| `#[1 2 3] #[10 20 30] -` | `#[9 18 27]` | `#[-9 -18 -27]` |
| `2 ratiosemi` | `log2(2) / 12` | `12` |
| `[2 0 1] cyc Z finite` | 1 | 0 |
| `[97 98 99] unspell` | reads uninitialized memory | `"abc"` |
| `skipWhile` on `ZList`, cutoff mid-chunk | wrong values, reads past buffer | correct (fixed in sapf-sc; see `CHANGELOG.md`) |

### Numeric differences

- scsynth computes in float32, sapf in double. `round`, `roundUp` and `trunc` differ on a few frames.

- Below zero, sapf gives NaN for `sqrt`, `rsqrt`, `ampdb` and fractional `^`. SC keeps the sign or uses the magnitude.

- `log`, `log2`, `log10` of a negative `Sig` are finite on Linux x86 (nova-simd path), NaN on macOS and ARM.

- Triggers: sapf fires on every sample above zero; SC fires on the rise above zero.

- `ola` voices start on the 64-frame block holding their onset, up to 63 frames early.

## Words

### Removed (127)

| Group | Words |
|-|-|
| MIDI (33) | `mctl`, `mbend`, `mgate`, `mpoly`, `mprog`, `mtouch`, `mlastkey`, `mlastvel` and their `1` variants; `xmctl`, `xmbend`, `xmpoly`, `xmtouch`, `xmlastvel` and their `1` variants; `midiStart`, `midiStop`, `midiRestart`, `midiList`, `midiDebug`, `midiConnectInput`, `midiDisconnectInput` |
| Mouse (8) | `mousex`, `mousey`, `xmousex`, `xmousey` and their `1` variants |
| Live control (1) | `zctl`; prelude words built on it (`zplug`, `set`) do not run |
| Oscillators (13) | `blip`, `dsf1`, `dsf3`, `klang`, `oscp`, `par`, `smoothsaw`, `smoothsawpwm`, `sosc`, `ssaw`, `tsinosc`, `vosim`, `vsaw` |
| Noise (14) | `blue`, `violet`, `gray`, `gray64`, `pink0`, `velvet`, `crackle`, `toosh`, `tooshp`, `citywhite`, `rawhite`, `wangwhite`, `xorwhite`, `xorwhite2` |
| Filters (17) | `apf`, `lsf`, `lsf1`, `hsf`, `peq`, `hilbert`, `ampf`, `leaky`, `lag3`, `lagud2`, `lagud3`, `rlpfc`, `rhpfc`, `rlpf2c`, `rhpf2`, `rhpf2c`, `lpcombc` |
| Envelopes (24) | `dadsr`, `dahdsr`, `endfade`; `cosenv`, `hanenv`, `han2env`, `parenv`, `quadenv`, `octenv`, `trienv`, `tri2env`, `trapezenv`, `trapez2env` and their `t` variants; `gaussenv` |
| Sequencing and triggers (11) | `gates`, `cubics`, `tempo`, `beats`, `tr`, `ntr`, `gate`, `pdiv`, `pause`, `tsig`, `krc` |
| Spectral and utility (6) | `fft`, `ifft`, `kaiser`, `sgram`, `record`, `bench` |

`ugen` reaches any SC UGen by name, so several removed words can be rebuilt by hand, e.g. `[in freq] 'Hilbert 'ar 2 ugen`.

### Added (6)

| Word | Stack |
|-|-|
| `>sd` | `(channels name -->)` writes `name.scsyndef` |
| `>nrt` | `(channels seconds name -->)` writes an NRT score with `ola` voices |
| `ctl` | `(default name --> sig)` named synth parameter |
| `ugen` | `(inputs name rate numOutputs --> out)` any SC UGen |
| `soundin` | `(channel --> sig)` hardware input |
| `playbuf` | `(filename rate loop --> channels)` |

## Features

| Feature | sapf | sapf-sc |
|-|-|-|
| Real-time playback | yes | yes, via scsynth |
| Export to SuperCollider | no | `.scsyndef`, NRT score |
| Named synth parameters | no | `ctl` |
| Arbitrary UGens | no | `ugen`, plus any plugin in `STAC_PLUGINS` |
| Audio input | no | `soundin` (`STAC_INPUTS`) |
| Remote server | no | `STAC_SCSYNTH=host:port` |
| Sample playback | `sf>` into memory | `sf>`, `playbuf` (looping, rate) from scsynth buffers |
| MIDI input | CoreMIDI, RtMidi | removed |
| Mouse input | macOS | removed |
| Manta controller (`-m`) | yes | removed |
| Spectrogram, FFT | `sgram`, `fft`, `ifft` | removed |
| Live patching (`zctl`, `zplug`, `set`) | yes | removed; needs `/n_set` on running synths |
| Record while playing | `record` | removed; use `>sf` |
| CPU benchmark | `bench` | removed |
| Sample-level inspection of signals | yes | no; signals are graph nodes |
| Target hardware | desktop | desktop, and the SSP via `scsy` (`>nrt` scores, `@buf` sidecars) |
| Platforms | macOS (upstream); macOS, Linux, Windows (fork) | macOS, Linux tested; Windows branches untested |
| Tests | ctest, gtest | ctest, gtest, pytest conformance against sapf renders |

## Compatibility

- `prelude.stac`: 280 of 285 definitions load. The 5 dropped use `hilbert`, `fft` or `kaiser`.

- `examples/sapf.stac`: 216 of 312 examples run; 160 play or render. Most failures use `lpcombc`, mouse words, `violet`, `klang`, `pdiv`, `zplug` or `set`.

- Identical output for old programs is not a goal. See the policy in [dev/upstream-sapf-bugs.md](dev/upstream-sapf-bugs.md).
