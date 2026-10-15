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
[1 2 3] inf demand.dseq            ; Dseq([1, 2, 3], inf)
```

A UGen's word is its SC class name in lower case. No two of SC's 407 class names differ only in case. SC class names average 7.7 characters and 2.2 capitals; the lower-case word needs no shift key.

### The spec

`spec/sclang-reference.json` is extracted from SC 3.14.1's class library:

| | Count |
|-|-|
| UGen classes | 407 |
| `ar` methods | 279 |
| `kr` methods | 244 |
| `ir` methods | 35 |
| `new` methods (demand rate, `Out`-like) | 77 |
| methods with a fixed output count | 610 |
| methods whose output count is an argument (`PlayBuf`, `In`...) | 25 |

Each method lists argument names, defaults, output count and operator index. `scripts/gen_ugens.py` generates `src/engine/SpecUGens.inc` from it; a test fails when the table is stale.

The spec has no wire order. `Duty`'s arguments are `dur reset level doneAction`; its inputs are `dur reset doneAction level`. `scripts/sclang_wire.py` reads each rate method's `multiNew` or `multiNewList` call from the SC 3.14.1 class library in `build/_deps` into `spec/sclang-wire.json`. It then fills each map with the probe's argument values and checks that it reproduces the probe's recorded inputs.

| Status | Methods |
|-|-|
| verified: reproduces the inputs, all arguments distinct | 365 |
| consistent: reproduces them, but two arguments shared a value | 214 |
| unverified: an expression the extractor does not read | 19 |
| mismatch: envelopes, `Klank` specs, `Poll` labels, `Control`s, operators | 18 |
| unparsed: sclang composites and calls through other methods | 19 |

579 of 635 methods (91%) have a trusted order. The rest need entries in `spec/stac-ugens.json`, or stay hidden. The extractor reproduced all 17 demand wire orders first written by hand. sclang 3.13 cannot compile the 3.14 class library, which uses keyword arguments, so a runtime probe would need sclang 3.14.1.

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

**Namespaces.** 228 UGens. A category gets a namespace when it forms its own sub-language, targets one setting, or clashes with interpreter words.

| Namespace | Category | UGens | Examples |
|-|-|-|-|
| `fft` | FFT, Convolution | 49 | `fft.fft`, `fft.ifft`, `fft.pv_magfreeze` |
| `buffer` | Buffer, Buffer>Info, Delays>Buffer | 38 | `buffer.playbuf`, `buffer.bufrd`, `buffer.localbuf` |
| `demand` | Demand | 24 | `demand.dseq`, `demand.dwhite`, `demand.demand`, `demand.duty` |
| `chaos` | Generators>Chaotic | 23 | `chaos.henonn`, `chaos.logistic` |
| `io` | InOut, less `in`, `out` and `maxlocalbufs` | 15 | `io.localin`, `io.diskin`, `io.replaceout` |
| `info` | Info | 17 | `info.samplerate`, `info.nodeid`; `Poll` is the `.poll` method |
| `analysis` | Analysis | 16 | `analysis.amplitude`, `analysis.pitch`, `analysis.onsets` |
| `random` | Random | 11 | `random.rand`, `random.exprand`, `random.trand` |
| `beqsuite` | Filters>BEQSuite | 8 | `beqsuite.blowpass`, `beqsuite.bpeakeq` |
| `node` | Synth control | 8 | `node.freeself`, `node.detectsilence` |
| `bela` | Bela | 7 | `bela.analogin` |
| `ambisonics` | Multichannel>Ambisonics | 5 | `ambisonics.panb2`, `ambisonics.decodeb2` |
| `ui` | User interaction | 4 | `ui.mousex`, `ui.keystate` |
| `granular` | Generators>Granular | 3 | `granular.grainsin`, `granular.grainfm` |

**Placed by hand: 9 classes.** `blocksize`, `nodeid` to `info`; `clearbuf`, `setbuf` to `buffer`; `scopeout2` to `io`, which have no category. `randid`, `randseed` to `random`. `in`, `out` bare: the phrasebook's effect phrase (P12) needs `in`, and `out` is its pair.

**Hidden: 14 classes** the compiler emits and no one types: `binaryopugen`, `unaryopugen`, `muladd`, `sum3`, `sum4`, `control`, `trigcontrol`, `lagcontrol`, `audiocontrol`, `outputproxy`, `listdugen`, `infougenbase`, `bufinfougenbase`, and `maxlocalbufs`, which `LocalBuf` emits.

**The index `ug`** holds all 393 public UGens under their plain names. `ug.<TAB>` browses the set; `ug.lpf` reaches a bare UGen that a user binding hides.

`splay` and `splayaz` are in the spec but are sclang code that emits other UGens. The prelude's `splay` takes one argument and replaces the generated word; see [Prelude words](#prelude-words).

### Clashes

Against the 530 core interpreter words, one bare UGen clashes: `ramp`. The core's `ramp` clamps to 0..1; SC's `Ramp` is a linear lag. The core's `ramp` appears in neither `prelude.stac` nor `examples/sapf.stac`, so it moves to `sapf.ramp`.

`rand`, `irand` and `linrand` do not clash: their UGens live in `random`. `1 5 rand` draws a real when the def is built. `1 5 random.rand` draws a new value per synth. The pair marks the tier, as `rrand` and `Rand` do in sclang.

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

The rule misfires where a stoplist name is a class's main input: `level` in `Duty` and `TDuty`, `reset` in `Dreset`. `spec/stac-ugens.json` overrides those 3 of 21 demand classes.

Keyword: a Form in place of the arguments names any of them, tail parameters included, and takes SC's defaults for the rest.

```
{:in sig :pos .5 :level .8} pan2
{:numChannels 2 :bufnum b :rate b buffer.bufratescale.kr :loop 1} buffer.playbuf
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

Built: all three. `.ir` on a node also used at another rate makes a second node, since no UGen converts to scalar rate. `.poll` labels the output `UGen(Class)` as sclang does; nanosynth's default label is the bare class name. The `Poll` is attached to the polled node and compiles wherever that node does, since `Poll` writes no output for a channel to read.

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
| `buffer` | `buffer.read`, `buffer.load`, `buffer.alloc` | `buffer.playbuf`, `buffer.bufrd`... |
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

2. Binds `NAME` to a callable `Def`, after the body compiles, so the body cannot call its own name. `{...} NAME` starts a synth with those control values and pushes its `Node`.

3. Sends the def with its first synth: one `/d_recv` whose completion message is the `/s_new`. Sending at the statement would wait on every def for a server that may not run. `.add` sends it at once.

Running the statement again redefines and resends the def. Synths already playing keep the old graph, as in SC. A def name that hides a UGen or core word gets a warning.

If the body leaves channels, the def writes them with `Out` to a control `out`, default 0, appended after the parameters, unless the def declares `out` itself. If it leaves none, the body writes its own, as `out ug.out`. Controls take their slot order from parameter order within each kind, ir, tr, ar, kr as in SC, which settles the `scsy` sidecar slot question in TODO.md. Every declared parameter is a control, used or not.

`` `NAME `` pushes the `Def` without calling it. `.add` resends it; `.write` writes `NAME.scsyndef`. The `scsy` sidecar is not written yet; see phase 3 below.

Dot methods that act, `.set`, `.release`, `.releaseIn`, `.free`, `.add` and `.write`, leave nothing on the stack. The REPL prints the stack after each line, so a returned value would print after every `x.free`.

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
| `"a.wav" buffer.read` | `Buffer.read(s, "a.wav")` | `/b_allocRead` |
| `#[0 .5 1] buffer.load` | `Buffer.loadCollection` | `/b_alloc`, `/b_setn` |
| `["/n_set" 1000 "freq" 440] send` | `s.sendMsg(...)` | as given |

Add actions go in the Form: `{:freq 660 :target g :addAction 'tail}`. The default is the head of the root node 0. See open question 9.

Built: `/s_new` from a `Def`, `set`, `release`, `releaseIn`, `free`, `bus.audio`, `bus.control` with `v b.set`, `buffer.read`, `buffer.load`, `buffer.alloc`, `{} node.group`, `send`. Not built: freeing a bus or buffer, and `play` pushing a `Node`.

- `node.group` takes the placement Form a def call takes, `{:target g :addAction 'tail}`; `{}` places it at the head of the root. A group is a `Node`, so `set`, `release` and `free` act on all its synths.
- `bus.control` takes control buses from 16383 down, 4096 in all, clear of the low buses sclang allocates from 0.
- `send` sends a whole real as an int and any other as a float, as sclang sends an `Integer` and a `Float`.

- `bus.audio` takes buses from 959 down; `ola`, which rotates through 512 and up, wraps below the lowest taken. Buses are not freed; 384 channels remain for `bus.audio` before it reports none left.
- `buffer.read` puts a file in the sound-file buffer `sf>` would use for it, 0..63, so a file has one buffer. `buffer.load` and `buffer.alloc` take buffers 64..511, below the wave tables. Each sapf process takes its own block of 100000 node IDs from 3000000, chosen from its process ID, so a second or restarted sapf does not reuse the IDs of synths still running. `play`'s fixed IDs from 2000000 still can.

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

- `:scale` holds its list; it does not advance, as other list fields do.

Built: `pbind`, `play` on an event stream on the default clock, `player.stop`, the pitch chain below, chords, `'rest`, and `score` for event streams. Each event follows SC's default event: `amp` .1, `dur` 1, `legato` .8; only the def's controls are sent; `sustain` or `dur * legato` sets when a def with `gate` is released. A function `:instrument` takes each argument from the event's field of that name; `freq`, `amp`, `dur`, `legato` and `sustain` get the derived values; `:out` sets the bus it writes to. `dur` and `sustain` are in beats, so `sustain T` lasts `sustain` seconds only at 1 beat per second. Nothing checks that its channels end; one that does not end plays until `stop`.

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

Built: `pdef`, on the default clock. It pushes the name's player, the same object each run, so `.stop` ends whichever stream is current. Not built: a quant other than 1; `pdef` on another clock.

Built: `ndef`. Each run compiles the channels times SC's `EnvGate` and starts the synth in the bundle that releases the previous one. `nd.stop` fades out; `{:fadeTime t} nd.set` sets the fade. Not built: SC's private bus and monitor, so one ndef cannot yet read another.

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

`ola` stays in `sapf`, in C++. The core sends a pattern to an effect with `:out` and `bus.audio`, as SC's `Pbind(\out, bus)` does. Reasons: `stac-phase5.md`.

### Clocks

`2 clock.new = clk` makes a clock at 2 beats per second. `p clk.play` plays on it; a method keeps `play`'s arity fixed. `{:tempo 3} clk.set` changes tempo. The default clock runs at 1 beat per second.

Built as described. Event times and releases are in beats, converted to server time when sent, .2 s ahead. A tempo change therefore misses bundles already sent. `score` times events at 1 beat per second.

### Rendering

- `item seconds "name" render` writes `name.wav` through the NRT renderer. `item` is channels or an event stream.

- `item seconds "name" score` writes `name.osc` for `scsy`, replacing `>nrt`.

- Both take an explicit duration. sapf-sc's `>sf` infers it from where channels end, and refuses many graphs. SC's `Score.render` takes a duration.

Built: both. `render` writes to `$STAC_RECORDINGS`, as `>sf` does. Events render to 2 channels, SC's default `numOutputBusChannels`; a mono def leaves the second silent.

## Demand rate

The demand UGens live in `demand`. They take a finite list:

```
[8 impulse.kr 0 [60 63 67 70] inf demand.dseq demand.demand.kr midicps saw .1 *] play
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

Built: `env.perc`, `env.adsr`, `env.asr`, `env.linen` and `env.new`, with SC's defaults for the arguments they do not take: level 1, curve -4 (`linen`: `'lin`). `ug.envgen` and `ug.ienvgen` take an `Env`, or a list already in SC's envelope format. `.done` and `.kr` apply in either order.

## Completion

The REPL reads lines with replxx (`src/engine/VM.cpp:255`) and sets no completion callback today. replxx provides `replxx_set_completion_callback` (`thirdparty/replxx/include/replxx.h:309`) and `replxx_set_hint_callback` (`:355`).

### Rules

- A prefix with no dot completes against core words, bare UGens, namespace names and user bindings.

- A prefix with no dot also offers namespaced matches: `playb<TAB>` gives `buffer.playbuf`. A player need not remember which namespace SC's categories chose.

- A prefix with a dot splits at the last dot. The completer looks up the namespace at runtime and offers its keys: `fft.pv_mag<TAB>`, `env.<TAB>`.

- After a value, as in `sinosc.k<TAB>`, the completer cannot know the stack's type without evaluating the line. It offers the `Sig` methods.

- An undefined bare word that names a namespaced UGen gets a suggestion: `"playbuf" is undefined; buffer.playbuf?`.

### Fan-out

| List | Size | After 2 letters |
|-|-|-|
| bare UGens | 165 | median 1, largest `lf` 14 |
| `fft` | 49 | `pv` 34 |
| `buffer` | 38 | `bu` 17 |
| `demand` | 24 | median 1 |
| other namespaces | 3 to 23 | short |

`ug` needs no sub-namespaces. The two large lists share an SC prefix inside the namespace (`pv_`, `buf`); see open question 5.

### Hints

The hint callback shows a word's stack effect after it is typed, from the help string every primitive carries:

```
440 sinosc            (freq --> sig)
```

Built: the rules and hints above, in `Completion.cpp`, unit-tested in `tests/unit/test_completion.cpp`. Two refinements:

- After a value, the completer offers the methods of every stac object, not only `Sig`'s: `n.rel<TAB>` gives `release` and `releaseIn`.
- `ug` indexes every UGen, so a namespaced match names its category namespace, and `ug` only for a bare UGen: `pv_magf<TAB>` gives `fft.pv_magfreeze` alone. The undefined-word suggestion follows the same rule, in sapf and in the linter's W1.

### Editors

Live coding usually sends text from an editor, which gets no REPL completion. `stac --words` prints every binding with its namespace and stack effect, one per line, for editor plugins. Not built.

## sapf.stac

sapf's 122 DSP words keep their C++ and live in the namespace `sapf`, with sapf's argument order and units. `sapf.stac` binds them unqualified and holds the prelude definitions built on them:

```
sapf,sinosc = sinosc  sapf,lpf = lpf
```

Rewriting them in stac over the bare UGens (`\freq phase [{:freq freq :phase phase 2pi *} ug.sinosc] = sinosc`) was the other reading; `stac-phase5.md` gives the reasons for keeping the C++.

- [../ugens.md](../ugens.md) moves with it; its fidelity figures describe this library.

- The conformance tests against sapf renders test this library, not the core.

- `examples/sapf.stac` runs with `-p prelude.stac -p sapf.stac`. `sapf.stac` uses the prelude's words, so it loads second.

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
   Done: the binary `stac`, its prompt `stac>`, the `STAC_*` variables (also `STAC_LOG`, `STAC_HISTORY`, `STAC_EXAMPLES`, `STAC_README`, `STAC_BIN`), `prelude.stac`, `examples/sapf.stac`. Phase 5 stopped reading the `SAPF_` names. Not renamed: the repository, the C++ identifiers and CMake options (`SapfEngine`, `SAPF_EMBED_SCSYNTH`), and the default `~/sapf-log.txt` and `~/sapf-history.txt`, which would lose a user's history. `SAPF_ORIG` names original sapf and stays.

2. **Spec UGens.** Generate a primitive per class into its namespace and `ug`. Tail parameters, keyword Forms, `Sig` methods, `Namespace`. Test SCgf parity with nanosynth for every class.
   Done: 374 words, every namespace in the grouping above, and 155 of the 165 bare UGens, which live in `ug` only until phase 5. `.kr`, `.ir`, `.done`, `.poll`. Of the 393 public classes, 19 are not generated.
   - `spec/stac-ugens.json` encodes the grouping above: a class goes by its first help category, from `HelpSource/Classes/*.schelp`, unless placed by hand. The generated counts match the tables.
   - Not generated, with the reason in `spec/stac-ugens.json`:
     - sclang code that emits other UGens: `Changed`, `CompanderD`, `HilbertFIR`, `VarLag`, `Splay`, `SplayAz`, `DynKlank`, `DynKlang`, `Dunique`, `LocalBuf`, `Tap`, `UnpackFFT`, `InBus`, and `Dstutter`, a deprecated alias of `Ddup`.
     - strings and other objects as arguments: `Poll`, `Dpoll`, `SendReply`, `SendPeakRMS`, `InRect`.
   - A `numChannels` argument sets the output count and is not sent, as in `PlayBuf` and `BufRd`. It must be a whole number. `MFCC`'s `numcoeff` sets the count and is sent.
   - `Klank` and `Klang` take `[freqs amps times]` and send it as sclang's `flop.flat`. An omitted row takes sclang's fill.
   - An `ar` method that calls `asAudioRateInput` raises a constant input to `DC.ar` and a control-rate one to `K2A.ar`, as sclang does. `.kr` compiles from the inputs as given.
   - A call takes the defaults of the rate it builds at. `.kr` keeps them: `ug.leakdc.kr`'s `coef` is `ar`'s .995, not `kr`'s .9. This changes a value in 5 classes: `LeakDC`, `Gendy1`, `Gendy2` (`minfreq`, `maxfreq`), `Delay1`, `Delay2` (`x1`, `x2`).
   - `x ? y` defaults, as in `Stepper`'s `resetval ? min`, apply when a keyword call omits the argument.
   - Deviations from sclang: `DecodeB2` sends its `orientation`; sclang's source assigns 0.5 inside its call. `Onsets` takes `odftype` as a number, default 3; sclang also takes a symbol.
   - `tests/py/test_ugen_sweep.py` compiles 262 classes with a distinct value per argument and compares the bytes with nanosynth's. 110 are not swept: missing from nanosynth, demand rate, or lists, output counts, raised inputs or no outputs. `test_spec_ugens.py` covers some of those by hand.

3. **Defs and nodes.** The `def` statement, callable `Def`, `synthdef`, `set`, `release`, `releaseIn`, `free`. The `env`, `bus`, `buffer`, `node` namespaces and the `Env` type.
   Done so far: the `Env` type and the `env` namespace, which unblock `envgen` and `ienvgen`; the `def` statement, `synthdef`, `Def` and `Node` with `set`, `release`, `releaseIn` and `free`. `bus.audio`, `bus.control`, `buffer.read`, `buffer.load`, `buffer.alloc`, `node.group`, `send`. Not yet: `play` pushing a `Node`, moved to phase 5 because it changes what `play` leaves. The `scsy` sidecar from `.write` waits for a source of ranges: `scsy` needs a sidecar only for ranges, units, curves, CV or a slot order other than the def's (`TODO.md`), and `def` declares none of them.

4. **Patterns.** `pbind`, chords, pitch keys, `clock`, `pdef`, `ndef`, `render`, `score`. Rewrite `ola` on them, deferred to phase 5 and then dropped (`stac-phase5.md`). Completion and hints.
   Done: `pbind`, chords, pitch keys, a function as `:instrument`, `play` and `.stop`, `score`, `render`, `clock.new`, `clk.play`, `clk.set`, `pdef`, `ndef`, completion and hints. Not built: `stac --words` for editors.

5. **Split.** Move sapf's words to `sapf`. Bind the 165 bare UGens. Move the core's `ramp` to `sapf.ramp`. Remove the `ZList` signal path, `kr`, `ctl`, `>sd`, `>sf`, `>nrt`, and `N`/`T` on `Sig`.
   Done. Details and counts: `stac-phase5.md`.
   - `sapf` holds 123 words: sapf's DSP words, among them `kr`, `ctl`, `>sd`, `>sf`, `>sfo` and `>nrt`; `ola`; `ramp`; and versions of `N`, `keep`, `NZ`, `T` and `play` that keep sapf's behaviour. `sapf.stac` binds them and holds the prelude's 48 definitions that use them. `-p` repeats.
   - 153 of the 156 bare UGens are global words. `clip`, `wrap` and `fold` stay the core's: they compute on reals and build `Clip`, `Wrap` and `Fold` on a `Sig`, as sclang's methods do. `iwrap`, `ifold` and `*+` stay too.
   - The core's `N`, `keep`, `NZ` and `T` reject a `Sig`. `play` pushes the synth's `Node`.
   - The `SAPF_` names are not read.
   - The tests load `prelude.stac` and `sapf.stac` unless they test the core. `compile_sapf` writes defs with `synthdef` and `.write`, so they have an `out` control.
   - `ola` moved to `sapf` unchanged; it is not rewritten on `pbind`.

Phases 1-4 break no program. Phase 5 breaks every program that does not load `sapf.stac`. What it breaks, counted: `stac-phase5.md`.

## Open questions

1. **Rate inference.** sapf-sc lowers a node whose consumers are all `kr`. Decided for generated words: they take no part, neither lowered nor lowering their inputs, because a `kr` UGen such as `FFT` or `Amplitude` reads its whole audio input. The inference stays for sapf's words until phase 5 removes them.

2. **`.ar` on a `kr` UGen.** Raise it with `K2A`, or reject it.

3. **Render duration.** Explicit only, or inferred when every channel has a `doneAction`. `render` is built with explicit only; inference can be added without breaking it.

4. **`!`.** sapf's `!` applies a function; SC's duplicates. stac keeps sapf's and uses `X` to duplicate.

5. **Prefix stripping inside a namespace.** Deferred. Phase 2 generates SC's names unstripped: `buffer.bufrd`, `fft.pv_magfreeze`, `demand.dseq`. Stripping (`buffer.rd`, `demand.seq`) is planned; decided 2026-10-15: do it with question 6 as the last stage. Options and counts: `stac-names.md`.

6. **Namespace names.** Deferred. Phase 2 uses full names: `buffer`, `demand`, `analysis`, `random`, `ambisonics`, `granular`, `beqsuite`. Acronyms stay: `fft`, `io`, `ui`. Short names are planned. Stripping and short names trade against each other (`dem.dseq` and `demand.seq` have the same length), so pick them together, as the last stage.

7. **Control lag.** SC's `\freq.kr(440, 0.1)`. A `def` parameter could take it as `freq:440~.1`. `~` is a name character today, so this needs `def`-only lexing, as `:` does.

8. **Array controls.** Whether `freqs:[100 200]` makes a multichannel control, as SC's `freqs=#[100, 200]` does.

9. **Synth options and control values share one Form.** `target` and `addAction` then cannot name parameters. Only one phrasebook phrase needs an option; a second Form costs a word per call.

10. **Calling a def with all defaults** is `{} blip`. An optional Form reads better but makes the word's arity depend on the stack.

11. **Routines.** sclang has `fork` and `wait`. stac's VM has no coroutines. Event streams cover sequencing; a function scheduled on a clock (`clk 4 fun sched`) covers the rest.

12. **Spec location.** Decided: vendored as `spec/sclang-reference.json`, from nanosynth `7a212da`. Regenerate it in nanosynth with `make sclang-reference`, then copy it.

13. **`--words` format.** Plain lines, or JSON for editor plugins.
