# sapf-sc

sapf interpreter for supercollider

sapf-sc is the [sapf](../sapf) interpreter with its audio engine removed. UGen words build a SuperCollider UGen graph, and `>sd` writes it as a `.scsyndef` file for scsynth.

```
.4 0 lfsaw 2 * [8 7.23] 0 lfsaw .25 * 5/3 + + ohz 0 sinosc .04 * .2 0 4 combn "bubbles" >sd
```

This writes `bubbles.scsyndef` to `$SAPF_SYNTHDEFS`, or to the current directory when it is unset. The def has one `Out.ar` to bus 0 with one input per channel.

## Listening

`play` sends the def to a running scsynth and starts a synth; `stop` frees all synths.

```
make server                            # nanosynth's scsynth on 127.0.0.1:57110; Ctrl-C quits
build/sapf
440 0 sinosc .1 * play
stop
```

Any scsynth works, e.g. SuperCollider's own: `scsynth -u 57110`. `$SAPF_SCSYNTH` sets another `host:port`. `play` waits up to 1 s for scsynth to confirm the def and reports an error otherwise. A def over about 64 KB does not fit in one UDP packet; write it with `>sd` and load it instead.

## Model

- A UGen word returns a `Sig`, one output of a graph node. A UGen with several outputs, such as `pan2`, returns a list of `Sig`s.

- Math on a `Sig` builds a `BinaryOpUGen` or `UnaryOpUGen`. Math on reals is computed in the interpreter, so constants fold.

- Lists expand to channels, as in sapf. They are lazy, so UGens are created in the order channels are forced.

- Operations on individual samples or on a stream's elements fail on a `Sig`: `reverse`, `at`, `sort`, `skip`, reductions. A SynthDef cannot express them.

- `N`, `NZ` and `T` on a `Sig` end it: an `EnvGen` drops to zero after that many frames or seconds and frees the synth. `line`, `xline` and `fadeout` end the same way. As in sapf, a channel computed from a finite signal is zero from the frame that signal ends.

- Durations convert to frames at the server's sample rate (`SampleRate.ir`), not the interpreter's.

- UGens run at audio rate. `kr` gives a control-rate copy, and an audio-rate UGen that feeds only control-rate sapf UGens becomes control rate too: `.4 0 lfsaw 2 * kr` computes the LFSaw at control rate. A UGen also used at audio rate stays one UGen, read through `A2K`, so a noise source is not split in two. Inputs of a `ugen` node stay audio rate, because a control-rate analysis UGen such as `Amplitude` reads every input sample. A control-rate channel reaches `Out` through `K2A`, one block late.

## Words added

| Word | Stack | Effect |
|-|-|-|
| `>sd` | `(channels name -->)` | Writes `name.scsyndef`. |
| `play` | `(channels -->)` | Sends the def to scsynth and starts a synth. |
| `stop` | `(-->)` | Frees every synth on that scsynth. |
| `kr` | `(in --> out)` | Control-rate version of `in`. `soundin` and `ugen` keep their rate; `kr` reads them through `A2K`. |
| `ctl` | `(default name --> sig)` | A control-rate synth parameter. Names must be unique in a def. |
| `ugen` | `(inputs name rate numOutputs --> out)` | Any SC UGen. `rate` is `'ar`, `'kr` or `'ir`. |
| `soundin` | `(channel --> sig)` | Hardware input `channel`, from 0. |

```
440 'freq ctl 0 sinosc .1 'amp ctl * "sine" >sd
[440 0] 'SinOsc 'ar 1 ugen .1 * "sine2" >sd
```

[docs/ugens.md](docs/ugens.md) maps each sapf UGen word to its SC UGen and states how closely it matches.

## Build and test

```
make build                       # build/sapf
make test                        # ctest, then pytest (needs uv)
make reference SAPF_ORIG=...     # regenerates tests/fixtures/ref with original sapf
```

pytest runs six suites:

- `test_scgf_parity.py`: sapf's SCgf bytes equal [nanosynth](../nanosynth)'s for the same graph.

- `test_conformance.py`: scsynth renders each def in NRT mode (non-real-time, via nanosynth). The output is compared with original sapf's render of the same expression, stored in `tests/fixtures/ref`.

- `test_errors.py`: operations a SynthDef cannot express fail and write no file.

- `test_rates.py`: the rates compilation assigns under `kr`.

- `test_play.py`: `play` and `stop` against `scripts/server.py`, with silent synths. Skipped without an audio device.

- `test_kr_smoke.py`: every mapped word under `kr` runs in scsynth without errors and gives finite, non-silent output. It does not compare with sapf.

## Prelude

`sapf-prelude.txt` is sapf's prelude without the 26 definitions that use DSP words sapf-sc does not map, such as `lines`, `curves`, `hilbert` and `fft`, or that build on them, such as `adsrenv` and the `*-syn` instruments. Load it with `-p sapf-prelude.txt` or `$SAPF_PRELUDE`. `sapf-examples.txt` is unchanged and still uses words that no longer exist.

## License

GPL-3.0, as sapf.
