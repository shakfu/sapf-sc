# std

The module std, stac's standard library; prelude.stac imports it.

## range mapping

### std.+-

`std.+- (a b --> sig sig)`

Converts a centre and a deviation to the high and low bounds: c + d, c - d.

Use it where a word takes the high bound first.

```stac
1 .2 +-           ;-> 1.2 0.8
```

### std.-+

`std.-+ (a b --> sig sig)`

Converts a centre and a deviation to the low and high bounds: c - d, c + d.

Use it to state a range as a value plus or minus a spread.

```stac
1 .2 -+           ;-> 0.8 1.2
```

### std.MS

`std.MS (a b --> sig sig)`

Converts left and right to mid and side, each scaled by `hrt`. Applied twice, it returns the input.

Use it to process the centre and the width of a stereo signal separately.

```stac
.5 .2 MS          ;-> 0.494975 0.212132
.7 .3 MS MS       ;-> 0.7 0.3
```

### std.biexp

`std.biexp (in lo hi --> sig)`

Maps a bipolar input exponentially to lo to hi.

Use it to sweep a frequency with an LFO so that equal swings sound equal.

```stac
0 100 400 biexp   ;-> 200
```

### std.biexpc

`std.biexpc (in lo hi --> sig)`

Maps a bipolar input exponentially to lo to hi, clipping the input to -1..1 first.

Use it when the swept frequency must stay inside its range.

```stac
2 100 400 biexpc  ;-> 400
```

### std.bilin

`std.bilin (in lo hi --> sig)`

Maps a bipolar input, -1 to 1, linearly to lo to hi. It does not clip.

Use it to scale an oscillator's output, such as an LFO, to a parameter's range.

```stac
0 10 20 bilin     ;-> 15
2 sinosc.kr 100 200 bilin  ; an LFO from 100 to 200
```

### std.bilinc

`std.bilinc (in lo hi --> sig)`

Maps a bipolar input linearly to lo to hi, clipping the input to -1..1 first.

Use it when the input may exceed -1..1 and the output must stay in range.

```stac
-2 10 20 bilinc   ;-> 10
```

### std.cdv

`std.cdv (lo hi --> sig sig)`

Converts low and high bounds to a centre and a deviation; the inverse of `-+`.

Use it to drive an oscillator around the middle of a range.

```stac
.8 1.2 cdv        ;-> 1 0.2
```

### std.divmul

`std.divmul (a b --> sig sig)`

Leaves a / b, then a * b.

Use it where the lower value must come first.

```stac
6 3 divmul        ;-> 2 18
```

### std.expexp

`std.expexp (in a b c d --> sig)`

Maps the exponential interval a..b to the exponential interval c..d.

Use it to map one frequency range onto another, keeping ratios.

```stac
200 100 400 1 16 expexp  ;-> 4
```

### std.expexpc

`std.expexpc (in a b c d --> sig)`

Maps a..b to c..d, both exponential, clipping the input to a..b first.

Use it when the input frequency may leave its range.

```stac
800 100 400 1 16 expexpc  ;-> 16
```

### std.explin

`std.explin (in a b c d --> sig)`

Maps the exponential interval a..b to the linear interval c..d.

Use it to turn a frequency into a position, such as a fader setting.

```stac
200 100 400 0 10 explin  ;-> 5
```

### std.explinc

`std.explinc (in a b c d --> sig)`

Maps a..b exponentially to c..d linearly, clipping the input to a..b first.

Use it when the frequency may leave its range and the position must not.

```stac
800 100 400 0 10 explinc  ;-> 10
```

### std.linexp

`std.linexp (in a b c d --> sig)`

Maps the linear interval a..b to the exponential interval c..d.

Use it to turn a linear control into a frequency.

```stac
5 0 10 100 400 linexp  ;-> 200
```

### std.linexpc

`std.linexpc (in a b c d --> sig)`

Maps a..b linearly to c..d exponentially, clipping the input to a..b first.

Use it when the control may overshoot and the frequency must stay in range.

```stac
15 0 10 100 400 linexpc  ;-> 400
```

### std.linlin

`std.linlin (in a b c d --> sig)`

Maps the interval a..b linearly to c..d. It does not clip.

Use it to convert between two linear scales, such as MIDI velocity to amplitude.

```stac
5 0 10 0 1 linlin ;-> 0.5
64 0 127 0 1 linlin  ;-> 0.503937
```

### std.linlinc

`std.linlinc (in a b c d --> sig)`

Maps a..b linearly to c..d, clipping the input to a..b first.

Use it when the input may leave its range and the output must not.

```stac
15 0 10 0 1 linlinc  ;-> 1
```

### std.muldiv

`std.muldiv (a b --> sig sig)`

Leaves a * b, then a / b.

Use it to make two values a ratio apart around a, such as two detuned partials.

```stac
6 3 muldiv        ;-> 18 2
```

### std.uniexp

`std.uniexp (in lo hi --> sig)`

Maps a unipolar input exponentially to lo to hi. lo and hi must have the same sign.

Use it for frequencies and other ranges heard in ratios, so equal steps sound equal.

```stac
.5 100 400 uniexp ;-> 200
```

### std.uniexpc

`std.uniexpc (in lo hi --> sig)`

Maps a unipolar input exponentially to lo to hi, clipping the input to 0..1 first.

Use it for a cutoff or frequency that must stay inside its range.

```stac
2 100 400 uniexpc ;-> 400
```

### std.unilin

`std.unilin (in lo hi --> sig)`

Maps a unipolar input, 0 to 1, linearly to lo to hi. It does not clip.

Use it to scale a 0..1 control, such as an envelope, to a parameter's range.

```stac
.5 10 20 unilin   ;-> 15
1.5 10 20 unilin  ;-> 25
```

### std.unilinc

`std.unilinc (in lo hi --> sig)`

Maps a unipolar input linearly to lo to hi, clipping the input to 0..1 first.

Use it when the input may overshoot and the output must stay in range.

```stac
1.5 10 20 unilinc ;-> 20
```

## common counts

### std.-1s

`std.-1s (n --> list)`

Makes a list of n minus ones.

Use it as a constant row of values, such as initial levels or weights.

```stac
4 -1s              ;-> [-1 -1 -1 -1]
```

### std.-1z

`std.-1z (n --> list)`

Makes a signal of n minus ones.

Use it where a word needs a constant signal, such as the padding `padz` adds.

```stac
4 -1z              ;-> #[-1 -1 -1 -1]
```

### std.0s

`std.0s (n --> list)`

Makes a list of n zeroes.

Use it as a constant row of values, such as initial levels or weights.

```stac
4 0s              ;-> [0 0 0 0]
```

### std.0z

`std.0z (n --> list)`

Makes a signal of n zeroes.

Use it where a word needs a constant signal, such as the padding `padz` adds.

```stac
4 0z              ;-> #[0 0 0 0]
```

### std.1N

`std.1N (a --> list)`

Keeps the first 1 item of a list: `1 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 1N            ;-> [0]
```

### std.1N>

`std.1N> (a --> list)`

Drops the first 1 item of a list: `1 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 1N>  ;-> [2 3 4 5 6 7 8 9 10]
```

### std.1s

`std.1s (n --> list)`

Makes a list of n ones.

Use it as a constant row of values, such as initial levels or weights.

```stac
4 1s              ;-> [1 1 1 1]
```

### std.1z

`std.1z (n --> list)`

Makes a signal of n ones.

Use it where a word needs a constant signal, such as the padding `padz` adds.

```stac
4 1z              ;-> #[1 1 1 1]
```

### std.2N

`std.2N (a --> list)`

Keeps the first 2 items of a list: `2 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 2N            ;-> [0 1]
```

### std.2N>

`std.2N> (a --> list)`

Drops the first 2 items of a list: `2 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 2N>  ;-> [3 4 5 6 7 8 9 10]
```

### std.2X

`std.2X (a --> list)`

Makes a list of 2 copies of its argument: `2 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 2X              ;-> [7 7]
```

### std.2XZ

`std.2XZ (a --> list)`

Makes a signal of 2 copies of its argument: `2 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 2XZ             ;-> #[7 7]
```

### std.2s

`std.2s (n --> list)`

Makes a list of n twos.

Use it as a constant row of values, such as initial levels or weights.

```stac
4 2s              ;-> [2 2 2 2]
```

### std.2z

`std.2z (n --> list)`

Makes a signal of n twos.

Use it where a word needs a constant signal, such as the padding `padz` adds.

```stac
4 2z              ;-> #[2 2 2 2]
```

### std.3N

`std.3N (a --> list)`

Keeps the first 3 items of a list: `3 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 3N            ;-> [0 1 2]
```

### std.3N>

`std.3N> (a --> list)`

Drops the first 3 items of a list: `3 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 3N>  ;-> [4 5 6 7 8 9 10]
```

### std.3X

`std.3X (a --> list)`

Makes a list of 3 copies of its argument: `3 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 3X              ;-> [7 7 7]
```

### std.3XZ

`std.3XZ (a --> list)`

Makes a signal of 3 copies of its argument: `3 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 3XZ             ;-> #[7 7 7]
```

### std.4N

`std.4N (a --> list)`

Keeps the first 4 items of a list: `4 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 4N            ;-> [0 1 2 3]
```

### std.4N>

`std.4N> (a --> list)`

Drops the first 4 items of a list: `4 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 4N>  ;-> [5 6 7 8 9 10]
```

### std.4X

`std.4X (a --> list)`

Makes a list of 4 copies of its argument: `4 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 4X              ;-> [7 7 7 7]
```

### std.4XZ

`std.4XZ (a --> list)`

Makes a signal of 4 copies of its argument: `4 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 4XZ             ;-> #[7 7 7 7]
```

### std.5N

`std.5N (a --> list)`

Keeps the first 5 items of a list: `5 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 5N            ;-> [0 1 2 3 4]
```

### std.5N>

`std.5N> (a --> list)`

Drops the first 5 items of a list: `5 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 5N>  ;-> [6 7 8 9 10]
```

### std.5X

`std.5X (a --> list)`

Makes a list of 5 copies of its argument: `5 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 5X              ;-> [7 7 7 7 7]
```

### std.5XZ

`std.5XZ (a --> list)`

Makes a signal of 5 copies of its argument: `5 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 5XZ             ;-> #[7 7 7 7 7]
```

### std.6N

`std.6N (a --> list)`

Keeps the first 6 items of a list: `6 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 6N            ;-> [0 1 2 3 4 5]
```

### std.6N>

`std.6N> (a --> list)`

Drops the first 6 items of a list: `6 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 6N>  ;-> [7 8 9 10]
```

### std.6X

`std.6X (a --> list)`

Makes a list of 6 copies of its argument: `6 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 6X              ;-> [7 7 7 7 7 7]
```

### std.6XZ

`std.6XZ (a --> list)`

Makes a signal of 6 copies of its argument: `6 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 6XZ             ;-> #[7 7 7 7 7 7]
```

### std.7N

`std.7N (a --> list)`

Keeps the first 7 items of a list: `7 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 7N            ;-> [0 1 2 3 4 5 6]
```

### std.7N>

`std.7N> (a --> list)`

Drops the first 7 items of a list: `7 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 7N>  ;-> [8 9 10]
```

### std.7X

`std.7X (a --> list)`

Makes a list of 7 copies of its argument: `7 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 7X              ;-> [7 7 7 7 7 7 7]
```

### std.7XZ

`std.7XZ (a --> list)`

Makes a signal of 7 copies of its argument: `7 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 7XZ             ;-> #[7 7 7 7 7 7 7]
```

### std.8N

`std.8N (a --> list)`

Keeps the first 8 items of a list: `8 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 8N            ;-> [0 1 2 3 4 5 6 7]
```

### std.8N>

`std.8N> (a --> list)`

Drops the first 8 items of a list: `8 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 8N>  ;-> [9 10]
```

### std.8X

`std.8X (a --> list)`

Makes a list of 8 copies of its argument: `8 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 8X              ;-> [7 7 7 7 7 7 7 7]
```

### std.8XZ

`std.8XZ (a --> list)`

Makes a signal of 8 copies of its argument: `8 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 8XZ             ;-> #[7 7 7 7 7 7 7 7]
```

### std.9N

`std.9N (a --> list)`

Keeps the first 9 items of a list: `9 N`.

Use it to take a finite part of a list or an infinite stream.

```stac
nat 9N            ;-> [0 1 2 3 4 5 6 7 8]
```

### std.9N>

`std.9N> (a --> list)`

Drops the first 9 items of a list: `9 N>`.

Use it to skip the start of a list, such as a stream's first values.

```stac
[1 2 3 4 5 6 7 8 9 10] 9N>  ;-> [10]
```

### std.9X

`std.9X (a --> list)`

Makes a list of 9 copies of its argument: `9 X`.

Use it to fill a list of channels or a chord with one value.

```stac
7 9X              ;-> [7 7 7 7 7 7 7 7 7]
```

### std.9XZ

`std.9XZ (a --> list)`

Makes a signal of 9 copies of its argument: `9 XZ`.

Use it where a word needs a signal (`#[...]`) rather than a list.

```stac
7 9XZ             ;-> #[7 7 7 7 7 7 7 7 7]
```

## other

### std.-inf

`std.-inf (--> const)`

Negative infinity, -1/0.

Use it as a lower bound that every number exceeds, such as the start of a running maximum.

```stac
-inf              ;-> -inf
```

### std.A4

`std.A4 (--> const)`

The pitch of A 440 Hz in octaves above 1 Hz: log2 440, about 8.78.

Use it as the reference when working in octave units; `2^` turns a pitch back into Hz.

```stac
A4                ;-> 8.78136
A4 2^             ;-> 440
```

### std.C4

`std.C4 (--> const)`

The pitch of middle C in octaves above 1 Hz: `A4` less 9 semitones, about 8.03.

Use it as the reference pitch for scales built from middle C in octave units.

```stac
C4                ;-> 8.03136
```

### std.C4hz

`std.C4hz (--> const)`

The frequency of middle C in Hz, about 261.63.

Use it as a root frequency for just-intonation chords and scales.

```stac
C4hz              ;-> 261.626
C4hz d7 *         ;-> [261.626 327.032 392.438 457.845]
```

### std.M6

`std.M6 (--> const)`

The frequency ratios of a major sixth chord: [1 1.25 1.5 1.66667].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 M6 * size   ;-> 4
```

### std.M7

`std.M7 (--> const)`

The frequency ratios of a major seventh chord: [1 1.25 1.5 1.875].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 M7 * size   ;-> 4
```

### std.M7s5

`std.M7s5 (--> const)`

The frequency ratios of a major seventh chord with a sharp fifth: [1 1.25 1.5625 1.875].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 M7s5 * size   ;-> 4
```

### std.USkb40

`std.USkb40 (--> const)`

The 40 keys of a US keyboard's bottom four rows as a string, bottom row first.

Use a character's index in it to map typed keys to pitches.

```stac
USkb40 size       ;-> 39
```

### std.USkb49

`std.USkb49 (--> const)`

A US keyboard's keys as a string, with `~` marking the keys that have no pitch.

Use it with `USkb40` when the mapping needs the punctuation keys too.

```stac
USkb49 size       ;-> 51
```

### std.c

`std.c`

Clears the stack: a short name for `clear`.

Type it at the REPL between experiments; in source, write `clear`.

```stac
1 2 3 c           ;->
```

### std.cd

`std.cd`

Clears all but the top value: a short name for `cleard`.

Type it at the REPL to keep a result and drop the rest; in source, write `cleard`.

```stac
1 2 3 cd          ;-> 3
```

### std.d7

`std.d7 (--> const)`

The frequency ratios of a dominant seventh chord: [1 1.25 1.5 1.75].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7 * size   ;-> 4
```

### std.d7b5

`std.d7b5 (--> const)`

The frequency ratios of a dominant seventh chord with a flat fifth: [1 1.25 1.4 1.75].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7b5 * size   ;-> 4
```

### std.d7b5b9

`std.d7b5b9 (--> const)`

The frequency ratios of a dominant seventh chord with a flat fifth and a flat ninth: [1 1.25 1.4 1.75 2.1].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7b5b9 * size   ;-> 5
```

### std.d7b5s9

`std.d7b5s9 (--> const)`

The frequency ratios of a dominant seventh chord with a flat fifth and a sharp ninth: [1 1.25 1.4 1.75 2.33333].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7b5s9 * size   ;-> 5
```

### std.d7b9

`std.d7b9 (--> const)`

The frequency ratios of a dominant seventh chord with a flat ninth: [1 1.25 1.5 1.75 2.1].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7b9 * size   ;-> 5
```

### std.d7b9h

`std.d7b9h (--> const)`

The frequency ratios of a dominant seventh chord with a 17/8 flat ninth: [1 1.25 1.5 1.75 2.125].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7b9h * size   ;-> 5
```

### std.d7s5b9h

`std.d7s5b9h (--> const)`

The frequency ratios of a dominant seventh chord with a sharp fifth and a 17/8 flat ninth: [1 1.25 1.5625 1.75 2.125].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7s5b9h * size   ;-> 5
```

### std.d7s5s9

`std.d7s5s9 (--> const)`

The frequency ratios of a dominant seventh chord with a sharp fifth and a sharp ninth: [1 1.25 1.5625 1.75 2.33333].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7s5s9 * size   ;-> 5
```

### std.d7s5s9h

`std.d7s5s9h (--> const)`

The frequency ratios of a dominant seventh chord with a sharp fifth and a 19/8 sharp ninth: [1 1.25 1.5625 1.75 2.375].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7s5s9h * size   ;-> 5
```

### std.d7s9

`std.d7s9 (--> const)`

The frequency ratios of a dominant seventh chord with a sharp ninth: [1 1.25 1.5 1.75 2.33333].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7s9 * size   ;-> 5
```

### std.d7s9h

`std.d7s9h (--> const)`

The frequency ratios of a dominant seventh chord with a 19/8 sharp ninth: [1 1.25 1.5 1.75 2.375].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d7s9h * size   ;-> 5
```

### std.d9

`std.d9 (--> const)`

The frequency ratios of a dominant ninth chord: [1 1.25 1.5 1.75 2.25].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d9 * size   ;-> 5
```

### std.d9s5

`std.d9s5 (--> const)`

The frequency ratios of a dominant ninth chord with a sharp fifth: [1 1.25 1.5625 1.75 2.25].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 d9s5 * size   ;-> 5
```

### std.dim7

`std.dim7 (--> const)`

The frequency ratios of a diminished seventh chord, its notes given from 15/8: [1.875 1.125 1.33333 1.6].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 dim7 * size   ;-> 4
```

### std.dim7h

`std.dim7h (--> const)`

The frequency ratios of a diminished seventh chord with a 17/10 seventh: [1 1.2 1.4 1.7].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 dim7h * size   ;-> 4
```

### std.dim7p

`std.dim7p (--> const)`

The frequency ratios of a diminished seventh chord with a 42/25 seventh: [1 1.2 1.4 1.68].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 dim7p * size   ;-> 4
```

### std.e

`std.e (--> const)`

Euler's number, the base of the natural logarithm, about 2.718.

Use it where a formula names e; `exp` raises it to a power.

```stac
e                 ;-> 2.71828
```

### std.freqLimit

`std.freqLimit (--> const)`

The lower of 24 kHz and `nyq90`: a ceiling for filter cutoffs.

Clip a modulated cutoff to it so a filter never goes unstable near the Nyquist frequency.

```stac
freqLimit         ;-> 24000
```

### std.hrt

`std.hrt (--> const)`

The square root of 1/2, about 0.7071: the gain of each channel in an equal-power centre pan.

Use it to keep power constant when one signal feeds two channels, as `MS` does.

```stac
hrt               ;-> 0.707107
hrt aa *          ;-> 0.5
```

### std.inf

`std.inf (--> const)`

Positive infinity, 1/0.

Use it as a length or bound that never ends, such as the repeat count of a demand sequence.

```stac
inf               ;-> inf
5 inf <            ;-> 1
```

### std.ji-major

`std.ji-major (--> const)`

A just-intonation major scale, one octave, 8 ratios: [1 1.125 1.25 1.33333 1.5 1.66667 1.875 2].

Multiply it by a root frequency for the scale's pitches; index it with a degree for one pitch.

```stac
ji-major size   ;-> 8
220 ji-major 5th *  ;-> 330
```

### std.ji-minor

`std.ji-minor (--> const)`

A just-intonation minor scale, one octave, 8 ratios: [1 1.125 1.2 1.33333 1.5 1.6 1.8 2].

Multiply it by a root frequency for the scale's pitches; index it with a degree for one pitch.

```stac
ji-minor size   ;-> 8
220 ji-minor 5th *  ;-> 330
```

### std.ji-pyth

`std.ji-pyth (--> const)`

A Pythagorean major scale built from fifths, 7 ratios: [1 1.125 1.26562 1.33333 1.5 1.6875 1.89844].

Multiply it by a root frequency for the scale's pitches; index it with a degree for one pitch.

```stac
ji-pyth size    ;-> 7
220 ji-pyth 5th *  ;-> 330
```

### std.lydian

`std.lydian (--> const)`

A just-intonation Lydian scale with a 5/3 sixth, 7 ratios: [1 1.125 1.25 1.40625 1.5 1.66667 1.875].

Multiply it by a root frequency for the scale's pitches; index it with a degree for one pitch.

```stac
lydian size     ;-> 7
220 lydian 5th *  ;-> 330
```

### std.lydian27

`std.lydian27 (--> const)`

A just-intonation Lydian scale with a Pythagorean 27/16 sixth, 7 ratios: [1 1.125 1.25 1.40625 1.5 1.6875 1.875].

Multiply it by a root frequency for the scale's pitches; index it with a degree for one pitch.

```stac
lydian27 size   ;-> 7
220 lydian27 5th *  ;-> 330
```

### std.m6

`std.m6 (--> const)`

The frequency ratios of a minor sixth chord: [1 1.2 1.5 1.66667].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 m6 * size   ;-> 4
```

### std.m6h

`std.m6h (--> const)`

The frequency ratios of a minor sixth chord with a 17/10 sixth: [1 1.2 1.5 1.7].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 m6h * size   ;-> 4
```

### std.m7

`std.m7 (--> const)`

The frequency ratios of a minor seventh chord: [1 1.2 1.5 1.8].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 m7 * size   ;-> 4
```

### std.m7b5

`std.m7b5 (--> const)`

The frequency ratios of a half-diminished seventh chord: [1 1.2 1.4 1.8].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 m7b5 * size   ;-> 4
```

### std.mM7

`std.mM7 (--> const)`

The frequency ratios of a minor-major seventh chord: [1 1.2 1.5 1.875].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 mM7 * size   ;-> 4
```

### std.n7

`std.n7 (--> const)`

The frequency ratios of a neutral seventh chord, built on 11/9: [1 1.22222 1.5 1.83333].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 n7 * size   ;-> 4
```

### std.nan

`std.nan (--> const)`

Not a number, 0/0. It compares unequal to every value, itself included.

Use it to test how a word handles invalid input.

```stac
nan               ;-> nan
nan nan ==        ;-> 0
```

### std.nyq90

`std.nyq90 (--> const)`

90% of the Nyquist frequency, half the interpreter's sample rate (`-r`).

Use it to keep a filter cutoff or partial safely below the Nyquist frequency.

```stac
nyq90             ;-> 43200
```

### std.par1

`std.par1 (--> const)`

The frequency ratios of a chord of 4 ratios named par1 in sapf's prelude: [1 1.11111 1.4 1.55556].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 par1 * size   ;-> 4
```

### std.par2

`std.par2 (--> const)`

The frequency ratios of a chord of 4 ratios named par2 in sapf's prelude: [1 1.25 1.75 2.1875].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 par2 * size   ;-> 4
```

### std.phi

`std.phi (--> const)`

The golden ratio, (1 + sqrt 5) / 2, about 1.618.

Use it for proportions that avoid simple ratios, such as inharmonic partials or durations.

```stac
phi               ;-> 1.61803
100 phi *         ;-> 161.803
```

### std.predo

`std.predo (et -->)`

Prints a row for each harmonic 16 to 36: the harmonic, its nearest step in an et-step equal temperament, its pitch in cents above its octave, and that step's error in cents.

Use it to judge how well an equal temperament fits the harmonics 16 to 36.

```stac
12 predo
```

### std.pythlydian

`std.pythlydian (--> const)`

A Pythagorean Lydian scale, 7 ratios; note its 27/32 sixth lies below the root: [1 1.125 1.26562 1.42383 1.5 0.84375 1.89844].

Multiply it by a root frequency for the scale's pitches; index it with a degree for one pitch.

```stac
pythlydian size  ;-> 7
220 pythlydian 5th *  ;-> 330
```

### std.s

`std.s`

Frees every synth: a short name for `stop`.

Type it at the REPL when sound must end at once; in source, write `stop`.

```stac
`s               ;-> {Prim, stop}
```

### std.sM7

`std.sM7 (--> const)`

The frequency ratios of a supermajor seventh chord, built on 9/7: [1 1.28571 1.5 1.92857].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 sM7 * size   ;-> 4
```

### std.sm7

`std.sm7 (--> const)`

The frequency ratios of a subminor seventh chord, built on 7/6: [1 1.16667 1.5 1.75].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 sm7 * size   ;-> 4
```

### std.ssus69

`std.ssus69 (--> const)`

The frequency ratios of a suspended 6/9 chord in septimal ratios: [1 1.14286 1.5 1.71429].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 ssus69 * size   ;-> 4
```

### std.sus69

`std.sus69 (--> const)`

The frequency ratios of a suspended 6/9 chord: [1 1.11111 1.5 1.66667].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 sus69 * size   ;-> 4
```

### std.sus69py

`std.sus69py (--> const)`

The frequency ratios of a suspended 6/9 chord in Pythagorean ratios: [1 1.125 1.5 1.6875].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 sus69py * size   ;-> 4
```

### std.sus7

`std.sus7 (--> const)`

The frequency ratios of a suspended seventh chord: [1 1.33333 1.5 1.77778].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 sus7 * size   ;-> 4
```

### std.sus9

`std.sus9 (--> const)`

The frequency ratios of a suspended ninth chord: [1 1.33333 1.5 1.77778 2.25].

Multiply it by a root frequency for the chord's pitches, then play them as one chord or a list of channels.

```stac
220 sus9 * size   ;-> 5
```

## list generation

### std./X

`std./X (x n --> list)`

Divides x by n and repeats the result n times.

Use it to split a duration into n equal parts, such as a tuplet.

```stac
1 4 /X            ;-> [0.25 0.25 0.25 0.25]
1 3 /X +/         ;-> 1
```

### std.nnat

`std.nnat (n --> list)`

Makes a list of the natural numbers 0 to n - 1: `nat n N`.

Use it for 0-based indices, such as scale degrees.

```stac
5 nnat            ;-> [0 1 2 3 4]
```

### std.nnatz

`std.nnatz (n --> list)`

Makes a signal of the natural numbers 0 to n - 1.

Use it where a word needs the numbers as a signal.

```stac
3 nnatz           ;-> #[0 1 2]
```

### std.nord

`std.nord (n --> list)`

Makes a list of the counting numbers 1 to n: `ord n N`.

Use it for harmonic numbers or 1-based indices.

```stac
5 nord            ;-> [1 2 3 4 5]
220 4 nord *      ;-> [220 440 660 880]
```

### std.nordz

`std.nordz (n --> list)`

Makes a signal of the counting numbers 1 to n.

Use it where a word needs the numbers as a signal.

```stac
3 nordz           ;-> #[1 2 3]
```

## list ops

### std.0at

`std.0at (a --> any)`

Gets the item at index 0, counting from 0: `0 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 0at  ;-> 10
```

### std.1at

`std.1at (a --> any)`

Gets the item at index 1, counting from 0: `1 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 1at  ;-> 20
```

### std.2at

`std.2at (a --> any)`

Gets the item at index 2, counting from 0: `2 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 2at  ;-> 30
```

### std.3at

`std.3at (a --> any)`

Gets the item at index 3, counting from 0: `3 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 3at  ;-> 40
```

### std.4at

`std.4at (a --> any)`

Gets the item at index 4, counting from 0: `4 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 4at  ;-> 50
```

### std.5at

`std.5at (a --> any)`

Gets the item at index 5, counting from 0: `5 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 5at  ;-> 60
```

### std.6at

`std.6at (a --> any)`

Gets the item at index 6, counting from 0: `6 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 6at  ;-> 70
```

### std.7at

`std.7at (a --> any)`

Gets the item at index 7, counting from 0: `7 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 7at  ;-> 80
```

### std.8at

`std.8at (a --> any)`

Gets the item at index 8, counting from 0: `8 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 8at  ;-> 90
```

### std.9at

`std.9at (a --> any)`

Gets the item at index 9, counting from 0: `9 at`.

Use it to pick one field of a list used as a record, such as a voice's pitch.

```stac
[10 20 30 40 50 60 70 80 90 100] 9at  ;-> 100
```

## ordinal accessing

### std.10th

`std.10th (a --> any)`

Gets the tenth item, at index 9.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 10th  ;-> 100
```

### std.1st

`std.1st (a --> any)`

Gets the first item, at index 0.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 1st  ;-> 10
```

### std.2nd

`std.2nd (a --> any)`

Gets the second item, at index 1.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 2nd  ;-> 20
```

### std.3rd

`std.3rd (a --> any)`

Gets the third item, at index 2.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 3rd  ;-> 30
```

### std.4th

`std.4th (a --> any)`

Gets the fourth item, at index 3.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 4th  ;-> 40
```

### std.5th

`std.5th (a --> any)`

Gets the fifth item, at index 4.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 5th  ;-> 50
```

### std.6th

`std.6th (a --> any)`

Gets the sixth item, at index 5.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 6th  ;-> 60
```

### std.7th

`std.7th (a --> any)`

Gets the seventh item, at index 6.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 7th  ;-> 70
```

### std.8th

`std.8th (a --> any)`

Gets the eighth item, at index 7.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 8th  ;-> 80
```

### std.9th

`std.9th (a --> any)`

Gets the ninth item, at index 8.

Use it where a position reads better in words than as an index.

```stac
[10 20 30 40 50 60 70 80 90 100] 9th  ;-> 90
```

### std.divmod

`std.divmod (a b --> num num)`

Leaves a's integer quotient by b, then its remainder: `a b idiv` and `a b imod`.

Use it to split a count into whole groups and a rest, such as beats into bars.

```stac
17 5 divmod       ;-> 3 2
-7 2 divmod       ;-> -4 1
```

### std.last

`std.last (a --> any)`

Gets a list's last item, or 0 when the list is empty.

Use it to read the end of a finite list without computing its size.

```stac
[10 20 30] last   ;-> 30
[] last           ;-> 0
```

### std.runlengths

`std.runlengths (a --> list)`

Counts each run of equal consecutive items.

Use it to find repeated notes or rhythmic groupings in a list.

```stac
[1 1 2 2 2 3 1 1] runlengths  ;-> [2 3 1 2]
```

### std.separate

`std.separate (a b --> list list)`

Splits a list by a list of 0s and 1s: the items marked 1, then the items marked 0.

Use it to divide a list in two by a test computed for each item.

```stac
[1 2 3 4 5 6] [1 0 1 0 1 0] separate  ;-> [1 3 5] [2 4 6]
```

### std.span

`std.span (a b --> list list)`

Splits a list where its marks first turn 0: the leading items marked 1, then the rest. `separate` instead splits by every mark.

Use it to take the run of items that pass a test from the front of a list.

```stac
[1 2 3 4 5 6] [1 1 1 0 0 1] span  ;-> [1 2 3] [4 5 6]
```

## zero padding

### std.N/2

`std.N/2 (a --> list)`

Keeps the first half of a finite list or signal.

Use it to drop the mirrored upper half of a spectrum.

```stac
[1 2 3 4 5 6] N/2 ;-> [1 2 3]
```

### std.dly

`std.dly (a t --> list)`

Delays a signal by t seconds, adding that many samples of zero before it.

Use it to offset a sound in time inside a list computation.

```stac
#[1 2 3] 2 sr/ dly  ;-> #[0 0 1 2 3]
```

### std.nextPow2

`std.nextPow2 (a --> num)`

The smallest power of two at or above a.

Use it to size a buffer or FFT for a given number of samples.

```stac
100 nextPow2      ;-> 128
```

### std.padz

`std.padz (a n --> list)`

Adds n zeroes after a list or signal.

Use it to leave silence after a sound held in a signal.

```stac
#[1 2 3] 2 padz   ;-> #[1 2 3 0 0]
[1 2 3] 2 padz    ;-> [1 2 3 0 0]
```

### std.padzto

`std.padzto (a n --> list)`

Adds zeroes after a list or signal until it holds n items.

Use it to bring a signal to a fixed length, such as a power of two for a table.

```stac
#[1 2 3] 5 padzto ;-> #[1 2 3 0 0]
```

### std.zpad

`std.zpad (a n --> list)`

Adds n zeroes before a list or signal.

Use it to delay a signal by n samples.

```stac
#[1 2 3] 2 zpad   ;-> #[0 0 1 2 3]
```

### std.zpadz

`std.zpadz (a n --> list)`

Adds n zeroes before and n after a list or signal.

Use it to centre a short signal in silence, as a filter's impulse response needs.

```stac
#[1 2 3] 1 zpadz  ;-> #[0 1 2 3 0]
```

## data flow operators

### std.abcf

`std.abcf (a b c f --> any any any)`

Applies f to each of a, b and c.

Use it to treat three values alike without naming them.

```stac
1 2 3 \x [x 10 *] abcf  ;-> 10 20 30
```

### std.abcfgh

`std.abcfgh (a b c f g h --> any any any)`

Applies f to a, g to b and h to c.

Use it to treat three values each in its own way.

```stac
1 2 3 `neg `sq `sqrt abcfgh  ;-> -1 4 1.73205
```

### std.abf

`std.abf (a b f --> any any)`

Applies f to a, then to b.

Use it to treat two values alike without naming them.

```stac
3 4 \x [x 10 *] abf  ;-> 30 40
```

### std.abfg

`std.abfg (a b f g --> any any)`

Applies f to a and g to b.

Use it to treat a pair of values differently, such as a frequency and an amplitude.

```stac
3 4 `neg `sq abfg ;-> -3 16
```

### std.abfg2

`std.abfg2 (a b f g --> any any)`

Applies the two-argument words f and g each to a and b.

Use it to combine one pair of values in two ways.

```stac
6 3 `+ `* abfg2   ;-> 9 18
```

### std.afg

`std.afg (a f g --> any any)`

Applies f to a, then g to a.

Use it to compute two things from one value.

```stac
5 `neg `sq afg    ;-> -5 25
```

### std.afgh

`std.afgh (a f g h --> any any any)`

Applies f, g and h to a, in that order.

Use it to compute three things from one value.

```stac
5 `neg `sq `sqrt afgh  ;-> -5 25 2.23607
```

## normalization

### std.absumto

`std.absumto (x y --> list)`

Scales a list so the sum of its absolute values is y.

Use it to fit signed values, such as panned levels, into a total.

```stac
[1 -1 2] 8 absumto  ;-> [2 -2 4]
```

### std.conormalize

`std.conormalize (x --> list)`

Scales a list of lists together, so the largest absolute value among all of them is 1.

Use it to normalise channels together and keep their balance.

```stac
[[1 2] [-4 2]] conormalize  ;-> [[0.25 0.5] [-1 0.5]]
```

### std.conormalizeb

`std.conormalizeb (x y --> list)`

Scales a list of lists together so the largest absolute value among them is y.

Use it to normalise channels together to a level below full scale.

```stac
[[1 2] [-4 2]] .5 conormalizeb  ;-> [[0.125 0.25] [-0.5 0.25]]
```

### std.dv

`std.dv (x d --> list)`

Adds an independent random deviation, up to plus or minus d, to each value.

Use it to humanise a list of times or levels.

```stac
[1 2 3] 0 dv      ;-> [1 2 3]
[1 2 3] .1 dv size  ;-> 3
```

### std.ldv

`std.ldv (x d --> list)`

Multiplies each value by 2 to a random power up to plus or minus d: a deviation in octaves.

Use it to detune a list of frequencies by up to d octaves.

```stac
[1 2 3] 0 ldv     ;-> [1 2 3]
[100 200] .01 ldv size  ;-> 2
```

### std.normalize

`std.normalize (x --> list)`

Scales a list so its largest absolute value is 1.

Use it to bring a list of amplitudes or a waveform to full scale.

```stac
[3 -6 1] normalize  ;-> [0.5 -1 0.166667]
```

### std.q12

`std.q12 (x --> num)`

Quantizes a ratio to the nearest semitone of 12-tone equal temperament.

Use it to snap a computed interval to the piano's tuning.

```stac
1.4 q12           ;-> 1.41421
```

### std.q72

`std.q72 (x --> num)`

Quantizes a ratio to the nearest step of 72-tone equal temperament, a twelfth of a semitone.

Use it for fine quantization that keeps intervals close to just.

```stac
1.4 q72           ;-> 1.40066
```

### std.qedo

`std.qedo (x n --> num)`

Quantizes a ratio to the nearest step of n equal divisions of the octave.

Use it to hear a just ratio in an equal temperament.

```stac
1.5 12 qedo       ;-> 1.49831
```

### std.sumcut

`std.sumcut (x y --> list)`

Cuts a list where its running sum reaches y, shortening the last item to fit.

Use it to take durations from a stream until they fill a total length.

```stac
[1 2 3 4] 7 sumcut  ;-> [1 2 3 1]
nat pi sumcut     ;-> [0 1 2 0.141593]
```

### std.sumto

`std.sumto (x y --> list)`

Scales a list so its items sum to y.

Use it to fit a rhythm's durations into a total length, such as one bar.

```stac
[1 1 2] 8 sumto   ;-> [2 2 4]
```

### std.sumto1

`std.sumto1 (x --> list)`

Scales a list so its items sum to 1.

Use it to turn weights into probabilities.

```stac
[1 1 2] sumto1    ;-> [0.25 0.25 0.5]
```

## statistics

### std.atf

`std.atf (a x --> num)`

The list's value at a fractional index, interpolated linearly between neighbours.

Use it to read a table between its points, such as a breakpoint curve.

```stac
[1 2 3 4] 1.5 atf ;-> 2.5
```

### std.deviation

`std.deviation (a --> num)`

The population standard deviation of a finite list: the square root of `variance`.

Use it to measure spread in the units of the values.

```stac
[1 2 3 4] deviation  ;-> 1.11803
```

### std.fac

`std.fac (a --> num)`

The factorial of a, computed as gamma(a + 1), so it also takes non-integers.

Use it for counting permutations, such as the orderings of a set of notes.

```stac
5 fac             ;-> 120
```

### std.mean

`std.mean (a --> num)`

The arithmetic mean of a finite list.

Use it to centre a list of values or summarise a measurement.

```stac
[1 2 3 4] mean    ;-> 2.5
```

### std.median

`std.median (a --> num)`

The median of a finite list: the middle value once sorted, or the mean of the two middle values.

Use it for a centre that one outlier cannot move.

```stac
[5 1 3 2 4] median  ;-> 3
[1 2 3 4] median  ;-> 2.5
```

### std.variance

`std.variance (a --> num)`

The population variance of a finite list: the mean squared deviation from the mean.

Use it to measure how far values spread.

```stac
[1 2 3 4] variance  ;-> 1.25
```

### std.vmag

`std.vmag (a --> num)`

The Euclidean length of a vector: the square root of the sum of squares.

Use it to measure the size of a list of values, such as partial amplitudes.

```stac
[3 4] vmag        ;-> 5
```

## bipolar random number generators

### std.brand

`std.brand (--> num)`

A random real from -1 to 1, drawn when the word runs.

Use it for one random offset made in the interpreter.

```stac
brand -1 1 inrange  ;-> 1
```

### std.brands

`std.brands (--> list)`

An infinite stream of random reals from -1 to 1.

Use it as a pattern field of random offsets, such as detuning.

```stac
brands 3 N size   ;-> 3
```

### std.nbrands

`std.nbrands (n --> list)`

A stream of n random reals from -1 to 1.

Use it for a finite sequence of random offsets.

```stac
4 nbrands size    ;-> 4
```

## each operators

### std.cart2

`std.cart2 (a b --> any any)`

Marks two lists for a cartesian product: every item of a with every item of b, a outermost.

Use it to build a table of every combination, such as each root with each interval.

```stac
[1 2] [10 20] cart2 +  ;-> [[11 21] [12 22]]
```

### std.cart3

`std.cart3 (a b c --> any any any)`

Marks three lists for a cartesian product.

Use it to build every combination of three parameters.

```stac
[1 2] [10 20] [100 200] cart3 \a b c [a b + c +] !  ;-> [[[111 211] [121 221]] [[112 212] [122 222]]]
```

### std.cart4

`std.cart4 (a b c d --> any any any any)`

Marks four lists for a cartesian product.

Use it to build every combination of four parameters.

```stac
[1 2] [10 20] [100 200] [1000 2000] cart4 \a b c d [a b + c + d +] ! size  ;-> 2
```

### std.each2

`std.each2 (a b --> any any)`

Marks two lists to be iterated together by the next word: `a @ b @`.

Use it to pair items of two lists, the first with the first, and so on.

```stac
[1 2] [10 20] each2 +  ;-> [11 22]
```

### std.each3

`std.each3 (a b c --> any any any)`

Marks three lists to be iterated together by the next word.

Use it to combine items of three lists position by position.

```stac
[1 2] [10 20] [100 200] each3 \a b c [a b + c +] !  ;-> [111 222]
```

### std.each4

`std.each4 (a b c d --> any any any any)`

Marks four lists to be iterated together by the next word.

Use it to combine items of four lists position by position.

```stac
[1 2] [10 20] [100 200] [1000 2000] each4 \a b c d [a b + c + d +] !  ;-> [1111 2222]
```

### std.rcart2

`std.rcart2 (a b --> any any)`

Marks two lists for a cartesian product with b outermost.

Use it when the table should be grouped by the second list.

```stac
[1 2] [10 20] rcart2 +  ;-> [[11 12] [21 22]]
```

### std.rcart3

`std.rcart3 (a b c --> any any any)`

Marks three lists for a cartesian product, the last list outermost.

Use it when the table should be grouped by the third list.

```stac
[1 2] [10 20] [100 200] rcart3 \a b c [a b + c +] !  ;-> [[[111 112] [121 122]] [[211 212] [221 222]]]
```

### std.rcart4

`std.rcart4 (a b c d --> any any any any)`

Marks four lists for a cartesian product, the last list outermost.

Use it when the table should be grouped by the fourth list.

```stac
[1 2] [10 20] [100 200] [1000 2000] rcart4 \a b c d [a b + c + d +] ! size  ;-> 2
```

## numbers

### std.coprime

`std.coprime (a b --> num)`

Leaves 1 when a and b share no factor but 1, else 0.

Use it to choose rhythm lengths that do not realign early, such as 8 against 9.

```stac
8 9 coprime       ;-> 1
8 12 coprime      ;-> 0
```

### std.coprimes

`std.coprimes (a --> num)`

Leaves 1 when every pair of the list's items is coprime. It compares the product with the least common multiple, so it is exact while the product stays below 2^53.

Use it to check that a set of delay lengths shares no factor, so their echoes do not coincide.

```stac
[8 9 25] coprimes ;-> 1
[8 12 9] coprimes ;-> 0
```

### std.factors

`std.factors (x --> list)`

All factors of an integer, as a signal in ascending order.

Use it to find the subdivisions a count of beats allows.

```stac
12 factors        ;-> #[1 2 3 4 6 12]
```

### std.pfactors

`std.pfactors (x --> list)`

The distinct prime factors of an integer, as a signal.

Use it to analyse a ratio's limit, such as 7 in 7/4.

```stac
12 pfactors       ;-> #[2 3]
```

## list co-length operations

### std.cycLongest

`std.cycLongest (x --> list)`

Extends every list to the length of the longest by cycling it.

Use it where a short voice should repeat from its start.

```stac
[[1 2 3] [4 5]] cycLongest  ;-> [[1 2 3] [4 5 4]]
```

### std.cyclcm

`std.cyclcm (x --> list)`

Cycles every list to the least common multiple of their lengths, so all realign at the end.

Use it for polyrhythms that should complete together.

```stac
[[1 2 3] [4 5]] cyclcm  ;-> [[1 2 3 1 2 3] [4 5 4 5 4 5]]
```

### std.longest

`std.longest (x --> list)`

Extends every list to the length of the longest, repeating each list's last item.

Use it to pair voices where a short voice should hold its last value.

```stac
[[1 2 3] [4 5]] longest  ;-> [[1 2 3] [4 5 5]]
```

### std.mirLongest

`std.mirLongest (x --> list)`

Extends every list to the length of the longest with `mircyc`.

Use it where a short voice should turn back rather than restart.

```stac
[[1 2 3] [4 5]] mirLongest  ;-> [[1 2 3] [4 5 4]]
```

### std.mircyc

`std.mircyc (x --> list)`

Cycles a list forward then backward, without repeating its end points.

Use it for a pattern that rises and falls, such as an arpeggio up and down.

```stac
[1 2 3] mircyc 8 N  ;-> [1 2 3 2 1 2 3 2]
```

### std.shortest

`std.shortest (x --> list)`

Cuts every list in a list of lists to the length of the shortest.

Use it to pair voices note for note without leftovers.

```stac
[[1 2 3] [4 5]] shortest  ;-> [[1 2] [4 5]]
```

## dsp functions

### std.duration

`std.duration (a --> num)`

The duration in seconds of a finite signal at the interpreter's sample rate.

Use it to time other material against a sound held in a signal.

```stac
96000 0z duration ;-> 1
```

### std.sincfill

`std.sincfill (n nz --> list)`

A sinc wave, sin(x)/x, of n samples spanning nz zero crossings.

Use it as a band-limited impulse or the kernel of a low-pass filter.

```stac
8 2 sincfill size ;-> 8
```

### std.sr*

`std.sr* (x --> sig)`

Converts seconds to samples at the interpreter's sample rate (`-r`).

Use it to state a duration as a count of samples.

```stac
1 sr*             ;-> 96000
```

### std.sr/

`std.sr/ (x --> sig)`

Converts samples to seconds at the interpreter's sample rate (`-r`).

Use it to state a length in samples as a duration.

```stac
48000 sr/         ;-> 0.5
```

## events

### std.ev-dur

`std.ev-dur (in --> num)`

The total duration of an event list: the sum of its `dt` fields.

Use it to know how long a list of events lasts before scheduling the next part.

```stac
[{:dt .25} {:dt .5} {:dt .25}] ev-dur  ;-> 1
```

### std.ev-end

`std.ev-end (in --> num)`

The latest end time in an event list: each event's start plus its `dur`.

Use it to find when the last note of an event list stops sounding.

```stac
[{:dt .25 :dur 1} {:dt .5 :dur .1} {:dt .25 :dur .2}] ev-end  ;-> 1
```

### std.ev-reverse

`std.ev-reverse (in --> list)`

Reverses an event list in time, recomputing each `dt` from the reversed start times.

Use it to play a phrase backwards.

```stac
[{:dt .25 :x 1} {:dt .5 :x 2} {:dt .75 :x 3}] ev-reverse.x  ;-> [3 2 1]
```

### std.ev-starts

`std.ev-starts (in --> list)`

The start time of each event in an event list, from the running sum of `dt`.

Use it to place each event on a timeline.

```stac
[{:dt .25} {:dt .5} {:dt .25}] ev-starts  ;-> [0 0.25 0.75]
```

## range checking

### std.exrange

`std.exrange (in lo hi --> sig)`

Leaves 1 when the input lies strictly between lo and hi, else 0.

Use it when a value on a bound must fail the test.

```stac
10 0 10 exrange   ;-> 0
5 0 10 exrange    ;-> 1
```

### std.inrange

`std.inrange (in lo hi --> sig)`

Leaves 1 when the input lies in lo..hi, bounds included, else 0.

Use it to test a value, or as a mask for `?` over a list.

```stac
5 0 10 inrange    ;-> 1
10 0 10 inrange   ;-> 1
```

## composite ugens

### std.famp

`std.famp (freq root expon --> sig)`

An amplitude compensation factor: (root / freq) to the power expon.

Use it to scale partials so high frequencies do not sound louder than low ones; expon .3 is a gentle tilt.

```stac
440 220 .3 famp   ;-> 0.812252
```

### std.randsign

`std.randsign (in --> sig)`

Flips the input's sign with probability 1/2.

Use it to vary the phase of partials or the direction of a movement.

```stac
-3 randsign abs   ;-> 3
```

## pitch operations

### std.invert

`std.invert (x --> list)`

Mirrors a list of pitches about the middle of its range: max + min - x.

Use it to turn a melody upside down within its own range.

```stac
[1 5 4 3] invert  ;-> [5 1 2 3]
```

### std.monotonic

`std.monotonic (x m --> list)`

Adds multiples of m wherever the list falls, so it never descends.

Use it to lay pitch classes out as a rising line, m 12 adding octaves.

```stac
[0 4 2 5] 12 monotonic  ;-> [0 4 14 17]
```

### std.replacehead

`std.replacehead (x v --> list)`

Replaces a list's first item with v.

Use it to change the first note of a sequence, such as its anchor pitch.

```stac
[1 2 3] 9 replacehead  ;-> [9 2 3]
```

## borrowed from APL

### std.iota

`std.iota (s --> list)`

Makes a matrix of the counting numbers from 1 in the given shape.

Use it as a test matrix or a grid of indices.

```stac
[2 3] iota        ;-> [[1 2 3] [4 5 6]]
```

### std.reshape

`std.reshape (a s --> list)`

Reshapes a list into a matrix of the given dimensions, cycling the input if it is short.

Use it to lay a flat list out as rows, such as a sequence as bars.

```stac
[1 2 3 4 5 6] [2 3] reshape  ;-> [[1 2 3] [4 5 6]]
```

## random walks

### std.iwalk

`std.iwalk (step lo hi --> list)`

An infinite integer random walk: each value moves by at most step and folds back into lo..hi.

Use it for a melody that wanders by small steps within a register.

```stac
1 0 10 iwalk 20 N 0 10 inrange &/  ;-> 1
```

### std.iwalk2

`std.iwalk2 (step limit --> list)`

An infinite integer random walk folded into -limit..limit.

Use it for a wandering offset centred on 0, such as scale degrees around a root.

```stac
1 5 iwalk2 20 N -5 5 inrange &/  ;-> 1
```

### std.walk

`std.walk (step lo hi --> list)`

An infinite random walk of reals: each value moves by at most step and folds back into lo..hi.

Use it for a parameter that drifts smoothly within a range, set once per event.

```stac
.1 0 1 walk 20 N 0 1 inrange &/  ;-> 1
```

### std.walk2

`std.walk2 (step limit --> list)`

An infinite random walk of reals folded into -limit..limit.

Use it for a drifting offset centred on 0, such as a slowly wandering pan.

```stac
.2 3 walk2 20 N -3 3 inrange &/  ;-> 1
```

## conditional mapping

### std.mapif

`std.mapif (list bools fun --> list)`

Applies fun to the items whose flag is 1 and passes the others through.

Use it to change only some notes of a sequence, chosen by a computed mask.

```stac
[1 2 3 4] [1 0 1 0] \x[x 10 *] mapif  ;-> [10 2 30 4]
```

## unipolar random number generators

### std.nurands

`std.nurands (n --> list)`

A stream of n random reals from 0 to 1.

Use it for a finite random sequence, such as one bar of velocities.

```stac
4 nurands size    ;-> 4
```

### std.urand

`std.urand (--> num)`

A random real from 0 to 1, drawn when the word runs.

Use it for one random choice made in the interpreter, such as a starting value.

```stac
urand 0 1 inrange ;-> 1
```

### std.urands

`std.urands (--> list)`

An infinite stream of random reals from 0 to 1.

Use it as a pattern field that changes every event.

```stac
urands 3 N size   ;-> 3
```

## printing

### std.prall

`std.prall (list -->)`

Prints each item of a finite list on its own line.

Use it to read a long list at the REPL.

```stac
[1 2 3] prall
```

## 12 tone pitch set operations

### std.ps-inv

`std.ps-inv (x --> list)`

Inverts a pitch set: negates its pitches, reverses their order, and normalizes with `ps-norm`. An ascending set stays ascending.

Use it to turn a chord into its mirror image, such as a major triad into a minor one.

```stac
[0 4 7] ps-inv    ;-> [0 3 7]
```

### std.ps-norm

`std.ps-norm (x --> list)`

Transposes a pitch set so its first pitch is 0, modulo 12.

Use it to compare pitch sets regardless of transposition.

```stac
[3 7 10] ps-norm  ;-> [0 4 7]
```

### std.ps-rot

`std.ps-rot (x i --> list)`

Rotates a pitch set by i places and normalizes it: an inversion in the chordal sense.

Use it to get one inversion of a chord as a normalized set.

```stac
[0 4 7] 1 ps-rot  ;-> [0 5 9]
```

### std.ps-rots

`std.ps-rots (x --> list)`

Lists every normalized rotation of a pitch set.

Use it to see all inversions of a chord at once.

```stac
[0 4 7] ps-rots   ;-> [[0 4 7] [0 5 9] [0 3 8]]
```

## complex <-> polar

### std.px

`std.px (r t --> sig sig)`

Converts polar form, magnitude and angle in radians, to x and y.

Use it to build a partial from an amplitude and a phase.

```stac
5 0 px            ;-> 5 0
```

### std.xp

`std.xp (x y --> sig sig)`

Converts the complex number x + iy to polar form: magnitude, then angle in radians.

Use it to get the amplitude and phase of a partial given as real and imaginary parts.

```stac
3 4 xp            ;-> 5 0.927295
```

## rhythm

### std.rdiv

`std.rdiv (n p --> list)`

Divides n beats at random into p whole-beat durations that sum to n.

Use it to generate a random rhythm of fixed total length.

```stac
8 3 rdiv size     ;-> 3
8 3 rdiv +/       ;-> 8
```

## amplitude

### std.rms

`std.rms (in --> num)`

The root-mean-square amplitude of a finite list.

Use it to measure the level of a block of samples or a list of amplitudes.

```stac
[1 -1 1 -1] rms   ;-> 1
```

### std.rmsdb

`std.rmsdb (in --> num)`

The root-mean-square amplitude of a finite list, in decibels.

Use it to compare levels on a scale that matches loudness.

```stac
[.5 .5] rmsdb     ;-> -6.0206
```
