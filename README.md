# stac

A concatenative language for live coding SuperCollider.

stac is postfix: words take their arguments from a stack and leave results on it. Its UGens are SuperCollider's, with SC's arguments, defaults and units; its patterns, `pdef` and `ndef` follow SC's `Pbind`, `Pdef` and `Ndef`. It runs scsynth inside its own process, so it needs no SuperCollider install. stac grew from James McCartney's [sapf](https://github.com/lfnoise/sapf), whose interpreter it keeps; sapf's own words are a library, `sapf.stac`.

```sh
build/stac -p prelude.stac
```
```stac
[440 sinosc .1 *] play = n
n.free
```

- [docs/phrasebook.md](docs/phrasebook.md): 31 everyday phrases, sclang beside stac. Start here.
- [docs/dictionary/](docs/dictionary/): every word, with what it does, when to use it and an example.
- [examples.txt](examples.txt): lines to paste into the REPL, one at a time.

Every example in both runs in the test suite.

## First sounds

```stac
[[440 442] sinosc .1 *] play                       ; stereo: a list of frequencies is a list of channels
stop                                                ; frees every synth and stops every pattern

synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .005 .2 env.asr gate envgen.kr.done * amp * 2 X]
{:freq 660} ping = x                                ; start a synth, keep its node
{:freq 880} x.set
x.release

{:instrument 'ping :degree [0 2 4 5 7] picks :scale 'minor :dur .125} pbind 'mel pdef
[[55 55.2] saw 800 .3 rlpf .1 *] 'drone ndef        ; run again, edited, to crossfade
```

## The language

- **Values and the stack.** `440 sinosc` pushes 440, then `sinosc` takes it and pushes a UGen. `[...]` collects what its code leaves into a list; a list of UGens plays as channels. `= name` binds the top value; `\args [body]` is a function.
- **Definitions.** `def` defines a function, with result types after `-->` that each call checks; `synthdef` a SynthDef, whose parameters are controls; `const` a constant.
- **Namespaces.** SC's 374 UGens are in `sc`, sapf's words in `sapf`, the core words in `stac`, and server objects in `bus`, `buffer`, `group`, `clock` and `env`. `import sc.*` binds names unqualified.
- **Preludes and modules.** A prelude chooses the global names a session starts with: `prelude.stac` imports `sc` and `std`, the standard library. A module is a file, `name.stac`; `import name.*` runs it in its own scope, keeps its `_` names private, and reloads it when it has changed.
- **Live editing.** A top-level name binds late: redefine a word and the patterns that call it use the new one from their next event. A REPL line or a loaded file lands all its changes at once.

The design and its reasoning are in [docs/dev/stac.md](docs/dev/stac.md); [docs/comparison.md](docs/comparison.md) compares stac with sapf.

## Running

```sh
build/stac [-p prelude] ... [-i] [file]
```

- With no file, stac starts the REPL. Tab completes names; the hint shows a word's stack effect.
- With a file, stac runs it, plays what it plays and exits when that ends; `-i` enters the REPL afterwards.
- `-p` loads a prelude and repeats: sapf programs run with `-p prelude.stac -p sapf.stac`. Without `-p`, stac loads `$STAC_PRELUDE`; with neither, only core words and qualified names such as `sc.sinosc` exist.
- `--words` prints every word, after the preludes, as JSON, for editors.

| Variable | Meaning | Default |
|-|-|-|
| `STAC_PRELUDE` | prelude file when no `-p` is given | none |
| `STAC_PATH` | directories, `:`-separated, where `import` looks for modules after the importing file's own | none |
| `STAC_SCSYNTH` | `host:port` of a running scsynth to use over UDP | the embedded server |
| `STAC_SYNTHDEFS` | where `.write` and `score` write files | the current directory |
| `STAC_RECORDINGS` | where `render` writes `.wav` files | the temporary directory |
| `STAC_DEVICE` | audio device of the embedded server, by name | the system's |
| `STAC_OUTPUTS`, `STAC_INPUTS` | its channels | 2, 0 |
| `STAC_SR` | its sample rate | the device's |
| `STAC_PLUGINS` | its UGen plugin folder | `plugins/` beside the executable |
| `STAC_LOG`, `STAC_HISTORY` | REPL log and history files | `~/stac-log.txt`, `~/stac-history.txt` |

Inputs default to 0: with an input stream, scsynth can hang at boot while another program holds the capture device.

Any scsynth works through `STAC_SCSYNTH`, e.g. SuperCollider's (`scsynth -u 57110`) or nanosynth's (`make server`). Over UDP a def larger than about 64 KB does not fit in one packet; write it with `.write` and load it instead.

## Tools

```sh
uv run python scripts/stactool.py fmt [--check] FILE...   # re-indent, keeping every token
uv run python scripts/stactool.py lint FILE...            # undefined words, hidden words, imports
uv run python scripts/dictionary.py check                 # dictionary entries for every word
```

## sapf programs

`sapf.stac` imports sapf's words over the SC UGens and adds the 48 prelude definitions built on them; `examples/sapf.stac` keeps 216 of sapf's 312 examples, 160 of which play or render.

```sh
build/stac -p prelude.stac -p sapf.stac
```
```stac
440 0 sinosc .1 * play                              ; sapf's sinosc: freq and phase
```

sapf's words keep sapf's argument order and units; the [sapf chapter](docs/dictionary/sapf.md) gives each one's SC equivalent, and [docs/ugens.md](docs/ugens.md) how closely each matches SC. Where original sapf has a bug, stac computes what sapf's code and documentation describe; see [docs/dev/upstream-sapf-bugs.md](docs/dev/upstream-sapf-bugs.md).

How the sapf words map onto scsynth:

- **Graphs.** A sapf UGen word builds an SC UGen. `kr` gives a control-rate copy, and an audio-rate UGen that feeds only control-rate sapf UGens becomes control rate too. `N`, `NZ` and `T` end a UGen with an `EnvGen` that frees the synth; `line`, `xline` and `fadeout` end the same way.
- **Sequences in a graph.** `seq`, `iseq`, `steps` and `imps` read their value streams when the def compiles and store them in a `Dseq`. An infinite stream keeps its first 1024 values and loops its period.
- **Polyphony.** `ola` returns `In.ar` on a private bus; each sound is its own synth writing to it. `play` starts the voices on schedule, .2 s ahead in timed bundles, and they stop with their main synth. A voice starts on the 64-frame block that holds its time.
- **Files.** `>sd` writes a `.scsyndef`, `>sf` renders a `.wav` until the first channel ends, `>sfo` then opens it, and `>nrt` writes an NRT score with every `ola` voice. The SSP's `scsy` plays a `>nrt` score: its voices start on schedule and the score loops at its length.
- **Wave tables.** `wavefill`, `sawTbl` and the other table words make 30 third-octave tables; `osc` plays them with `VOsc` over buffers 512 upward. `play` and `>nrt` load the buffers.
- **Samples.** `sf>` and `playbuf` play a WAV or AIFF file with `PlayBuf` from buffers 0..63. `>sd` names the files in the def's sidecar, `name.txt`, for `scsy`.

## Build and test

```sh
make build                       # build/stac and build/plugins
make test                        # ctest, then pytest (needs uv)
make reference SAPF_ORIG=...     # regenerates tests/fixtures/ref with original sapf
```

The first build downloads SuperCollider 3.14.1's source and libsndfile 1.2.2 as pinned release tarballs, checked by SHA-256, and builds scsynth's library and core UGens with them; `cmake/EmbeddedScsynth.cmake` says how to build offline. Tested on macOS and Linux; Linux needs ALSA's headers (`libasound2-dev` or `alsa-lib-devel`). `-DSAPF_EMBED_SCSYNTH=OFF` builds without scsynth, for a server over UDP only.

`make test` runs the C++ unit tests (interpreter, parser, completion) and 23 pytest suites. Among them:

- `test_scgf_parity.py` and `test_ugen_sweep.py`: stac's SynthDef bytes equal [nanosynth](../nanosynth)'s for the same graph.
- `test_conformance.py`: scsynth renders sapf expressions offline, compared with original sapf's renders in `tests/fixtures/ref`.
- `test_defs.py`, `test_modules.py`, `test_patterns.py`: statements, modules, patterns, clocks, `pdef` and `ndef`, partly against a recording stand-in for scsynth.
- `test_play.py` and `test_play_embedded.py`: synths, buses and buffers on a real scsynth, silently. Skipped without an audio device.
- `test_dictionary.py` and `test_phrasebook.py`: every example in the documentation.

## License

GPL-3.0, as sapf.
