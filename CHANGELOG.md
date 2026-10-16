# Changelog

## [Unreleased]

### Changed

- The binary is `stac`, and its environment variables are `STAC_*`: `STAC_SCSYNTH`, `STAC_SYNTHDEFS`, `STAC_RECORDINGS`, `STAC_PRELUDE` and the rest. `sapf-prelude.txt` is `prelude.stac`; `sapf-examples.txt` is `examples/sapf.stac`. Phase 1 of `docs/dev/stac.md`. The `SAPF_` names are not read; rename them in your shell.
- sapf's DSP words, `kr`, `ctl`, `>sd`, `>sf`, `>sfo`, `>nrt`, `ola` and `ramp` are in the namespace `sapf`, not the core: `440 0 sapf.sinosc`. `prelude.stac` imports SC's UGens in their place, with SC's arguments: `440 sinosc` is `SinOsc.ar(440)`. `sapf.stac` imports the sapf words over them and holds the 48 prelude definitions built on them. Run sapf programs with `-p prelude.stac -p sapf.stac`; `-p` now repeats. Phase 5 of `docs/dev/stac.md`. The C++ stays; rewriting the words in stac would change output that 108 conformance cases check (`docs/dev/stac-phase5.md`).

  The core's `N`, `keep`, `NZ` and `T` take lists only; `sapf.stac`'s also end a UGen. `play` pushes the synth's `Node`; `sapf.stac`'s leaves nothing. `clip`, `wrap` and `fold` stay core words: they compute on reals and build SC's `Clip`, `Wrap` and `Fold` on a UGen.
- Every generated UGen is in one namespace, `sc`, under its lower-case SC class name: `sc.dseq`, `sc.pv_magfreeze`, `sc.playbuf`. The 14 category namespaces and `ug` are gone; SC has one flat class namespace, and 57 of its 400 UGen classes sit in two help categories, so a category gave no single home. Categories remain as help sections, `*** sc: demand ***`. `buffer` keeps `read`, `load` and `alloc`; `node.group` is `group.new`, as SC's `Group.new`. `stac` holds every core word, so `stac.clip` reaches one a binding hides. The core binds no bare UGens; a prelude imports them (`docs/dev/stac.md`, Proposal: canonical namespaces and import).

  ```
  import sc.*                                  ; every UGen but clip, fold, wrap, rand, irand, linrand
  import sapf.* sapf.N sapf.T                  ; a name imports even over a core word
  ```

  `import` binds while the file compiles, so it must come before a file's or line's other statements. A wildcard skips core words and `_` names; a later import replaces an earlier one. A statement over a word such as `sc import`, so the linter resolves imports without running the program; `stactool.py lint` follows them.
- Modules. `import geo.*` runs `geo.stac`, from the importing file's directory or `$STAC_PATH`, in its own scope, and binds the names it defines; names starting with `_` stay private, and the module's own imports do not leak. Running the import again after the file changed reloads it, and any module that imports it; a failed reload keeps the loaded version. `prelude.stac`'s 242 statements are the module `std.stac`, and `prelude.stac` is two imports. A module starts empty, rather than layered over the importer's names, so what a user binds cannot change a library's behaviour (`docs/dev/stac.md`, Proposal: modules).
- Top-level names bind late: redefining a word reaches every function that calls it, and players running patterns or `ola` voices, from their next event. sapf bound early, as Forth does, so an edited helper left its callers on the old version until they were rerun. A REPL line or a loaded file publishes its bindings when it ends, so an edit of several words lands at once, and an event sees one version throughout. Late binding by versioned snapshots over replacing values in place, which showed half of an edit to players and cost a lock per lookup (`docs/dev/stac.md`, Proposal: modules).

### Added

- 374 generated UGen words, for 374 of SC's 393 public UGen classes: in the namespace `sc`, e.g. `440 sc.sinosc`, with SC's argument names, order and defaults, e.g. `[1 2 3] inf sc.dseq`. A Form in place of the arguments is a keyword call: `{:dur .25 :level seq} sc.duty`. `numChannels` sets the output count: `{:numChannels 2 :bufnum b :loop 1} sc.playbuf`. `.kr` lowers any UGen's rate; `.done` sets a generated UGen's `doneAction` to 2. The namespace is a new word, so no existing program changes. 262 classes match nanosynth's SCgf byte for byte. Generated words run at the rate written: sapf-sc's rate inference neither lowers them nor lowers their inputs, so `FFT` reads an audio-rate source. Phase 2 in [docs/dev/stac.md](docs/dev/stac.md).
- The `def` statement, which compiles a SynthDef with named controls and binds a callable `Def`; `{...} NAME` starts a synth and pushes its `Node`, which takes `.set`, `.release`, `.releaseIn` and `.free`. `t_`, `i_` and `a_` parameters make trigger, scalar and audio controls. The def goes to the server with its first synth. `synthdef` is the same as a function. `def` is now a reserved word.

  ```
  def pad freq:220 amp:.2 gate:1 = [freq saw .01 .2 .6 1 env.adsr gate envgen.kr.done * amp *]
  {:freq 330} pad = n
  {:freq 440} n.set
  n.release
  ```
- `pbind` makes an event stream, a lazy list of Forms; `play` schedules one synth per event and pushes a player that `.stop` ends; `score` writes events to an NRT command file. Events follow SC's default event: pitch from `freq`, `midinote`, or `degree` in a `scale`; `amp` .1; a gate released after `dur * legato`; `'rest`; a list value plays a chord. First slice of phase 4.

  ```
  {:instrument 'ping :degree [0 2 4 5 7] picks :scale 'minor :dur .125} pbind play = p
  p.stop
  ```
- `clock.new` and `pdef`. A clock maps beats to server time, as SC's `TempoClock` does; `{:tempo t} clk.set` changes its tempo while events play. `pdef` replaces a named stream on the next whole beat of the default clock, as SC's `Pdef` with its default quant of 1; `events name clk.pdef` uses another clock, and `{:quant q} p.set` another quant. An event's `:tempo` sets its clock's tempo, as SC's `~tempo`; `score` and `render` follow it. Events and their releases are timed in beats, so a tempo change moves both. `play` now sends each event .2 s before it starts; it sent it .2 s before the next event, late by `dur - .2` s.

  ```
  2 clock.new = clk
  {:instrument 'ping :midinote [60 64] cyc :dur .5} pbind clk.play
  {:tempo 3} clk.set
  {:instrument 'ping :midinote [60 62 64] cyc :dur .25} pbind 'mel pdef = p
  ```
- `.ir` and `.poll` on a UGen, as SC's: `3 bufframes.ir`, `2 lfnoise1.kr.poll`. `.poll` prints at 10 Hz on scsynth's console, labelled `UGen(LFNoise1)` as sclang does; it returns its input, and its `Poll` compiles wherever that input does.
- `group.new`, `bus.control` and `send`. `{} group.new = g` makes a group that `{:target g}` places synths in and `g.free` frees. `2 bus.control = b  [.5 .25] b.set` allocates control buses from 16383 down and sets them. `["/n_free" 1000] send` sends any message, whole reals as ints as sclang's `sendMsg` does.
- The REPL completes names with tab and hints each word's stack effect, as in `sinosc  (freq --> out)`. A plain prefix also finds namespaced words, `pv_magf<TAB>` giving `sc.pv_magfreeze`; after a dot it offers the namespace's words or the methods of stac's objects. An undefined word that a namespace holds gets a suggestion, from sapf and from the linter: `"pv_magfreeze" is an undefined word; sc.pv_magfreeze?`. Names come from the running interpreter, so the planned renaming needs no change here.
- `:instrument` may be a function. Each argument takes the event's field of that name; `freq`, `amp`, `dur`, `legato` and `sustain` take the event's derived values. Each note's channels compile to a def, and notes that compile alike share one, as `ola` voices do in a score. `:out`, a bus index or Bus, sets the bus it writes to. Compiling per note over a def with controls lets a value set what a control cannot, such as a duration given to `T`. The channels must end, as an `ola` voice's do.

  ```
  {:instrument \freq sustain [freq 0 sinosc .1 * sustain T] :degree [0 2 4] cyc :dur .25} pbind play
  ```
- `ndef` plays channels under a name. Run again, it fades the new sound in and the old one out over `fadeTime`, .02 s by default, and pushes the same ndef. The fade is SC's `ProxySynthDef` wrapping: the channels times an `EnvGate`, released by `gate`. A gate envelope per synth was chosen over an `XFade2` on a bus, which needs a mixing synth that lives as long as the ndef. As in SC, the synth writes a private bus and a monitor synth copies it to bus 0. `nd.ar` reads the bus, so one ndef can process another; `nd.stop` and `nd.play` fade the monitor; `nd.end` fades out both.

  ```
  [[55 55.2] saw .1 *] 'drone ndef = d
  {:fadeTime 2} d.set
  [[55 55.2] saw 800 .3 rlpf .1 *] 'drone ndef
  d.stop
  ```
- `render` `(item seconds name -->)` writes `name.wav` with the embedded scsynth, from channels or an event stream. Events render to 2 channels, SC's default for `Score.render`. The duration is explicit, as in SC's `Score.render`, because `>sf`'s inferred length refuses graphs whose end is unknown, and event streams are often infinite.
- `bus.audio`, `buffer.read`, `buffer.load` and `buffer.alloc`, for effects on private buses and samples in buffers. A Bus, Buffer or Node is its index as a UGen input or a control value: `1 bus.audio = fx  {:bus fx :addAction 'tail} verb`. `buffer.load` waits for `/b_alloc` to finish before `/b_setn`, which scsynth would otherwise apply to no buffer. `b.free` frees a bus or buffer for reuse, as SC's `free`.
- The `Env` type and the `env` namespace, SC's envelopes: `.01 .2 .6 1 env.adsr gate envgen.kr.done`. `env.perc`, `env.adsr`, `env.asr`, `env.linen` take SC's times positionally and its other defaults; `env.new` takes levels, times and curves, a curve being a number, a UGen or a shape such as `'exp`. `envgen` and `ienvgen` send an `Env` in SC's two array formats. First slice of phase 3.
- `stac --words` prints every word, after the preludes, as JSON: name, section, stack effect, help, automap mask and source. JSON over plain lines, because editor plugins and the planned dictionary of words need the help too.
- `scripts/sclang_wire.py` writes `spec/sclang-wire.json`: each UGen rate method's input order, read from its `multiNew` call in SC's class library and checked against the inputs sclang built in `spec/sclang-reference.json`. 579 of 635 methods get a trusted order; `gen_ugens.py` uses them in place of hand-written orders. Read from source rather than probed at run time, because the probe needs sclang 3.14.1 and the installed 3.13 cannot compile the 3.14 class library.
- `spec/sclang-reference.json`: 407 UGen classes from SC 3.14.1, with args, defaults and rates, vendored from nanosynth `7a212da`. stac's generated UGen words read it. Vendored rather than read from `../nanosynth`, so a build needs no sibling checkout.
- `scripts/stactool.py`, a formatter and linter for sapf source. Its lexer ports `Parser.cpp`'s token rules and keeps comments, so formatting changes only indentation and trailing whitespace, and it checks that the token stream is unchanged. The linter resolves names in source order, as the compiler does, so it reports words used before they are bound. It reads built-in names from `helpall` in `build/sapf`, so its word list cannot drift from the binary.

  ```
  uv run python scripts/stactool.py fmt [--check] [--indent N|tab] FILE...
  uv run python scripts/stactool.py lint [--prelude prelude.stac] FILE...
  ```

### Fixed

- `{}` segfaulted when printed or read with a key: it evaluated to a Form with no table. It is now an empty Form. Upstream sapf has the same bug; see [docs/dev/upstream-sapf-bugs.md](docs/dev/upstream-sapf-bugs.md).
- `skipWhile` with a predicate function returned wrong values on `Z` lists when the cutoff fell mid-chunk. The tail copy offset the source by `i * sizeof(V)` bytes instead of `i * sizeof(Z)`, so it copied the wrong elements and could read past the end of the source buffer.
- The `sapf` executable linked `libsapf.a` twice, which made Apple ld warn `ignoring duplicate libraries`. It now links only `sapf_server` when present, and gets the engine through it.

### Removed

- Dead `kOneThird` and `NCubicLinez` in `StreamOps.cpp`, left over when `line` and `xline` moved to scsynth UGens.
