# stac syntax proposal 1: UGen names, namespaces and rates

Status: proposal, 2026-10-15. Revises the "UGens" section of [stac.md](stac.md). Nothing here is implemented unless marked "works today".

stac is for live coding. Everyday UGens should be short bare words with no shift key. Specialised groups should sit in namespaces, so each group completes as one short list.

## Proposal

```
440 0 sinosc                    ; SinOsc.ar(440, 0)
.2 0 sinosc.kr 200 * 400 +      ; SinOsc.kr(0.2, 0) * 200 + 400
1 5 rand                        ; a real, drawn when the def is built
1 5 rnd.rand                    ; Rand(1, 5): a UGen, one value per synth
[1 2 3] 4 dem.dseq              ; Dseq([1, 2, 3], 4)
```

1. A UGen's name is its SC class name in lower case.
2. Everyday UGens are bare words: 163 of them.
3. Specialised UGens live in 14 namespaces: 231 of them.
4. SC's help categories decide which is which.
5. A call runs at the highest rate the class offers. `.kr` and `.ir` on the result lower it.

## Spellings

| Spelling | Chars | Shift presses | Used for |
|-|-|-|-|
| `SinOsc.ar` | 9 | 2 | dropped |
| `sinosc` | 6 | 0 | everyday UGens |
| `fft.pv_magfreeze` | 16 | 0 | specialised UGens |
| `ug.sinosc` | 9 | 0 | any UGen, through the index namespace `ug` |

SC class names average 7.7 characters and 2.2 capitals. Lower case loses nothing: no two of the 407 class names differ only in case.

## Grouping

Each SC help file names its categories, e.g. `categories:: UGens>Filters>Linear` in `HelpSource/Classes/LPF.schelp`. The build reads them from the SuperCollider source it already fetches, so no one sorts 407 classes by hand. Figures below come from SC 3.14.1's `HelpSource` and `../nanosynth/spec/sclang-reference.json`.

### Bare

These categories hold the UGens most patches use:

| Category | Examples |
|-|-|
| Generators>Deterministic | `sinosc`, `saw`, `pulse`, `lfsaw`, `blip`, `klank` |
| Generators>Stochastic | `whitenoise`, `pinknoise`, `dust`, `lfnoise1`, `gendy1` |
| Generators>Single-value | `dc` |
| Filters, Filters>Linear, Filters>Nonlinear, Filters>Pitch | `lpf`, `rlpf`, `bpf`, `moogff`, `lag`, `ringz`, `hilbert`, `pitchshift` |
| Delays | `delayc`, `combn`, `allpassc`, `pluck` |
| Envelopes | `envgen`, `line`, `xline`, `linen` |
| Multichannel>Panners, Multichannel>Select | `pan2`, `balance2`, `splay`, `select`, `xfade2` |
| Triggers | `trig`, `latch`, `phasor`, `stepper`, `toggleff` |
| Maths | `clip`, `wrap`, `fold`, `linexp`, `inrange` |
| Conversion | `a2k`, `k2a`, `t2a` |
| Reverbs, Dynamics | `freeverb`, `gverb`, `limiter`, `compander` |

163 UGens. Among them, after 2 typed letters the median prefix matches 1 name; the largest group is `lf` with 14.

### Namespaced

| Namespace | Category | UGens | Examples |
|-|-|-|-|
| `fft` | FFT, Convolution | 49 | `fft.fft`, `fft.ifft`, `fft.pv_magfreeze`, `fft.convolution2` |
| `buf` | Buffer, Buffer>Info, Delays>Buffer | 38 | `buf.playbuf`, `buf.bufrd`, `buf.localbuf`, `buf.bufcombn` |
| `dem` | Demand | 24 | `dem.dseq`, `dem.dwhite`, `dem.duty`, `dem.demandenvgen` |
| `chaos` | Generators>Chaotic | 23 | `chaos.henonn`, `chaos.lorenzl`, `chaos.logistic` |
| `io` | InOut | 18 | `io.in`, `io.out`, `io.localin`, `io.diskin` |
| `info` | Info | 17 | `info.samplerate`, `info.poll`, `info.nodeid` |
| `ana` | Analysis, Analysis>Amplitude, Analysis>Pitch | 16 | `ana.amplitude`, `ana.pitch`, `ana.onsets` |
| `rnd` | Random | 11 | `rnd.rand`, `rnd.exprand`, `rnd.trand`, `rnd.randseed` |
| `beq` | Filters>BEQSuite | 8 | `beq.blowpass`, `beq.bpeakeq` |
| `node` | Synth control | 8 | `node.freeself`, `node.detectsilence`, `node.done` |
| `bela` | Bela | 7 | `bela.analogin` |
| `amb` | Multichannel>Ambisonics | 5 | `amb.panb2`, `amb.decodeb2` |
| `ui` | User interaction | 4 | `ui.mousex`, `ui.keystate` |
| `gran` | Generators>Granular | 3 | `gran.grainsin`, `gran.grainfm` |

A category gets a namespace when it meets any of these:

- it forms its own sub-language: FFT chains, demand chains, buffer indices;
- it targets one setting: Bela, ambisonics, mouse and keyboard;
- its names clash with interpreter words: Random.

Seven classes are placed by hand. Five have no category: `blocksize`, `nodeid` to `info`; `clearbuf`, `setbuf` to `buf`; `scopeout2` to `io`. `randid` and `randseed`, filed under Generators>Stochastic, go to `rnd` with the other random UGens.

### Hidden

13 classes are emitted by the compiler and never typed: `binaryopugen`, `unaryopugen`, `muladd`, `sum3`, `sum4`, `control`, `trigcontrol`, `lagcontrol`, `audiocontrol`, `outputproxy`, `listdugen`, `infougenbase`, `bufinfougenbase`.

### The index `ug`

`ug` holds all 394 public UGens under their plain names. `ug.<TAB>` browses the whole set, and `ug.lpf` reaches a bare UGen that a user binding hides. No one needs to type it otherwise.

## Clashes

With 163 bare UGens and 530 core interpreter words, one name clashes: `ramp`. The core's `ramp` clamps to 0..1; SC's `Ramp` is a linear lag. The core's `ramp` appears in neither `sapf-prelude.txt` nor `sapf-examples.txt`, so it moves to `sapf.ramp` and `ramp` is the UGen.

`rand`, `irand` and `linrand` no longer clash: their UGens live in `rnd`. The pair `rand` and `rnd.rand` marks the tier, as `rrand` and `Rand` do in sclang.

No namespace name matches a core word or a bare UGen.

## Namespaces holding both tiers

[stac.md](stac.md) proposes interpreter-side groups: `env`, `bus`, `buf`, `clock`, and node control. Where a topic has both interpreter words and UGens, one namespace holds both:

| Namespace | Interpreter words | UGens |
|-|-|-|
| `buf` | `buf.read`, `buf.load`, `buf.alloc` | `buf.playbuf`, `buf.bufrd`... |
| `node` | `node.group` | `node.freeself`, `node.done`... |
| `env` | `env.perc`, `env.adsr`, `env.asr`, `env.linen`, `env.new` | none; `envgen` is bare |
| `bus` | `bus.audio`, `bus.control` | none |
| `clock` | `clock.new` | none |

A topic completes as one list. Within these namespaces, the name no longer shows the tier; the stack effect in the hint does.

Methods on values are separate: `x.free` on a `Node` value is not `node.free`, SC's `Free` UGen.

## Rates

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

191 classes have one rate method, so a rate suffix carries no information for them. 216 have a choice.

### Default

The default is the highest rate, not `kr`. The two errors differ in kind:

- A UGen at `ar` that could run at `kr` sounds the same and uses more CPU. Not measured.
- A UGen at `kr` that should run at `ar` sounds wrong. `kr` updates once per 64-frame block, 750 Hz at 48 kHz, so an oscillator above 375 Hz aliases.

SC has no default rate.

### Lowering

`.kr` and `.ir` are methods on `Sig`, applied to the value a UGen word leaves:

```
.2 0 sinosc.kr                  ; parses as: .2 0 sinosc .kr
3.9 10 .5 chaos.logistic.kr     ; parses as: 3.9 10 .5 chaos .logistic .kr
```

- Parsing: a dot ends a word, and dots chain. Works today.
- Semantics: when the node has no consumer yet, `.kr` sets its rate, giving `SinOsc.kr`. Otherwise it inserts `A2K`. sapf-sc's `toKR` already does both.
- `.kr` on a class without `kr` is an error. `.kr` on a real returns the real.

sapf-sc's rate inference lowers a node whose consumers are all `kr`. Phase 5 of [stac.md](stac.md) removes it. Keeping it is open question 1.

## The `Namespace` object

Forms work as namespaces today:

```
{:Osc {:sin \f self [f 2 *] :k 7} :v 42} = m
m,v                             ; 42: comma looks up without applying
m,Osc,k                         ; 7: lookups chain
5 m.Osc.sin                     ; 10: dot applies a function, pushes anything else
m = M   5 M,Osc.sin             ; 10: a namespace can be aliased
```

A Form's `.name` passes the Form itself as an extra last argument, `self`. `1 2 m.add`, with `add` bound to `\a b [a b +]`, gets the Form as `b` and fails. `1 2 m,add !` works.

stac's namespaces therefore are not Forms. Each is a `Namespace` object whose `dot` applies the named word without `self`, and whose `comma` pushes it unapplied. `Object::dot` is virtual (`include/ObjectBase.hpp:85`), so this needs no parser change. Inferred from the code; not built.

User modules have the same `self` problem and use the same object. A loader that runs a file in a fresh scope and returns its bindings as a `Namespace` is not part of this proposal; `load` runs a file in the global workspace.

## Words, not classes

`sinosc` and `fft.pv_magfreeze` are generated primitives. A word bound to a Form pushes the Form instead of calling it, so a UGen cannot be a Form.

- Rate belongs to the result, not the class.
- A UGen class is not a value. `` `sinosc `` pushes the primitive as a function, which covers mapping over oscillator types: ``[`sinosc `lfsaw] @ \f [440 0 f] !`` gives two channels. Works today. For namespaced UGens, `chaos,henonn` does the same once `Namespace` has `comma`.
- Help and argument metadata are attached to the primitive, as for every built-in word.

## Arguments

Unchanged from [stac.md](stac.md): positional in SC's order without `mul` and `add`, or a Form of named arguments with SC's defaults for the rest.

```
{:in sig :pos .5} pan2
```

## Definitions

A `def` statement defines a SynthDef. It replaces sapf-sc's `ctl` and `>sd`.

```
def beep freq:440 amp:.1 = [freq 0 sinosc amp *]
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
- A default is a literal number: `440`, `.1`, `-3`, `2pi`, `5/4`. A parameter without one defaults to 0. SC stores defaults as constants in the def file, so expressions are not allowed.
- The body compiles as a function body, with each parameter bound to a control output.

### Running a `def`

1. Compiles the SynthDef from the body.
2. Sends it (`/d_recv`) if a server runs, else before the first synth.
3. Binds `NAME` to a callable `Def`. `{...} NAME` starts a synth with those control values and returns its `Node`.

A def name then starts a synth as a UGen name builds a UGen. The callable `Def` is a new VM type; not built.

Running the statement again redefines and resends the def. Synths already playing keep the old graph, as in SC.

If the body leaves channels, the def adds `io.out` to a control `out`, default 0, as SC's `Function.play` does. If it leaves none, the body writes its own `io.out`. Controls take their slot order from parameter order.

`` `NAME `` pushes the `Def` without calling it. A `Def` has two methods: `.add` resends it; `.write` writes `NAME.scsyndef` and the `scsy` sidecar.

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
def beep freq:440 amp:.1 = [freq 0 sinosc amp *]

{:freq 660} beep = x
{:freq 880 :amp .2} x.set
x.free
```

A gated note. `release` sets `gate` to 0; `doneAction` 2 frees the synth when the envelope ends:

```
def pad freq:220 amp:.2 gate:1 = [
    freq saw 2000 .3 rlpf
    {:envelope .01 .2 .6 1 env.adsr :gate gate :doneAction 2} envgen *
    amp *
]

{:freq 330} pad = n
n.release
```

A percussive note that frees itself. `i_dur` is read once, at the start:

```
def ping freq:880 i_dur:.4 = [
    freq 0 sinosc
    {:envelope .002 i_dur env.perc :doneAction 2} envgen * .3 *
]

{:freq 1320 :i_dur 1} ping
```

A retriggerable synth. Each `set` of `t_trig` restarts the envelope; the synth stays alive:

```
def blip freq:200 t_trig = [
    freq 0 sinosc
    {:envelope .001 .2 env.perc :gate t_trig} envgen *
]

{} blip = b
{:t_trig 1} b.set                ; one hit
{:t_trig 1 :freq 300} b.set      ; another, higher
```

A stereo def. A list result writes consecutive channels from `out`:

```
def wide freq:110 amp:.1 = [[freq freq 1.003 *] saw amp *]
```

An effect on a private bus. The source writes to the bus; the effect reads it and writes to the hardware. The effect goes at the tail, so it runs after the source:

```
def verb in mix:.4 = [in 2 io.in mix .8 .5 freeverb]

2 bus.audio = fx
{:in fx :addAction 'tail} verb = v
{:out fx} wide
{:mix .7} v.set
```

A def played by a pattern. Each event starts one synth of `pad`; `legato` decides when it is released:

```
{:instrument 'pad
 :freq [220 275 330 440] cyc
 :dur .25
 :legato .8} pbind play
```

A def written for the SSP, replacing `>sd`:

```
def hiss cutoff:1200 res:.3 = [whitenoise .2 * cutoff res rlpf]
`hiss.write                      ; hiss.scsyndef, hiss.txt
```

No def. `play` on a function compiles a temporary def, as `{ ... }.play` does in SC:

```
[440 0 sinosc .1 *] play = y
y.free
```

### Defs built by code

The `def` statement takes a literal name and literal defaults. Code that builds defs, e.g. in a loop, uses the function `synthdef`:

```
(fun defaults name --> Def)
```

`fun`'s argument names become controls, `defaults` is a Form, `name` a symbol. It returns the `Def` unbound and unsent:

```
[['h2 2] ['h3 3] ['h5 5]] @ \p [
    p = [name n]
    \freq amp [freq n * 0 sinosc amp *] {:freq 220 :amp .1} name synthdef .add
] !
```

### Today

sapf-sc has `ctl` and `>sd`, which write a def with named controls:

```
440 'freq ctl 0 sinosc .1 'amp ctl * "sine" >sd
```

It has no `synth`, `set` or `release`. `play` starts an unnamed synth; `stop` frees every synth.

## Completion

Namespaces make completion effective: each groups a topic into one short list. The REPL reads lines with replxx (`src/engine/VM.cpp:255`) and sets no completion callback today. replxx provides `replxx_set_completion_callback` (`thirdparty/replxx/include/replxx.h:309`) and `replxx_set_hint_callback` (`:355`).

### Rules

- A prefix with no dot completes against core words, bare UGens, namespace names and user bindings.
- A prefix with a dot splits at the last dot. The completer looks up the namespace object at runtime and offers its keys: `fft.pv_mag<TAB>`, `env.<TAB>`. User namespaces complete the same way.
- After a value, as in `sinosc.k<TAB>`, the completer cannot know the stack's type without evaluating the line. It offers the `Sig` methods `kr`, `ir`, `ar`.

### Fan-out

| List | Size | After 2 letters |
|-|-|-|
| bare UGens | 163 | median 1, largest `lf` 14 |
| `fft` | 49 | `pv` 34 |
| `buf` | 38 | `bu` 17 |
| `dem` | 24 | `d` + 1 letter: median 1 |
| other namespaces | 3 to 23 | short |

The largest lists, `fft.pv_` and `buf.buf`, carry an SC prefix inside the namespace. Open question 4 covers stripping it.

### Hints

The hint callback shows a word's stack effect after it is typed:

```
440 0 sinosc          (freq phase --> out)
```

The text comes from the help string every primitive already carries. With full positional arity, this is how a player recalls argument order.

### Editors

Live coding usually sends text from an editor, which gets no REPL completion. `stac --words` would print every binding with its namespace and stack effect, one per line, for editor plugins to read.

## Changes to stac.md

| stac.md | This proposal |
|-|-|
| `440 0 SinOsc.ar` | `440 0 sinosc` |
| UGen classes are global Forms with `ar` `kr` `ir` methods | UGen words are primitives, bare or in a namespace; `.kr` `.ir` are `Sig` methods |
| every call names its rate | highest rate by default |
| `Env.perc`, `Bus.audio`, `Buffer.read`, `Clock.new`, `group` | `env.perc`, `bus.audio`, `buf.read`, `clock.new`, `node.group` |
| `fun defaults name def` | `def NAME PARAM* = [BODY]` statement; the function form is `synthdef` |
| `'sine {...} synth` | `{...} sine`: a def name starts a synth |

## Open questions

1. Rate inference: remove it, as stac.md proposes, or keep it so unmarked LFOs feeding only `kr` inputs lower themselves. It does not lower the common case, an LFO driving an `ar` filter's frequency.
2. `.ar`: on a `kr` UGen, raise it with `K2A`, or reject it.
3. Category boundaries: `io.in` and `io.out` are common enough to be bare; `klank` and `gendy1` are rare enough to namespace. The category rule keeps the build mechanical; exceptions cost a hand-kept list.
4. Prefix stripping inside a namespace: `buf.rd` for `buf.bufrd`, `fft.magfreeze` for `fft.pv_magfreeze`, `dem.seq` for `dem.dseq`. Shorter, but SC's help is then searched under a different name.
5. Namespace names: `dem`, `ana`, `amb`, `rnd` are short and opaque; `demand`, `analysis`, `ambi`, `random` read better and complete after 2 letters anyway.
6. Short aliases for frequent long names, e.g. `lfn1` for `lfnoise1`. Each alias is a second name to learn and lengthens completion lists; a prelude can add them per user.
7. `--words` format: plain lines, or JSON for editor plugins.
8. Control lag: SC's `\freq.kr(440, 0.1)` lags a control. A `def` parameter could take it after the default, as `freq:440~.1`. `~` is a name character today (`foo~` parses as one word), so this needs the same `def`-only lexing as `:`.
9. Array controls: SC's `freqs=#[100, 200]` makes a multichannel control. Whether `freqs:[100 200]` does the same is undecided.
10. Synth options and control values share one Form: `{:in fx :addAction 'tail} verb`. `target` and `addAction` then cannot name parameters. A second Form avoids that at the cost of a word per call.
11. Calling a def name always takes a Form, so a synth with all defaults is `{} blip`. An optional Form would read better but makes the word's arity depend on the stack.
