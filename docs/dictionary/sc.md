# sc

SuperCollider's UGens, in the namespace sc; prelude.stac imports them.

## uncategorised

### sc.a2k

`sc.a2k (in --> out)`

Audio to control rate converter (SC help).

Use it to feed an audio signal to a control-rate input without aliasing the control. `.kr` on most UGens does the same at the source.

```stac
[440 sinosc a2k 1 + 220 * sinosc .1 *] play
```

### sc.apf

`sc.apf (in freq radius --> out)`

A two-pole all-pass filter at freq with pole radius radius, from 0 to 1.

Use it to shift phase near freq without changing the level, as in phaser stages.

```stac
[whitenoise 1000 .9 apf .1 *] play
```

### sc.allpassc

`sc.allpassc (in maxdelaytime delaytime decaytime --> out)`

Schroeder allpass delay line with cubic interpolation (SC help).

Use it for diffusion in reverbs and for chorus-like smearing; cubic interpolation keeps a moving delaytime smooth.

```stac
[2 dust .5 * .2 .05 1 allpassc] play
```

### sc.allpassl

`sc.allpassl (in maxdelaytime delaytime decaytime --> out)`

Schroeder allpass delay line with linear interpolation (SC help).

Use it where allpassc costs too much; linear interpolation is cheaper and slightly duller when delaytime moves.

```stac
[2 dust .5 * .2 .05 1 allpassl] play
```

### sc.allpassn

`sc.allpassn (in maxdelaytime delaytime decaytime --> out)`

Schroeder allpass delay line with no interpolation (SC help).

Use it with a fixed delaytime; with no interpolation it is the cheapest of the three.

```stac
[2 dust .5 * .2 .05 1 allpassn] play
```

### sc.bpf

`sc.bpf (in freq rq --> out)`

2nd order Butterworth bandpass filter (SC help).

Use it to keep one band of a noisy source; rq is the bandwidth divided by freq, so .1 is narrow. resonz rings more for the same rq.

```stac
[whitenoise 1000 .1 bpf] play
```

### sc.bpz2

`sc.bpz2 (in --> out)`

Two zero fixed midpass (SC help).

Use it as a fixed, cheap band-pass at a quarter of the sample rate, e.g. to thin a noise source.

```stac
[whitenoise bpz2 .1 *] play
```

### sc.brf

`sc.brf (in freq rq --> out)`

2nd order Butterworth band reject filter (SC help).

Use it to notch out one band, such as a hum or a whistle; rq sets the notch width over freq.

```stac
[whitenoise 1000 .3 brf .1 *] play
```

### sc.brz2

`sc.brz2 (in --> out)`

Two zero fixed midcut (SC help).

Use it as a fixed, cheap notch at a quarter of the sample rate.

```stac
[whitenoise brz2 .1 *] play
```

### sc.balance2

`sc.balance2 (left right pos --> out)`

Stereo signal balancer (SC help).

Use it to move a stereo pair left or right without collapsing it to mono, as pan2 would. pos runs from -1 to 1.

```stac
[300 saw 301 saw .5 lfnoise1.kr balance2 .1 *] play
```

### sc.ball

`sc.ball (in g damp friction --> out)`

Physical model of a bouncing object (SC help).

Use it to make a bouncing value from an input height; g is gravity, damp and friction slow it down.

```stac
[.5 1 0 .01 ball 400 * 500 + sinosc .1 *] play
```

### sc.blip

`sc.blip (freq numharm --> out)`

Band limited impulse oscillator (SC help).

Use it for a buzzy, band-limited pulse train; numharm sets the number of harmonics, so lower values sound darker.

```stac
[110 20 blip .1 *] play
```

### sc.brownnoise

`sc.brownnoise ( --> out)`

Brown Noise (SC help).

Use it for a dark, rumbling noise; its spectrum falls 6 dB per octave, against pinknoise's 3 dB.

```stac
[brownnoise .1 *] play
```

### sc.cosc

`sc.cosc (bufnum freq beats --> out)`

Chorusing wavetable oscillator (SC help).

Use it for a thick, beating wavetable tone; beats sets the beat frequency between its two copies. The buffer must hold a wavetable.

```stac
1024 1 buffer.alloc = b
[b 200 .7 cosc .1 *] play
```

### sc.clip

`sc.clip (in lo hi --> out)`

Clip a signal outside given thresholds (SC help).

Use it to hard-limit a signal between lo and hi. The core word clip also works on reals; sc.clip always builds the UGen.

```stac
[440 sinosc 2 * -.5 .5 sc.clip .1 *] play
```

### sc.clipnoise

`sc.clipnoise ( --> out)`

Clip Noise (SC help).

Use it for the loudest noise at a given peak: every sample is -1 or 1.

```stac
[clipnoise .05 *] play
```

### sc.coingate

`sc.coingate (prob in --> out)`

Statistical gate (SC help).

Use it to pass each trigger with probability prob, for sparse random rhythms.

```stac
[.005 .1 env.perc .5 8 impulse.kr coingate.kr envgen.kr 800 sinosc * .2 *] play
```

### sc.combc

`sc.combc (in maxdelaytime delaytime decaytime --> out)`

Comb delay line with cubic interpolation (SC help).

Use it for echoes and pitched resonance; a short delaytime and a long decaytime ring at 1/delaytime Hz. Cubic interpolation suits a moving delay.

```stac
[.5 dust .1 * .2 .2 2 combc] play
```

### sc.combl

`sc.combl (in maxdelaytime delaytime decaytime --> out)`

Comb delay line with linear interpolation (SC help).

Use it where combc costs too much; linear interpolation is cheaper.

```stac
[.5 dust .1 * .2 .2 2 combl] play
```

### sc.combn

`sc.combn (in maxdelaytime delaytime decaytime --> out)`

Comb delay line with no interpolation (SC help).

Use it with a fixed delaytime for the cheapest comb.

```stac
[.5 dust .1 * .01 .005 1 combn] play
```

### sc.compander

`sc.compander (in control thresh slopeBelow slopeAbove clampTime relaxTime --> out)`

Compressor, expander, limiter, gate and ducker (SC help).

Use it to compress, expand, limit or gate a signal by the level of control, which may be the signal itself; with a different control it ducks.

```stac
[whitenoise .3 * = n  n n .1 1 .5 .01 .1 compander] play
```

### sc.crackle

`sc.crackle (chaosParam --> out)`

Chaotic noise function (SC help).

Use it for crackling noise; chaosParam near 1.9 gives sparse crackles, near 2.0 a denser hiss.

```stac
[1.95 crackle .1 *] play
```

### sc.dc

`sc.dc (in --> out)`

Create a constant amplitude signal (SC help).

Use it to output a constant as a UGen, e.g. to fill a channel or start a feedback loop.

```stac
[.1 dc 440 sinosc *] play
```

### sc.decay

`sc.decay (in decayTime --> out)`

Exponential decay (SC help).

Use it to turn triggers into percussive envelopes: each impulse decays by 60 dB over decayTime.

```stac
[4 impulse .2 decay whitenoise * .2 *] play
```

### sc.decay2

`sc.decay2 (in attackTime decayTime --> out)`

Exponential decay (SC help).

Use it where decay clicks: attackTime rounds the start of each envelope.

```stac
[4 impulse .01 .2 decay2 1000 sinosc * .2 *] play
```

### sc.degreetokey

`sc.degreetokey (bufnum in octave --> out)`

Convert signal to modal pitch (SC help).

Use it to quantise a signal to a scale held in a buffer, so an LFO plays notes of the scale. octave is the number of steps per octave.

```stac
[0 2 4 5 7 9 11] buffer.load = scale
[scale 2 lfnoise0.kr 7 * 7 + 12 degreetokey 48 + nnhz saw .05 *] play
```

### sc.delay1

`sc.delay1 (in x1 --> out)`

Single sample delay (SC help).

Use it for one-sample differences and filters built by hand; x1 is the value before the first sample.

```stac
[whitenoise aa 0 delay1 - .1 *] play
```

### sc.delay2

`sc.delay2 (in x1 x2 --> out)`

Two sample delay (SC help).

Use it like delay1 when a filter needs the sample two back.

```stac
[whitenoise aa 0 0 delay2 + .1 *] play
```

### sc.delayc

`sc.delayc (in maxdelaytime delaytime --> out)`

Simple delay line with cubic interpolation (SC help).

Use it for a delay that moves, such as chorus or flanging; maxdelaytime fixes its memory.

```stac
[300 saw .1 * aa .02 .01 .005 .5 lfnoise1.kr * + delayc +] play
```

### sc.delayl

`sc.delayl (in maxdelaytime delaytime --> out)`

Simple delay line with linear interpolation (SC help).

Use it where delayc costs too much; linear interpolation is cheaper.

```stac
[300 saw .1 * aa .02 .01 delayl +] play
```

### sc.delayn

`sc.delayn (in maxdelaytime delaytime --> out)`

Simple delay line with no interpolation (SC help).

Use it for a fixed delay at the lowest cost, such as a slapback.

```stac
[2 impulse .1 decay whitenoise * aa .2 .15 delayn + .2 *] play
```

### sc.dust

`sc.dust (density --> out)`

Random impulses (SC help).

Use it for random positive impulses at density per second, as a trigger source or a crackle.

```stac
[20 dust .5 *] play
```

### sc.dust2

`sc.dust2 (density --> out)`

Random impulses (SC help).

Use it like dust when the impulses should be bipolar, from -1 to 1.

```stac
[20 dust2 .5 *] play
```

### sc.envgen

`sc.envgen (envelope gate --> out)`

Envelope generator (SC help).

Use it to play an Env from env; gate holds a sustaining envelope open, and .done frees the synth at its end.

```stac
[.01 .3 env.perc 1 envgen.kr.done 600 sinosc * .2 *] play
```

### sc.fos

`sc.fos (in a0 a1 b1 --> out)`

First order filter section (SC help).

Use it to build a first-order filter from its coefficients: out = a0*in + a1*in[-1] + b1*out[-1].

```stac
[whitenoise .5 .5 0 fos .1 *] play
```

### sc.fsinosc

`sc.fsinosc (freq --> out)`

Fast sine oscillator (SC help).

Use it for a cheaper sine when freq does not change much; it can drift at high frequencies, where sinosc stays exact.

```stac
[440 fsinosc .1 *] play
```

### sc.fold

`sc.fold (in lo hi --> out)`

Fold a signal outside given thresholds (SC help).

Use it to fold a signal back into lo..hi, a wavefolder distortion. The core word fold also works on reals; sc.fold always builds the UGen.

```stac
[220 sinosc 3 * -1 1 sc.fold .1 *] play
```

### sc.formant

`sc.formant (fundfreq formfreq bwfreq --> out)`

Formant oscillator (SC help).

Use it for vowel-like tones: fundfreq is the pitch, formfreq the formant centre, bwfreq its width.

```stac
[110 800 200 formant .1 *] play
```

### sc.formlet

`sc.formlet (in freq attacktime decaytime --> out)`

FOF-like filter (SC help).

Use it to turn impulses into grains that ring at freq, as in FOF synthesis.

```stac
[10 impulse 1000 .01 .1 formlet .2 *] play
```

### sc.freeverb

`sc.freeverb (in mix room damp --> out)`

A reverb (SC help).

Use it for a cheap mono reverb; mix, room and damp each run from 0 to 1.

```stac
[.5 dust .5 * .5 .8 .3 freeverb] play
```

### sc.freeverb2

`sc.freeverb2 (in in2 mix room damp --> out)`

A two-channel reverb (SC help).

Use it like freeverb on a stereo pair; it leaves two channels.

```stac
[1 dust .5 * aa .5 .8 .3 freeverb2] play
```

### sc.freqshift

`sc.freqshift (in freq --> out)`

Frequency Shifter (SC help).

Use it to shift every partial by freq Hz, which breaks harmonic ratios, unlike a pitch shift.

```stac
[220 saw .1 * 50 freqshift] play
```

### sc.gverb

`sc.gverb (in roomsize revtime damping inputbw spread drylevel earlyreflevel taillevel maxroomsize --> out)`

A two-channel reverb (SC help).

Use it for a large, dense stereo reverb; roomsize is in metres and revtime in seconds. It costs more than freeverb.

```stac
[.5 dust .3 * 30 3 .5 .5 15 1 .7 .5 300 gverb] play
```

### sc.gate

`sc.gate (in trig --> out)`

Gate or hold (SC help).

Use it to hold the last value of in while trig is not positive.

```stac
[440 sinosc 1 0 .5 lfpulse.kr gate .1 *] play
```

### sc.gendy1

`sc.gendy1 (ampdist durdist adparam ddparam minfreq maxfreq ampscale durscale initCPs knum --> out)`

Dynamic stochastic synthesis generator (SC help).

Use it for Xenakis-style stochastic waveforms that drift between minfreq and maxfreq.

```stac
[1 1 1 1 440 660 .5 .5 12 12 gendy1 .05 *] play
```

### sc.gendy2

`sc.gendy2 (ampdist durdist adparam ddparam minfreq maxfreq ampscale durscale initCPs knum a c --> out)`

Dynamic stochastic synthesis generator (SC help).

Use it like gendy1 with a Lehmer random generator; a and c are its parameters.

```stac
[1 1 1 1 440 660 .5 .5 12 12 1.17 .31 gendy2 .05 *] play
```

### sc.gendy3

`sc.gendy3 (ampdist durdist adparam ddparam freq ampscale durscale initCPs knum --> out)`

Dynamic stochastic synthesis generator (SC help).

Use it when the stochastic waveform should hold a pitch: freq fixes the period.

```stac
[1 1 1 1 440 .5 .5 12 12 gendy3 .05 *] play
```

### sc.graynoise

`sc.graynoise ( --> out)`

Bit-flipping Noise (SC help).

Use it for a noise with a raspy, digital colour.

```stac
[graynoise .05 *] play
```

### sc.hpf

`sc.hpf (in freq --> out)`

2nd order Butterworth highpass filter (SC help).

Use it to remove low frequencies below freq, 12 dB per octave; rhpf adds a resonant peak.

```stac
[whitenoise 2000 hpf .1 *] play
```

### sc.hpz1

`sc.hpz1 (in --> out)`

Two point difference filter (SC help).

Use it as a cheap, gentle high-pass: half the difference of each sample and the one before.

```stac
[whitenoise hpz1 .1 *] play
```

### sc.hpz2

`sc.hpz2 (in --> out)`

Two zero fixed midcut (SC help).

Use it as a cheap two-zero high-pass, steeper than hpz1.

```stac
[whitenoise hpz2 .1 *] play
```

### sc.hasher

`sc.hasher (in --> out)`

Scrambled value with a hash function (SC help).

Use it to turn any value into a repeatable pseudo-random value, e.g. to map a counter to noise.

```stac
[4 impulse.kr pulsecount.kr hasher.kr 400 * 600 + sinosc .1 *] play
```

### sc.hilbert

`sc.hilbert (in --> out)`

Applies the Hilbert transform to an input signal (SC help).

Use it to make a quadrature pair for frequency shifting or single-sideband modulation; it leaves 2 channels, real and imaginary.

```stac
[220 sinosc hilbert .1 *] play
```

### sc.ienvgen

`sc.ienvgen (envelope index --> out)`

Envelope generator for polling values from an Env (SC help).

Use it to read an Env at a position in seconds, so a signal can scrub through it.

```stac
[[0 1 .3 0] [.1 .5 1] 'lin env.new .1 lfsaw.kr 1 + 1.6 * ienvgen 400 * 200 + sinosc .1 *] play
```

### sc.impulse

`sc.impulse (freq --> out)`

Impulse oscillator (SC help).

Use it for single-sample clicks at freq per second, as a clock for triggers.

```stac
[4 impulse .1 decay 800 sinosc * .2 *] play
```

### sc.in

`sc.in (bus numChannels --> out)`

Read a signal from a bus (SC help).

Use it to read numChannels from a bus, e.g. an effect reading a private bus that other synths write to.

```stac
1 bus.audio = b
[b 1 in .5 *] play
```

### sc.inrange

`sc.inrange (in lo hi --> out)`

Tests if a signal is within a given range (SC help).

Use it to get 1 where in lies within lo..hi and 0 elsewhere. std's inrange hides this UGen after the prelude, so write sc.inrange.

```stac
[.5 lfnoise1.kr -.2 .2 sc.inrange 440 sinosc * .1 *] play
```

### sc.integrator

`sc.integrator (in coef --> out)`

A leaky integrator (SC help).

Use it to accumulate a signal; coef below 1 makes it leak back to 0.

```stac
[4 impulse .99 integrator .1 *] play
```

### sc.k2a

`sc.k2a (in --> out)`

Control to audio rate converter (SC help).

Use it to raise a control signal to audio rate by linear interpolation.

```stac
[.5 lfnoise1.kr k2a 440 sinosc * .1 *] play
```

### sc.klang

`sc.klang (specificationsArrayRef --> out)`

Sine oscillator bank (SC help).

Use it for a fixed bank of sine partials: a list of freqs, amplitudes and phases in one argument.

```stac
[[[440 660 880] [.1 .05 .03]] klang] play
```

### sc.klank

`sc.klank (specificationsArrayRef input --> out)`

Bank of resonators (SC help).

Use it to excite a bank of resonators: a list of freqs, amplitudes and ring times, then the input.

```stac
[[[800 1071 1353] [1 1 1] [1 1 1]] 2 impulse .1 * klank] play
```

### sc.lfclipnoise

`sc.lfclipnoise (freq --> out)`

Clipped noise (SC help).

Use it for a random square wave that jumps between -1 and 1 at about freq per second.

```stac
[4 lfclipnoise.kr 200 * 400 + sinosc .1 *] play
```

### sc.lfcub

`sc.lfcub (freq --> out)`

A sine like shape made of two cubic pieces (SC help).

Use it as a cheap LFO close to a sine, made of two cubic pieces.

```stac
[1 lfcub.kr 100 * 400 + sinosc .1 *] play
```

### sc.lfdclipnoise

`sc.lfdclipnoise (freq --> out)`

Dynamic clipped noise (SC help).

Use it like lfclipnoise when freq itself moves quickly; the dynamic version follows it at once.

```stac
[4 lfdclipnoise.kr 200 * 400 + sinosc .1 *] play
```

### sc.lfdnoise0

`sc.lfdnoise0 (freq --> out)`

Dynamic step noise (SC help).

Use it like lfnoise0 when freq moves fast; it changes rate without waiting for the next step.

```stac
[8 lfdnoise0.kr 300 * 500 + sinosc .1 *] play
```

### sc.lfdnoise1

`sc.lfdnoise1 (freq --> out)`

Dynamic ramp noise (SC help).

Use it like lfnoise1 when freq is modulated; it follows rate changes at once.

```stac
[8 lfdnoise1.kr 300 * 500 + sinosc .1 *] play
```

### sc.lfdnoise3

`sc.lfdnoise3 (freq --> out)`

Dynamic cubic noise (SC help).

Use it like lfdnoise1 when the movement should be smooth: cubic interpolation.

```stac
[2 lfdnoise3.kr 300 * 500 + sinosc .1 *] play
```

### sc.lfgauss

`sc.lfgauss (duration width iphase loop --> out)`

Gaussian function oscillator (SC help).

Use it for Gaussian grain envelopes; duration is the period, width the curve's spread.

```stac
[.1 .1 0 1 lfgauss 800 sinosc * .1 *] play
```

### sc.lfnoise0

`sc.lfnoise0 (freq --> out)`

Step noise (SC help).

Use it for random steps freq times a second, a sample-and-hold sound.

```stac
[8 lfnoise0.kr 300 * 500 + sinosc .1 *] play
```

### sc.lfnoise1

`sc.lfnoise1 (freq --> out)`

Ramp noise (SC help).

Use it for random ramps, the usual slow modulation source.

```stac
[1 lfnoise1.kr 300 * 500 + sinosc .1 *] play
```

### sc.lfnoise2

`sc.lfnoise2 (freq --> out)`

Quadratic noise (SC help).

Use it for smoother random movement than lfnoise1: quadratic segments.

```stac
[1 lfnoise2.kr 300 * 500 + sinosc .1 *] play
```

### sc.lfpar

`sc.lfpar (freq --> out)`

Parabolic oscillator (SC help).

Use it as a cheap, sine-like LFO made of parabolas.

```stac
[2 lfpar.kr 100 * 400 + sinosc .1 *] play
```

### sc.lfpulse

`sc.lfpulse (freq iphase width --> out)`

Pulse oscillator (SC help).

Use it as a non-band-limited pulse for LFOs and gates; it runs from 0 to 1, width sets the duty.

```stac
[4 0 .3 lfpulse.kr 400 * 300 + sinosc .1 *] play
```

### sc.lfsaw

`sc.lfsaw (freq --> out)`

Sawtooth oscillator (SC help).

Use it as a non-band-limited saw for LFOs and sweeps, from -1 to 1. saw is the audio-rate version without aliasing.

```stac
[.5 lfsaw.kr 200 * 400 + sinosc .1 *] play
```

### sc.lftri

`sc.lftri (freq --> out)`

Triangle oscillator (SC help).

Use it as a triangle LFO, from -1 to 1.

```stac
[1 lftri.kr 200 * 400 + sinosc .1 *] play
```

### sc.lpf

`sc.lpf (in freq --> out)`

2nd order Butterworth lowpass filter (SC help).

Use it to darken a sound by removing frequencies above freq, 12 dB per octave; rlpf adds resonance.

```stac
[110 saw 800 lpf .1 *] play
```

### sc.lpz1

`sc.lpz1 (in --> out)`

Two point average filter (SC help).

Use it as a cheap, gentle low-pass: the average of each sample and the one before.

```stac
[whitenoise lpz1 .1 *] play
```

### sc.lpz2

`sc.lpz2 (in --> out)`

Two zero fixed lowpass (SC help).

Use it as a cheap two-zero low-pass, steeper than lpz1.

```stac
[whitenoise lpz2 .1 *] play
```

### sc.lag

`sc.lag (in lagTime --> out)`

Exponential lag (SC help).

Use it to smooth a stepping control, such as a value set with .set; lagTime is the 60 dB time.

```stac
[2 lfnoise0.kr .2 lag 300 * 500 + sinosc .1 *] play
```

### sc.lag2

`sc.lag2 (in lagTime --> out)`

Exponential lag (SC help).

Use it like lag with a smoother curve: two lags in series.

```stac
[2 lfnoise0.kr .2 lag2 300 * 500 + sinosc .1 *] play
```

### sc.lag2ud

`sc.lag2ud (in lagTimeU lagTimeD --> out)`

Exponential lag (SC help).

Use it like lag2 with separate times for rising and falling values.

```stac
[2 lfnoise0.kr .05 .5 lag2ud 300 * 500 + sinosc .1 *] play
```

### sc.lag3

`sc.lag3 (in lagTime --> out)`

Exponential lag (SC help).

Use it for the smoothest of the lags: three in series.

```stac
[2 lfnoise0.kr .2 lag3 300 * 500 + sinosc .1 *] play
```

### sc.lag3ud

`sc.lag3ud (in lagTimeU lagTimeD --> out)`

Exponential lag (SC help).

Use it like lag3 with separate times for rising and falling values.

```stac
[2 lfnoise0.kr .05 .5 lag3ud 300 * 500 + sinosc .1 *] play
```

### sc.lagud

`sc.lagud (in lagTimeU lagTimeD --> out)`

Exponential lag (SC help).

Use it like lag with separate times up and down, e.g. a fast attack and a slow release on an envelope follower.

```stac
[2 lfnoise0.kr .05 .5 lagud 300 * 500 + sinosc .1 *] play
```

### sc.lastvalue

`sc.lastvalue (in diff --> out)`

Output the last value before the input changed (SC help).

Use it to output the previous distinct value when in changes by more than diff.

```stac
[2 lfnoise0.kr .1 lastvalue 300 * 500 + sinosc .1 *] play
```

### sc.latch

`sc.latch (in trig --> out)`

Sample and hold (SC help).

Use it to sample in at each trigger and hold it: sample and hold.

```stac
[whitenoise 8 impulse latch 300 * 500 + sinosc .1 *] play
```

### sc.leakdc

`sc.leakdc (in coef --> out)`

Remove DC (SC help).

Use it after asymmetric distortion or feedback to remove DC offset; coef near 1 cuts only the lowest frequencies.

```stac
[220 saw .5 + .995 leakdc .1 *] play
```

### sc.leastchange

`sc.leastchange (a b --> out)`

Output least changed (SC help).

Use it to follow whichever of two inputs changed less since the last sample.

```stac
[1 lfnoise1.kr 5 lfnoise1.kr leastchange 200 * 400 + sinosc .1 *] play
```

### sc.limiter

`sc.limiter (in level dur --> out)`

Peak limiter (SC help).

Use it on a master output to keep peaks below level; dur is its look-ahead, which delays the signal by that much.

```stac
[200 saw 3 * .5 .01 limiter] play
```

### sc.linexp

`sc.linexp (in srclo srchi dstlo dsthi --> out)`

Map a linear range to an exponential range (SC help).

Use it to map a linear control onto an exponential range, such as frequency. std's linexp hides this UGen after the prelude, so write sc.linexp.

```stac
[1 lfnoise1.kr -1 1 200 2000 sc.linexp sinosc .1 *] play
```

### sc.linpan2

`sc.linpan2 (in pos --> out)`

Two channel linear pan (SC help).

Use it to pan with linear gains; the centre is 6 dB down, which pan2 avoids.

```stac
[pinknoise .2 * 1 lfnoise1.kr linpan2] play
```

### sc.linxfade2

`sc.linxfade2 (inA inB pan --> out)`

Two channel linear crossfade (SC help).

Use it to crossfade two signals with linear gains; pan runs from -1 (inA) to 1 (inB).

```stac
[220 saw 220 sinosc .5 lfnoise1.kr linxfade2 .1 *] play
```

### sc.line

`sc.line (start end dur --> out)`

Line generator (SC help).

Use it for a straight ramp from start to end over dur seconds; .done frees the synth at its end.

```stac
[200 800 2 line.done sinosc .1 *] play
```

### sc.linen

`sc.linen (gate attackTime susLevel releaseTime --> out)`

Simple linear envelope generator (SC help).

Use it for a simple attack-sustain-release envelope held by gate; .done frees the synth after release.

```stac
[1 .01 .1 .5 linen 440 sinosc *] play
```

### sc.mantissamask

`sc.mantissamask (in bits --> out)`

Reduce precision (SC help).

Use it for bit-crush style distortion: bits keeps that many bits of the mantissa.

```stac
[440 sinosc 3 mantissamask .1 *] play
```

### sc.median

`sc.median (length in --> out)`

Median filter (SC help).

Use it to remove spikes from a signal with a running median over length samples. std's median hides this UGen after the prelude, so write sc.median.

```stac
[3 440 sinosc 50 dust2 + sc.median .1 *] play
```

### sc.mideq

`sc.mideq (in freq rq db --> out)`

Parametric filter (SC help).

Use it to boost or cut one band by db; rq sets the band's width.

```stac
[pinknoise .2 * 1000 .3 12 mideq] play
```

### sc.moddif

`sc.moddif (x y mod --> out)`

Minimum difference of two values in modulo arithmetics (SC help).

Use it to get the shortest distance between two values in modulo arithmetic, e.g. between two phases.

```stac
[1 lfsaw.kr .3 1 moddif 400 * 200 + sinosc .1 *] play
```

### sc.moogff

`sc.moogff (in freq gain --> out)`

Moog VCF implementation, designed by Federico Fontana (SC help).

Use it for the Moog ladder sound: freq is the cutoff and gain, from 0 to 4, the resonance.

```stac
[110 saw 1 lfnoise1.kr 1000 * 1200 + 3 moogff .1 *] play
```

### sc.mostchange

`sc.mostchange (a b --> out)`

Output most changed (SC help).

Use it to follow whichever of two inputs changed more since the last sample.

```stac
[1 lfnoise1.kr 5 lfnoise1.kr mostchange 200 * 400 + sinosc .1 *] play
```

### sc.normalizer

`sc.normalizer (in level dur --> out)`

Flattens dynamics (SC help).

Use it to raise a signal to a constant peak level; like limiter it looks ahead by dur.

```stac
[440 sinosc .3 lfnoise1.kr * .2 .01 normalizer] play
```

### sc.onepole

`sc.onepole (in coef --> out)`

One pole filter (SC help).

Use it as the cheapest low-pass or, with negative coef, high-pass; coef from -1 to 1.

```stac
[whitenoise .95 onepole .1 *] play
```

### sc.onezero

`sc.onezero (in coef --> out)`

One zero filter (SC help).

Use it as a cheap one-zero filter; coef .5 averages, -.5 differences.

```stac
[whitenoise .5 onezero .1 *] play
```

### sc.osc

`sc.osc (bufnum freq --> out)`

Interpolating wavetable oscillator (SC help).

Use it to play a wavetable held in a buffer at freq; the buffer must be in wavetable format.

```stac
1024 1 buffer.alloc = b
[b 220 osc .1 *] play
```

### sc.oscn

`sc.oscn (bufnum freq --> out)`

Noninterpolating wavetable oscillator (SC help).

Use it like osc where the table is long enough not to need interpolation.

```stac
1024 1 buffer.alloc = b
[b 220 oscn .1 *] play
```

### sc.out

`sc.out (bus channelsArray --> out)`

Write a signal to a bus (SC help).

Use it to write channels to a bus inside a synthdef that writes its own output; a body that leaves channels gets an Out to its out control already.

```stac
synthdef beep2 [0 440 sinosc .1 * out pop]
{} beep2
```

### sc.psingrain

`sc.psingrain (freq dur amp --> out)`

Very fast sine grain with a parabolic envelope (SC help).

Use it for a short sine grain with a parabolic envelope, at a fixed frequency.

```stac
[880 .1 .2 psingrain] play
```

### sc.pan2

`sc.pan2 (in pos --> out)`

Two channel equal power pan (SC help).

Use it to place a mono signal in stereo with equal power; pos runs from -1 (left) to 1 (right).

```stac
[pinknoise .2 * 1 lfnoise1.kr pan2] play
```

### sc.pan4

`sc.pan4 (in xpos ypos --> out)`

Four channel equal power pan (SC help).

Use it to pan over 4 speakers; xpos is left to right and ypos back to front.

```stac
[pinknoise .2 * 1 lfnoise1.kr 1 lfnoise1.kr pan4] play
```

### sc.panaz

`sc.panaz (numChans in pos level width orientation --> out)`

Azimuth panner (SC help).

Use it to pan around a ring of numChans speakers; pos 0 to 2 goes once around.

```stac
[4 pinknoise .2 * .2 lfsaw.kr 1 2 .5 panaz] play
```

### sc.phasor

`sc.phasor (trig rate start end resetPos --> out)`

A resettable linear ramp between two levels (SC help).

Use it as a resettable ramp, e.g. a read position for bufrd that loops from start to end. rate is the step per sample, so sampledur ramps by 1 per second.

```stac
[0 sampledur 0 1 0 phasor 400 * 200 + sinosc .1 *] play
```

### sc.pinknoise

`sc.pinknoise ( --> out)`

Pink Noise (SC help).

Use it for a balanced noise with equal energy per octave, softer on the ear than whitenoise.

```stac
[pinknoise .1 *] play
```

### sc.pitchshift

`sc.pitchshift (in windowSize pitchRatio pitchDispersion timeDispersion --> out)`

Time domain pitch shifter (SC help).

Use it to shift pitch by pitchRatio without changing duration; windowSize is in seconds.

```stac
[220 saw .1 * .2 1.5 0 0 pitchshift] play
```

### sc.pluck

`sc.pluck (in trig maxdelaytime delaytime decaytime coef --> out)`

A Karplus-Strong UGen (SC help).

Use it for plucked strings: a burst of in, gated by trig, through a comb tuned by delaytime.

```stac
[whitenoise .1 * 2 impulse .01 .005 3 .3 pluck] play
```

### sc.pulse

`sc.pulse (freq width --> out)`

Band limited pulse wave (SC help).

Use it for a band-limited pulse wave; width .5 is a square, lower values thinner.

```stac
[110 .3 pulse .1 *] play
```

### sc.pulsecount

`sc.pulsecount (trig --> out)`

Pulse counter (SC help).

Use it to count triggers, e.g. as a step index.

```stac
[4 impulse.kr pulsecount.kr 8 % 100 * 200 + sinosc .1 *] play
```

### sc.pulsedivider

`sc.pulsedivider (trig div start --> out)`

Pulse divider (SC help).

Use it to pass every div-th trigger, e.g. for a slower clock from a fast one.

```stac
[8 impulse 4 0 pulsedivider .1 decay whitenoise * .2 *] play
```

### sc.rhpf

`sc.rhpf (in freq rq --> out)`

A resonant high pass filter (SC help).

Use it for a resonant high-pass; rq is 1/Q, so smaller values ring more.

```stac
[110 saw 1000 .2 rhpf .1 *] play
```

### sc.rlpf

`sc.rlpf (in freq rq --> out)`

A resonant low pass filter (SC help).

Use it for the classic resonant low-pass sweep; rq is 1/Q.

```stac
[110 saw 1 lfnoise1.kr 800 * 1000 + .2 rlpf .1 *] play
```

### sc.ramp

`sc.ramp (in lagTime --> out)`

Break a continuous signal into line segments (SC help).

Use it to turn steps into linear segments of lagTime seconds; lag curves exponentially instead.

```stac
[2 lfnoise0.kr .2 ramp 300 * 500 + sinosc .1 *] play
```

### sc.resonz

`sc.resonz (in freq bwr --> out)`

Resonant filter (SC help).

Use it for a resonant band-pass whose gain stays constant; bwr is bandwidth over freq.

```stac
[whitenoise 800 .05 resonz .5 *] play
```

### sc.ringz

`sc.ringz (in freq decaytime --> out)`

Ringing filter (SC help).

Use it to make an impulse ring at freq for decaytime seconds, a struck-object sound.

```stac
[2 impulse 1200 .5 ringz .1 *] play
```

### sc.runningmax

`sc.runningmax (in trig --> out)`

Track maximum level (SC help).

Use it to track the largest value since the last trig.

```stac
[1 lfnoise1.kr 1 impulse.kr runningmax 300 * 400 + sinosc .1 *] play
```

### sc.runningmin

`sc.runningmin (in trig --> out)`

Track minimum level (SC help).

Use it to track the smallest value since the last trig.

```stac
[1 lfnoise1.kr 1 impulse.kr runningmin 300 * 400 + sinosc .1 *] play
```

### sc.sos

`sc.sos (in a0 a1 a2 b1 b2 --> out)`

Second order filter section (biquad) (SC help).

Use it to build any biquad from its 5 coefficients, e.g. ones computed elsewhere.

```stac
[whitenoise .2 0 -.2 1.8 -.9 sos .02 *] play
```

### sc.saw

`sc.saw (freq --> out)`

Band limited sawtooth (SC help).

Use it as the band-limited saw for audio, rich in harmonics before a filter. lfsaw is the cheaper LFO version.

```stac
[110 saw .1 *] play
```

### sc.schmidt

`sc.schmidt (in lo hi --> out)`

Schmidt trigger (SC help).

Use it to turn a noisy signal into clean on/off states: 1 above hi, 0 below lo.

```stac
[1 lfnoise1.kr -.2 .2 schmidt 200 * 300 + sinosc .1 *] play
```

### sc.select

`sc.select (which array --> out)`

Select output from an array of inputs (SC help).

Use it to choose one of several signals by an index signal.

```stac
[1 0 .5 lfpulse.kr [220 sinosc 220 saw] select .1 *] play
```

### sc.sendtrig

`sc.sendtrig (in id value --> out)`

Send a trigger message from the server back to the client (SC help).

Use it to send a value to the client at each trigger, e.g. to watch a signal; the server sends /tr messages. It has no output; a synthdef keeps it in the graph, a plain play list would not.

```stac
synthdef watch [1 impulse.kr 0 1 lfnoise1.kr sendtrig pop 440 sinosc .1 *]
{} watch
```

### sc.setresetff

`sc.setresetff (trig --> out)`

Set-reset flip flop (SC help).

Use it as a set-reset flip-flop: a trigger at trig sets it to 1, one at reset to 0.

```stac
[{:trig 1 impulse.kr :reset 3 impulse.kr} setresetff 400 * 200 + sinosc .1 *] play
```

### sc.sinosc

`sc.sinosc (freq --> out)`

Interpolating sine wavetable oscillator (SC help).

Use it for a pure tone, an LFO with .kr, or a carrier for FM.

```stac
[440 sinosc .1 *] play
440 sinosc         ;-> SinOsc.ar
```

### sc.sinoscfb

`sc.sinoscfb (freq feedback --> out)`

Feedback FM oscillator (SC help).

Use it for a one-oscillator FM sound; feedback above 1 adds noise-like harmonics.

```stac
[110 1.2 sinoscfb .1 *] play
```

### sc.slew

`sc.slew (in up dn --> out)`

Slew rate limiter (SC help).

Use it to limit how fast a signal rises and falls, in units per second.

```stac
[2 0 .5 lfpulse.kr 2 4 slew 400 * 200 + sinosc .1 *] play
```

### sc.spring

`sc.spring (in spring damp --> out)`

Physical model of a resonating spring (SC help).

Use it to model a mass on a spring driven by in; spring is the stiffness, damp the damping.

```stac
[2 impulse.kr 20 .001 spring 300 * 400 + sinosc .1 *] play
```

### sc.stepper

`sc.stepper (trig reset min max step resetval --> out)`

Pulse counter (SC help).

Use it as a counter that wraps between min and max by step at each trigger, for sequencing.

```stac
[4 impulse.kr 0 0 7 1 0 stepper.kr 100 * 200 + sinosc .1 *] play
```

### sc.sweep

`sc.sweep (trig rate --> out)`

Triggered linear ramp (SC help).

Use it for a ramp that rises at rate per second and restarts at each trig.

```stac
[1 impulse.kr 400 sweep 200 + sinosc .1 *] play
```

### sc.syncsaw

`sc.syncsaw (syncFreq sawFreq --> out)`

Hard sync sawtooth wave (SC help).

Use it for hard-sync sweeps: a saw at sawFreq restarted at syncFreq.

```stac
[110 1 lfnoise1.kr 300 * 400 + syncsaw .1 *] play
```

### sc.t2a

`sc.t2a (in offset --> out)`

Control rate trigger to audio rate trigger converter (SC help).

Use it to turn a control-rate trigger into an audio-rate impulse, with offset samples of delay.

```stac
[2 impulse.kr 0 t2a .1 decay whitenoise * .2 *] play
```

### sc.t2k

`sc.t2k (in --> out)`

Audio rate trigger to control rate trigger converter (SC help).

Use it to turn an audio-rate trigger into a control-rate one without losing it between blocks.

```stac
[10 dust t2k 300 * 200 + sinosc .1 *] play
```

### sc.tball

`sc.tball (in g damp friction --> out)`

Physical model of a bouncing object (SC help).

Use it like ball where the bounces should come out as triggers.

```stac
[.5 1 0 .01 tball .1 decay 800 sinosc * .2 *] play
```

### sc.tdelay

`sc.tdelay (in dur --> out)`

Trigger delay (SC help).

Use it to delay triggers by dur seconds.

```stac
[1 impulse .5 tdelay .1 decay whitenoise * .2 *] play
```

### sc.timer

`sc.timer (trig --> out)`

Returns time since last triggered (SC help).

Use it to measure the time between triggers, e.g. to follow a tempo.

```stac
[2 dust.kr timer.kr 400 * 200 + sinosc .1 *] play
```

### sc.toggleff

`sc.toggleff (trig --> out)`

Toggle flip flop (SC help).

Use it to flip between 0 and 1 at each trigger.

```stac
[2 impulse.kr toggleff.kr 200 * 300 + sinosc .1 *] play
```

### sc.trig

`sc.trig (in dur --> out)`

Timed trigger (SC help).

Use it to turn a trigger into a gate of dur seconds that keeps the trigger's value.

```stac
[2 impulse .1 trig 600 sinosc * .1 *] play
```

### sc.trig1

`sc.trig1 (in dur --> out)`

Timed trigger (SC help).

Use it like trig when the gate should always be 1.

```stac
[2 impulse .1 trig1 600 sinosc * .1 *] play
```

### sc.twopole

`sc.twopole (in freq radius --> out)`

Two pole filter (SC help).

Use it as a two-pole resonator at freq; radius near 1 rings longer.

```stac
[whitenoise 800 .99 twopole .01 *] play
```

### sc.twozero

`sc.twozero (in freq radius --> out)`

Two zero filter (SC help).

Use it as a two-zero notch at freq; radius sets its depth.

```stac
[whitenoise 800 .9 twozero .1 *] play
```

### sc.vosc

`sc.vosc (bufpos freq --> out)`

Variable wavetable oscillator (SC help).

Use it to morph between consecutive wavetable buffers: bufpos moves between them.

```stac
1024 1 buffer.alloc = a  1024 1 buffer.alloc = b
[a 220 vosc .1 *] play
```

### sc.vosc3

`sc.vosc3 (bufpos freq1 freq2 freq3 --> out)`

Three variable wavetable oscillators (SC help).

Use it like vosc with three detuned oscillators summed.

```stac
1024 1 buffer.alloc = a
[a 220 221 222 vosc3 .05 *] play
```

### sc.varsaw

`sc.varsaw (freq iphase width --> out)`

Variable duty saw (SC help).

Use it for a saw whose slope ratio is width: .5 is a triangle, near 0 or 1 a saw. It is not band-limited.

```stac
[110 0 .3 varsaw .1 *] play
```

### sc.vibrato

`sc.vibrato (freq rate depth delay onset rateVariation depthVariation iphase trig --> out)`

The Vibrato oscillator models a slow frequency modulation (SC help).

Use it to add natural vibrato to a frequency: rate and depth, with a delay and onset before it starts.

```stac
[440 6 .02 0 0 .04 .1 0 0 vibrato sinosc .1 *] play
```

### sc.whitenoise

`sc.whitenoise ( --> out)`

White noise (SC help).

Use it for flat-spectrum noise, the usual input for filters and percussion.

```stac
[whitenoise .05 *] play
```

### sc.wrap

`sc.wrap (in lo hi --> out)`

Wrap a signal outside given thresholds (SC help).

Use it to wrap a signal back into lo..hi. The core word wrap also works on reals; sc.wrap always builds the UGen.

```stac
[220 sinosc 3 * -1 1 sc.wrap .1 *] play
```

### sc.xfade2

`sc.xfade2 (inA inB pan --> out)`

Equal power two channel cross fade (SC help).

Use it to crossfade two signals with equal power; pan runs from -1 (inA) to 1 (inB).

```stac
[220 saw 220 sinosc .5 lfnoise1.kr xfade2 .1 *] play
```

### sc.xline

`sc.xline (start end dur --> out)`

Exponential line generator (SC help).

Use it for an exponential ramp, which sounds even for frequency and amplitude; start and end must not be 0.

```stac
[200 2000 2 xline.done sinosc .1 *] play
```

## buffer

### sc.bufallpassc

`sc.bufallpassc (buf in delaytime decaytime --> out)`

Buffer based all pass delay line with cubic interpolation (SC help).

Use it like allpassc when the delay memory should live in a buffer you allocate, e.g. to share or resize it.

```stac
48000 1 buffer.alloc = b
[b 2 dust .5 * .05 1 bufallpassc] play
```

### sc.bufallpassl

`sc.bufallpassl (buf in delaytime decaytime --> out)`

Buffer based all pass delay line with linear interpolation (SC help).

Use it like bufallpassc at lower cost; linear interpolation.

```stac
48000 1 buffer.alloc = b
[b 2 dust .5 * .05 1 bufallpassl] play
```

### sc.bufallpassn

`sc.bufallpassn (buf in delaytime decaytime --> out)`

Buffer based all pass delay line with no interpolation (SC help).

Use it like bufallpassc with a fixed delay; no interpolation.

```stac
48000 1 buffer.alloc = b
[b 2 dust .5 * .05 1 bufallpassn] play
```

### sc.bufchannels

`sc.bufchannels (bufnum --> out)`

Current number of channels of soundfile in buffer (SC help).

Use it to make a graph adapt to the buffer it reads, e.g. a file of unknown width.

```stac
48000 1 buffer.alloc = b
b bufchannels        ;-> BufChannels.kr
```

### sc.bufcombc

`sc.bufcombc (buf in delaytime decaytime --> out)`

Buffer based comb delay line with cubic interpolation (SC help).

Use it like combc with its memory in a buffer, so the longest delay is the buffer's length.

```stac
48000 1 buffer.alloc = b
[b .5 dust .1 * .2 2 bufcombc] play
```

### sc.bufcombl

`sc.bufcombl (buf in delaytime decaytime --> out)`

Buffer based comb delay line with linear interpolation (SC help).

Use it like bufcombc at lower cost; linear interpolation.

```stac
48000 1 buffer.alloc = b
[b .5 dust .1 * .2 2 bufcombl] play
```

### sc.bufcombn

`sc.bufcombn (buf in delaytime decaytime --> out)`

Buffer based comb delay line with no interpolation (SC help).

Use it like bufcombc with a fixed delay; no interpolation.

```stac
48000 1 buffer.alloc = b
[b .5 dust .1 * .2 2 bufcombn] play
```

### sc.bufdelayc

`sc.bufdelayc (buf in delaytime --> out)`

Buffer based simple delay line with cubic interpolation (SC help).

Use it like delayc with its memory in a buffer.

```stac
48000 1 buffer.alloc = b
[b 2 impulse .1 decay whitenoise * .3 bufdelayc] play
```

### sc.bufdelayl

`sc.bufdelayl (buf in delaytime --> out)`

Buffer based simple delay line with linear interpolation (SC help).

Use it like bufdelayc at lower cost; linear interpolation.

```stac
48000 1 buffer.alloc = b
[b 2 impulse .1 decay whitenoise * .3 bufdelayl] play
```

### sc.bufdelayn

`sc.bufdelayn (buf in delaytime --> out)`

Buffer based simple delay line with no interpolation (SC help).

Use it like bufdelayc with a fixed delay; no interpolation.

```stac
48000 1 buffer.alloc = b
[b 2 impulse .1 decay whitenoise * .3 bufdelayn] play
```

### sc.bufdur

`sc.bufdur (bufnum --> out)`

Current duration of soundfile in buffer (SC help).

Use it to loop or scrub a sample at its own length, e.g. as the period of a read ramp.

```stac
48000 1 buffer.alloc = b
b bufdur             ;-> BufDur.kr
```

### sc.bufframes

`sc.bufframes (bufnum --> out)`

Current number of frames allocated in the buffer (SC help).

Use it as the end point of a read or write position, so the graph fits the buffer.

```stac
48000 1 buffer.alloc = b
b bufframes          ;-> BufFrames.kr
```

### sc.bufratescale

`sc.bufratescale (bufnum --> out)`

Buffer rate scaling in respect to server samplerate (SC help).

Use it as playbuf's rate to play a file recorded at another sample rate at its own pitch.

```stac
48000 1 buffer.alloc = b
[1 b b bufratescale 0 0 1 playbuf .1 *] play
```

### sc.bufrd

`sc.bufrd (numChannels bufnum phase loop interpolation --> out)`

Buffer reading oscillator (SC help).

Use it to read a buffer at any position signal, e.g. a phasor for looping or a noise for scrubbing; interpolation 2 is linear, 4 cubic.

```stac
48000 1 buffer.alloc = b
[1 b 0 1 0 b bufframes 0 phasor 1 2 bufrd .1 *] play
```

### sc.bufsamplerate

`sc.bufsamplerate (bufnum --> out)`

Buffer sample rate (SC help).

Use it to compute rates or durations from the rate a file was recorded at.

```stac
48000 1 buffer.alloc = b
b bufsamplerate      ;-> BufSampleRate.kr
```

### sc.bufsamples

`sc.bufsamples (bufnum --> out)`

Current number of samples in buffer (SC help).

Use it where a graph needs frames times channels, the buffer's total sample count.

```stac
48000 1 buffer.alloc = b
b bufsamples         ;-> BufSamples.kr
```

### sc.bufwr

`sc.bufwr (inputArray bufnum phase loop --> out)`

Buffer writing oscillator (SC help).

Use it to write a signal into a buffer at a position signal, the writing pair of bufrd.

```stac
48000 1 buffer.alloc = b
[440 sinosc .1 * b 0 1 0 b bufframes 0 phasor 1 bufwr] play
```

### sc.clearbuf

`sc.clearbuf (buf --> out)`

Sets every sample of buf to 0 when the synth starts.

Use it to start a buffer used as delay memory from silence.

```stac
48000 1 buffer.alloc = b
b clearbuf           ;-> ClearBuf.ir
```

### sc.dbufrd

`sc.dbufrd (bufnum phase loop --> out)`

Buffer read demand ugen (SC help).

Use it to step through a buffer's values on demand, e.g. a sequence table read by demand or duty.

```stac
[1 2 3 4] buffer.load = b
[8 impulse.kr 0 b [0 1 2 3] inf dseq 1 dbufrd demand.kr 100 * 200 + sinosc .1 *] play
```

### sc.dbufwr

`sc.dbufwr (input bufnum phase loop --> out)`

Buffer write demand ugen (SC help).

Use it to write values into a buffer on demand, e.g. to record a demand sequence.

```stac
4 1 buffer.alloc = b
[8 impulse.kr 0 [5 6 7] inf dseq b [0 1 2 3] inf dseq 1 dbufwr demand.kr 100 * 200 + sinosc .1 *] play
```

### sc.deltaprd

`sc.deltaprd (buffer phase delTime interp --> out)`

Tap a delay line from a DelTapWr UGen (SC help).

Use it to read a delay line written by deltapwr at several delay times, a multi-tap delay; interp 1 is none, 2 linear, 4 cubic.

```stac
48000 1 buffer.alloc = b
[b .5 dust .2 * deltapwr = tap  b tap .25 2 deltaprd  b tap .5 2 deltaprd +] play
```

### sc.deltapwr

`sc.deltapwr (buffer in --> out)`

Write to a buffer for a DelTapRd UGen (SC help).

Use it to write the delay line that deltaprd taps; its output is the write position the taps need.

```stac
48000 1 buffer.alloc = b
[b .5 dust .2 * deltapwr = tap  b tap .3 2 deltaprd] play
```

### sc.detectindex

`sc.detectindex (bufnum in --> out)`

Search a buffer for a value (SC help).

Use it to find where a value sits in a table, e.g. to map a pitch back to a scale step.

```stac
[200 300 400 500] buffer.load = b
[b 300 detectindex 100 * 200 + sinosc .1 *] play
```

### sc.foldindex

`sc.foldindex (bufnum in --> out)`

Index into a table with a signal (SC help).

Use it like index when an out-of-range position should fold back into the table.

```stac
[200 300 400 500] buffer.load = b
[b 1 lfnoise0.kr 4 * 2 + foldindex sinosc .1 *] play
```

### sc.grainbuf

`sc.grainbuf (numChannels trigger dur sndbuf rate pos interp pan envbufnum maxGrains --> out)`

Granular synthesis with sound stored in a buffer (SC help).

Use it for granular synthesis from a sample: trigger starts a grain of dur seconds at pos, from 0 to 1 through the buffer.

```stac
48000 1 buffer.alloc = b
[{:numChannels 1 :trigger 20 impulse :dur .1 :sndbuf b :pos .2 lfsaw.kr 1 + .5 *} grainbuf .1 *] play
```

### sc.index

`sc.index (bufnum in --> out)`

Index into a table with a signal (SC help).

Use it to look up a value in a table by an integer position, e.g. a scale of frequencies.

```stac
[200 300 400 500] buffer.load = b
[b 1 lfnoise0.kr 2 * 2 + index sinosc .1 *] play
```

### sc.indexinbetween

`sc.indexinbetween (bufnum in --> out)`

Finds the (lowest) point in the Buffer at which the input signal lies in-between the two values (SC help).

Use it to find the fractional position at which a value lies in a sorted table, the inverse of indexl.

```stac
[200 300 400 500] buffer.load = b
[b 350 indexinbetween 100 * 200 + sinosc .1 *] play
```

### sc.indexl

`sc.indexl (bufnum in --> out)`

Index into a table with a signal, linear interpolated (SC help).

Use it like index when the position moves smoothly, so the value glides between table entries.

```stac
[200 300 400 500] buffer.load = b
[b .5 lfnoise1.kr 2 * 2 + indexl sinosc .1 *] play
```

### sc.playbuf

`sc.playbuf (numChannels bufnum rate trigger startPos loop --> out)`

Sample playback oscillator (SC help).

Use it to play a sample, e.g. one read with buffer.read; rate 1 is the original speed, loop 1 repeats it.

```stac
48000 1 buffer.alloc = b
[1 b 1 0 0 1 playbuf .1 *] play
```

### sc.recordbuf

`sc.recordbuf (inputArray bufnum offset recLevel preLevel run loop --> out)`

Record or overdub into a Buffer (SC help).

Use it to record or overdub a signal into a buffer; preLevel sets how much of the old content stays.

```stac
48000 1 buffer.alloc = b
[440 sinosc .1 * b 0 1 0 1 1 recordbuf] play
```

### sc.scopeout

`sc.scopeout (inputArray bufnum --> out)`

Writes channels to a buffer for an oscilloscope display. ScopeOut2 replaces it.

Use it only with tools that still read ScopeOut buffers.

```stac
48000 1 buffer.alloc = b
[440 sinosc b scopeout] play
```

### sc.setbuf

`sc.setbuf (buf values offset --> out)`

Writes values into buf from offset when the synth starts.

Use it to fill a buffer from inside a graph, e.g. a small table for index.

```stac
[200 300 400 500] buffer.load = b
b [1 2 3] 0 setbuf   ;-> SetBuf.ir
```

### sc.shaper

`sc.shaper (bufnum in --> out)`

Wave shaper (SC help).

Use it for waveshaping distortion: the buffer holds the transfer function in wavetable format, and in reads it from -1 to 1.

```stac
1024 1 buffer.alloc = b
[b 220 sinosc shaper .1 *] play
```

### sc.tgrains

`sc.tgrains (numChannels trigger bufnum rate centerPos dur pan amp interp --> out)`

Buffer granulator (SC help).

Use it for a granulator that pans each grain; centerPos is in seconds and dur sets each grain's length.

```stac
48000 1 buffer.alloc = b
[{:numChannels 2 :trigger 20 impulse :bufnum b :centerPos .5 :dur .1} tgrains .1 *] play
```

### sc.warp1

`sc.warp1 (numChannels bufnum pointer freqScale windowSize envbufnum overlaps windowRandRatio interp --> out)`

Warp a buffer with a time pointer (SC help).

Use it to stretch or compress a sample in time without changing pitch: pointer runs from 0 to 1 through the buffer.

```stac
48000 1 buffer.alloc = b
[{:numChannels 1 :bufnum b :pointer .1 lfsaw.kr 1 + .5 *} warp1 .1 *] play
```

### sc.wrapindex

`sc.wrapindex (bufnum in --> out)`

Index into a table with a signal (SC help).

Use it like index when an out-of-range position should wrap around the table, e.g. for a repeating arpeggio.

```stac
[200 300 400 500] buffer.load = b
[b 4 impulse.kr pulsecount.kr wrapindex sinosc .1 *] play
```

## demand

### sc.dbrown

`sc.dbrown (lo hi step length --> out)`

Demand rate brownian movement generator (SC help).

Use it for a random walk between lo and hi in steps up to step, read on demand.

```stac
[8 impulse.kr 0 0 12 1 inf dbrown demand.kr 60 + nnhz saw .05 *] play
```

### sc.dconst

`sc.dconst (sum in tolerance --> out)`

Constrain a demand-rate stream to a given sum (SC help).

Use it to cut a stream of durations so they sum to sum, e.g. a random rhythm that fills one bar.

```stac
[4 [.25 .5] inf drand 0 dconst inf dseq 0 1 duty 400 * 200 + sinosc .1 *] play
```

### sc.ddup

`sc.ddup (n in --> out)`

Demand rate input replicator (SC help).

Use it to repeat each value of a stream n times, e.g. to hold a note for several steps.

```stac
[8 impulse.kr 0 2 [60 63 67] inf dseq ddup demand.kr nnhz saw .05 *] play
```

### sc.demand

`sc.demand (trig reset demandUGens --> out)`

Demand results from demand rate UGens (SC help).

Use it to read the next value of demand-rate streams at each trigger: the bridge from dseq and its kin to a signal.

```stac
[8 impulse.kr 0 [60 63 67 70] inf dseq demand.kr nnhz saw .05 *] play
```

### sc.demandenvgen

`sc.demandenvgen (level dur shape curve gate --> out)`

Demand rate envelope generator (SC help).

Use it for an envelope whose levels and durations come from demand streams, so each segment can differ.

```stac
[[0 1 .3] inf dseq [.1 .2] inf dseq 1 0 1 demandenvgen 400 * 200 + sinosc .1 *] play
```

### sc.dgeom

`sc.dgeom (start grow length --> out)`

Demand rate geometric series UGen (SC help).

Use it for a geometric series on demand: each value is the last times grow.

```stac
[8 impulse.kr 0 100 1.1 20 dgeom demand.kr sinosc .1 *] play
```

### sc.dibrown

`sc.dibrown (lo hi step length --> out)`

Demand rate brownian movement generator (SC help).

Use it like dbrown when the walk should stay on integers.

```stac
[8 impulse.kr 0 0 12 2 inf dibrown demand.kr 60 + nnhz saw .05 *] play
```

### sc.diwhite

`sc.diwhite (lo hi length --> out)`

Demand rate white noise random generator (SC help).

Use it for random integers from lo to hi on demand, e.g. scale degrees.

```stac
[8 impulse.kr 0 0 12 inf diwhite demand.kr 60 + nnhz saw .05 *] play
```

### sc.drand

`sc.drand (list repeats --> out)`

Demand rate random sequence generator (SC help).

Use it to choose a random element of list at each demand, with repeats.

```stac
[8 impulse.kr 0 [60 63 67] inf drand demand.kr nnhz saw .05 *] play
```

### sc.dreset

`sc.dreset (in reset --> out)`

Demand rate reset (SC help).

Use it to restart a demand stream when reset changes from non-positive to positive.

```stac
[8 impulse.kr 0 0 1 inf dseries 1 impulse.kr dreset demand.kr 50 * 200 + sinosc .1 *] play
```

### sc.dseq

`sc.dseq (list repeats --> out)`

Demand rate sequence generator (SC help).

Use it for a fixed sequence on demand, repeats times; inf repeats forever.

```stac
[8 impulse.kr 0 [60 63 67 70] inf dseq demand.kr nnhz saw .05 *] play
```

### sc.dser

`sc.dser (list repeats --> out)`

Demand rate sequence generator (SC help).

Use it like dseq when repeats should count values, not passes through the list.

```stac
[8 impulse.kr 0 [60 63 67 70] 6 dser inf dseq demand.kr nnhz saw .05 *] play
```

### sc.dseries

`sc.dseries (start step length --> out)`

Demand rate arithmetic series UGen (SC help).

Use it for an arithmetic series on demand: start, then start plus step each time.

```stac
[8 impulse.kr 0 0 1 inf dseries 12 % 60 + demand.kr nnhz saw .05 *] play
```

### sc.dshuf

`sc.dshuf (list repeats --> out)`

Demand rate random sequence generator (SC help).

Use it to play a list in a random order that stays the same for each pass.

```stac
[8 impulse.kr 0 [60 63 67 70] inf dshuf demand.kr nnhz saw .05 *] play
```

### sc.dswitch

`sc.dswitch (list index --> out)`

Demand rate generator for embedding different inputs (SC help).

Use it to choose which of several streams to embed next; the chosen stream plays to its end.

```stac
[8 impulse.kr 0 [[60 63] 1 dseq [67 70] 1 dseq] [0 1] inf drand dswitch demand.kr nnhz saw .05 *] play
```

### sc.dswitch1

`sc.dswitch1 (list index --> out)`

Demand rate generator for switching between inputs (SC help).

Use it to read one value from the stream that index selects, at each demand.

```stac
[8 impulse.kr 0 [[60 63] inf dseq [67 70] inf dseq] [0 1] inf drand dswitch1 demand.kr nnhz saw .05 *] play
```

### sc.duty

`sc.duty (dur reset level --> out)`

Demand results from demand rate UGens (SC help).

Use it to output demand values for durations that also come from a stream: a sequencer with its own rhythm.

```stac
[[.25 .125] inf dseq 0 [60 63 67] inf dseq duty nnhz saw .05 *] play
```

### sc.dwhite

`sc.dwhite (lo hi length --> out)`

Demand rate white noise random generator (SC help).

Use it for random reals from lo to hi on demand.

```stac
[8 impulse.kr 0 200 800 inf dwhite demand.kr sinosc .1 *] play
```

### sc.dwrand

`sc.dwrand (list weights repeats --> out)`

Demand rate weighted random sequence generator (SC help).

Use it to choose from list with weights, which should sum to 1.

```stac
[8 impulse.kr 0 [60 63 67] [.6 .3 .1] inf dwrand demand.kr nnhz saw .05 *] play
```

### sc.dxrand

`sc.dxrand (list repeats --> out)`

Demand rate random sequence generator (SC help).

Use it like drand without the same element twice in a row.

```stac
[8 impulse.kr 0 [60 63 67] inf dxrand demand.kr nnhz saw .05 *] play
```

### sc.tduty

`sc.tduty (dur reset level --> out)`

Demand results as trigger from demand rate UGens (SC help).

Use it like duty when the output should be a trigger at each step, with level as its height.

```stac
[[.25 .125] inf dseq 0 1 tduty .1 decay whitenoise * .2 *] play
```

## analysis

### sc.ampcomp

`sc.ampcomp (freq root exp --> out)`

Basic psychoacoustic amplitude compensation (SC help).

Use it to scale amplitude by frequency so low and high notes sound equally loud; root is the reference frequency.

```stac
[1 lfnoise0.kr 500 * 600 + = f  f sinosc f 300 .3333 ampcomp * .1 *] play
```

### sc.ampcompa

`sc.ampcompa (freq root minAmp rootAmp --> out)`

Basic psychoacoustic amplitude compensation (ANSI A-weighting curve) (SC help).

Use it like ampcomp with the ANSI A-weighting curve.

```stac
[1 lfnoise0.kr 500 * 600 + = f  f sinosc f 0 .32 1 ampcompa * .1 *] play
```

### sc.amplitude

`sc.amplitude (in attackTime releaseTime --> out)`

Amplitude follower (SC help).

Use it as an envelope follower: it tracks in's level, with attackTime and releaseTime in seconds.

```stac
[1 dust .3 decay whitenoise * = s  s .01 .1 amplitude 300 * 200 + sinosc .1 *] play
```

### sc.beattrack

`sc.beattrack (chain lock --> out)`

Autocorrelation beat tracker (SC help).

Use it to find a tempo in an FFT chain of the input; it outputs beat, half-beat and quarter-beat triggers and the tempo.

```stac
1024 1 buffer.alloc = b
[{:chain {:buffer b :in 2 impulse} fft} beattrack 0 at .1 decay 800 sinosc * .1 *] play
```

### sc.beattrack2

`sc.beattrack2 (busindex numfeatures windowsize phaseaccuracy lock weightingscheme --> out)`

Template matching beat tracker (SC help).

Use it to track tempo from a set of feature values on control buses, numfeatures of them from busindex.

```stac
2 bus.control = f
[{:busindex f :numfeatures 2} beattrack2 0 at .1 decay 800 sinosc * .1 *] play
```

### sc.keytrack

`sc.keytrack (chain keydecay chromaleak --> out)`

Key tracker (SC help).

Use it to estimate a musical key from an FFT chain; it outputs a key number from 0 to 23.

```stac
4096 1 buffer.alloc = b
[{:chain {:buffer b :in 220 saw} fft} keytrack 10 * 200 + sinosc .1 *] play
```

### sc.loudness

`sc.loudness (chain smask tmask --> out)`

Extraction of instantaneous loudness in sones (SC help).

Use it to measure perceived loudness in sones from an FFT chain.

```stac
1024 1 buffer.alloc = b
[{:chain {:buffer b :in pinknoise} fft} loudness 50 * 200 + sinosc .1 *] play
```

### sc.mfcc

`sc.mfcc (chain numcoeff --> out)`

Mel frequency cepstral coefficients (SC help).

Use it to describe timbre as numcoeff cepstral coefficients, for machine listening.

```stac
1024 1 buffer.alloc = b
[{:chain {:buffer b :in pinknoise} fft :numcoeff 13} mfcc 0 at 400 * 200 + sinosc .1 *] play
```

### sc.onsets

`sc.onsets (chain threshold odftype relaxtime floor mingap medianspan whtype rawodf --> out)`

Onset detector (SC help).

Use it to detect note onsets in an FFT chain; threshold sets the sensitivity.

```stac
512 1 buffer.alloc = b
[{:chain {:buffer b :in 2 dust .2 decay whitenoise *} fft :threshold .5} onsets .1 decay 800 sinosc * .1 *] play
```

### sc.peak

`sc.peak (in trig --> out)`

Track peak signal amplitude (SC help).

Use it to track the largest absolute value since the last trig.

```stac
[440 sinosc 1 lfnoise1.kr * 1 impulse.kr peak 300 * 200 + sinosc .1 *] play
```

### sc.peakfollower

`sc.peakfollower (in decay --> out)`

Track peak signal amplitude (SC help).

Use it as a peak envelope follower whose value falls by decay each sample.

```stac
[1 dust .3 decay whitenoise * .999 peakfollower 300 * 200 + sinosc .1 *] play
```

### sc.pitch

`sc.pitch (in initFreq minFreq maxFreq execFreq maxBinsPerOctave median ampThreshold peakThreshold downSample clar --> out)`

Autocorrelation pitch follower (SC help).

Use it to track the fundamental of an input; it outputs the frequency and a has-frequency flag.

```stac
[{:in 1 lfnoise0.kr 200 * 400 + sinosc} pitch 0 at saw .05 *] play
```

### sc.runningsum

`sc.runningsum (in numsamp --> out)`

Running sum over n frames (SC help).

Use it for the sum of the last numsamp samples, e.g. squared, for an RMS level.

```stac
[440 sinosc aa * 40 runningsum 40 / sqrt 300 * 200 + sinosc .1 *] play
```

### sc.slope

`sc.slope (in --> out)`

Slope of signal (SC help).

Use it for the rate of change of a signal, per second.

```stac
[1 lfnoise1.kr slope.kr 100 * 400 + sinosc .1 *] play
```

### sc.zerocrossing

`sc.zerocrossing (in --> out)`

Zero crossing frequency follower (SC help).

Use it as a cheap frequency estimate for simple waveforms.

```stac
[1 lfnoise0.kr 200 * 400 + sinosc zerocrossing saw .05 *] play
```

## ambisonics

### sc.bipanb2

`sc.bipanb2 (inA inB azimuth gain --> out)`

2D Ambisonic B-format panner (SC help).

Use it to encode a stereo pair into 2D B-format at azimuth, from -1 to 1; it outputs w, x and y.

```stac
[300 saw .1 * 301 saw .1 * .2 lfsaw.kr 1 bipanb2] play
```

### sc.decodeb2

`sc.decodeb2 (numChans w x y orientation --> out)`

2D Ambisonic B-format decoder (SC help).

Use it to decode 2D B-format w, x, y to a ring of numChans speakers.

```stac
[pinknoise .1 * .2 lfsaw.kr 1 panb2 = [w x y]  4 w x y .5 decodeb2] play
```

### sc.panb

`sc.panb (in azimuth elevation gain --> out)`

Ambisonic B-format panner (SC help).

Use it to encode a mono signal into 3D B-format at azimuth and elevation; it outputs w, x, y and z.

```stac
[pinknoise .1 * .2 lfsaw.kr 0 1 panb] play
```

### sc.panb2

`sc.panb2 (in azimuth gain --> out)`

2D Ambisonic B-format panner (SC help).

Use it to encode a mono signal into 2D B-format; decode it with decodeb2.

```stac
[pinknoise .1 * .2 lfsaw.kr 1 panb2] play
```

### sc.rotate2

`sc.rotate2 (x y pos --> out)`

Rotate a sound field (SC help).

Use it to rotate a B-format field or a stereo pair by pos, from -1 to 1 around the circle.

```stac
[300 saw .1 * 301 saw .1 * .2 lfsaw.kr rotate2] play
```

## beqsuite

### sc.ballpass

`sc.ballpass (in freq rq --> out)`

All Pass Filter (SC help).

Use it for an all-pass at freq with a resonance set by rq, e.g. in a phaser.

```stac
[110 saw 1000 1 ballpass .1 *] play
```

### sc.bbandpass

`sc.bbandpass (in freq bw --> out)`

Band Pass Filter (SC help).

Use it for a band-pass whose width bw is in octaves.

```stac
[whitenoise 1000 .3 bbandpass .2 *] play
```

### sc.bbandstop

`sc.bbandstop (in freq bw --> out)`

Band reject filter (SC help).

Use it for a notch whose width bw is in octaves.

```stac
[whitenoise 1000 1 bbandstop .1 *] play
```

### sc.bhipass

`sc.bhipass (in freq rq --> out)`

2nd order resonant high pass filter, 12 dB per octave (SC help).

Use it for a high-pass with a steeper, cleaner response than hpf; rq is 1/Q.

```stac
[110 saw 800 .5 bhipass .1 *] play
```

### sc.bhishelf

`sc.bhishelf (in freq rs db --> out)`

Hi Shelf (SC help).

Use it to boost or cut everything above freq by db; rs sets the slope.

```stac
[pinknoise .1 * 2000 1 -12 bhishelf] play
```

### sc.blowpass

`sc.blowpass (in freq rq --> out)`

2nd order resonant low pass filter, 12 dB per octave (SC help).

Use it for a low-pass with a steeper, cleaner response than lpf; rq is 1/Q.

```stac
[110 saw 800 .5 blowpass .1 *] play
```

### sc.blowshelf

`sc.blowshelf (in freq rs db --> out)`

Low Shelf (SC help).

Use it to boost or cut everything below freq by db; rs sets the slope.

```stac
[pinknoise .1 * 200 1 6 blowshelf] play
```

### sc.bpeakeq

`sc.bpeakeq (in freq rq db --> out)`

Parametric equalizer (SC help).

Use it to boost or cut a band around freq by db, a parametric equaliser band.

```stac
[pinknoise .1 * 1000 .5 6 bpeakeq] play
```

## bela

### sc.analogin

`sc.analogin (analogPin --> out)`

Read data from an analog input (SC help).

Use it on a Bela board to read a sensor on analogPin as a signal from 0 to 1.

```stac
[0 analogin 800 * 200 + sinosc .1 *] play
```

### sc.analogout

`sc.analogout (analogPin output --> out)`

Write data to an analog output (SC help).

Use it on a Bela board to write a signal to analogPin, e.g. to drive a control voltage; it has no output.

```stac
synthdef cv [0 1 lfsaw.kr analogout pop]
{} cv
```

### sc.belascopeout

`sc.belascopeout (offset channelsArray --> out)`

Bela's Oscilloscope interface (SC help).

Use it on a Bela board to send channels to Bela's oscilloscope from channel offset; it has no output.

```stac
synthdef scope [0 440 sinosc belascopeout pop]
{} scope
```

### sc.digitalio

`sc.digitalio (digitalPin output pinMode --> out)`

Read or write data to a digital pin (SC help).

Use it on a Bela board to read or write a digital pin; pinMode 0 reads, 1 writes.

```stac
[0 0 0 digitalio 440 sinosc * .1 *] play
```

### sc.digitalin

`sc.digitalin (digitalPin --> out)`

Read data from a digital input (SC help).

Use it on a Bela board to read a button or switch on digitalPin.

```stac
[0 digitalin 440 sinosc * .1 *] play
```

### sc.digitalout

`sc.digitalout (digitalPin output --> out)`

Write data to a digital output (SC help).

Use it on a Bela board to drive a digital pin, e.g. an LED; it has no output.

```stac
synthdef led [0 2 0 .5 lfpulse.kr digitalout pop]
{} led
```

### sc.multiplexanalogin

`sc.multiplexanalogin (analogPin muxChannel --> out)`

Read data from an analog input of the Bela board (SC help).

Use it on a Bela board with a multiplexer capelet to read more analog inputs than the board has.

```stac
[0 0 multiplexanalogin 800 * 200 + sinosc .1 *] play
```

## chaos

### sc.cuspl

`sc.cuspl (freq a b xi --> out)`

Cusp map chaotic generator (SC help).

Use it for a harsh chaotic tone; a and b shape the map, and linear interpolation smooths it.

```stac
[11025 1 1.9 0 cuspl .1 *] play
```

### sc.cuspn

`sc.cuspn (freq a b xi --> out)`

Cusp map chaotic generator (SC help).

Use it like cuspl with no interpolation, harsher still.

```stac
[11025 1 1.9 0 cuspn .1 *] play
```

### sc.fbsinec

`sc.fbsinec (freq im fb a c xi yi --> out)`

Feedback sine with chaotic phase indexing (SC help).

Use it for chaotic FM textures: a sine with feedback and a chaotic phase; cubic interpolation.

```stac
[11025 1 .1 1.1 .5 .1 .1 fbsinec .1 *] play
```

### sc.fbsinel

`sc.fbsinel (freq im fb a c xi yi --> out)`

Feedback sine with chaotic phase indexing (SC help).

Use it like fbsinec with linear interpolation.

```stac
[11025 1 .1 1.1 .5 .1 .1 fbsinel .1 *] play
```

### sc.fbsinen

`sc.fbsinen (freq im fb a c xi yi --> out)`

Feedback sine with chaotic phase indexing (SC help).

Use it like fbsinec with no interpolation.

```stac
[11025 1 .1 1.1 .5 .1 .1 fbsinen .1 *] play
```

### sc.gbmanl

`sc.gbmanl (freq xi yi --> out)`

Gingerbreadman map chaotic generator (SC help).

Use it for a chaotic signal from the gingerbreadman map, with linear interpolation.

```stac
[11025 1.2 2.1 gbmanl .1 *] play
```

### sc.gbmann

`sc.gbmann (freq xi yi --> out)`

Gingerbreadman map chaotic generator (SC help).

Use it like gbmanl with no interpolation.

```stac
[11025 1.2 2.1 gbmann .1 *] play
```

### sc.henonc

`sc.henonc (freq a b x0 x1 --> out)`

Henon map chaotic generator (SC help).

Use it for chaotic noise from the Henon map; a and b set the behaviour, cubic interpolation.

```stac
[11025 1.4 .3 0 0 henonc .1 *] play
```

### sc.henonl

`sc.henonl (freq a b x0 x1 --> out)`

Henon map chaotic generator (SC help).

Use it like henonc with linear interpolation.

```stac
[11025 1.4 .3 0 0 henonl .1 *] play
```

### sc.henonn

`sc.henonn (freq a b x0 x1 --> out)`

Henon map chaotic generator (SC help).

Use it like henonc with no interpolation.

```stac
[11025 1.4 .3 0 0 henonn .1 *] play
```

### sc.latoocarfianc

`sc.latoocarfianc (freq a b c d xi yi --> out)`

Latoocarfian chaotic generator (SC help).

Use it for chaotic tones from Clifford Pickover's map, with four shape parameters; cubic interpolation.

```stac
[11025 1 3 .5 .5 .5 .5 latoocarfianc .1 *] play
```

### sc.latoocarfianl

`sc.latoocarfianl (freq a b c d xi yi --> out)`

Latoocarfian chaotic generator (SC help).

Use it like latoocarfianc with linear interpolation.

```stac
[11025 1 3 .5 .5 .5 .5 latoocarfianl .1 *] play
```

### sc.latoocarfiann

`sc.latoocarfiann (freq a b c d xi yi --> out)`

Latoocarfian chaotic generator (SC help).

Use it like latoocarfianc with no interpolation.

```stac
[11025 1 3 .5 .5 .5 .5 latoocarfiann .1 *] play
```

### sc.lincongc

`sc.lincongc (freq a c m xi --> out)`

Linear congruential chaotic generator (SC help).

Use it for a linear congruential generator as a sound, from tones to noise; cubic interpolation.

```stac
[11025 1.1 .13 1 0 lincongc .1 *] play
```

### sc.lincongl

`sc.lincongl (freq a c m xi --> out)`

Linear congruential chaotic generator (SC help).

Use it like lincongc with linear interpolation.

```stac
[11025 1.1 .13 1 0 lincongl .1 *] play
```

### sc.lincongn

`sc.lincongn (freq a c m xi --> out)`

Linear congruential chaotic generator (SC help).

Use it like lincongc with no interpolation.

```stac
[11025 1.1 .13 1 0 lincongn .1 *] play
```

### sc.logistic

`sc.logistic (chaosParam freq init --> out)`

Chaotic noise function (SC help).

Use it for chaos from the logistic map; chaosParam from 3.57 to 4 is chaotic, below it periodic.

```stac
[3.9 1000 .5 logistic .1 *] play
```

### sc.lorenzl

`sc.lorenzl (freq s r b h xi yi zi --> out)`

Lorenz chaotic generator (SC help).

Use it for a signal from the Lorenz attractor; h is the step size, which sets its speed.

```stac
[11025 10 28 2.667 .05 .1 0 0 lorenzl .1 *] play
```

### sc.quadc

`sc.quadc (freq a b c xi --> out)`

General quadratic map chaotic generator (SC help).

Use it for chaos from a general quadratic map; cubic interpolation.

```stac
[11025 1 -1 -.75 0 quadc .1 *] play
```

### sc.quadl

`sc.quadl (freq a b c xi --> out)`

General quadratic map chaotic generator (SC help).

Use it like quadc with linear interpolation.

```stac
[11025 1 -1 -.75 0 quadl .1 *] play
```

### sc.quadn

`sc.quadn (freq a b c xi --> out)`

General quadratic map chaotic generator (SC help).

Use it like quadc with no interpolation.

```stac
[11025 1 -1 -.75 0 quadn .1 *] play
```

### sc.standardl

`sc.standardl (freq k xi yi --> out)`

Standard map chaotic generator (SC help).

Use it for chaos from the standard map; k near 1 is chaotic. Linear interpolation.

```stac
[11025 1 .5 0 standardl .1 *] play
```

### sc.standardn

`sc.standardn (freq k xi yi --> out)`

Standard map chaotic generator (SC help).

Use it like standardl with no interpolation.

```stac
[11025 1 .5 0 standardn .1 *] play
```

## fft

### sc.convolution

`sc.convolution (in kernel framesize --> out)`

Real-time convolver (SC help).

Use it to convolve two live signals, e.g. noise through a short pitched kernel; framesize sets the FFT size.

```stac
[whitenoise .05 * 440 sinosc 512 convolution .1 *] play
```

### sc.convolution2

`sc.convolution2 (in kernel trigger framesize --> out)`

Real-time fixed kernel convolver (SC help).

Use it to convolve a signal with an impulse response held in a buffer; trigger reloads the kernel.

```stac
2048 1 buffer.alloc = b
[whitenoise .05 * b 0 2048 convolution2] play
```

### sc.convolution2l

`sc.convolution2l (in kernel trigger framesize crossfade --> out)`

Real-time convolver with linear interpolation (SC help).

Use it like convolution2 when the kernel changes: it crossfades to the new one over crossfade periods.

```stac
2048 1 buffer.alloc = b
[whitenoise .05 * b 0 2048 1 convolution2l] play
```

### sc.convolution3

`sc.convolution3 (in kernel trigger framesize --> out)`

Time based convolver (SC help).

Use it for short kernels: it convolves in the time domain, with no FFT latency.

```stac
64 1 buffer.alloc = b
[whitenoise .05 * b 0 64 convolution3] play
```

### sc.fft

`sc.fft (buffer in hop wintype active winsize --> out)`

Fast Fourier Transform (SC help).

Use it to start a spectral chain: it fills buffer with frames of in; pv_ UGens transform the chain and ifft turns it back to sound.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 0 0 ifft] play
```

### sc.ffttrigger

`sc.ffttrigger (buffer hop polar --> out)`

Outputs the necessary signal for FFT chains, without doing an FFT on a signal (SC help).

Use it to drive a spectral chain built by hand, e.g. with packfft, without analysing a signal.

```stac
2048 1 buffer.alloc = b
b .5 0 ffttrigger    ;-> FFTTrigger.kr
```

### sc.ifft

`sc.ifft (buffer wintype winsize --> out)`

Inverse Fast Fourier Transform (SC help).

Use it to end a spectral chain and resynthesise sound; wintype and winsize should match the fft.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .5 pv_brickwall 0 0 ifft] play
```

### sc.pv_add

`sc.pv_add (bufferA bufferB --> out)`

Complex addition (SC help).

Use it to sum two spectra; the result plays both sources.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_add 0 0 ifft] play
```

### sc.pv_binscramble

`sc.pv_binscramble (buffer wipe width trig --> out)`

Scramble bins (SC help).

Use it for glitchy spectral rearrangement; trig picks a new random order.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .5 .2 2 impulse.kr pv_binscramble 0 0 ifft] play
```

### sc.pv_binshift

`sc.pv_binshift (buffer stretch shift interp --> out)`

Shift and stretch bin position (SC help).

Use it to stretch and shift the spectrum, which moves partials out of harmonic ratios.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 1.5 0 0 pv_binshift 0 0 ifft] play
```

### sc.pv_binwipe

`sc.pv_binwipe (bufferA bufferB wipe --> out)`

Combine low and high bins from two inputs (SC help).

Use it to splice the low bins of one spectrum with the high bins of another; wipe moves the split.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft .3 pv_binwipe 0 0 ifft] play
```

### sc.pv_brickwall

`sc.pv_brickwall (buffer wipe --> out)`

Zero bins (SC help).

Use it as an ideal spectral filter: positive wipe cuts the low bins, negative wipe the high ones.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .5 pv_brickwall 0 0 ifft] play
```

### sc.pv_conformalmap

`sc.pv_conformalmap (buffer areal aimag --> out)`

Complex plane attack (SC help).

Use it for an unusual spectral distortion that maps the complex plane.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .5 .5 pv_conformalmap 0 0 ifft] play
```

### sc.pv_conj

`sc.pv_conj (buffer --> out)`

Complex conjugate (SC help).

Use it to conjugate each bin, which reverses phases; on its own the change is subtle.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft pv_conj 0 0 ifft] play
```

### sc.pv_copy

`sc.pv_copy (bufferA bufferB --> out)`

Copy an FFT buffer (SC help).

Use it to branch a chain: it copies one FFT buffer into another, so two transforms can follow the same analysis.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft c pv_copy 0 0 ifft] play
```

### sc.pv_copyphase

`sc.pv_copyphase (bufferA bufferB --> out)`

Copy magnitudes and phases (SC help).

Use it for cross-synthesis: magnitudes from the first chain, phases from the second.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_copyphase 0 0 ifft] play
```

### sc.pv_diffuser

`sc.pv_diffuser (buffer trig --> out)`

Random phase shifting (SC help).

Use it to smear transients by randomising phases; trig picks new ones.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 1 impulse.kr pv_diffuser 0 0 ifft] play
```

### sc.pv_div

`sc.pv_div (bufferA bufferB --> out)`

Complex division (SC help).

Use it to divide one spectrum by another, a crude deconvolution.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_div 0 0 ifft] play
```

### sc.pv_hainsworthfoote

`sc.pv_hainsworthfoote (buffer proph propf threshold waittime --> out)`

FFT onset detector (SC help).

Use it to detect onsets in a spectral chain; it outputs a trigger at each one.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 1 0 .9 .5 pv_hainsworthfoote .1 decay 800 sinosc * .1 *] play
```

### sc.pv_jensenandersen

`sc.pv_jensenandersen (buffer propsc prophfe prophfc propsf threshold waittime --> out)`

FFT feature detector for onset detection (SC help).

Use it to detect onsets by four weighted spectral features; it outputs a trigger.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .25 .25 .25 .25 1 .04 pv_jensenandersen .1 decay 800 sinosc * .1 *] play
```

### sc.pv_localmax

`sc.pv_localmax (buffer threshold --> out)`

Pass bins which are a local maximum (SC help).

Use it to keep only spectral peaks above threshold, a sinusoidal skeleton of the sound.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 10 pv_localmax 0 0 ifft] play
```

### sc.pv_magabove

`sc.pv_magabove (buffer threshold --> out)`

Pass bins above a threshold (SC help).

Use it as a spectral gate: bins below threshold go to 0.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 10 pv_magabove 0 0 ifft] play
```

### sc.pv_magbelow

`sc.pv_magbelow (buffer threshold --> out)`

Pass bins below a threshold (SC help).

Use it to keep only the quiet bins, the residue under a sound's peaks.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 10 pv_magbelow 0 0 ifft] play
```

### sc.pv_magclip

`sc.pv_magclip (buffer threshold --> out)`

Clip bins to a threshold (SC help).

Use it to flatten loud bins to threshold, a spectral limiter.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 5 pv_magclip 0 0 ifft] play
```

### sc.pv_magdiv

`sc.pv_magdiv (bufferA bufferB zeroed --> out)`

Division of magnitudes (SC help).

Use it to divide magnitudes, which whitens one spectrum by another; zeroed sets the smallest divisor.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft .0001 pv_magdiv 0 0 ifft] play
```

### sc.pv_magfreeze

`sc.pv_magfreeze (buffer freeze --> out)`

Freeze magnitudes (SC help).

Use it to hold a spectrum while freeze is positive, a spectral sustain.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 1 0 .5 lfpulse.kr pv_magfreeze 0 0 ifft] play
```

### sc.pv_magmul

`sc.pv_magmul (bufferA bufferB --> out)`

Multiply magnitudes (SC help).

Use it for cross-synthesis by multiplying magnitudes, a vocoder-like effect.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_magmul 0 0 ifft .1 *] play
```

### sc.pv_magnoise

`sc.pv_magnoise (buffer --> out)`

Multiply magnitudes by noise (SC help).

Use it to roughen a spectrum by random magnitudes.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft pv_magnoise 0 0 ifft] play
```

### sc.pv_magshift

`sc.pv_magshift (buffer stretch shift --> out)`

Shift and stretch magnitude bin position (SC help).

Use it like pv_binshift on magnitudes only, keeping the phases in place.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 1.5 0 pv_magshift 0 0 ifft] play
```

### sc.pv_magsmear

`sc.pv_magsmear (buffer bins --> out)`

Average magnitudes across bins (SC help).

Use it to blur a spectrum across bins neighbours.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 5 pv_magsmear 0 0 ifft] play
```

### sc.pv_magsquared

`sc.pv_magsquared (buffer --> out)`

Square magnitudes (SC help).

Use it to exaggerate spectral peaks by squaring magnitudes; scale the output down.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft pv_magsquared 0 0 ifft .01 *] play
```

### sc.pv_max

`sc.pv_max (bufferA bufferB --> out)`

Maximum magnitude (SC help).

Use it to keep the louder of two spectra in each bin.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_max 0 0 ifft] play
```

### sc.pv_min

`sc.pv_min (bufferA bufferB --> out)`

Minimum magnitude (SC help).

Use it to keep the quieter of two spectra in each bin, which leaves what they share.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_min 0 0 ifft] play
```

### sc.pv_mul

`sc.pv_mul (bufferA bufferB --> out)`

Complex multiply (SC help).

Use it to multiply two spectra as complex numbers, the spectral form of convolution.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft pv_mul 0 0 ifft .1 *] play
```

### sc.pv_phaseshift

`sc.pv_phaseshift (buffer shift integrate --> out)`

Shift phase (SC help).

Use it to rotate every bin's phase by shift radians; integrate accumulates the shift per frame.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 1.57 0 pv_phaseshift 0 0 ifft] play
```

### sc.pv_phaseshift270

`sc.pv_phaseshift270 (buffer --> out)`

Shift phase by 270 degrees (SC help).

Use it for a fixed quarter-turn back, with pv_phaseshift90 its opposite.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft pv_phaseshift270 0 0 ifft] play
```

### sc.pv_phaseshift90

`sc.pv_phaseshift90 (buffer --> out)`

Shift phase by 90 degrees (SC help).

Use it for a fixed quarter-turn of phase, e.g. to build a quadrature pair.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft pv_phaseshift90 0 0 ifft] play
```

### sc.pv_randcomb

`sc.pv_randcomb (buffer wipe trig --> out)`

Pass random bins (SC help).

Use it to drop a random fraction wipe of the bins; trig picks new ones.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .5 2 impulse.kr pv_randcomb 0 0 ifft] play
```

### sc.pv_randwipe

`sc.pv_randwipe (bufferA bufferB wipe trig --> out)`

Crossfade in random bin order (SC help).

Use it to crossfade two spectra bin by bin in random order; wipe runs from 0 to 1.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft .5 lfnoise1.kr .5 * .5 + 0 pv_randwipe 0 0 ifft] play
```

### sc.pv_rectcomb

`sc.pv_rectcomb (buffer numTeeth phase width --> out)`

Make gaps in spectrum (SC help).

Use it to notch the spectrum with numTeeth gaps, a spectral comb.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft 8 0 .5 pv_rectcomb 0 0 ifft] play
```

### sc.pv_rectcomb2

`sc.pv_rectcomb2 (bufferA bufferB numTeeth phase width --> out)`

Make gaps in spectrum (SC help).

Use it like pv_rectcomb to alternate bins between two spectra.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[{:buffer b :in whitenoise .1 *} fft {:buffer c :in 220 saw .1 *} fft 8 0 .5 pv_rectcomb2 0 0 ifft] play
```

### sc.packfft

`sc.packfft (chain bufsize magsphases frombin tobin zeroothers --> out)`

Pack separate demand-rate FFT bin streams into an FFT chain buffer (SC help).

Use it to write magnitudes and phases from demand streams into a chain, so a spectrum can be built by hand.

```stac
2048 1 buffer.alloc = b
[b .5 0 ffttrigger 2048 [1 0 .5 0] 0 1 1 packfft 0 0 ifft] play
```

### sc.partconv

`sc.partconv (in fftsize irbufnum --> out)`

Real-time partitioned convolution (SC help).

Use it for long reverb impulse responses: it convolves in partitions of fftsize. The buffer must hold the prepared response.

```stac
2048 1 buffer.alloc = b
[whitenoise .05 * 1024 b partconv] play
```

### sc.speccentroid

`sc.speccentroid (buffer --> out)`

Spectral centroid (SC help).

Use it to measure brightness: the spectral centre of mass, in Hz.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft speccentroid .001 * 200 * 200 + sinosc .1 *] play
```

### sc.specflatness

`sc.specflatness (buffer --> out)`

Spectral Flatness measure (SC help).

Use it to tell noise from tone: near 1 for noise, near 0 for a pure tone.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft specflatness 400 * 200 + sinosc .1 *] play
```

### sc.specpcile

`sc.specpcile (buffer fraction interpolate binout --> out)`

Find a percentile of FFT magnitude spectrum (SC help).

Use it to find the frequency below which fraction of the spectral energy lies, e.g. .9 for a rolloff point.

```stac
2048 1 buffer.alloc = b
[{:buffer b :in whitenoise .1 *} fft .9 0 0 specpcile .1 * 200 + sinosc .1 *] play
```

### sc.stereoconvolution2l

`sc.stereoconvolution2l (in kernelL kernelR trigger framesize crossfade --> out)`

Stereo real-time convolver with linear interpolation (SC help).

Use it like convolution2l with two kernels, for a stereo impulse response.

```stac
2048 1 buffer.alloc = b  2048 1 buffer.alloc = c
[whitenoise .05 * b c 0 2048 1 stereoconvolution2l] play
```

### sc.unpack1fft

`sc.unpack1fft (chain bufsize binindex whichmeasure --> out)`

Unpack a single value (magnitude or phase) from an FFT chain (SC help).

Use it to read one bin's magnitude or phase from a chain as a demand stream.

```stac
2048 1 buffer.alloc = b
[8 impulse.kr 0 {:buffer b :in whitenoise .1 *} fft 2048 10 0 unpack1fft demand.kr 200 + sinosc .1 *] play
```

## granular

### sc.grainfm

`sc.grainfm (numChannels trigger dur carfreq modfreq index pan envbufnum maxGrains --> out)`

Granular synthesis with frequency modulated sine tones (SC help).

Use it for clouds of short FM grains; each trigger starts one with the current carfreq, modfreq and index.

```stac
[{:numChannels 1 :trigger 20 impulse :dur .1 :carfreq 440 :modfreq 200 :index 2} grainfm .1 *] play
```

### sc.grainin

`sc.grainin (numChannels trigger dur in pan envbufnum maxGrains --> out)`

Granulate an input signal (SC help).

Use it to granulate a live input: each trigger takes a grain of dur seconds.

```stac
[{:numChannels 1 :trigger 20 impulse :dur .05 :in 220 saw} grainin .1 *] play
```

### sc.grainsin

`sc.grainsin (numChannels trigger dur freq pan envbufnum maxGrains --> out)`

Granular synthesis with sine tones (SC help).

Use it for clouds of sine grains, the simplest granular texture.

```stac
[{:numChannels 1 :trigger 30 dust :dur .05 :freq 1 lfnoise1.kr 400 * 800 +} grainsin .1 *] play
```

## info

### sc.blocksize

`sc.blocksize ( --> out)`

The number of samples in one control block, 64 by default.

Use it to convert between control blocks and samples inside a graph.

```stac
blocksize            ;-> BlockSize.ir
```

### sc.checkbadvalues

`sc.checkbadvalues (in id post --> out)`

Test for infinity, not-a-number, and denormals (SC help).

Use it while debugging a graph that blows up: it reports infinities and NaNs in in, tagged with id.

```stac
[440 sinosc 0 2 checkbadvalues .1 *] play
```

### sc.controldur

`sc.controldur ( --> out)`

Duration of one block (SC help).

Use it for the length of one control block in seconds, e.g. to scale a per-block increment.

```stac
controldur           ;-> ControlDur.ir
```

### sc.controlrate

`sc.controlrate ( --> out)`

Server control rate (SC help).

Use it for the number of control blocks per second, 750 at 48 kHz.

```stac
controlrate          ;-> ControlRate.ir
```

### sc.nodeid

`sc.nodeid ( --> out)`

The ID of the synth the graph runs in.

Use it to tag messages a synth sends back, e.g. with sendtrig, so the client knows which synth sent them.

```stac
nodeid               ;-> NodeID.ir
```

### sc.numaudiobuses

`sc.numaudiobuses ( --> out)`

Number of audio busses (SC help).

Use it to check a bus index against the server's limit.

```stac
numaudiobuses        ;-> NumAudioBuses.ir
```

### sc.numbuffers

`sc.numbuffers ( --> out)`

Number of open buffers (SC help).

Use it to check a buffer index against the server's limit.

```stac
numbuffers           ;-> NumBuffers.ir
```

### sc.numcontrolbuses

`sc.numcontrolbuses ( --> out)`

Number of control busses (SC help).

Use it to check a control bus index against the server's limit.

```stac
numcontrolbuses      ;-> NumControlBuses.ir
```

### sc.numinputbuses

`sc.numinputbuses ( --> out)`

Number of input busses (SC help).

Use it to find the first private bus: input buses follow the output buses.

```stac
numinputbuses        ;-> NumInputBuses.ir
```

### sc.numoutputbuses

`sc.numoutputbuses ( --> out)`

Number of output busses (SC help).

Use it to adapt a graph to the number of hardware outputs.

```stac
numoutputbuses       ;-> NumOutputBuses.ir
```

### sc.numrunningsynths

`sc.numrunningsynths ( --> out)`

Number of currently running synths (SC help).

Use it to watch server load, e.g. to thin a texture when many synths run.

```stac
numrunningsynths     ;-> NumRunningSynths.kr
```

### sc.radianspersample

`sc.radianspersample ( --> out)`

Number of radians per sample (SC help).

Use it to turn a frequency into a phase increment per sample in hand-built oscillators.

```stac
radianspersample     ;-> RadiansPerSample.ir
```

### sc.sampledur

`sc.sampledur ( --> out)`

Duration of one sample (SC help).

Use it for the length of one sample in seconds, e.g. as phasor's rate for a ramp of 1 per second.

```stac
sampledur            ;-> SampleDur.ir
```

### sc.samplerate

`sc.samplerate ( --> out)`

Server sample rate (SC help).

Use it to scale frequencies or lengths to the server's rate, e.g. half of it is the Nyquist frequency.

```stac
samplerate           ;-> SampleRate.ir
```

### sc.sanitize

`sc.sanitize (in replace --> out)`

Remove infinity, NaN, and denormals (SC help).

Use it after a feedback loop that can blow up: it replaces infinities, NaNs and denormals with replace.

```stac
[440 sinosc 0 sanitize .1 *] play
```

### sc.subsampleoffset

`sc.subsampleoffset ( --> out)`

Offset from synth start within one sample (SC help).

Use it with offsetout for sample-accurate grain onsets within a block.

```stac
subsampleoffset      ;-> SubsampleOffset.ir
```

## io

### sc.diskin

`sc.diskin (numChannels bufnum loop --> out)`

Stream in audio from a file (SC help).

Use it to stream a long sound file from disk; the buffer must be cued with the file before the synth starts.

```stac
32768 1 buffer.alloc = b
[1 b 1 diskin .1 *] play
```

### sc.diskout

`sc.diskout (bufnum channelsArray --> out)`

Record to a soundfile to disk (SC help).

Use it to record channels to a sound file on disk through a buffer opened for writing.

```stac
32768 1 buffer.alloc = b
[b 440 sinosc .1 * diskout] play
```

### sc.infeedback

`sc.infeedback (bus numChannels --> out)`

Read signal from a bus with a current or one cycle old timestamp (SC help).

Use it to read a bus that a synth later in the order writes, e.g. for feedback between synths; nd.ar reads an ndef this way.

```stac
1 bus.audio = b
[b 1 infeedback .5 * 440 sinosc .05 * +] play
```

### sc.intrig

`sc.intrig (bus numChannels --> out)`

Generate a trigger anytime a bus is set (SC help).

Use it to get a trigger whenever a control bus is set, e.g. by b.set.

```stac
1 bus.control = b
[b 1 intrig .1 decay 800 sinosc * .1 *] play
```

### sc.lagin

`sc.lagin (bus numChannels lag --> out)`

Read a control signal from a bus with a lag (SC help).

Use it to read a control bus with a lag, so values set by b.set glide.

```stac
1 bus.control = b
[b 1 .2 lagin 400 * 200 + sinosc .1 *] play
```

### sc.localin

`sc.localin (numChannels default --> out)`

Define and read from buses local to a synth (SC help).

Use it with localout for feedback inside one synth; it reads what localout wrote one block earlier.

```stac
synthdef fb [1 0 localin = x  2 impulse .1 decay whitenoise * x .9 * + = y  y localout pop  y .2 *]
{} fb
```

### sc.localout

`sc.localout (channelsArray --> out)`

Write to buses local to a synth (SC help).

Use it to write the feedback signal that localin reads; it has no output, so use it in a synthdef.

```stac
synthdef fb2 [1 0 localin .95 * .1 dust .1 * + = y  y localout pop  y .1 *]
{} fb2
```

### sc.offsetout

`sc.offsetout (bus channelsArray --> out)`

Write a signal to a bus with sample accurate timing (SC help).

Use it like out when a grain must start on its exact sample, not at the next block.

```stac
synthdef grain2 [0 .01 .05 env.perc 1 envgen.kr.done 1200 sinosc * offsetout]
{} grain2
```

### sc.replaceout

`sc.replaceout (bus channelsArray --> out)`

Send signal to a bus, overwriting previous contents (SC help).

Use it in an effect that processes a bus in place: it overwrites the bus instead of adding to it.

```stac
synthdef quieter bus [bus bus 2 in .5 * replaceout pop]
{:bus 0 :addAction 'tail} quieter
```

### sc.scopeout2

`sc.scopeout2 (inputArray scopeNum maxFrames scopeFrames --> out)`

Writes channels to a scope buffer for SuperCollider's oscilloscope.

Use it to feed a scope window when one is open in sclang.

```stac
[440 sinosc 0 1024 1024 scopeout2] play
```

### sc.sharedin

`sc.sharedin (bus numChannels --> out)`

Read from a shared control bus (SC help).

Use it to read a control bus shared with the client in an internal server; stac's servers do not share memory, so use in.

```stac
[0 1 sharedin 400 * 200 + sinosc .1 *] play
```

### sc.sharedout

`sc.sharedout (bus channelsArray --> out)`

Write to a shared control bus (SC help).

Use it to write a control bus shared with the client in an internal server; it has no output.

```stac
synthdef shout [0 1 lfnoise1.kr sharedout pop 440 sinosc .1 *]
{} shout
```

### sc.vdiskin

`sc.vdiskin (numChannels bufnum rate loop sendID --> out)`

Stream in audio from a file, with variable rate (SC help).

Use it like diskin with a variable playback rate.

```stac
32768 1 buffer.alloc = b
[1 b 1 0 0 vdiskin .1 *] play
```

### sc.xout

`sc.xout (bus xfade channelsArray --> out)`

Send signal to a bus, crossfading with previous contents (SC help).

Use it in an effect to crossfade between the bus's input and the effect, xfade from 0 to 1.

```stac
synthdef wet bus [bus bus 1 in 800 lpf = x  bus .5 x xout pop]
{:bus 0 :addAction 'tail} wet
```

## node

### sc.detectsilence

`sc.detectsilence (in amp time --> out)`

Detect when input falls below an amplitude threshold (SC help).

Use it to free a synth once its sound has died away: with .done it frees the synth after amp stays low for time seconds.

```stac
[1 impulse .3 decay 600 sinosc * .2 * = s  s s .001 .2 detectsilence.done 0 * +] play
```

### sc.done

`sc.done (src --> out)`

Monitors another UGen to see when it is finished (SC help).

Use it to get 1 once a UGen with a doneAction, such as line, has finished.

```stac
[200 800 1 line = l  l done 100 * l + sinosc .1 *] play
```

### sc.free

`sc.free (trig id --> out)`

When triggered, frees a node (SC help).

Use it to free another node, by id, when trig turns positive.

```stac
[1 impulse.kr 2000000 free 0 * 440 sinosc .1 * +] play
```

### sc.freeself

`sc.freeself (in --> out)`

When triggered, free enclosing synth (SC help).

Use it to end the synth when in turns positive, e.g. after a given time.

```stac
[2 0 .5 lfpulse.kr freeself 0 * 440 sinosc .1 * +] play
```

### sc.freeselfwhendone

`sc.freeselfwhendone (src --> out)`

Free the enclosing synth when a UGen is finished (SC help).

Use it to free the synth when a UGen such as line finishes, where that UGen has no doneAction of its own.

```stac
[1 2 1 line = l  l freeselfwhendone 0 * 440 sinosc l * .1 * +] play
```

### sc.pause

`sc.pause (gate id --> out)`

When triggered, pauses a node (SC help).

Use it to pause or resume another node, by id: gate 0 pauses, 1 resumes.

```stac
[1 0 .5 lfpulse.kr 2000000 pause 0 * 440 sinosc .1 * +] play
```

### sc.pauseself

`sc.pauseself (in --> out)`

When triggered, pause enclosing synth (SC help).

Use it to pause the synth when in turns positive; set it running again from the client.

```stac
[2 0 .5 lfpulse.kr pauseself 0 * 440 sinosc .1 * +] play
```

### sc.pauseselfwhendone

`sc.pauseselfwhendone (src --> out)`

Pauses the enclosing synth when src, a UGen with a done flag, has finished.

Use it like freeselfwhendone when the synth should stay paused for reuse.

```stac
[1 2 1 line = l  l pauseselfwhendone 0 * 440 sinosc l * .1 * +] play
```

## random

### sc.exprand

`sc.exprand (lo hi --> out)`

Exponential single random number generator (SC help).

Use it for one random value per synth with an exponential distribution, the right choice for frequencies.

```stac
[200 2000 exprand sinosc .1 *] play
```

### sc.irand

`sc.irand (lo hi --> out)`

Single integer random number generator (SC help).

Use it for one random integer per synth, from lo to hi. The core word irand draws one when the graph is built; sc.irand draws a new one for each synth.

```stac
[0 12 sc.irand 60 + nnhz saw .05 *] play
```

### sc.linrand

`sc.linrand (lo hi minmax --> out)`

Skewed random number generator (SC help).

Use it for one random value per synth skewed toward lo (minmax 0) or hi (minmax 1). The core word linrand draws one when the graph is built.

```stac
[200 800 0 sc.linrand sinosc .1 *] play
```

### sc.nrand

`sc.nrand (lo hi n --> out)`

Sum of uniform distributions (SC help).

Use it for one random value per synth from the sum of n uniform draws, which centres the values for larger n.

```stac
[200 800 3 nrand sinosc .1 *] play
```

### sc.rand

`sc.rand (lo hi --> out)`

Single random number generator (SC help).

Use it for one random value per synth, drawn when it starts. The core word rand draws one when the graph is built, so every synth shares it.

```stac
[200 800 sc.rand sinosc .1 *] play
200 800 sc.rand         ;-> Rand.ir
```

### sc.randid

`sc.randid (id --> out)`

Set the synth's random generator ID (SC help).

Use it to pick which of the server's random generators the synth's random UGens use, so synths can share or separate their sequences.

```stac
[1 randid 0 * 1 lfnoise0.kr 300 * 400 + sinosc .1 * +] play
```

### sc.randseed

`sc.randseed (trig seed --> out)`

Sets the synth's random generator seed (SC help).

Use it to reset a random generator to seed at each trig, so a random sequence repeats.

```stac
[1 impulse.kr 42 randseed 0 * 8 lfnoise0.kr 300 * 400 + sinosc .1 * +] play
```

### sc.texprand

`sc.texprand (lo hi trig --> out)`

Triggered exponential random number generator (SC help).

Use it for a new exponentially distributed random value at each trigger.

```stac
[200 2000 4 impulse.kr texprand sinosc .1 *] play
```

### sc.tirand

`sc.tirand (lo hi trig --> out)`

Triggered integer random number generator (SC help).

Use it for a new random integer at each trigger, e.g. a scale step per beat.

```stac
[0 12 4 impulse.kr tirand 60 + nnhz saw .05 *] play
```

### sc.trand

`sc.trand (lo hi trig --> out)`

Triggered random number generator (SC help).

Use it for a new random value at each trigger.

```stac
[200 800 4 impulse.kr trand sinosc .1 *] play
```

### sc.twindex

`sc.twindex (in array normalize --> out)`

Triggered windex (SC help).

Use it to choose an index from a list of weights at each trigger; normalize 1 scales the weights to sum to 1.

```stac
[4 impulse.kr [.5 .3 .2] 1 twindex 100 * 300 + sinosc .1 *] play
```

## ui

### sc.keystate

`sc.keystate (keycode minval maxval lag --> out)`

Respond to the state of a key (SC help).

Use it to switch a value by a computer key, keycode, between minval and maxval; it works where the server has a display.

```stac
[0 0 1 .1 keystate 440 sinosc * .1 *] play
```

### sc.mousebutton

`sc.mousebutton (minval maxval lag --> out)`

Mouse button UGen (SC help).

Use it to switch a value by the mouse button between minval and maxval; it works where the server has a display.

```stac
[0 1 .1 mousebutton 440 sinosc * .1 *] play
```

### sc.mousex

`sc.mousex (minval maxval warp lag --> out)`

Cursor tracking UGen (SC help).

Use it to play a parameter with the mouse's horizontal position; warp 1 maps exponentially.

```stac
[200 2000 1 .1 mousex sinosc .1 *] play
```

### sc.mousey

`sc.mousey (minval maxval warp lag --> out)`

Cursor tracking UGen (SC help).

Use it to play a parameter with the mouse's vertical position.

```stac
[200 2000 1 .1 mousey sinosc .1 *] play
```
