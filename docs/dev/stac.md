# stac

Status: proposal, 2026-10-15. Nothing here is implemented unless marked "works today".

stac is a concatenative language for SuperCollider. The interpreter plays the part of sclang: values, lists, patterns, scheduling. scsynth does all DSP. The syntax and core come from sapf. UGen names, arguments, units, rates and the server model come from SuperCollider.

stac does not implement sapf. [shakfu/sapf](https://github.com/shakfu/sapf) does. [../comparison.md](../comparison.md) lists where sapf-sc already departs from sapf; stac makes those departures the design.

## Principles

1. **Two tiers.** Lang values live in the interpreter. Graph values are nodes of a SynthDef. No sample is ever computed in the interpreter.

2. **SC names, SC units.** `SinOsc.ar` takes `freq` in Hz and `phase` in radians, as in SC. One spec file defines every UGen.

3. **Explicit rate.** Every UGen call names its rate. stac infers none.

4. **Lazy lists are patterns.** sapf's infinite streams already do what sclang's `Pattern` classes do. stac schedules them.

5. **sapf compatibility is a library.** `sapf.stac` defines sapf's words on top of stac. The core does not carry them.

6. **No new syntax** where the sapf parser already expresses the construct.

## Core kept from sapf

These are unchanged: the parser, the VM, postfix evaluation, number suffixes (`4k`, `20m`, `5/4`), strings, `'symbols`, `\a b [...]` functions, `= x` binding and destructuring, `[...]` lists, `{...}` forms with inheritance, automapping, `@` each, `/` `\` `^` reduce, scan and pairs, and the stream words (`cyc`, `picks`, `rands`, `by`, `grow`, `take`, `skip`, `hang`...).

## Values

| Type | Tier | Notes |
|-|-|-|
| Real, String, Symbol, List, Form, Function, Ref | lang | from sapf |
| `#[...]` | lang | a float array: buffer data, wave tables. Not a signal |
| `Sig` | graph | one output of a UGen; from sapf-sc |
| `Env` | lang | levels, times, curves, release and loop nodes; a UGen input flattens it |
| `Def` | lang | a compiled SynthDef |
| `Node`, `Group` | lang | a synth or group on the server |
| `Bus`, `Buffer` | lang | a server resource with an index; a UGen input reads its index |
| `Clock` | lang | tempo and beat time |

Removed: the `ZList` signal. A program that reads or rearranges samples in the interpreter has no stac form. It writes the samples into a `Buffer` instead.

## UGens

### Calls

```
440 0 SinOsc.ar                        ; SinOsc.ar(440, 0)
[100 101] Saw.ar 800 .3 RLPF.ar        ; RLPF.ar(Saw.ar([100, 101]), 800, 0.3)
5 LFNoise1.kr 200 * 400 +              ; LFNoise1.kr(5).range(200, 600)
```

`SinOsc` is a Form. `.ar` looks up `ar` in it and applies it with the Form as its last argument, `self`. This works today:

```
{:f \a b self [a b + self,k *] :k 10} = K
3 4 K.f                                ; 70
{K :k 100} = K2
3 4 K2.f                               ; 700, f inherited
```

So `Foo.ar`, `Foo.kr`, `Foo.ir` and `Foo.new` need no parser change. Each UGen class is a Form inheriting from a `UGen` Form whose methods are primitives.

### The spec

`../nanosynth/spec/sclang-reference.json` is extracted from SC 3.14.1's class library. It holds:

| | Count |
|-|-|
| UGen classes | 407 |
| `ar` methods | 279 |
| `kr` methods | 244 |
| `ir` methods | 35 |
| `new` methods (demand rate, `Out`-like) | 77 |
| methods with a fixed output count | 610 |
| methods whose output count is an argument (`PlayBuf`, `In`...) | 25 |

Each method lists argument names, defaults, output count and operator index. stac generates its UGen Forms from this file at build time. `sc3-plugins-reference.json` adds the sc3-plugins set. nanosynth already compiles from the same spec, so SCgf parity tests can cover every class.

A variable output count comes from its `numChannels` argument, which must be a constant.

### Arguments

- Positional: a call takes every argument except `mul` and `add`, in SC's order. `*` and `+` replace those two; `*+` does both.

- Keyword: a Form in place of the arguments names some and takes defaults for the rest. A Form is never a valid UGen input, so the two cases cannot be confused.

```
{:in sig :pos .5} Pan2.ar              ; Pan2.ar(sig, 0.5), level defaults to 1
[{:freq 440} {:freq 660}] SinOsc.ar    ; two channels
```

- Lists expand to channels, as in both sapf and SC.

- `ugen` stays for a UGen absent from the spec: `[in] 'Foo 'ar 1 ugen`.

### Pseudo-UGens

sclang classes such as `Mix`, `Splay` and `LinLin` are sclang code, not server UGens. stac defines them in the prelude. `Mix` is `+/`.

### Operators

Math on a `Sig` builds `BinaryOpUGen` or `UnaryOpUGen` and folds constants; this works today. Where sapf's operator name differs from SC's selector, stac takes SC's: `midicps` for `nnhz`, `cpsmidi` for `hznn`. `sapf.stac` aliases the sapf names.

### Rates

A UGen runs at the rate its call names. `K2A.ar` and `A2K.kr` convert. sapf-sc's inference (an audio-rate UGen read only at control rate becomes control rate) and its `kr` word are removed.

## SynthDefs

A function's arguments become controls:

```
\freq amp gate [
    {:freq freq :phase 0} SinOsc.ar amp *
    {:envelope .01 .3 .5 1 Env.adsr :gate gate :doneAction 2} EnvGen.kr *
] {:freq 440 :amp .1 :gate 1} 'sine def = sine

sine.add                               ; /d_recv
sine.write                             ; sine.scsyndef, and the scsy sidecar
```

- `def` is `(fun defaults name --> Def)`. It calls `fun` with one `Control` output per argument. Each default comes from the Form, else 0.

- Prefixes follow SC's `SynthDef`: `t_` gives a `TrigControl`, `i_` an `ir` control, `a_` an `AudioControl`. Others are `kr`.

- If `fun` leaves channels, `def` adds `Out.ar` to a control `out`, default 0, as SC's `Function.play` does. If it leaves none, the graph writes its own `Out`.

- `def` replaces `ctl` and `>sd`. Controls get their slot order from argument order, which settles the `scsy` sidecar slot question in TODO.md.

`play` on a function or channels compiles a temporary def and starts it, as `{ ... }.play` does in SC. It returns a `Node`.

```
[440 0 SinOsc.ar .1 *] play = x
x.free
```

## Server objects

| stac | sclang | OSC |
|-|-|-|
| `'sine {:freq 660} synth` | `Synth(\sine, [freq: 660])` | `/s_new` |
| `{:freq 880} x.set` | `x.set(\freq, 880)` | `/n_set` |
| `x.release` | `x.release` | `/n_set gate 0` |
| `x.free` | `x.free` | `/n_free` |
| `group` | `Group()` | `/g_new` |
| `2 Bus.audio` | `Bus.audio(s, 2)` | none; interpreter allocator |
| `"a.wav" Buffer.read` | `Buffer.read(s, "a.wav")` | `/b_allocRead` |
| `#[0 .5 1] Buffer.load` | `Buffer.loadCollection` | `/b_alloc`, `/b_setn` |
| `["/n_set" 1000 "freq" 440] send` | `s.sendMsg(...)` | as given |

Add actions go in the Form: `{:freq 660 :target g :addAction 'tail}`.

`set` on a running node restores live control, which sapf-sc lost with `zctl`, `zplug` and `set`. Buses and buffers reuse the allocators sapf-sc already has: private buses 512..959, sample buffers 0..63, wave tables from 512.

## Patterns

An event is a Form. An event stream is a lazy list of Forms. `pbind` builds one from a Form of fields:

```
{:instrument 'sine
 :midinote [60 64 67 72] cyc
 :dur [.25 .25 .5] cyc
 :amp .1} pbind = p

p 4 take                               ; the first 4 events
p play = player                        ; schedules synths on the default clock
player.stop
```

- A list field advances once per event. A real or `Sig` field repeats. The stream ends when its shortest finite field ends, the automapping rule sapf already uses.

- `play` on an event stream starts one synth per event, `dur` beats apart, with `legato` setting when it releases. Bundles go out .2 s ahead, as the `ola` scheduler does now.

- `'rest` in any field makes a rest.

- Pitch: `freq`, else `midinote midicps`. SC's `degree`, `scale` and `octave` chain is prelude code, if wanted.

- `:instrument` may also be a function. stac compiles it per event and reuses defs that compile alike, as `ola` voices do now.

sclang's pattern classes map to existing stream words:

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

`ola` becomes prelude code over `pbind` and is removed from the core.

### Clocks

`2 Clock.new` makes a clock at 2 beats per second. `p clk play` plays on it. `{:tempo 3} clk.set` changes tempo. The default clock runs at 1 beat per second.

### Rendering

- `item seconds "name" render` writes `name.wav` through the NRT renderer. `item` is channels, a function or an event stream.

- `item seconds "name" score` writes `name.osc` for `scsy`, replacing `>nrt`.

- Both take an explicit duration. sapf-sc's `>sf` infers it from where channels end, and refuses many graphs. SC's `Score.render` takes a duration.

## Demand rate

`Dseq.new` and the other demand UGens take a finite list. An infinite list is an error; `1024 take` makes it finite. sapf-sc's silent 1024-value cap is removed.

`seq`, `iseq`, `steps`, `imps`, `lines`, `xlines` and `curves` move to `sapf.stac`.

## Envelopes

| stac | sclang |
|-|-|
| `levels times curves Env.new` | `Env(levels, times, curves)` |
| `atk rel Env.perc` | `Env.perc(atk, rel)` |
| `a d s r Env.adsr` | `Env.adsr(a, d, s, r)` |
| `atk sus rel Env.linen` | `Env.linen(atk, sus, rel)` |
| `atk rel Env.asr` | `Env.asr(atk, 1, rel)` |

Curves are reals or the symbols `'lin 'exp 'sin 'wel 'sqr 'cub 'step 'hold`. `freeSelf` is 2, the `doneAction` that frees the synth. `N`, `T` and `NZ` on a `Sig` no longer end a synth; an envelope or `Line.kr` with `doneAction` does.

## sapf.stac

`sapf.stac` defines sapf's UGen words with sapf's argument order and units:

```
\freq phase [freq phase 2pi * SinOsc.ar] = sinosc
\in freq [in freq LPF.ar] = lpf
```

- [../ugens.md](../ugens.md) moves with it; its fidelity figures describe this library.

- The conformance tests against sapf renders test this library, not the core.

- `sapf-examples.txt` runs with `-p sapf.stac`.

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

Each phase leaves `make test` passing.

1. **Rename.** Binary, environment variables, files, docs. No change in behaviour.

2. **Spec UGens.** Generate `Foo.ar` Forms from the spec; keyword Forms. Old words remain. Test SCgf parity with nanosynth for every generated class.

3. **Defs and nodes.** `def`, `synth`, `set`, `free`, `release`, `Bus`, `Buffer`, `Group`.

4. **Patterns.** `pbind`, `Clock`, `play` on event streams, `render`, `score`. Rewrite `ola` on them.

5. **Split.** Move sapf words to `sapf.stac`. Remove the `ZList` signal path, rate inference, `kr`, `ctl`, `>sd`, `>sf`, `>nrt`, and `N`/`T` on `Sig`.

Phase 5 breaks every existing program. Phases 1-4 break none.

## Open questions

1. **Positional arity.** Full argument count is predictable but verbose: `EnvGen.kr` takes 6. The alternative is required arguments only, with the rest by keyword; SC's spec has defaults for most arguments, so "required" needs a curated list.

2. **Rate inference.** Removing it makes graphs predictable and costs typing. A `.auto` method could keep it opt-in.

3. **Render duration.** Explicit only, or inferred when every channel has a `doneAction`.

4. **`!`.** sapf's `!` applies a function. SC's `!` duplicates. stac keeps sapf's.

5. **Routines.** sclang has `fork` and `wait`. stac's VM has no coroutines. Event streams cover sequencing; a function scheduled on a clock (`clk 4 fun sched`) covers the rest without them.

6. **Spec location.** Vendor `sclang-reference.json` into this repo, or read it from nanosynth at build time.
