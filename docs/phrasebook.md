# stac phrasebook

The things a live coder types most, in sclang and in stac. Each phrase gives the sclang, then the stac, then what to notice. The words are in the [dictionary](dictionary/).

Every stac block here runs in the test suite (`tests/py/test_phrasebook.py`), after `prelude.stac` and the setup below, with a stand-in server; the sclang blocks are not run. A line ending in `;-> values` states what it leaves.

## Setup

Two instruments the pattern phrases play.

```stac-setup
synthdef ping freq:440 amp:.1 gate:1 "a sine with a short attack, released by gate." [
    freq sinosc  .005 .2 env.asr gate envgen.kr.done *  amp *  2 X
]
synthdef pad freq:220 amp:.1 gate:1 "a saw through a low-pass, adsr on gate." [
    freq saw 2000 .3 rlpf  .01 .2 .6 1 env.adsr gate envgen.kr.done *  amp *  2 X
]
```

## Sound

### P1. Test tone

```sclang
{ SinOsc.ar(440) * 0.1 }.play;
```
```stac
[440 sinosc .1 *] play
```

`[...]` collects the channels; `play` compiles them into a SynthDef and starts it.

### P2. Stop everything

```sclang
s.freeAll;      // or Cmd-.
```
```stac
stop
```

`stop` also ends every pattern and `ndef`.

### P3. Stereo

```sclang
{ SinOsc.ar([440, 442]) * 0.1 }.play;
{ SinOsc.ar(440) ! 2 * 0.1 }.play;
```
```stac
[[440 442] sinosc .1 *] play
[440 sinosc 2 X .1 *] play
```

A list of frequencies makes a list of channels, as in SC. `2 X` copies one channel twice.

### P4. Additive partials

```sclang
{ Mix(SinOsc.ar([1, 2, 3, 4] * 200, 0, [1, 0.5, 0.33, 0.25])) * 0.1 }.play;
```
```stac
[[1 2 3 4] 200 * sinosc [1 .5 .33 .25] * +/ .1 *] play
```

`+/` sums a list; it replaces `Mix`.

### P5. LFO on a filter

```sclang
{ RLPF.ar(Saw.ar(110), LFNoise1.kr(2).range(300, 3000), 0.2) * 0.1 }.play;
```
```stac
[110 saw  2 lfnoise1.kr 300 3000 bilin  .2 rlpf  .1 *] play
```

`bilin` maps a -1..1 signal onto a linear range, as `.range` does; `biexp` maps it exponentially, as `.exprange`.

### P6. Detuned spread

```sclang
{ Splay.ar(Saw.ar([100, 100.5, 99.7, 200.3])) * 0.1 }.play;
```
```stac
[[100 100.5 99.7 200.3] saw [-1 -.33 .33 1] pan2 +/ .05 *] play
```

`pan2` places each voice; `+/` sums the four stereo pairs into one.

## Definitions

### P7. Percussive note

```sclang
SynthDef(\click, { |freq = 880, amp = 0.2|
    var env = EnvGen.kr(Env.perc(0.002, 0.4), doneAction: 2);
    Out.ar(0, SinOsc.ar(freq) * env * amp ! 2)
}).add;
Synth(\click, [freq: 1320]);
```
```stac
synthdef click freq:880 amp:.2 [freq sinosc .002 .4 env.perc 1 envgen.kr.done * amp * 2 X]
{:freq 1320} click
```

`.done` sets the envelope's `doneAction` to 2, so the synth frees itself.

### P8. Gated note

```sclang
x = Synth(\pad, [freq: 330]);
x.release;
```
```stac
{:freq 330} pad = x
x.release
```

`pad` is defined in the setup. `.release` sets its `gate` to 0.

### P9. Hi-hat

```sclang
SynthDef(\hat, { |amp = 0.3|
    var env = EnvGen.kr(Env.perc(0.001, 0.05), doneAction: 2);
    Out.ar(0, HPF.ar(WhiteNoise.ar, 8000) * env * amp ! 2)
}).add;
```
```stac
synthdef hat amp:.3 [whitenoise 8000 hpf .001 .05 env.perc 1 envgen.kr.done * amp * 2 X]
{} hat
```

### P10. Kick

```sclang
SynthDef(\kick, { |amp = 0.5|
    var env = EnvGen.kr(Env.perc(0.001, 0.3), doneAction: 2);
    Out.ar(0, SinOsc.ar(XLine.kr(200, 50, 0.1)) * env * amp ! 2)
}).add;
```
```stac
synthdef kick amp:.5 [200 50 .1 xline.kr sinosc .001 .3 env.perc 1 envgen.kr.done * amp * 2 X]
{} kick
```

### P11. Echo in place

```sclang
{ var s = Decay2.ar(Dust.ar(2), 0.01, 0.2) * SinOsc.ar(800);
  s + CombC.ar(s, 0.5, 0.375, 4) * 0.2 }.play;
```
```stac
[2 dust .01 .2 decay2 800 sinosc * = s  s s .5 .375 4 combc + .2 *] play
```

`= s` binds a name inside the list, as sclang's `var`.

### P12. Effect on a bus

```sclang
SynthDef(\verb, { |in, mix = 0.4|
    Out.ar(0, FreeVerb.ar(In.ar(in, 2), mix, 0.8, 0.5))
}).add;
b = Bus.audio(s, 2);
v = Synth(\verb, [in: b], addAction: \addToTail);
Synth(\pad, [out: b]);
```
```stac
synthdef verb bus mix:.4 [bus 2 in mix .8 .5 freeverb]
2 bus.audio = fx
{:bus fx :addAction 'tail} verb = v
{:out fx} pad
```

The reverb runs at the tail, after its source. A bus is its index as a control value.

## Control

### P13. Change a running synth

```sclang
x.set(\freq, 330, \amp, 0.1);
```
```stac
{} pad = x
{:freq 330 :amp .1} x.set
```

### P14. Release

```sclang
x.release;
x.release(4);   // over 4 seconds
```
```stac
{} pad = x
x.release
{} pad = y
4 y.releaseIn
```

### P15. Read a value

```sclang
{ SinOsc.ar(LFNoise1.kr(2).poll * 200 + 400) * 0.1 }.play;
```
```stac
[2 lfnoise1.kr.poll 200 * 400 + sinosc .1 *] play
```

`.poll` prints the value on scsynth's console 10 times a second and passes it on.

### P16. A control shared by many synths

```sclang
c = Bus.control(s, 1); c.set(440);
{ SinOsc.ar(In.kr(c)) * 0.1 }.play;
c.set(660);
```
```stac
1 bus.control = c
440 c.set
[c 1 in.kr sinosc .1 *] play
660 c.set
```

## Patterns

### P17. Melody

```sclang
Pbind(\instrument, \ping, \midinote, Pseq([60, 62, 64, 67], inf), \dur, 0.25).play;
```
```stac
{:instrument 'ping :midinote [60 62 64 67] cyc :dur .25} pbind play = p
p.stop
```

`cyc` repeats a list forever, as `Pseq(list, inf)`.

### P18. Random notes from a scale

```sclang
Pbind(\instrument, \ping, \degree, Prand([0, 2, 4, 5, 7], inf),
      \scale, Scale.minor, \dur, 0.125).play;
```
```stac
{:instrument 'ping :degree [0 2 4 5 7] picks :scale 'minor :dur .125} pbind play = p
p.stop
```

`picks` chooses at random, as `Prand`.

### P19. Chords

```sclang
Pbind(\instrument, \pad, \midinote, Pseq([[60, 64, 67], [62, 65, 69]], inf),
      \dur, 1, \legato, 0.9).play;
```
```stac
{:instrument 'pad :midinote [[60 64 67] [62 65 69]] cyc :dur 1 :legato .9} pbind play = p
p.stop
```

A list in a field plays one synth per item.

### P20. Replace a running pattern

```sclang
Pdef(\mel, Pbind(\instrument, \ping, \midinote, Pseq([60, 62, 64], inf), \dur, 0.25)).play;
Pdef(\mel, Pbind(\instrument, \ping, \midinote, Pseq([67, 65], inf), \dur, 0.125));
```
```stac
{:instrument 'ping :midinote [60 62 64] cyc :dur .25} pbind 'mel pdef = p
{:instrument 'ping :midinote [67 65] cyc :dur .125} pbind 'mel pdef
p.stop
```

The second line replaces the first on the next beat.

### P21. Replace a running sound

```sclang
Ndef(\drone, { Saw.ar([55, 55.2]) * 0.1 }).play;
Ndef(\drone, { RLPF.ar(Saw.ar([55, 55.2]), 800, 0.3) * 0.1 });
```
```stac
[[55 55.2] saw .1 *] 'drone ndef = d
[[55 55.2] saw 800 .3 rlpf .1 *] 'drone ndef
d.end
```

The second line crossfades into the first over `fadeTime`, .02 s unless `{:fadeTime t} d.set` changes it.

### P22. Filter one sound with another

```sclang
Ndef(\src, { Saw.ar([55, 55.2]) * 0.2 });
Ndef(\filt, { RLPF.ar(Ndef(\src).ar, LFNoise1.kr(1).exprange(200, 2000), 0.2) }).play;
```
```stac
[[55 55.2] saw .2 *] 'src ndef = src
src.stop
[src.ar  1 lfnoise1.kr 200 2000 biexp  .2 rlpf] 'filt ndef = filt
filt.end
src.end
```

`src.stop` silences the source's monitor; `src.ar` still reads its bus.

### P23. Change the tempo

```sclang
t = TempoClock(2);
Pbind(\instrument, \ping, \dur, 1).play(t);
t.tempo = 3;
```
```stac
2 clock.new = t
{:instrument 'ping :dur 1} pbind t.play = p
{:tempo 3} t.set
p.stop
```

Event times and releases are in beats, so both follow the new tempo.

### P24. Replace on the bar

```sclang
Pdef(\beat).quant = 4;
Pdef(\beat, Pbind(\instrument, \ping, \dur, 0.5)).play;
```
```stac
{:instrument 'ping :dur .5} pbind 'beat pdef = b
{:quant 4} b.set
{:instrument 'ping :midinote 72 :dur .25} pbind 'beat pdef
b.stop
```

The replacement waits for the next multiple of 4 beats.

## Buffers and sequencing

### P25. Loop a sample

```sclang
b = Buffer.read(s, "loop.wav");
{ PlayBuf.ar(1, b, BufRateScale.kr(b), loop: 1) * 0.5 }.play;
```
```stac
"tests/fixtures/ref/adsr.wav" buffer.read = b
[{:numChannels 1 :bufnum b :rate b bufratescale.kr :loop 1} playbuf .5 *] play
```

`playbuf` takes many arguments, so a keyword Form names the ones that matter. Any WAV or AIFF file works.

### P26. Step sequence inside a synth

```sclang
{ var f = Demand.kr(Impulse.kr(8), 0, Dseq([60, 63, 67, 70], inf)).midicps;
  Saw.ar(f) * 0.1 }.play;
```
```stac
[8 impulse.kr 0 [60 63 67 70] inf dseq demand.kr nnhz saw .1 *] play
```

`nnhz` is SC's `midicps`.

## Output

### P27. Render to a file

```sclang
// Score.recordNRT, or s.record and s.stopRecording around a performance
```
```stac
[440 sinosc .1 *] .5 "tone" render
{:instrument 'ping :midinote [60 64 67] cyc :dur .25} pbind 1 "arpeggio" render
```

`render` runs scsynth offline, faster than real time, into `$STAC_RECORDINGS`.

### P28. Write a score

```sclang
Pbind(\instrument, \ping, \dur, 0.25).asScore(4).saveToFile("score.osc");
```
```stac
{:instrument 'ping :midinote [60 62] cyc :dur .25 :tempo 2} pbind 4 "melody" score
```

An event's `:tempo` sets the score's tempo from that event on.

## Live coding

### P29. Edit a helper while it plays

```sclang
~shape = { |f| f * 2 };
Pbind(\instrument, \ping, \freq, Pfunc { ~shape.(220) }, \dur, 0.25).play;
~shape = { |f| f * 3 };    // the next event uses the new function
```
```stac
def shape f --> num [f 2 *]
{:instrument \f [f shape sinosc .01 .2 env.perc 1 envgen.kr.done * .1 *] :f 220 :dur .25} pbind play = p
def shape f --> num [f 3 *]
p.stop
```

A top-level name binds late: the running pattern reads the new `shape` from its next event. A REPL line or a loaded file lands all its changes at once.

### P30. A constant

```stac
const tuning "the reference A in Hz." [432]
69 nnhz tuning 440 / *   ;-> 432
```

`=` cannot rebind `tuning`; a module reload can change it.

### P31. A library of your own

A module is a file. Its names starting with `_` stay private; the rest are its namespace.

```text
;; drums.stac
synthdef kick amp:.5 [200 50 .1 xline.kr sinosc .001 .3 env.perc 1 envgen.kr.done * amp * 2 X]
def _accent n --> num [n 4 % 0 == .4 * .6 +]
def beat n --> list [n 1 to @ \i [i _accent] !]
```

```stac
import std.*
[1 2 3] mean   ;-> 2
```

`import drums.*` binds `kick` and `beat`, not `_accent`. Editing `drums.stac` and running the import again reloads it; code already playing picks up the change at its next event. `std`, the standard library, is a module of the same kind.

## Style

A one-liner suits performance; a sound worth keeping gets one stage per line.

```stac
[2 dust .01 .2 decay2 800 sinosc * = s  s s .5 .375 4 combc + .2 *] play
```

The same sound, kept:

```stac
def echo in --> sig "a 375 ms echo." [in .5 .375 4 combc]

synthdef clicks rate:2 ring:800 amp:.2 "rings dust at a pitch, with an echo." [
    rate dust .01 .2 decay2      = clicks   ; 10 ms attack, 200 ms decay
    clicks ring sinosc *         = rung
    rung rung echo +  amp *
]
{:ring 1200} clicks
```

Rules, in order of effect:

1. One stage per line, with its result named by `= name`: source, colouring, effect, level.
2. Factor a reusable stage into a `def` with a help string; its stack effect shows in `--words` and the REPL's hints.
3. Promote a kept sound to a `synthdef`. Its numbers become controls that `.set` changes while it plays.
4. Comment numbers, not words: `; 10 ms attack` explains `.01`; `decay2` needs no comment.
5. Use a keyword Form where positional numbers are ambiguous: `{:in s :maxdelaytime .5 :delaytime .375 :decaytime 4} combc` names each value.
6. Whitespace matters in 3 places: `-2` and `- 2`, `5/4` and `5 / 4`, and `2pi`. A dot before a digit starts a number: `*.5` is `*` then `.5`.
