# stac phrasebook

Status: working document, 2026-10-15.

The things a live coder types most, in sclang and in stac. Each stac line is checked against the design in [stac.md](stac.md) and [stac-syntax-1.md](stac-syntax-1.md). Friction found here decides open questions in those documents.

Each phrase shows:

- `sclang`: idiomatic SuperCollider.
- `stac draft`: the current design, as written.
- `stac fix`: the same line with the candidates in [Candidates](#candidates), where they help.

Argument orders come from `../nanosynth/spec/sclang-reference.json` (SC 3.14.1). No stac line runs today unless marked "works today".

## Sound

### P1. Test tone

```sclang
{ SinOsc.ar(440) * 0.1 }.play;
```
```stac-draft
[440 0 sinosc .1 *] play
```
```stac-fix
[440 sinosc .1 *] play
```

The draft passes `phase` 0 because positional calls take every argument. C1 drops it.

### P2. Stop everything

```sclang
s.freeAll;      // or Cmd-.
```
```stac-draft
stop
```

### P3. Stereo

```sclang
{ SinOsc.ar([440, 442]) * 0.1 }.play;
{ SinOsc.ar(440) ! 2 * 0.1 }.play;
```
```stac-draft
[[440 442] 0 sinosc .1 *] play
[440 0 sinosc 2 X .1 *] play
```
```stac-fix
[[440 442] sinosc .1 *] play
[440 sinosc 2 X .1 *] play
```

`2 X` copies a value into a list. On a UGen output it gives two channels; works today.

### P4. Additive partials

```sclang
{ Mix(SinOsc.ar([1, 2, 3, 4] * 200, 0, [1, 0.5, 0.33, 0.25])) * 0.1 }.play;
```
```stac-draft
[[1 2 3 4] 200 * 0 sinosc [1 .5 .33 .25] * +/ .1 *] play
```
```stac-fix
[[1 2 3 4] 200 * sinosc [1 .5 .33 .25] * +/ .1 *] play
```

`+/` is sapf's sum reduction; it replaces `Mix`. The graph part works today.

### P5. LFO on a filter

```sclang
{ RLPF.ar(Saw.ar(110), LFNoise1.kr(2).range(300, 3000), 0.2) * 0.1 }.play;
```
```stac-draft
[110 saw  2 lfnoise1.kr 1350 * 1650 +  .2 rlpf .1 *] play
```
```stac-fix
[110 saw  2 lfnoise1.kr 300 3000 range  .2 rlpf .1 *] play
```

The draft has no `range`, so the player computes the scale and offset. C3 adds `range`.

### P6. Detuned spread

```sclang
{ Splay.ar(Saw.ar([100, 100.5, 99.7, 200.3])) * 0.1 }.play;
```
```stac-draft
[[100 100.5 99.7 200.3] saw 1 1 0 1 splay .1 *] play
```
```stac-fix
[[100 100.5 99.7 200.3] saw splay .1 *] play
```

`Splay` is sclang code, so `splay` is prelude code with an arity of stac's choosing. The draft copies SC's 5 arguments; C1 cuts them to 2, and the prelude can take 1.

## Definitions

### P7. Percussive note

```sclang
SynthDef(\ping, { |freq = 880, amp = 0.2|
    var env = EnvGen.kr(Env.perc(0.002, 0.4), doneAction: 2);
    Out.ar(0, SinOsc.ar(freq) * env * amp ! 2)
}).add;
Synth(\ping, [freq: 1320]);
```
```stac-draft
def ping freq:880 amp:.2 = [
    freq 0 sinosc
    {:envelope .002 .4 env.perc :doneAction 2} envgen.kr *
    amp * 2 X
]
{:freq 1320} ping
```
```stac-fix
def ping freq:880 amp:.2 = [freq sinosc .002 .4 env.perc 1 envgen.kr.done * amp * 2 X]
{:freq 1320} ping
```

`envgen` takes 6 arguments, so the draft uses the keyword Form. C1 cuts it to `envelope gate`; C2's `.done` sets `doneAction` 2.

### P8. Gated note

```sclang
SynthDef(\pad, { |freq = 220, amp = 0.2, gate = 1|
    var env = EnvGen.kr(Env.adsr(0.01, 0.2, 0.6, 1), gate, doneAction: 2);
    Out.ar(0, RLPF.ar(Saw.ar(freq), 2000, 0.3) * env * amp ! 2)
}).add;
x = Synth(\pad);
x.release;
```
```stac-draft
def pad freq:220 amp:.2 gate:1 = [
    freq saw 2000 .3 rlpf
    {:envelope .01 .2 .6 1 env.adsr :gate gate :doneAction 2} envgen.kr *
    amp * 2 X
]
{} pad = x
x.release
```
```stac-fix
def pad freq:220 amp:.2 gate:1 = [
    freq saw 2000 .3 rlpf  .01 .2 .6 1 env.adsr gate envgen.kr.done *  amp * 2 X
]
{} pad = x
x.release
```

### P9. Hi-hat

```sclang
SynthDef(\hat, { |amp = 0.3|
    var env = EnvGen.kr(Env.perc(0.001, 0.05), doneAction: 2);
    Out.ar(0, HPF.ar(WhiteNoise.ar, 8000) * env * amp ! 2)
}).add;
```
```stac-draft
def hat amp:.3 = [
    whitenoise 8000 hpf
    {:envelope .001 .05 env.perc :doneAction 2} envgen.kr *
    amp * 2 X
]
```
```stac-fix
def hat amp:.3 = [whitenoise 8000 hpf .001 .05 env.perc 1 envgen.kr.done * amp * 2 X]
```

### P10. Kick

```sclang
SynthDef(\kick, { |amp = 0.5|
    var env = EnvGen.kr(Env.perc(0.001, 0.3), doneAction: 2);
    Out.ar(0, SinOsc.ar(XLine.kr(200, 50, 0.1)) * env * amp ! 2)
}).add;
```
```stac-draft
def kick amp:.5 = [
    200 50 .1 0 xline.kr 0 sinosc
    {:envelope .001 .3 env.perc :doneAction 2} envgen.kr *
    amp * 2 X
]
```
```stac-fix
def kick amp:.5 = [200 50 .1 xline.kr sinosc .001 .3 env.perc 1 envgen.kr.done * amp * 2 X]
```

`xline` ends with `doneAction`, which C1 drops.

### P11. Echo in place

```sclang
{ var s = Decay2.ar(Dust.ar(2), 0.01, 0.2) * SinOsc.ar(800);
  s + CombC.ar(s, 0.5, 0.375, 4) * 0.2 }.play;
```
```stac-draft
[2 dust .01 .2 decay2 800 0 sinosc * = s  s s .5 .375 4 combc + .2 *] play
```
```stac-fix
[2 dust .01 .2 decay2 800 sinosc * = s  s s .5 .375 4 combc + .2 *] play
```

`= s` binds a name inside the list. Works today, with sapf-sc's `dust`, which also takes `amp`: `2 1 dust`.

### P12. Effect on a bus

```sclang
SynthDef(\verb, { |in, mix = 0.4|
    Out.ar(0, FreeVerb.ar(In.ar(in, 2), mix, 0.8, 0.5))
}).add;
b = Bus.audio(s, 2);
v = Synth(\verb, [in: b], addAction: \addToTail);
Synth(\pad, [out: b]);
```
```stac-draft
def verb in mix:.4 = [in 2 io.in mix .8 .5 freeverb]
2 bus.audio = fx
{:in fx :addAction 'tail} verb = v
{:out fx} pad
```

`io.in` is the one namespaced word an everyday def needs. `addAction` shares the Form with control values (open question 10 in stac-syntax-1.md).

## Control

### P13. Change a running synth

```sclang
x.set(\freq, 330, \amp, 0.1);
```
```stac-draft
{:freq 330 :amp .1} x.set
```

### P14. Release

```sclang
x.release;
x.release(4);   // over 4 seconds
```
```stac-draft
x.release
```

The draft has no release time. SC sets `gate` to `-1 - time`; a separate method, e.g. `4 x.releaseIn`, keeps arities fixed.

### P15. Read a value

```sclang
{ LFNoise1.kr(2).poll; ... }.play;
```
```stac-draft
[{:trig 10 impulse.kr :in 2 lfnoise1.kr} info.poll ...] play
```
```stac-fix
[2 lfnoise1.kr.poll ...] play
```

`poll` sits in `info`, and `Poll` takes 4 arguments. C9 makes `.poll` a `Sig` method that prints 10 times a second and returns its input, as SC's `.poll` does.

## Patterns

### P16. Melody

```sclang
Pbind(\instrument, \ping, \midinote, Pseq([60, 62, 64, 67], inf), \dur, 0.25).play;
```
```stac-draft
{:instrument 'ping :midinote [60 62 64 67] cyc :dur .25} pbind play
```

### P17. Random notes from a scale

```sclang
Pbind(\instrument, \ping, \degree, Prand([0, 2, 4, 5, 7], inf),
      \scale, Scale.minor, \dur, 0.125).play;
```
```stac-draft
{:instrument 'ping :midinote [60 62 63 65 67] picks :dur .125} pbind play
```
```stac-fix
{:instrument 'ping :degree [0 2 4 5 7] picks :scale 'minor :dur .125} pbind play
```

The draft has no degrees or scales, so the player spells MIDI notes. C7 adds SC's pitch keys.

### P18. Chords

```sclang
Pbind(\instrument, \pad, \midinote, Pseq([[60, 64, 67], [62, 65, 69]], inf),
      \dur, 1, \legato, 0.9).play;
```
```stac-draft
{:instrument 'pad :midinote [[60 64 67] [62 65 69]] cyc :dur 1 :legato .9} pbind play
```

The draft does not say what a list element of a field means. C5: one synth per element, as in SC. `[[60 64 67] [62 65 69]] cyc` already yields one chord per step today.

### P19. Replace a running pattern

```sclang
Pdef(\mel, Pbind(\instrument, \ping, \midinote, Pseq([60, 62, 64], inf), \dur, 0.25)).play;
Pdef(\mel, Pbind(\instrument, \ping, \midinote, Pseq([67, 65], inf), \dur, 0.125));
```
```stac-fix
{:instrument 'ping :midinote [60 62 64] cyc :dur .25} pbind 'mel pdef
{:instrument 'ping :midinote [67 65] cyc :dur .125} pbind 'mel pdef
```

The draft has no form for this. C6 adds `pdef`: the second line replaces the first at the next beat.

### P20. Replace a running sound

```sclang
Ndef(\drone, { Saw.ar([55, 55.2]) * 0.1 }).play;
Ndef(\drone, { RLPF.ar(Saw.ar([55, 55.2]), 800, 0.3) * 0.1 });
```
```stac-fix
[[55 55.2] saw .1 *] 'drone ndef
[[55 55.2] saw 800 .3 rlpf .1 *] 'drone ndef
```

C6 adds `ndef`: the second line crossfades into the first.

## Buffers and sequencing

### P21. Loop a sample

```sclang
b = Buffer.read(s, "loop.wav");
{ PlayBuf.ar(2, b, BufRateScale.kr(b), loop: 1) * 0.5 }.play;
```
```stac-draft
"loop.wav" buf.read = b
[2 b b buf.bufratescale.kr 1 0 1 0 buf.playbuf .5 *] play
```
```stac-fix
"loop.wav" buf.read = b
[{:numChannels 2 :bufnum b :rate b buf.bufratescale.kr :loop 1} buf.playbuf .5 *] play
```

`playbuf` takes 7 arguments. C1 drops only `doneAction`, so the keyword Form stays the readable choice. A prelude word for the common case, e.g. `b loopbuf`, would serve live use.

### P22. Step sequence inside a synth

```sclang
{ var f = Demand.kr(Impulse.kr(8), 0, Dseq([60, 63, 67, 70], inf)).midicps;
  Saw.ar(f) * 0.1 }.play;
```
```stac-draft
[8 0 impulse.kr 0 [60 63 67 70] 1000000 dem.dseq dem.demand.kr midicps saw .1 *] play
```
```stac-fix
[8 impulse.kr 0 [60 63 67 70] inf dem.dseq dem.demand.kr midicps saw .1 *] play
```

stac has no `inf`; sapf-sc reports it undefined. C4 adds it.

## Output

### P23. Render to a file

```sclang
// Score.recordNRT, or s.record / s.stopRecording around a performance
```
```stac-draft
[440 0 sinosc .1 *] 4 "tone" render
```

## Candidates

| | Candidate | Phrases | Mechanism |
|-|-|-|-|
| C1 | Trailing tail parameters leave positional arity | P1, P3, P4, P6-P11, P15, P21, P22 | see below |
| C2 | `.done` on a `Sig` sets its `doneAction` to 2 | P7-P10 | like `.kr`, edits the node just built |
| C3 | `range`, `exprange`: map a bipolar signal to lo..hi | P5 | prelude; assumes -1..1 |
| C4 | `inf` | P22 | a constant |
| C5 | A list element of an event field plays one synth per element | P18 | pattern player |
| C6 | `pdef` and `ndef`: named, replaceable players | P19, P20 | pattern player; crossfade for `ndef` |
| C7 | SC's pitch keys: `degree`, `scale`, `octave`, `root` | P17 | prelude event processing |
| C8 | `releaseIn`: release over a time | P14 | `/n_set gate -1-time` |
| C9 | `.poll` on a `Sig` | P15 | `Poll` on a 10 Hz `Impulse`, returns its input |

### C1: tail parameters

A positional call omits trailing arguments whose names are in this list:

`phase`, `iphase`, `doneAction`, `levelScale`, `levelBias`, `timeScale`, `trigger`, `startPos`, `reset`, `level`, `levelComp`, `center`

Omitted arguments take SC's defaults. The keyword Form still reaches them. Only trailing ones are dropped, so argument positions never shift.

Across the 407 classes, C1 changes 33. They include the most used: `SinOsc` 2 to 1, `LFSaw`, `LFTri`, `Impulse` 2 to 1, `EnvGen` 6 to 2, `Line`, `XLine` 4 to 3, `Pan2` 3 to 2, `Splay` 5 to 2. Mean arity barely moves, from 3.0 to 2.9: most classes have no tail parameters. 124 classes take more than 3 arguments; 113 still do under C1.

The list is a convention to keep by hand. It is 12 names, not a table of 407 classes.

## What the phrasebook settles

| Open question | Evidence | Settled as |
|-|-|-|
| Positional arity (stac.md 1) | 12 of 23 phrases pass a value only to fill a tail parameter, or switch to the keyword Form | C1 |
| Default rate (stac-syntax-1 1) | `.kr` appears only on envelopes and LFOs, where SC users also write it | highest rate by default stands |
| Short aliases (stac-syntax-1 6) | the longest everyday names are `lfnoise1`, `freeverb`, `envgen` | not needed |
| Namespace boundaries (stac-syntax-1 3) | everyday phrases need `io.in` once and `info.poll` never, given C9 | category rule stands; move `in` and `out` bare |
| Synth options in the control Form (stac-syntax-1 10) | P12 is the only phrase that needs one | open; rare enough to keep simple |
| Patterns (stac.md) | P18-P20 need C5 and C6 | add both to the pattern design |
