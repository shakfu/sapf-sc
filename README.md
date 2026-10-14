# sapf-sc

sapf interpreter for supercollider

sapf-sc is the [sapf](https://github.com/lfnoise/sapf) interpreter with its macOS-only audio engine replaced by SuperCollider's scsynth, embedded as a library. UGen words build a SuperCollider UGen graph, and `>sd` writes it as a `.scsyndef` file for scsynth.

```
.4 0 lfsaw 2 * [8 7.23] 0 lfsaw .25 * 5/3 + + ohz 0 sinosc .04 * .2 0 4 combn "bubbles" >sd
```

This writes `bubbles.scsyndef` to `$SAPF_SYNTHDEFS`, or to the current directory when it is unset. The def has one `Out.ar` to bus 0 with one input per channel.

## Listening

`build/sapf` runs the embedded scsynth inside the sapf process. The first `play` starts it on the system's audio device; `stop` frees every synth and ends the scheduling of `ola` voices.

```
build/sapf
440 0 sinosc .1 * play
stop
```

Run with a file, `sapf` plays what the file plays and exits when it ends; Ctrl-C ends an endless one. A synth ends when its signal ends, as in sapf, and the `ola` voices a synth reads stop with it.

The embedded server reads these when it starts:

| Variable | Meaning | Default |
|-|-|-|
| `SAPF_DEVICE` | audio device, by name | the system's |
| `SAPF_OUTPUTS`, `SAPF_INPUTS` | channels | 2, 2 |
| `SAPF_SR` | sample rate | the device's |
| `SAPF_PLUGINS` | UGen plugin folder | `plugins/` beside the executable |

`>sf` renders to a file instead, as sapf's does: `channels "name" >sf` writes `name.wav` to `$SAPF_RECORDINGS`, or the temp folder, at the interpreter's rate (`-r`), until the first channel ends. A channel ends where `N`, `T`, `line` and the other finite words end it; `>sf` refuses channels that never end, or end when a signal decides, as a finite `seq` does. The embedded scsynth renders the `>nrt` score in NRT mode, so `ola` voices, wave tables and samples come out as `play` would play them. `>sfo` then opens the file.

To use a server of your own, set `SAPF_SCSYNTH` to its `host:port`; `play` and `stop` then talk to it over UDP. Any scsynth works, e.g. SuperCollider's own (`scsynth -u 57110`) or nanosynth's (`make server`). Over UDP, `play` waits up to 1 s for scsynth to confirm the def and reports an error otherwise, and a def over about 64 KB does not fit in one packet; write it with `>sd` and load it instead.

## Model

- A UGen word returns a `Sig`, one output of a graph node. A UGen with several outputs, such as `pan2`, returns a list of `Sig`s.

- Math on a `Sig` builds a `BinaryOpUGen` or `UnaryOpUGen`. Math on reals is computed in the interpreter, so constants fold.

- Lists expand to channels, as in sapf. They are lazy, so UGens are created in the order channels are forced.

- Operations on individual samples or on a stream's elements fail on a `Sig`: `reverse`, `at`, `sort`, `skip`, reductions. A SynthDef cannot express them.

- `N`, `NZ` and `T` on a `Sig` end it: an `EnvGen` drops to zero after that many frames or seconds and frees the synth. `line`, `xline` and `fadeout` end the same way. As in sapf, a channel computed from a finite signal is zero from the frame that signal ends.

- `seq`, `iseq`, `steps` and `imps` read their value streams when the def compiles and store them in a `Dseq`. A finite stream ends the signal. An infinite stream keeps its first 1024 values and loops its period, or all 1024 values if none repeats. Random streams therefore repeat after 1024 values.

- Durations convert to frames at the server's sample rate (`SampleRate.ir`), not the interpreter's.

- UGens run at audio rate. `kr` gives a control-rate copy, and an audio-rate UGen that feeds only control-rate sapf UGens becomes control rate too: `.4 0 lfsaw 2 * kr` computes the LFSaw at control rate. A UGen also used at audio rate stays one UGen, read through `A2K`, so a noise source is not split in two. Inputs of a `ugen` node stay audio rate, because a control-rate analysis UGen such as `Amplitude` reads every input sample. A control-rate channel reaches `Out` through `K2A`, one block late.

## Polyphony

`ola` (sapf's overlap add) returns `In.ar` on a private bus, so effects after it compile into the main def. Each sound is its own synth writing to that bus:

- `play` starts the main synth, then a thread that starts each voice at its time, .2 s ahead in a timed bundle. The voices stop when sapf exits; run a file with `-i` to keep them going.
- `>nrt` writes an NRT score (scsynth's non-real-time command file) with the main def and every voice up to a time. `scsynth -N name.osc _ out.wav 48000 WAV float` renders it.
- `>sd` rejects `ola`: one SynthDef cannot start synths.

On the SSP, `scsy` plays a `>nrt` score: Load it as a def. Its main def takes `p1`..`p16` and the sidecar; its voices start on schedule and the score loops at its length, so random choices repeat after that. Voices that compile alike share one def; `scsy` holds at most 900. See `scsy`'s README in the SSP repo. Private buses run from 512 to 959, below the 32 buses `scsy` keeps for controls.

scsynth starts a synth at the start of the 64-frame block that holds its time, so a voice can start up to 63 frames early. `OffsetOut` would start it on the exact frame but crashes scsynth's NRT renderer.

## Wave tables

`wavefill`, `sawTbl`, `sqrTbl`, `triTbl` and `parTbl` return sapf data: 30 third-octave tables of 4096 samples in one signal. `osc` plays them with `VOsc` over 30 consecutive scsynth buffers from 512 upward, picking tables by frequency as sapf does. `play` and `>nrt` load the buffers, so a `>nrt` score carries its tables to the SSP; `>sd` rejects `osc`, because a SynthDef cannot load buffers. Original sapf's tables are broken, so these follow the formula its code describes; see [docs/dev/upstream-sapf-bugs.md](docs/dev/upstream-sapf-bugs.md).

## Samples

`sf>` and `playbuf` play a WAV or AIFF file with `PlayBuf` from one of scsynth's buffers 0..63. `sf>` plays it once at rate 1, as sapf's `sf>` reads it: one signal per channel, ending with the file. Neither resamples a file recorded at another rate; sapf did not either.

```
"loop.wav" 1 1 playbuf 0 at .5 * play       ; looped, first channel
"hit.wav" sf> play                           ; once
```

- `play` loads the file into scsynth by its absolute path.
- `>nrt` writes the path as given. On the SSP a relative path is relative to the score's folder.
- `>sd` writes `// @buf N path` lines into the sidecar, `name.txt`, and keeps its other lines. `scsy` loads the files before the synth starts; copy them beside the def.

## Words added

| Word | Stack | Effect |
|-|-|-|
| `>sd` | `(channels name -->)` | Writes `name.scsyndef`. |
| `play` | `(channels -->)` | Sends the def to scsynth and starts a synth. |
| `stop` | `(-->)` | Frees every synth on that scsynth and stops `ola` voices. |
| `>sf` | `(channels name -->)` | Renders `name.wav` until the first channel ends. |
| `>sfo` | `(channels name -->)` | As `>sf`, then opens the file. |
| `>nrt` | `(channels seconds name -->)` | Writes `name.osc`, an NRT score of channels and their `ola` voices. |
| `sf>` | `(filename --> channels)` | A sound file's channels, once. |
| `playbuf` | `(filename rate loop --> channels)` | A sound file's channels at `rate`, looped when `loop` is not 0. |
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
make build                       # build/sapf and build/plugins
make test                        # ctest, then pytest (needs uv)
make reference SAPF_ORIG=...     # regenerates tests/fixtures/ref with original sapf
```

The first build downloads SuperCollider 3.14.1's source and libsndfile 1.2.2 as pinned release tarballs, checked by SHA-256, and builds scsynth's library and core UGens with them; see `cmake/EmbeddedScsynth.cmake`, which also says how to build offline. Tested on macOS and Linux. Linux needs ALSA's headers (`libasound2-dev` or `alsa-lib-devel`). `-DSAPF_EMBED_SCSYNTH=OFF` builds without scsynth, for writing defs or a server over UDP only.

pytest runs ten suites:

- `test_scgf_parity.py`: sapf's SCgf bytes equal [nanosynth](../nanosynth)'s for the same graph.

- `test_conformance.py`: scsynth renders each def, or each `>nrt` score for `ola`, in NRT mode (non-real-time, via nanosynth). The output is compared with original sapf's render of the same expression, stored in `tests/fixtures/ref`.

- `test_errors.py`: operations a SynthDef cannot express fail and write no file.

- `test_rates.py`: the rates compilation assigns under `kr`.

- `test_wavetables.py`: `osc` against wave tables computed in numpy.

- `test_sf.py`: `>sf` renders against original sapf's, and its length rule.

- `test_samples.py`: `sf>` and `playbuf` against the sound file, and the sidecar `>sd` writes.

- `test_play.py`: `play` and `stop` against `scripts/server.py` over UDP, with silent synths. Skipped without an audio device.

- `test_play_embedded.py`: `play` with the embedded server. A separate module, because `scripts/server.py` holds the raw ALSA devices while it runs.

- `test_kr_smoke.py`: every mapped word under `kr` runs in scsynth without errors and gives finite, non-silent output. It does not compare with sapf.

## Prelude

`sapf-prelude.txt` is sapf's prelude without the 5 of its 285 definitions that use words sapf-sc does not map: `hilbert`, `fft` and `kaiser`, or definitions built on them. Load it with `-p sapf-prelude.txt` or `$SAPF_PRELUDE`.

`sapf-examples.txt` keeps the 216 of sapf's 312 examples that run; 160 of them play or render. The rest use words with no SC mapping, most often `lpcombc`, the mouse controls, `violet`, `klang` and `pdiv`, or sapf's live-patching `zplug` and `set`.

## License

GPL-3.0, as sapf.
