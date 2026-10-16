# syntax

Statements, syntax and methods: what stac reads but does not bind as words.

## statements

### synthdef

`synthdef (--> namespace)`

Defines a SynthDef: `synthdef NAME CONTROL* [HELP] [ BODY ]`. Each control is `name` or `name:default`; `t_`, `i_` and `a_` prefixes make trigger, scalar and audio controls. `{...} NAME` starts a synth and pushes its node. `synthdef` is also the namespace of `synthdef.new`, `(fun defaults name --> synthdef)`, which builds one from code, as SC's `SynthDef.new`.

Use the statement for a sound you start many times with different values, or change while it plays with `.set`. Use `synthdef.new` when code makes the names or defaults, e.g. in a loop.

```stac
synthdef ping freq:880 amp:.1 "a short sine." [freq sinosc .002 .3 env.perc 1 envgen.kr.done * amp *]
{:freq 660} ping = n
\freq [freq saw .1 *] {:freq 220} 'buzz synthdef.new = buzz
{} buzz
```

### def

`def`

Defines a function: `def NAME PARAM* [--> RESULT*] [HELP] [ BODY ]`. The parameters take their values from the stack, last parameter from the top. Each call checks the count and types of the values the body leaves, when `-->` declares them.

Use it for every named function in a library or a performance file. The generated stack effect goes into `--words` and the dictionary. Result types: `sig num list fun form str env node bus buffer synthdef clock any`, or `...` for a varying count; `num` and `sig` accept lists, as automapping makes them.

```stac
def dbl x --> num "doubles x." [x 2 *]
5 dbl          ;-> 10
[1 2] dbl      ;-> [2 4]
```

### const

`const`

Defines a constant: `const NAME [HELP] [ BODY ]`. The body runs once and must leave 1 value. `=` cannot rebind the name; a second `const` of it is an error too.

Use it in a library for a value its functions read, so no later binding can change it. In a performance file, prefer `=` for values you mean to change while playing.

```stac
const hrt "sqrt(2)/2, the mid-side gain." [.5 sqrt]
hrt            ;-> 0.707107
```

### import

`import`

Binds names from a namespace or module: `import NS.*`, `import NS.name`, or `import NS` for the namespace alone. `NS.*` skips core words and names starting with `_`; a named import binds even over a core word. A module is a file `NS.stac` beside the importing file or in `$STAC_PATH`. Imports come before a file's other statements.

Use it in a prelude to choose the global names a program starts with, and in a module to state what it uses. Running the import again after editing a module reloads it.

```stac
import sc
440 sc.sinosc .1 * play
```

## syntax

### =

`=`

Binds the top value to a name: `x = name`. `= [a b]` binds the items of a list, `= (a b)` the top values of the stack. At the top level it binds in the workspace, and a later rebinding reaches running code from its next event; in a function it binds a local.

Use it to name a value instead of moving it around the stack. More than two values in play is a sign to bind them.

```stac
3 = x
x x *          ;-> 9
[1 2 3] = [a b c]
c b a          ;-> 3 2 1
```

### \

`\`

Starts an anonymous function: `\ARG* [ BODY ]`. An optional help string comes before the body. Applying the function binds its arguments from the stack and runs the body.

Use it inline, for `@`, `:instrument` and other words that take a function. Name a reusable function with `def`.

```stac
[1 2 3] @ \x [x x *] !  ;-> [1 4 9]
3 \x [x 1 +] !          ;-> 4
```

### [ ]

`[ ]`

Evaluates the code inside and collects every value it leaves into a list.

Use it to build a list of values or channels. A list of UGens plays as consecutive channels.

```stac
[1 2 3 +]      ;-> [1 5]
[300 400] sinosc .1 * play
```

### { }

`{ }`

Makes a form: `{:key value ...}`, a table of named values. `{parent :key value}` inherits the keys of `parent`. `f,key` reads a value; `f.key` applies a function stored under it, passing the form as `self`.

Use it for an event, a synth's controls, or keyword arguments to a UGen: `{:freq 660} ping`, `{:freq 2000 :rq .3 :in x} rlpf`.

```stac
{:a 1 :b 2} = f
f,b            ;-> 2
{f :c 3} = g
g,a            ;-> 1
```

### ( )

`( )`

Groups code. The code inside cannot consume values from outside the parentheses; reaching below them is a stack underflow.

Use it to make the extent of a sub-expression visible, and to catch a word that takes more values than you meant.

```stac
1 2 (3 4 +) *  ;-> 1 14
```

### #[ ]

`#[ ]`

Makes a signal list: a finite list of reals stored as floats, not a list of values.

Use it for buffer data and wave tables. It is not a UGen; a graph reads it through a buffer.

```stac
#[0 .5 1] buffer.load = b
```

### '

`'`

Pushes a symbol: `'name`, a string read without evaluating the name.

Use it for names a word takes, such as a def's name, a scale (`'minor`), an add action (`'tail`) or `'rest` in a pattern.

```stac
'freq          ;-> freq
```

### `

```

Pushes the value bound to a name without applying it: `` `name ``.

Use it to pass a function or a SynthDef as a value, e.g. to `!`, `.write` or a pattern's `:instrument`.

```stac
\x [x 1 +] = inc
3 `inc !        ;-> 4
```

### ,

`,`

Reads a key from a form or a namespace without applying it: `f,key`, `sc,dseq`.

Use it to fetch a function from a namespace as a value, or a field from a form. `.` applies it instead.

```stac
{:freq 440} = e
e,freq         ;-> 440
```

### .

`.`

Applies a member: a namespace's word (`sc.sinosc`), a form's function with the form as `self`, or an object's method (`n.free`).

Use it to call a word by its canonical name, and to act on a node, bus, buffer, clock, player or ndef.

```stac
{:sq \x self [x x *]} = m
3 m.sq         ;-> 9
```

### @

`@`

Marks an argument to iterate: the next word runs once per item and the results form a list. `@1` and `@2` set the loop order, outer first; `@@` iterates two levels deep.

Use it to apply a word across a list where automapping does not, or to make a table of combinations.

```stac
[1 2] @ [10 20] +        ;-> [[11 21] [12 22]]
[1 2] @2 [10 20] @1 +    ;-> [[11 12] [21 22]]
[[1 2] [3 4]] @@ 10 *    ;-> [[10 20] [30 40]]
```

### :

`:`

Starts a key in a form, `{:freq 440}`, and a control's default in a synthdef, `freq:440`.

```stac
{:freq 440 :amp .1} = e
e,amp          ;-> 0.1
```

### ;

`;`

Starts a comment that runs to the end of the line.

```stac
1 2 +   ; three
```

### "

`"`

Delimits a string: `"text"`. A doubled quote, `""`, is a quote. Two backslashes and a letter are an escape: `\\n` a newline, `\\t` a tab; one backslash is itself.

Use it for file names, messages and help strings.

```stac
"a.wav" size   ;-> 5
```

### op/ op\ op^

`op/ op\ op^`

Every binary operator has forms over one list. `op/` reduces the list with the operator; `op\` scans it, keeping each running result; `op^` keeps the first item and then applies the operator to each item and the one before. `op\i` and `op^i` take an initial value: the scan starts from it, and the pairwise form puts it first.

Use them for sums, running totals and differences of a list without a loop: `+/` sums, `+\` integrates, `-^` differentiates.

```stac
[1 2 3 4] +/        ;-> 10
[1 2 3 4] +\        ;-> [1 3 6 10]
[1 2 4 7] -^        ;-> [1 1 2 3]
[1 2 3] 10 +\i      ;-> [10 11 13 16]
[1 2 3] 10 -^i      ;-> [10 1 1]
```

## methods

### .add

`.add`

SynthDef: sends the def to the server now, rather than with its first synth.

Use it before a pattern or score refers to the def by name.

```stac
synthdef beep freq:440 [freq sinosc .1 *]
`beep.add
```

### .ar

`.ar`

Ndef: reads the ndef's private bus at audio rate, as SC's `Ndef(\x).ar`. It reads with `InFeedback`, so the reader may run before the ndef's synth.

Use it to process one ndef in another, e.g. a filter over a drone.

```stac
[55 saw .1 *] 'drone ndef = d
d.stop
[d.ar 800 .3 rlpf] 'filtered ndef
```

### .done

`.done`

UGen: sets the UGen's `doneAction` to 2, so the synth frees itself when the UGen finishes.

Use it on the envelope or line that ends a note, so released synths do not accumulate.

```stac
[440 sinosc .01 .5 env.perc 1 envgen.kr.done *] play
```

### .end

`.end`

Ndef: fades out the ndef's synth and its monitor over `fadeTime`.

Use it to end an ndef entirely; `.stop` silences it but keeps it running for readers.

```stac
[110 saw .05 *] 'bass ndef = b
b.end
```

### .free

`.free`

Node: frees the synth or group now. Bus or buffer: returns it to its allocator; a buffer is also freed on the server. Freeing twice is an error.

Use it on a node to stop a sound at once, and on a bus or buffer you no longer need in a long session.

```stac
[440 sinosc .1 *] play = n
n.free
1 bus.audio = b
b.free
```

### .ir

`.ir`

UGen: a scalar-rate copy, computed once when the synth starts, as SC's `.ir`.

Use it for a value that must not change during a note, such as a random pitch.

```stac
[200 800 sc.rand.ir sinosc .1 *] play
```

### .kr

`.kr`

UGen: the control-rate version, computed once per 64-sample block.

Use it for modulators and envelopes, which need no audio rate and cost less at control rate.

```stac
[5 lfnoise1.kr 400 * 600 + sinosc .1 *] play
```

### .pdef

`.pdef`

Clock: `events name clk.pdef` plays a pdef on that clock, replacing the named stream on the clock's beats.

Use it to keep a pattern in time with a clock other than the default.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
4 clock.new = c
{:instrument 'ping :dur 1} pbind 'beat c.pdef = p
p.stop
```

### .play

`.play`

Clock: `events clk.play` plays an event stream on the clock. Ndef: starts the monitor again after `.stop`.

Use it on a clock to play at its tempo; on an ndef to hear it again.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
2 clock.new = c
{:instrument 'ping :dur 1} pbind c.play = p
p.stop
```

### .poll

`.poll`

UGen: prints its value on scsynth's console 10 times a second, labelled with the class, and returns the UGen.

Use it to see what a control signal does while the synth plays.

```stac
[2 lfnoise1.kr.poll 400 * 600 + sinosc .1 *] play
```

### .release

`.release`

Node: sets its `gate` control to 0, so an envelope on `gate` releases.

Use it to end a held note at its release time, rather than cutting it with `.free`.

```stac
synthdef pad freq:220 gate:1 [freq saw .01 .2 .6 1 env.adsr gate envgen.kr.done * .1 *]
{} pad = n
n.release
```

### .releaseIn

`.releaseIn`

Node: `t n.releaseIn` releases over `t` seconds, as SC's `release(t)`: it sets `gate` to `-1 - t`.

Use it to fade a held note out over a time you choose rather than the envelope's own.

```stac
synthdef pad freq:220 gate:1 [freq saw .01 .2 .6 1 env.adsr gate envgen.kr.done * .1 *]
{} pad = n
4 n.releaseIn
```

### .set

`.set`

Changes values from a form or a value. Node: its controls, `{:freq 880} n.set`. Control bus: its values, `[.5 .25] b.set`. Clock: `{:tempo 3}`. Ndef: `{:fadeTime 2}`. Pdef player: `{:quant 4}`.

Use it to change a running sound or its timing without restarting it.

```stac
synthdef beep freq:440 [freq sinosc .1 *]
{} beep = n
{:freq 880} n.set
n.free
```

### .stop

`.stop`

Player: ends the event stream. Ndef: fades out the monitor; the synth keeps running for readers.

Use it to stop a pattern, or to silence an ndef that another ndef still reads.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
{:instrument 'ping :dur .25} pbind play = p
p.stop
```

### .write

`.write`

SynthDef: writes `NAME.scsyndef` to `$STAC_SYNTHDEFS`, or the current directory.

Use it to load the def in another program, such as the SSP's `scsy` or sclang.

```stac
synthdef hiss cutoff:1200 [whitenoise .2 * cutoff .3 rlpf]
`hiss.write
```
