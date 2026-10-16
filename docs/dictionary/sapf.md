# sapf

sapf's words, in the namespace sapf, and sapf.stac's definitions over them.

## unary math ops

### sapf.ramp

`sapf.ramp (x --> z)`

Clamps x to 0..1: 0 below 0, 1 above 1, x between.

The bare `ramp` is SC's `Ramp`, a linear lag; `clip` with 0 and 1 does the same on reals.

## list ordering operations

### sapf.N

`sapf.N (list n --> list)`

Takes the first n items of a list. On a UGen it ends the synth after n frames.

The core `N` takes lists only; a UGen ends with an envelope and doneAction 2.

### sapf.keep

`sapf.keep (list n --> list)`

As `sapf.N`: the first n items of a list, or a UGen ended after n frames.

The core `keep` takes lists only.

### sapf.NZ

`sapf.NZ (signal n --> signal)`

Takes the first n items of a signal. On a UGen it ends the synth after n frames.

The core `NZ` takes lists only.

### sapf.T

`sapf.T (signal t --> signal)`

Takes the first t seconds of a signal. On a UGen it ends the synth after t seconds, through an `EnvGen`.

The core `T` takes lists only; a UGen ends with an envelope and doneAction 2.

## dsp operations

### sapf.hanning

`sapf.hanning (n --> out)`

A signal of n frames holding a Hanning window.

### sapf.hamming

`sapf.hamming (n --> out)`

A signal of n frames holding a Hamming window.

### sapf.blackman

`sapf.blackman (n --> out)`

A signal of n frames holding a Blackman window.

### sapf.seg

`sapf.seg (in hops durs --> out)`

Cuts a signal into a stream of segments of the given durations, one starting every hop.

### sapf.wseg

`sapf.wseg (in hops window --> out)`

Cuts a signal into a stream of windowed segments as long as the window, one starting every hop.

## random signals

### sapf.randz

`sapf.randz (a b --> r)`

A signal of uniform random reals from a to b.

### sapf.coinz

`sapf.coinz (p --> r)`

A signal of 1 with probability p, else 0.

### sapf.eprandz

`sapf.eprandz (a b --> r)`

A signal of uniform random integers from a to b, never repeating the previous value.

### sapf.rand2z

`sapf.rand2z (a --> r)`

A signal of uniform random reals from -a to a.

### sapf.irandz

`sapf.irandz (a b --> r)`

A signal of uniform random integers from a to b.

### sapf.irand2z

`sapf.irand2z (a --> r)`

A signal of uniform random integers from -a to a. Its help line says reals; it returns integers.

### sapf.xrandz

`sapf.xrandz (a b --> r)`

A signal of exponentially distributed random reals from a to b.

### sapf.linrandz

`sapf.linrandz (a b --> r)`

A signal of linearly distributed random reals from a to b, weighted towards a.

### sapf.ilinrandz

`sapf.ilinrandz (a b --> r)`

A signal of linearly distributed random integers from a to b, weighted towards a.

### sapf.wrandz

`sapf.wrandz (w --> r)`

A signal of random indices into the weights w. The weights should sum to 1.

### sapf.pickz

`sapf.pickz (a --> r)`

A signal of items chosen at random from the finite list a.

### sapf.wpickz

`sapf.wpickz (a w --> r)`

A signal of items chosen from a with the weights w. w has a's length and should sum to 1.

## finite random signals

### sapf.nrandz

`sapf.nrandz (n a b --> r)`

n uniform random reals from a to b, as a finite signal.

### sapf.ncoinz

`sapf.ncoinz (n p --> r)`

n values, each 1 with probability p, else 0, as a finite signal.

### sapf.neprandz

`sapf.neprandz (n a b --> r)`

n uniform random integers from a to b, never repeating the previous value.

### sapf.nrand2z

`sapf.nrand2z (n a --> r)`

n uniform random reals from -a to a, as a finite signal.

### sapf.nirandz

`sapf.nirandz (n a b --> r)`

n uniform random integers from a to b, as a finite signal.

### sapf.nirand2z

`sapf.nirand2z (n a --> r)`

n uniform random integers from -a to a. Its help line says reals; it returns integers.

### sapf.nxrandz

`sapf.nxrandz (n a b --> r)`

n exponentially distributed random reals from a to b.

### sapf.nlinrandz

`sapf.nlinrandz (n a b --> r)`

n linearly distributed random reals from a to b, weighted towards a.

### sapf.nilinrandz

`sapf.nilinrandz (n a b --> r)`

n linearly distributed random integers from a to b, weighted towards a.

### sapf.nwrandz

`sapf.nwrandz (n w --> r)`

n random indices into the weights w. The weights should sum to 1.

### sapf.npickz

`sapf.npickz (n a --> r)`

n items chosen at random from the finite signal a.

### sapf.nwpickz

`sapf.nwpickz (n a w --> r)`

n items chosen from a with the weights w. w has a's length and should sum to 1.

## SuperCollider UGens

### sapf.sinosc

`sapf.sinosc (freq phase --> out)`

A sine oscillator. Phase is in cycles; it builds `SinOsc` with phase times 2pi.

With SC's arguments: `sc.sinosc` takes freq only; its phase is a keyword, in radians.

### sapf.sinoscfb

`sapf.sinoscfb (freq phase feedback --> out)`

A sine oscillator whose phase is modulated by its own output. Phase must be 0. `SinOscFB`.

With SC's arguments: `sc.sinoscfb` (freq feedback), no phase.

### sapf.sinoscm

`sapf.sinoscm (freq phase mul add --> out)`

A sine oscillator times mul plus add. Phase is in cycles. `SinOsc`.

With SC's arguments: `sc.sinosc`, then `*` and `+`.

### sapf.lfsaw

`sapf.lfsaw (freq phase --> out)`

A sawtooth from -1 to 1, not band limited. Phase is in cycles and constant. `LFSaw`.

With SC's arguments: `sc.lfsaw` takes freq only; its iphase runs 0 to 2.

### sapf.lftri

`sapf.lftri (freq phase --> out)`

A triangle from -1 to 1, not band limited. Phase is in cycles along the rising segment. `LFTri`.

With SC's arguments: `sc.lftri` takes freq only.

### sapf.lfpulse

`sapf.lfpulse (freq phase duty --> out)`

A pulse from 0 to 1, not band limited. Phase is in cycles; duty is the high fraction. `LFPulse`.

With SC's arguments: `sc.lfpulse` (freq iphase width).

### sapf.lfpulseb

`sapf.lfpulseb (freq phase duty --> out)`

A pulse from -1 to 1, not band limited. Phase is in cycles; duty is the high fraction. `LFPulse`.

With SC's arguments: `sc.lfpulse`, then `2 * 1 -`.

### sapf.lfsquare

`sapf.lfsquare (freq phase --> out)`

A square wave from -1 to 1, not band limited. Phase is in cycles. `LFPulse`.

With SC's arguments: `sc.lfpulse` with width .5, then `2 * 1 -`.

### sapf.saw

`sapf.saw (freq phase --> out)`

A sawtooth from -1 to 1, not band limited. Phase is in cycles. `LFSaw`.

With SC's arguments: `sc.saw` is band limited and takes freq only.

### sapf.tri

`sapf.tri (freq phase --> out)`

A triangle from -1 to 1, not band limited. Phase is in cycles. `LFTri`.

With SC's arguments: `sc.lftri`.

### sapf.square

`sapf.square (freq phase --> out)`

A square wave from -1 to 1, not band limited. Phase is in cycles. `LFPulse`.

With SC's arguments: `sc.lfpulse`, then `2 * 1 -`.

### sapf.pulse

`sapf.pulse (freq phase duty --> out)`

A pulse from -duty to 1 - duty, not band limited, so its mean is 0. Phase is in cycles.

With SC's arguments: `sc.pulse` (freq width) is band limited.

### sapf.impulse

`sapf.impulse (freq phase --> out)`

A train of single-frame impulses, not band limited. Phase is in cycles. `Impulse`.

With SC's arguments: `sc.impulse` takes freq only.

### sapf.white

`sapf.white (amp --> out)`

White noise scaled by amp. `WhiteNoise`.

With SC's arguments: `sc.whitenoise` takes no inputs; multiply by amp.

### sapf.pink

`sapf.pink (amp --> out)`

Pink noise scaled by amp. `PinkNoise`.

With SC's arguments: `sc.pinknoise`, times amp.

### sapf.brown

`sapf.brown (amp --> out)`

Brown noise scaled by amp. `BrownNoise`.

With SC's arguments: `sc.brownnoise`, times amp.

### sapf.dust

`sapf.dust (density amp --> out)`

Random impulses from 0 to amp, density per second on average. `Dust`.

With SC's arguments: `sc.dust` (density), times amp.

### sapf.dust2

`sapf.dust2 (density amp --> out)`

Random impulses from -amp to amp, density per second on average. `Dust2`.

With SC's arguments: `sc.dust2` (density), times amp.

### sapf.lfnoise0

`sapf.lfnoise0 (freq --> out)`

Random steps at freq per second. `LFNoise0`.

With SC's arguments: `sc.lfnoise0`, the same input.

### sapf.lfnoise1

`sapf.lfnoise1 (freq --> out)`

Random ramps between values chosen at freq per second. `LFNoise1`.

With SC's arguments: `sc.lfnoise1`, the same input.

### sapf.lfnoise3

`sapf.lfnoise3 (freq --> out)`

Cubic interpolated random values at freq per second. `LFDNoise3`.

With SC's arguments: `sc.lfdnoise3`, the same input.

### sapf.lag

`sapf.lag (in decayTime --> out)`

A one-pole lag: the output follows the input with the given 60 dB time. `Lag`.

With SC's arguments: `sc.lag` (in lagTime).

### sapf.lag2

`sapf.lag2 (in decayTime --> out)`

Two one-pole lags in series. `Lag2`.

With SC's arguments: `sc.lag2` (in lagTime).

### sapf.lagud

`sapf.lagud (in upDecayTime downDecayTime --> out)`

A one-pole lag with separate rising and falling times. `LagUD`.

With SC's arguments: `sc.lagud` (in lagTimeU lagTimeD).

### sapf.lpf

`sapf.lpf (in freq --> out)`

A 12 dB per octave low pass. `LPF`.

With SC's arguments: `sc.lpf`, the same inputs.

### sapf.hpf

`sapf.hpf (in freq --> out)`

A 12 dB per octave high pass. `HPF`.

With SC's arguments: `sc.hpf`, the same inputs.

### sapf.lpf1

`sapf.lpf1 (in freq --> out)`

A 6 dB per octave low pass, built on `FOS`.

With SC's arguments: `sc.onepole` or `sc.fos` with computed coefficients.

### sapf.hpf1

`sapf.hpf1 (in freq --> out)`

A 6 dB per octave high pass, built on `FOS`.

With SC's arguments: `sc.fos` with computed coefficients.

### sapf.lpf2

`sapf.lpf2 (in freq --> out)`

A 24 dB per octave low pass, built on `SOS`.

With SC's arguments: `sc.sos` with computed coefficients, or two `sc.lpf`.

### sapf.hpf2

`sapf.hpf2 (in freq --> out)`

A 24 dB per octave high pass, built on `SOS`.

With SC's arguments: `sc.sos` with computed coefficients, or two `sc.hpf`.

### sapf.rlpf2

`sapf.rlpf2 (in freq rq --> out)`

A 24 dB per octave resonant low pass; rq is 1/Q. Built on `SOS`.

With SC's arguments: `sc.sos` with computed coefficients.

### sapf.rlpf

`sapf.rlpf (in freq rq --> out)`

A resonant low pass; rq is 1/Q. `RLPF`.

With SC's arguments: `sc.rlpf`, the same inputs.

### sapf.rhpf

`sapf.rhpf (in freq rq --> out)`

A resonant high pass; rq is 1/Q. `RHPF`.

With SC's arguments: `sc.rhpf`, the same inputs.

### sapf.bpf

`sapf.bpf (in freq bw --> out)`

A band pass with bandwidth bw in octaves. It builds `BPF` with rq `2^(bw/2) - 2^(-bw/2)`.

With SC's arguments: `sc.bpf` takes rq, not octaves.

### sapf.bsf

`sapf.bsf (in freq bw --> out)`

A band stop with bandwidth bw in octaves, converted to rq. `BRF`.

With SC's arguments: `sc.brf` takes rq.

### sapf.resonz

`sapf.resonz (in freq rq --> out)`

A two-pole resonant filter; rq is bandwidth over centre frequency. `Resonz`.

With SC's arguments: `sc.resonz` (in freq bwr).

### sapf.ringz

`sapf.ringz (in freq ringTime --> out)`

A resonant filter set by its 60 dB ring time in seconds. `Ringz`.

With SC's arguments: `sc.ringz` (in freq decaytime).

### sapf.formlet

`sapf.formlet (in freq atkTime dcyTime --> out)`

A formant filter with attack and decay times, as `Formlet`.

With SC's arguments: `sc.formlet` (in freq attacktime decaytime).

### sapf.klank

`sapf.klank (in freqs amps ringTimes --> out)`

A bank of `Ringz` filters; freqs, amps and ringTimes are lists.

With SC's arguments: `sc.klank` takes a specification array, then the input.

### sapf.leakdc

`sapf.leakdc (in coef --> out)`

Removes DC with a one-pole high pass; coef sets the pole. `LeakDC`.

With SC's arguments: `sc.leakdc`, the same inputs.

### sapf.decay

`sapf.decay (in decayTime --> out)`

An exponential decay for each impulse at the input. `Decay`.

With SC's arguments: `sc.decay`, the same inputs.

### sapf.decay2

`sapf.decay2 (in atkTime dcyTime --> out)`

An exponential attack and decay for each impulse at the input. `Decay2`.

With SC's arguments: `sc.decay2`, the same inputs.

### sapf.delayn

`sapf.delayn (in delay maxdelay --> out)`

A delay line without interpolation. Delay comes before maxdelay. `DelayN`.

With SC's arguments: `sc.delayn` (in maxdelaytime delaytime): the two times swap places.

### sapf.delayl

`sapf.delayl (in delay maxdelay --> out)`

A delay line with linear interpolation. Delay comes before maxdelay. `DelayL`.

With SC's arguments: `sc.delayl` (in maxdelaytime delaytime).

### sapf.delayc

`sapf.delayc (in delay maxdelay --> out)`

A delay line with cubic interpolation. Delay comes before maxdelay. `DelayC`.

With SC's arguments: `sc.delayc` (in maxdelaytime delaytime).

### sapf.combn

`sapf.combn (in delay maxdelay decayTime --> out)`

A comb filter without interpolation; decayTime is the 60 dB time. `CombN`.

With SC's arguments: `sc.combn` (in maxdelaytime delaytime decaytime): the delay times swap.

### sapf.combl

`sapf.combl (in delay maxdelay decayTime --> out)`

A comb filter with linear interpolation. `CombL`.

With SC's arguments: `sc.combl` (in maxdelaytime delaytime decaytime).

### sapf.combc

`sapf.combc (in delay maxdelay decayTime --> out)`

A comb filter with cubic interpolation. `CombC`.

With SC's arguments: `sc.combc` (in maxdelaytime delaytime decaytime).

### sapf.alpasn

`sapf.alpasn (in delay maxdelay decayTime --> out)`

An all pass delay without interpolation. `AllpassN`.

With SC's arguments: `sc.allpassn` (in maxdelaytime delaytime decaytime).

### sapf.alpasl

`sapf.alpasl (in delay maxdelay decayTime --> out)`

An all pass delay with linear interpolation. `AllpassL`.

With SC's arguments: `sc.allpassl` (in maxdelaytime delaytime decaytime).

### sapf.alpasc

`sapf.alpasc (in delay maxdelay decayTime --> out)`

An all pass delay with cubic interpolation. `AllpassC`.

With SC's arguments: `sc.allpassc` (in maxdelaytime delaytime decaytime).

### sapf.itd

`sapf.itd (in pan maxdelay --> [left right])`

Pans a mono input to two channels by interaural time delay, up to maxdelay. Built on `DelayC`.

### sapf.flange

`sapf.flange (in delay maxdelay --> out)`

A flanger: the input minus a cubic-interpolated delayed copy. delay may be negative; latency is maxdelay.

### sapf.flangep

`sapf.flangep (in delay maxdelay --> out)`

As `sapf.flange`, but it adds the delayed copy instead of subtracting it.

### sapf.pan2

`sapf.pan2 (in pos --> [left right])`

Pans a mono input across two channels; -1 is left, 1 right. `Pan2`.

With SC's arguments: `sc.pan2` (in pos).

### sapf.bal2

`sapf.bal2 (left right pos --> [left right])`

Balances a stereo pair; -1 keeps the left, 1 the right. `Balance2`.

With SC's arguments: `sc.balance2` (left right pos).

### sapf.rot2

`sapf.rot2 (left right pos --> [left right])`

Rotates a stereo pair; 1 and -1 are 180 degrees. `Rotate2`.

With SC's arguments: `sc.rotate2` (x y pos).

### sapf.fade2

`sapf.fade2 (left right pos --> out)`

An equal power crossfade; -1 is all left, 1 all right. `XFade2`.

With SC's arguments: `sc.xfade2` (inA inB pan).

### sapf.sah

`sapf.sah (in trigger --> out)`

Sample and hold: holds the input at each trigger. `Latch`.

With SC's arguments: `sc.latch` (in trig).

### sapf.line

`sapf.line (dur start end --> out)`

A linear ramp from start to end over dur seconds; then the synth ends. Duration comes first. `Line`.

With SC's arguments: `sc.line` (start end dur); add `.done` to end the synth.

### sapf.xline

`sapf.xline (dur start end --> out)`

An exponential ramp from start to end over dur seconds; then the synth ends. `XLine`.

With SC's arguments: `sc.xline` (start end dur); add `.done` to end the synth.

### sapf.adsr

`sapf.adsr ([attack decay sustain release] amp dur tempo --> envelope)`

An attack, decay, sustain, release envelope scaled by amp and held for dur. tempo must be a real. `EnvGen`.

In stac: `env.adsr` and `envgen`.

### sapf.fadein

`sapf.fadein (in fadeTime --> out)`

Fades the input in over fadeTime. `EnvGen`.

### sapf.fadeout

`sapf.fadeout (in sustainTime fadeTime --> out)`

Holds the input for sustainTime, fades it out over fadeTime, then ends the synth. `EnvGen`.

### sapf.seq

`sapf.seq (in trigger --> out)`

Takes the next value from a stream at each trigger and holds it. The values go into a `Dseq` read by `Demand`.

In stac: `sc.dseq` and `sc.demand`.

### sapf.iseq

`sapf.iseq (in trigger --> out)`

Takes the next value from a stream at each trigger and outputs it for one frame, 0 between. `Demand`.

### sapf.steps

`sapf.steps (values durs rate --> out)`

Outputs each value for its duration, durations divided by rate. `Duty`.

In stac: `sc.duty`.

### sapf.imps

`sapf.imps (values durs rate --> out)`

Outputs each value as a one-frame impulse at the start of its duration. `TDuty`.

In stac: `sc.tduty`.

### sapf.lines

`sapf.lines (values durs rate --> out)`

Linear segments between the values, each over its duration. `DemandEnvGen`.

### sapf.xlines

`sapf.xlines (values durs rate --> out)`

Exponential segments between the values, each over its duration. `DemandEnvGen`.

### sapf.curves

`sapf.curves (values curvatures durs rate --> out)`

Curved segments between the values, with a curvature per segment. `DemandEnvGen`.

### sapf.soundin

`sapf.soundin (channel --> out)`

A hardware input channel, from 0. `In` on the first input bus.

In stac: `sc.in` on bus `sc.numoutputbuses` plus the channel, as SC's `SoundIn` does.

### sapf.kr

`sapf.kr (in --> out)`

A control-rate version of its input. UGens that feed only control-rate UGens drop to control rate too.

In stac: the `.kr` method, which lowers one UGen and nothing else.

### sapf.ctl

`sapf.ctl (default name --> out)`

A named control-rate synth parameter with a default. `Control`.

In stac: a `synthdef` parameter.

### sapf.ugen

`sapf.ugen (inputs name rate numOutputs --> out)`

Builds any SC UGen by name from a list of inputs, a rate ('ar 'kr or 'ir) and an output count.

In stac: the `sc` words, which know each UGen's arguments.

### sapf.>sf

`sapf.>sf (channels filename -->)`

Renders channels to filename.wav until the first channel ends, with scsynth in non-real-time mode.

In stac: `render`, which takes an explicit duration.

### sapf.>sfo

`sapf.>sfo (channels filename -->)`

As `sapf.>sf`, then opens the file in the default application.

### sapf.>nrt

`sapf.>nrt (channels seconds name -->)`

Writes an NRT score of channels and their `ola` voices to name.osc.

In stac: `score`.

### sapf.>sd

`sapf.>sd (channels name -->)`

Writes channels as SynthDef name to name.scsyndef.

In stac: `synthdef`, then `.write`.

## play

### sapf.play

`sapf.play (channels -->)`

Plays channels and leaves nothing, as sapf's `play` does. An event stream plays as the core `play` plays it.

The core `play` leaves the synth's `Node`.

## clock

### sapf.ola

`sapf.ola (sounds hops rate numChannels --> out)`

Overlap add: each sound is a synth on a private bus, one per hop; the result reads that bus. `play` and `>nrt` start the voices.

In stac: `pbind` with `:out` and `bus.audio`; see docs/dev/stac-phase5.md.

## wave tables

### sapf.wavefill

`sapf.wavefill (amps phases smooth --> wavetable)`

Builds a set of 1/3 octave wavetables from partial amplitudes and phases. smooth reduces the Gibbs effect; 0 is none.

### sapf.osc

`sapf.osc (freq phase wavetable --> out)`

A band limited wavetable oscillator over a table from `wavefill`. `VOsc` over scsynth buffers.

With SC's arguments: `sc.osc` (bufnum freq) reads one buffer.

### sapf.parTbl

`sapf.parTbl (--> wavetable)`

A parabolic wavetable for `sapf.osc`.

### sapf.triTbl

`sapf.triTbl (--> wavetable)`

A triangle wavetable for `sapf.osc`.

### sapf.sqrTbl

`sapf.sqrTbl (--> wavetable)`

A square wavetable for `sapf.osc`.

### sapf.sawTbl

`sapf.sawTbl (--> wavetable)`

A sawtooth wavetable for `sapf.osc`.

### sapf.sf>

`sapf.sf> (filename --> channels)`

A WAV or AIFF file's channels, played once at rate 1 from an scsynth buffer. `PlayBuf`.

In stac: `buffer.read`, then `sc.playbuf`.

### sapf.playbuf

`sapf.playbuf (filename rate loop --> channels)`

A WAV or AIFF file's channels at rate, looped when loop is not 0. `PlayBuf`.

With SC's arguments: `sc.playbuf` takes numChannels and a buffer, not a file name.

## unipolar random number generators

### urandz

`urandz (--> r)`

A signal of uniform random numbers from 0 to 1.

### nurandz

`nurandz (n --> r)`

n uniform random numbers from 0 to 1, as a finite signal.

## bipolar random number generators

### brandz

`brandz (--> r)`

A signal of uniform random numbers from -1 to 1.

### nbrandz

`nbrandz (n --> r)`

n uniform random numbers from -1 to 1, as a finite signal.

## random walks

### iwalkz

`iwalkz (step lo hi --> signal)`

An integer random walk with the given step, kept between lo and hi.

### iwalk2z

`iwalk2z (step bounds --> signal)`

An integer random walk with the given step, kept between -bounds and bounds.

### walkz

`walkz (step lo hi --> signal)`

A random walk with the given step, kept between lo and hi.

### walk2z

`walk2z (step bounds --> signal)`

A random walk with the given step, kept between -bounds and bounds.

## composite ugens

### oltx

`oltx (sounds susTime xfadeTime density numChannels --> out)`

A texture of overlapping sounds, after SC2's `OverlapTexture`. Each sound fades in and out over xfadeTime around susTime; density sets the overlap. Built on `sapf.ola`.

### xftx

`xftx (sounds susTime xfadeTime numChannels --> out)`

A texture of crossfaded sounds, after SC2's `XFadeTexture`. Built on `sapf.ola`.

### lfo

`lfo (freq phase lo hi --> out)`

A sine LFO from lo to hi, linear in between. Phase is in cycles.

### xlfo

`xlfo (freq phase lo hi --> out)`

A sine LFO from lo to hi, exponential in between. Phase is in cycles.

### splay

`splay (x --> out)`

Spreads a list of mono sources evenly across the stereo field.

### rsplay

`rsplay (x --> out)`

Spreads a list of mono sources across the stereo field by rotation.

### splayd

`splayd (x maxdelay --> out)`

Spreads a list of mono sources across the stereo field by interaural time delay, up to maxdelay.

## simple allpass reverbs

### apverb4

`apverb4 (in delayTime decayTime --> out)`

A four-stage stereo all pass reverb with random delay times.

### apverb6

`apverb6 (in delayTime decayTime --> out)`

A six-stage stereo all pass reverb with random delay times.

## random sweeps and pans

### rline

`rline (dur lo hi --> out)`

A linear line over dur whose two ends are uniform random values between lo and hi.

### xrline

`xrline (dur lo hi --> out)`

A linear line over dur whose two ends are exponential random values between lo and hi.

### rxline

`rxline (dur lo hi --> out)`

An exponential line over dur whose two ends are uniform random values between lo and hi.

### xrxline

`xrxline (dur lo hi --> out)`

An exponential line over dur whose two ends are exponential random values between lo and hi.

### rpanl2

`rpanl2 (in --> out)`

Pans a mono input to a fixed random stereo position.

### rpan2

`rpan2 (in --> out)`

Pans a mono input to a fixed random stereo position by interaural time delay.

### rbal2

`rbal2 (a b --> out)`

Balances a stereo pair at a fixed random position.

### rrot2

`rrot2 (in --> out)`

Rotates a stereo pair by a fixed random amount.

### rsweeppan2

`rsweeppan2 (in dur --> out)`

Pans a mono input along a random sweep over dur.

## crossover filters

### crossover

`crossover (in freq --> high_band low_band)`

Linkwitz-Riley crossover: splits the input at freq into high and low bands that sum flat.

### crossover1

`crossover1 (in freq --> high_band low_band)`

First-order crossover: splits the input at freq into high and low bands that sum flat.

### midcrossover

`midcrossover (in freq bw --> outer_band inner_band)`

Splits the input into a band of width bw around freq and the rest, which sum flat.

### crossovermix

`crossovermix (a b freq --> out)`

Mixes a and b through a Linkwitz-Riley crossover at freq: the low band of one with the high band of the other.

### crossover1mix

`crossover1mix (a b freq --> out)`

Mixes a and b through a first-order crossover at freq.

### midcrossovermix

`midcrossovermix (a b freq bw --> out)`

Mixes a and b through a middle crossover of width bw around freq.

## delays with wet/dry mix

### combnw

`combnw (in delay maxdelay decay wet --> out)`

`sapf.combn` with a wet and dry mix; wet is the delayed fraction.

### comblw

`comblw (in delay maxdelay decay wet --> out)`

`sapf.combl` with a wet and dry mix; wet is the delayed fraction.

### combcw

`combcw (in delay maxdelay decay wet --> out)`

`sapf.combc` with a wet and dry mix; wet is the delayed fraction.

## synths

### blah

`blah (pch dyn dur pan -> outs)`

A synth voice: saw, filter, amplitude envelope. pch is a pitch, dyn a dynamic, pan a stereo position.

### lfblah

`lfblah (pch dyn dur pan -> outs)`

As `blah`, with a non band limited `sapf.lfsaw`.

### saw-syn

`saw-syn (freq db dur pan harms adsr --> out)`

A sawtooth synth voice: freq in Hz, db peak level, dur seconds, pan, harms setting the filter cutoff, adsr the envelope list.

### square-syn

`square-syn (freq db dur pan harms adsr --> out)`

A square wave synth voice, with the arguments of `saw-syn`.

### pulse-syn

`pulse-syn (freq duty db dur pan harms adsr --> out)`

A variable width pulse synth voice; duty is the pulse width. Otherwise as `saw-syn`.

### osc-syn

`osc-syn (freq db dur pan harms adsr wt --> out)`

A wavetable synth voice over the table wt. Otherwise as `saw-syn`.

### saw-rsyn

`saw-rsyn (freq db dur pan harms rq adsr --> out)`

As `saw-syn`, through a resonant low pass with rq 1/Q.

### square-rsyn

`square-rsyn (freq db dur pan harms rq adsr --> out)`

As `square-syn`, through a resonant low pass with rq 1/Q.

### fm-syn

`fm-syn (freq c m mx fb db dur pan adsr --> out)`

An FM synth voice: c and m multiply freq for carrier and modulator, mx is the index in cycles, fb the modulator's self feedback.

### fm-syn3

`fm-syn3 (freq c m mx fb db dur pan adsr --> out)`

As `fm-syn`, with three detuned carriers and one modulator.

## events

### ev-base

`ev-base`

A Form of the basic low-level fields of a music event, from sapf's event model.

In stac: `pbind`'s events follow SC's default event instead.

## event list operations

### sapf.evmerge

`sapf.evmerge (a b t --> c)`

Merges event list b, delayed by t seconds, into event list a, in time order by the events' `:dt`, the time to the next event. An event without `:dt` is an error.

In stac: `pbind` streams, whose events use `:dur`; `$` joins two of them in sequence.

### sapf.evdelay

`sapf.evdelay (a t --> c)`

Delays an event list by t seconds by putting a rest event in front.

In stac: `pbind` streams, whose events use `:dur`; `$` joins two of them in sequence.

### sapf.evrest

`sapf.evrest (t --> c)`

Pushes a rest event lasting t seconds: `{:out 0 :dur t :dt t}`.

In stac: `'rest` in any field of a `pbind` event.
