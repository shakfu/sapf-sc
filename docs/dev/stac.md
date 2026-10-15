# stac

Status: proposal, 2026-10-15. Nothing here is implemented unless marked "works today".

stac is a concatenative language for live coding SuperCollider. The interpreter plays the part of sclang: values, lists, patterns, scheduling. scsynth does all DSP. The syntax and core come from sapf. UGen names, arguments, units, rates and the server model come from SuperCollider.

stac does not implement sapf. [shakfu/sapf](https://github.com/shakfu/sapf) does. [../comparison.md](../comparison.md) lists where sapf-sc already departs from sapf; stac makes those departures the design. [stac-phrasebook.md](stac-phrasebook.md) tests this design against 23 everyday phrases.

## Principles

1. **Two tiers.** Lang values live in the interpreter. Graph values are nodes of a SynthDef. No sample is ever computed in the interpreter.
2. **SC's UGens, SC's units.** A UGen's word is its SC class name in lower case. It takes SC's arguments in SC's units: `sinosc`'s phase is in radians.
3. **Short to type.** Everyday words are bare, lower case, and need no shift key. Specialised groups sit in namespaces, which also scope tab completion.
4. **Audio rate unless lowered.** A UGen runs at the highest rate its class offers. `.kr` lowers it.
5. **Lazy lists are patterns.** sapf's infinite streams already do what sclang's `Pattern` classes do. stac schedules them.
6. **sapf compatibility is a library.** `sapf.stac` defines sapf's words on top of stac. The core does not carry them.
7. **Little new syntax.** One keyword, `def`. Everything else uses the sapf parser as it is.

## Core kept from sapf

These are unchanged: the parser, the VM, postfix evaluation, number suffixes (`4k`, `20m`, `5/4`), strings, `'symbols`, `\a b [...]` functions, `= x` binding and destructuring, `[...]` lists, `{...}` forms with inheritance, automapping, `@` each, `/` `\` `^` reduce, scan and pairs, and the stream words (`cyc`, `picks`, `rands`, `by`, `grow`, `take`, `skip`, `hang`, `X`...).

## Values

| Type | Tier | Notes |
|-|-|-|
| Real, String, Symbol, List, Form, Function, Ref | lang | from sapf |
| `#[...]` | lang | a float array: buffer data, wave tables. Not a signal |
| `Sig` | graph | one output of a UGen; from sapf-sc |
| `Env` | lang | levels, times, curves, release and loop nodes; its own type, not a Form |
| `Def` | lang | a compiled SynthDef; callable, starts a synth |
| `Node` | lang | a synth or group on the server |
| `Bus`, `Buffer` | lang | a server resource; as a UGen input, its index |
| `Clock` | lang | tempo and beat time |
| `Namespace` | lang | a named group of words |

`Env`, `Bus`, `Buffer` and `Node` are not Forms. A Form in a UGen's argument position means keyword arguments, so no value a UGen accepts may be a Form.

Removed: the `ZList` signal. A program that reads or rearranges samples in the interpreter has no stac form. It writes the samples into a `Buffer` instead.

## UGens

### Names

```
440 sinosc                      ; SinOsc.ar(440)
[100 101] saw 800 .3 rlpf       ; RLPF.ar(Saw.ar([100, 101]), 800, 0.3)
.2 sinosc.kr 200 * 400 +        ; SinOsc.kr(0.2) * 200 + 400
[1 2 3] inf dem.dseq            ; Dseq([1, 2, 3], inf)
```

A UGen's word is its SC class name in lower case. No two of SC's 407 class names differ only in case. SC class names average 7.7 characters and 2.2 capitals; the lower-case word needs no shift key.

### The spec

`../nanosynth/spec/sclang-reference.json` is extracted from SC 3.14.1's class library:

| | Count |
|-|-|
| UGen classes | 407 |
| `ar` methods | 279 |
| `kr` methods | 244 |
| `ir` methods | 35 |
| `new` methods (demand rate, `Out`-like) | 77 |
| methods with a fixed output count | 610 |
| methods whose output count is an argument (`PlayBuf`, `In`...) | 25 |

Each method lists argument names, defaults, output count and operator index. stac generates a primitive per class from it at build time. `sc3-plugins-reference.json` adds the sc3-plugins set. nanosynth compiles from the same spec, so SCgf parity tests can cover every class.

A variable output count comes from a `numChannels` argument, which must be a constant.

### Grouping

Each SC help file names its categories, e.g. `categories:: UGens>Filters>Linear` in `HelpSource/Classes/LPF.schelp`. The build reads them from the SuperCollider source it already fetches.

**Bare words.** 165 UGens: the categories most patches use, plus `in` and `out`.

| Category | Examples |
|-|-|
| Generators>Deterministic | `sinosc`, `saw`, `pulse`, `lfsaw`, `blip`, `klank` |
| Generators>Stochastic | `whitenoise`, `pinknoise`, `dust`, `lfnoise1`, `gendy1` |
| Generators>Single-value | `dc` |
| Filters, Filters>Linear, Filters>Nonlinear, Filters>Pitch | `lpf`, `rlpf`, `bpf`, `moogff`, `lag`, `ringz`, `hilbert`, `pitchshift` |
| Delays | `delayc`, `combn`, `allpassc`, `pluck` |
| Envelopes | `envgen`, `line`, `xline`, `linen` |
| Multichannel>Panners, Multichannel>Select | `pan2`, `balance2`, `select`, `xfade2` |
| Triggers | `trig`, `latch`, `phasor`, `stepper`, `toggleff` |
| Maths | `clip`, `wrap`, `fold`, `linexp`, `inrange` |
| Conversion | `a2k`, `k2a`, `t2a` |
| Reverbs, Dynamics | `freeverb`, `gverb`, `limiter`, `compander` |

**Namespaces.** 229 UGens. A category gets a namespace when it forms its own sub-language, targets one setting, or clashes with interpreter words.

| Namespace | Category | UGens | Examples |
|-|-|-|-|
| `fft` | FFT, Convolution | 49 | `fft.fft`, `fft.ifft`, `fft.pv_magfreeze` |
| `buf` | Buffer, Buffer>Info, Delays>Buffer | 38 | `buf.playbuf`, `buf.bufrd`, `buf.localbuf` |
| `dem` | Demand | 24 | `dem.dseq`, `dem.dwhite`, `dem.demand`, `dem.duty` |
| `chaos` | Generators>Chaotic | 23 | `chaos.henonn`, `chaos.logistic` |
| `io` | InOut, less `in` and `out` | 16 | `io.localin`, `io.diskin`, `io.replaceout` |
| `info` | Info | 17 | `info.samplerate`, `info.poll`, `info.nodeid` |
| `ana` | Analysis | 16 | `ana.amplitude`, `ana.pitch`, `ana.onsets` |
| `rnd` | Random | 11 | `rnd.rand`, `rnd.exprand`, `rnd.trand` |
| `beq` | Filters>BEQSuite | 8 | `beq.blowpass`, `beq.bpeakeq` |
| `node` | Synth control | 8 | `node.freeself`, `node.detectsilence` |
| `bela` | Bela | 7 | `bela.analogin` |
| `amb` | Multichannel>Ambisonics | 5 | `amb.panb2`, `amb.decodeb2` |
| `ui` | User interaction | 4 | `ui.mousex`, `ui.keystate` |
| `gran` | Generators>Granular | 3 | `gran.grainsin`, `gran.grainfm` |

**Placed by hand: 9 classes.** `blocksize`, `nodeid` to `info`; `clearbuf`, `setbuf` to `buf`; `scopeout2` to `io`, which have no category. `randid`, `randseed` to `rnd`. `in`, `out` bare: the phrasebook's effect phrase (P12) needs `in`, and `out` is its pair.

**Hidden: 13 classes** the compiler emits and no one types: `binaryopugen`, `unaryopugen`, `muladd`, `sum3`, `sum4`, `control`, `trigcontrol`, `lagcontrol`, `audiocontrol`, `outputproxy`, `listdugen`, `infougenbase`, `bufinfougenbase`.

**The index `ug`** holds all 394 public UGens under their plain names. `ug.<TAB>` browses the set; `ug.lpf` reaches a bare UGen that a user binding hides.

`splay` and `splayaz` are in the spec but are sclang code that emits other UGens. The prelude's `splay` takes one argument and replaces the generated word; see [Prelude words](#prelude-words).

### Clashes

Against the 530 core interpreter words, one bare UGen clashes: `ramp`. The core's `ramp` clamps to 0..1; SC's `Ramp` is a linear lag. The core's `ramp` appears in neither `sapf-prelude.txt` nor `sapf-examples.txt`, so it moves to `sapf.ramp`.

`rand`, `irand` and `linrand` do not clash: their UGens live in `rnd`. `1 5 rand` draws a real when the def is built. `1 5 rnd.rand` draws a new value per synth. The pair marks the tier, as `rrand` and `Rand` do in sclang.

No namespace name matches a core word or a bare UGen.

### Arguments

Positional: SC's order, without `mul` and `add`. `*` and `+` replace those; `*+` does both.

A positional call also omits trailing **tail parameters**: arguments whose names are in this list.

`phase`, `iphase`, `doneAction`, `levelScale`, `levelBias`, `timeScale`, `trigger`, `startPos`, `reset`, `level`, `levelComp`, `center`

Only trailing ones are dropped, so argument positions never shift. Omitted arguments take SC's defaults. The rule changes 33 of 407 classes, among them the most used:

| UGen | SC arguments | Positional |
|-|-|-|
| `sinosc` | freq phase | freq |
| `lfsaw`, `lftri`, `impulse` | freq iphase | freq |
| `envgen` | envelope gate levelScale levelBias timeScale doneAction | envelope gate |
| `line`, `xline` | start end dur doneAction | start end dur |
| `pan2` | in pos level | in pos |

12 of the phrasebook's 23 phrases passed a value only to fill a tail parameter, or fell back to keywords, before this rule.

Keyword: a Form in place of the arguments names any of them, tail parameters included, and takes SC's defaults for the rest.

```
{:in sig :pos .5 :level .8} pan2
{:numChannels 2 :bufnum b :rate b buf.bufratescale.kr :loop 1} buf.playbuf
```

Lists expand to channels, as in both sapf and SC. A list of Forms expands too.

`ugen` stays for a UGen absent from the spec: `[in] 'Foo 'ar 1 ugen`.

### Rates

Rate methods per class:

| Methods | Classes |
|-|-|
| `ar` `kr` | 193 |
| `new` | 75 |
| `ar` | 74 |
| `kr` | 28 |
| `ir` | 14 |
| `ir` `kr` | 11 |
| `ar` `ir` `kr` | 10 |
| `ar` `kr` `new` | 2 |

191 classes have one rate method. 216 have a choice.

A call runs at the highest rate its class offers: `ar`, else `kr`, else `ir`, else `new`. The two possible errors differ in kind:

- A UGen at `ar` that could run at `kr` sounds the same and uses more CPU. Not measured.
- A UGen at `kr` that should run at `ar` sounds wrong. `kr` updates once per 64-frame block, 750 Hz at 48 kHz, so an oscillator above 375 Hz aliases.

In the phrasebook, `.kr` appears only on envelopes and LFOs, where SC users also write it.

### `Sig` methods

A dot after a UGen word applies a method to the `Sig` it left. A dot ends a word and dots chain; this parses today.

| Method | Effect |
|-|-|
| `.kr`, `.ir` | lowers the rate. If the node has no consumer yet, sets its rate; otherwise inserts `A2K`. sapf-sc's `toKR` does both |
| `.done` | sets the node's `doneAction` to 2, freeing the synth when the node finishes. An error on a UGen without `doneAction` |
| `.poll` | adds `Poll` on a 10 Hz `Impulse`; returns its input, as SC's `.poll` does |

```
.2 sinosc.kr
.002 .4 env.perc 1 envgen.kr.done
2 lfnoise1.kr.poll
```

`.kr` on a UGen without `kr` is an error. `.kr` on a real returns the real.

### Operators

Math on a `Sig` builds `BinaryOpUGen` or `UnaryOpUGen` and folds constants; this works today. On reals it computes in the interpreter. Where sapf's operator name differs from SC's selector, stac takes SC's: `midicps` for `nnhz`, `cpsmidi` for `hznn`. `sapf.stac` aliases the sapf names.

### Prelude words

sclang pseudo-UGens and phrasebook helpers, written in stac:

| Word | Stack | Notes |
|-|-|-|
| `+/` | `(list --> sig)` | sum; replaces `Mix`. Works today |
| `splay` | `(list --> [l r])` | `Splay` with its defaults |
| `range` | `(sig lo hi --> sig)` | maps -1..1 to lo..hi; assumes a bipolar input |
| `exprange` | `(sig lo hi --> sig)` | as `range`, exponentially |
| `inf` | `(--> real)` | the largest repeat count, for demand UGens |

### Words, not classes

`sinosc` and `fft.pv_magfreeze` are generated primitives. A word bound to a Form pushes the Form instead of calling it, so a UGen word cannot be a Form.

- Rate belongs to the result, not the class.
- `` `sinosc `` pushes the primitive as a function: ``[`sinosc `lfsaw] @ \f [440 0 f] !`` gives two channels. Works today, with sapf-sc's argument counts.
- Help and argument metadata attach to the primitive, as for every built-in word.

## Namespaces

Forms work as namespaces today:

```
{:Osc {:sin \f self [f 2 *] :k 7} :v 42} = m
m,v                             ; 42: comma looks up without applying
m,Osc,k                         ; 7: lookups chain
5 m.Osc.sin                     ; 10: dot applies a function, pushes anything else
```

A Form's `.name` passes the Form itself as an extra last argument, `self`. `1 2 m.add`, with `add` bound to `\a b [a b +]`, fails; `1 2 m,add !` works.

stac's namespaces are `Namespace` objects. `dot` applies the named word without `self`; `comma` pushes it unapplied. `Object::dot` is virtual (`include/ObjectBase.hpp:85`), so this needs no parser change. Inferred from the code; not built.

Namespaces may hold both tiers when they share a topic:

| Namespace | Interpreter words | UGens |
|-|-|-|
| `buf` | `buf.read`, `buf.load`, `buf.alloc` | `buf.playbuf`, `buf.bufrd`... |
| `node` | `node.group` | `node.freeself`, `node.detectsilence`... |
| `env` | `env.perc`, `env.adsr`, `env.asr`, `env.linen`, `env.new` | none; `envgen` is bare |
| `bus` | `bus.audio`, `bus.control` | none |
| `clock` | `clock.new` | none |

A topic completes as one list. Within these, the hint shows the tier.

Methods on values are separate from namespaces: `x.free` on a `Node` is not `node.free`, SC's `Free` UGen.

User modules use the same object. A loader that runs a file in a fresh scope and returns its bindings as a `Namespace` is not designed yet; `load` runs a file in the global workspace.

## Definitions

A `def` statement defines a SynthDef. It replaces sapf-sc's `ctl` and `>sd`.

```
def beep freq:440 amp:.1 = [freq sinosc amp *]
```

### Grammar

```
def NAME PARAM* = [ BODY ]
PARAM := name | name:LITERAL
```

- `def` is reserved. The parser handles it in `parseWord`, as it handles `=` (`src/engine/Parser.cpp:630`). `def` is unused in sapf-sc, its prelude and its examples.
- The statement ends at the body's closing `]`. A body may span lines.
- Inside a `def`, `freq:440` reads a name, then a literal after the colon. Elsewhere `:` keeps its meaning as a Form key.
- The `def` parser consumes its own `=`. List destructuring (`= [a b]`) is unaffected.
- A default is a literal number: `440`, `.1`, `-3`, `2pi`, `5/4`. A parameter without one defaults to 0. SC stores defaults as constants in the def file.
- The body compiles as a function body. Each parameter is bound to a control output and hides any word of the same name inside the body: a parameter `in` hides the UGen `in`.

### Running a `def`

1. Compiles the SynthDef from the body.
2. Sends it (`/d_recv`) if a server runs, else before the first synth.
3. Binds `NAME` to a callable `Def`. `{...} NAME` starts a synth with those control values and returns its `Node`. The `Def` type is new; not built.

Running the statement again redefines and resends the def. Synths already playing keep the old graph, as in SC. A def name that hides a UGen or core word gets a warning.

If the body leaves channels, the def adds `out` to a control `out`, default 0, as SC's `Function.play` does. If it leaves none, the body writes its own `out`. Controls take their slot order from parameter order, which settles the `scsy` sidecar slot question in TODO.md.

`` `NAME `` pushes the `Def` without calling it. `.add` resends it; `.write` writes `NAME.scsyndef` and the `scsy` sidecar.

### Control types

Parameter prefixes follow SC's `SynthDef`:

| Parameter | Control | Behaviour |
|-|-|-|
| `freq` | `Control.kr` | updates once per block |
| `t_trig` | `TrigControl` | a `set` gives a one-block pulse, then 0 |
| `i_dur` | `Control.ir` | fixed when the synth starts |
| `a_in` | `AudioControl` | audio rate |

### Examples

A sustained tone, changed while it plays:

```
def beep freq:440 amp:.1 = [freq sinosc amp *]

{:freq 660} beep = x
{:freq 880 :amp .2} x.set
x.free
```

A gated note. `release` sets `gate` to 0; `.done` frees the synth when the envelope ends:

```
def pad freq:220 amp:.2 gate:1 = [
    freq saw 2000 .3 rlpf  .01 .2 .6 1 env.adsr gate envgen.kr.done *  amp * 2 X
]

{:freq 330} pad = n
n.release
```

A percussive note that frees itself. `i_dur` is read once, at the start:

```
def ping freq:880 i_dur:.4 = [freq sinosc .002 i_dur env.perc 1 envgen.kr.done * .3 * 2 X]

{:freq 1320 :i_dur 1} ping
```

A retriggerable synth. Each `set` of `t_trig` restarts the envelope; the synth stays alive:

```
def blip freq:200 t_trig = [freq sinosc .001 .2 env.perc t_trig envgen.kr *]

{} blip = b
{:t_trig 1} b.set
{:t_trig 1 :freq 300} b.set
```

A stereo def. A list result writes consecutive channels from `out`:

```
def wide freq:110 amp:.1 = [[freq freq 1.003 *] saw amp *]
```

An effect on a private bus, placed at the tail so it runs after its source:

```
def verb bus mix:.4 = [bus 2 in mix .8 .5 freeverb]

2 bus.audio = fx
{:bus fx :addAction 'tail} verb = v
{:out fx} wide
{:mix .7} v.set
```

A def written for the SSP:

```
def hiss cutoff:1200 res:.3 = [whitenoise .2 * cutoff res rlpf]
`hiss.write                      ; hiss.scsyndef, hiss.txt
```

No def. `play` takes channels, compiles a temporary def and starts it, as `{ ... }.play` does in SC. `[...]` is a list, evaluated at once, so `play` receives channels, not a function:

```
[440 sinosc .1 *] play = y
y.free
```

### Defs built by code

The `def` statement takes a literal name and literal defaults. Code that builds defs, e.g. in a loop, uses the function `synthdef`, `(fun defaults name --> Def)`. It returns the `Def` unbound and unsent:

```
[['h2 2] ['h3 3] ['h5 5]] @ \p [
    p = [name n]
    \freq amp [freq n * sinosc amp *] {:freq 220 :amp .1} name synthdef .add
] !
```

### Today

sapf-sc has `ctl` and `>sd`, which write a def with named controls:

```
440 'freq ctl 0 sinosc .1 'amp ctl * "sine" >sd
```

It has no `synth`, `set` or `release`. `play` starts an unnamed synth; `stop` frees every synth.

## Server objects

| stac | sclang | OSC |
|-|-|-|
| `{:freq 660} sine` | `Synth(\sine, [freq: 660])` | `/s_new` |
| `{:freq 880} x.set` | `x.set(\freq, 880)` | `/n_set` |
| `x.release` | `x.release` | `/n_set gate 0` |
| `4 x.releaseIn` | `x.release(4)` | `/n_set gate -5` |
| `x.free` | `x.free` | `/n_free` |
| `stop` | `s.freeAll` | `/g_freeAll 0` |
| `node.group` | `Group()` | `/g_new` |
| `2 bus.audio` | `Bus.audio(s, 2)` | none; interpreter allocator |
| `"a.wav" buf.read` | `Buffer.read(s, "a.wav")` | `/b_allocRead` |
| `#[0 .5 1] buf.load` | `Buffer.loadCollection` | `/b_alloc`, `/b_setn` |
| `["/n_set" 1000 "freq" 440] send` | `s.sendMsg(...)` | as given |

Add actions go in the Form: `{:freq 660 :target g :addAction 'tail}`. See open question 9.

`set` on a running node restores live control, which sapf-sc lost with `zctl`, `zplug` and `set`. Buses and buffers reuse sapf-sc's allocators: private buses 512..959, sample buffers 0..63, wave tables from 512.

## Patterns

An event is a Form. An event stream is a lazy list of Forms. `pbind` builds one from a Form of fields:

```
{:instrument 'ping
 :midinote [60 64 67 72] cyc
 :dur [.25 .25 .5] cyc
 :amp .1} pbind = p

p 4 take                        ; the first 4 events
p play = player                 ; schedules synths on the default clock
player.stop
```

- A list field advances once per event. A real field repeats. The stream ends when its shortest finite field ends, the automapping rule sapf already uses.
- A `Sig` in a field is an error. Events carry values, not graphs.
- `play` on an event stream starts one synth per event, `dur` beats apart. `legato` sets when it releases. Bundles go out .2 s ahead, as the `ola` scheduler does now.
- `'rest` in any field makes a rest.
- `:instrument` names a def. It may also be a function; stac compiles it per event and reuses defs that compile alike, as `ola` voices do now.

### Chords

A field value that is itself a list plays one synth per element, as in SC:

```
{:instrument 'pad :midinote [[60 64 67] [62 65 69]] cyc :dur 1 :legato .9} pbind play
```

`[[60 64 67] [62 65 69]] cyc` already yields one chord per step; works today.

### Pitch

`freq`, else `midinote midicps`, else SC's chain: `degree` indexes `scale`, offset by `octave` and `root`. Scales are symbols naming SC's tables: `'major`, `'minor`, `'dorian`...

```
{:instrument 'ping :degree [0 2 4 5 7] picks :scale 'minor :dur .125} pbind play
```

### Replacement

`pdef` and `ndef` name a player. Running the line again replaces it: a pattern at the next beat, a sound with a crossfade. They follow SC's `Pdef` and `Ndef`.

```
{:instrument 'ping :midinote [60 62 64] cyc :dur .25} pbind 'mel pdef
{:instrument 'ping :midinote [67 65] cyc :dur .125} pbind 'mel pdef

[[55 55.2] saw .1 *] 'drone ndef
[[55 55.2] saw 800 .3 rlpf .1 *] 'drone ndef
```

### Stream words

sclang's pattern classes map to existing sapf stream words:

| sclang | stac |
|-|-|
| `Pseq(list, inf)` | `list cyc` |
| `Pseq(list, n)` | `n list ncyc` |
| `Prand(list, inf)` | `list picks` |
| `Pwrand(list, w, inf)` | `list w wpicks` |
| `Pwhite(lo, hi)` | `lo hi rands` |
| `Pexprand(lo, hi)` | `lo hi xrands` |
| `Pseries(a, s)` | `a s by` |
| `Pgeom(a, g)` | `a g grow` |
| `Pn(x, n)` | `x n X` |
| `Pbind(...)` | `{...} pbind` |

`ola` becomes prelude code over `pbind` and leaves the core.

### Clocks

`2 clock.new = clk` makes a clock at 2 beats per second. `p clk.play` plays on it; a method keeps `play`'s arity fixed. `{:tempo 3} clk.set` changes tempo. The default clock runs at 1 beat per second.

### Rendering

- `item seconds "name" render` writes `name.wav` through the NRT renderer. `item` is channels or an event stream.
- `item seconds "name" score` writes `name.osc` for `scsy`, replacing `>nrt`.
- Both take an explicit duration. sapf-sc's `>sf` infers it from where channels end, and refuses many graphs. SC's `Score.render` takes a duration.

## Demand rate

The demand UGens live in `dem`. They take a finite list:

```
[8 impulse.kr 0 [60 63 67 70] inf dem.dseq dem.demand.kr midicps saw .1 *] play
```

An infinite list is an error; `1024 take` makes it finite. sapf-sc's silent 1024-value cap is removed.

`seq`, `iseq`, `steps`, `imps`, `lines`, `xlines` and `curves` move to `sapf.stac`.

## Envelopes

| stac | sclang |
|-|-|
| `levels times curves env.new` | `Env(levels, times, curves)` |
| `atk rel env.perc` | `Env.perc(atk, rel)` |
| `a d s r env.adsr` | `Env.adsr(a, d, s, r)` |
| `atk sus rel env.linen` | `Env.linen(atk, sus, rel)` |
| `atk rel env.asr` | `Env.asr(atk, 1, rel)` |

Each returns an `Env`; `envgen` flattens it into its inputs. Curves are reals or the symbols `'lin 'exp 'sin 'wel 'sqr 'cub 'step 'hold`.

`N`, `T` and `NZ` on a `Sig` no longer end a synth. `.done` on an envelope or `line` does.

## Completion

The REPL reads lines with replxx (`src/engine/VM.cpp:255`) and sets no completion callback today. replxx provides `replxx_set_completion_callback` (`thirdparty/replxx/include/replxx.h:309`) and `replxx_set_hint_callback` (`:355`).

### Rules

- A prefix with no dot completes against core words, bare UGens, namespace names and user bindings.
- A prefix with no dot also offers namespaced matches: `playb<TAB>` gives `buf.playbuf`. A player need not remember which namespace SC's categories chose.
- A prefix with a dot splits at the last dot. The completer looks up the namespace at runtime and offers its keys: `fft.pv_mag<TAB>`, `env.<TAB>`.
- After a value, as in `sinosc.k<TAB>`, the completer cannot know the stack's type without evaluating the line. It offers the `Sig` methods.
- An undefined bare word that names a namespaced UGen gets a suggestion: `"playbuf" is undefined; buf.playbuf?`.

### Fan-out

| List | Size | After 2 letters |
|-|-|-|
| bare UGens | 165 | median 1, largest `lf` 14 |
| `fft` | 49 | `pv` 34 |
| `buf` | 38 | `bu` 17 |
| `dem` | 24 | median 1 |
| other namespaces | 3 to 23 | short |

`ug` needs no sub-namespaces. The two large lists share an SC prefix inside the namespace (`pv_`, `buf`); see open question 5.

### Hints

The hint callback shows a word's stack effect after it is typed, from the help string every primitive carries:

```
440 sinosc            (freq --> sig)
```

### Editors

Live coding usually sends text from an editor, which gets no REPL completion. `stac --words` prints every binding with its namespace and stack effect, one per line, for editor plugins.

## sapf.stac

`sapf.stac` defines sapf's UGen words with sapf's argument order and units, in the namespace `sapf`:

```
\freq phase [{:freq freq :phase phase 2pi *} sinosc] = sinosc
\in freq [in freq lpf] = lpf
```

- [../ugens.md](../ugens.md) moves with it; its fidelity figures describe this library.
- The conformance tests against sapf renders test this library, not the core.
- `sapf-examples.txt` runs with `-p sapf.stac`, which binds the `sapf` words globally.
- The policy in [upstream-sapf-bugs.md](upstream-sapf-bugs.md) applies to this library only. The core follows SC.

## Naming

| Now | stac |
|-|-|
| repository `sapf-sc` | `stac` |
| binary `sapf` | `stac` |
| `SAPF_SCSYNTH`, `SAPF_SYNTHDEFS`, `SAPF_RECORDINGS`, `SAPF_DEVICE`, `SAPF_OUTPUTS`, `SAPF_INPUTS`, `SAPF_SR`, `SAPF_PLUGINS`, `SAPF_PRELUDE` | `STAC_*` |
| `sapf-prelude.txt` | `prelude.stac` |
| `sapf-examples.txt` | `examples/sapf.stac` |
| SSP plugin `sapf` (planned) | `stac` |

The licence stays GPL-3.0, with sapf's copyright notices and a statement of derivation.

## Phases

Each phase leaves `make test` passing. Until phase 5, new UGen words appear only in namespaces and `ug`, so they cannot collide with sapf-sc's words of the same name and different units, such as `sinosc`'s phase.

1. **Rename.** Binary, environment variables, files, docs. No change in behaviour.
2. **Spec UGens.** Generate a primitive per class into its namespace and `ug`. Tail parameters, keyword Forms, `Sig` methods, `Namespace`. Test SCgf parity with nanosynth for every class.
3. **Defs and nodes.** The `def` statement, callable `Def`, `synthdef`, `set`, `release`, `releaseIn`, `free`. The `env`, `bus`, `buf`, `node` namespaces and the `Env` type.
4. **Patterns.** `pbind`, chords, pitch keys, `clock`, `pdef`, `ndef`, `render`, `score`. Rewrite `ola` on them. Completion and hints.
5. **Split.** Move sapf's words to `sapf`. Bind the 165 bare UGens. Move the core's `ramp` to `sapf.ramp`. Remove the `ZList` signal path, `kr`, `ctl`, `>sd`, `>sf`, `>nrt`, and `N`/`T` on `Sig`.

Phases 1-4 break no program. Phase 5 breaks every program that does not load `sapf.stac`.

## Open questions

1. **Rate inference.** sapf-sc lowers a node whose consumers are all `kr`. Remove it, or keep it opt-in. It does not lower the common case, an LFO driving an `ar` filter's frequency.
2. **`.ar` on a `kr` UGen.** Raise it with `K2A`, or reject it.
3. **Render duration.** Explicit only, or inferred when every channel has a `doneAction`.
4. **`!`.** sapf's `!` applies a function; SC's duplicates. stac keeps sapf's and uses `X` to duplicate.
5. **Prefix stripping inside a namespace.** `buf.rd` for `buf.bufrd`, `fft.magfreeze` for `fft.pv_magfreeze`, `dem.seq` for `dem.dseq`. Shorter, but SC's help is then searched under a different name.
6. **Namespace names.** `dem`, `ana`, `amb`, `rnd` are short and opaque; `demand`, `analysis`, `ambi`, `random` read better and complete after 2 letters anyway.
7. **Control lag.** SC's `\freq.kr(440, 0.1)`. A `def` parameter could take it as `freq:440~.1`. `~` is a name character today, so this needs `def`-only lexing, as `:` does.
8. **Array controls.** Whether `freqs:[100 200]` makes a multichannel control, as SC's `freqs=#[100, 200]` does.
9. **Synth options and control values share one Form.** `target` and `addAction` then cannot name parameters. Only one phrasebook phrase needs an option; a second Form costs a word per call.
10. **Calling a def with all defaults** is `{} blip`. An optional Form reads better but makes the word's arity depend on the stack.
11. **Routines.** sclang has `fork` and `wait`. stac's VM has no coroutines. Event streams cover sequencing; a function scheduled on a clock (`clk 4 fun sched`) covers the rest.
12. **Spec location.** Vendor `sclang-reference.json` into this repo, or read it from nanosynth at build time.
13. **`--words` format.** Plain lines, or JSON for editor plugins.
