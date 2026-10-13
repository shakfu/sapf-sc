# sapf-sc

sapf interpreter for supercollider

sapf-sc is the [sapf](../sapf) interpreter with its audio engine removed. UGen words build a SuperCollider UGen graph, and `>sd` writes it as a `.scsyndef` file for scsynth.

```
.4 0 lfsaw 2 * [8 7.23] 0 lfsaw .25 * 5/3 + + ohz 0 sinosc .04 * .2 0 4 combn "bubbles" >sd
```

This writes `bubbles.scsyndef` to `$SAPF_SYNTHDEFS`, or to the current directory when it is unset. The def has one `Out.ar` to bus 0 with one input per channel.

## Model

- A UGen word returns a `Sig`, one output of a graph node. A UGen with several outputs, such as `pan2`, returns a list of `Sig`s.

- Math on a `Sig` builds a `BinaryOpUGen` or `UnaryOpUGen`. Math on reals is computed in the interpreter, so constants fold.

- Lists expand to channels, as in sapf. They are lazy, so UGens are created in the order channels are forced.

- Operations on individual samples or on a stream's elements fail on a `Sig`: `reverse`, `at`, `sort`, `skip`, reductions. A SynthDef cannot express them.

- `N`, `NZ` and `T` on a `Sig` end it: an `EnvGen` drops to zero after that many frames or seconds and frees the synth. `fadeout` frees the synth the same way.

- Durations convert to frames at the server's sample rate (`SampleRate.ir`), not the interpreter's.

## Words added

| Word | Stack | Effect |
|-|-|-|
| `>sd` | `(channels name -->)` | Writes `name.scsyndef`. |
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

pytest runs three suites:

- `test_scgf_parity.py`: sapf's SCgf bytes equal [nanosynth](../nanosynth)'s for the same graph.

- `test_conformance.py`: scsynth renders each def in NRT mode (non-real-time, via nanosynth). The output is compared with original sapf's render of the same expression, stored in `tests/fixtures/ref`.

- `test_errors.py`: operations a SynthDef cannot express fail and write no file.

## License

GPL-3.0, as sapf.
