# core

Core words: the language, its lists and streams, patterns, and server objects.

## stack ops

### clear

`clear (... -->)`

Removes every value from the stack.

Use it at the REPL to start a line from an empty stack. In a program, prefer words that consume exactly what they take.

```stac
1 2 3 clear       ;->
```

### cleard

`cleard (... a --> a)`

Removes every value except the top one.

Use it at the REPL to keep a result and drop the leftovers of earlier lines.

```stac
1 2 3 cleard      ;-> 3
```

### stackDepth

`stackDepth (--> n)`

Pushes the number of values on the stack.

Use it to check that a line leaves what you expect, as tests of stac's own words do.

```stac
1 2 3 stackDepth  ;-> 1 2 3 3
```

### aa

`aa (a --> a a)`

Pushes a second copy of the top value. Forth calls it `dup`.

Use it when a word needs the same value twice, such as squaring.

```stac
3 aa *            ;-> 9
```

### aaa

`aaa (a --> a a a)`

Pushes two more copies of the top value.

Use it when three words read the same value.

```stac
2 aaa             ;-> 2 2 2
```

### ba

`ba (a b --> b a)`

Swaps the top two values. Forth calls it `swap`.

Use it to put two arguments in the order a word takes them. When more than two values move, bind them with `=` instead.

```stac
1 2 ba            ;-> 2 1
10 3 ba -         ;-> -7
```

### bac

`bac (a b c --> b a c)`

Swaps the second and third values, leaving the top in place.

Use it to reorder the arguments under the top value without naming them.

```stac
1 2 3 bac         ;-> 2 1 3
```

### cba

`cba (a b c --> c b a)`

Reverses the top three values.

Use it to reverse three arguments at once.

```stac
1 2 3 cba         ;-> 3 2 1
```

### bca

`bca (a b c --> b c a)`

Moves the third value to the top. Forth calls it `rot`.

Use it to bring a deeper argument up for the next word.

```stac
1 2 3 bca         ;-> 2 3 1
```

### cab

`cab (a b c --> c a b)`

Moves the top value under the next two. Forth calls it `-rot`.

Use it to put a value aside under two others that the next words consume.

```stac
1 2 3 cab         ;-> 3 1 2
```

### bab

`bab (a b --> b a b)`

Copies the top value under the second. Forth calls it `tuck`.

Use it to keep a value for later while a word consumes it with the one below.

```stac
1 2 bab           ;-> 2 1 2
```

### aba

`aba (a b --> a b a)`

Copies the second value to the top. Forth calls it `over`.

Use it when a word needs the lower value again without losing the top.

```stac
1 2 aba           ;-> 1 2 1
```

### aab

`aab (a b --> a a b)`

Copies the second value in place, under the top.

Use it to duplicate the lower argument while keeping the top last.

```stac
1 2 aab           ;-> 1 1 2
```

### aabb

`aabb (a b --> a a b b)`

Copies each of the top two values in place.

Use it to apply two words to each of two values.

```stac
1 2 aabb          ;-> 1 1 2 2
```

### abab

`abab (a b --> a b a b)`

Copies the top two values as a pair. Forth calls it `2dup`.

Use it when two words each take the same pair of arguments.

```stac
3 4 abab          ;-> 3 4 3 4
3 4 abab * cab +  ;-> 12 7
```

### nip

`nip (a b --> b)`

Removes the second value, keeping the top. Forth has the same name.

Use it to drop an argument a word left below its result.

```stac
1 2 nip           ;-> 2
```

### pop

`pop (a -->)`

Removes the top value. Forth calls it `drop`.

Use it to discard a result you do not need.

```stac
1 2 pop           ;-> 1
```

## loops

### do

`do (list \item[..] -->)`

Applies the function to each item of a finite list, for its side effects; it pushes nothing.

Use it to print or send something per item. To collect results, use `@` and `!` instead.

```stac
[1 2 3] \x [x pr " " pr] do cr
```

## conditional ops

### equals

`equals (a b --> bool)`

Pushes 1 if two values have the same structure and contents, else 0. A cyclic structure may never finish.

Use it to compare lists and strings; `==` compares numbers item by item instead.

```stac
[1 2] [1 2] equals  ;-> 1
'a "a" equals       ;-> 1
```

### less

`less (a b --> bool)`

Pushes 1 if the first value sorts before the second, comparing lists item by item, else 0.

Use it to order strings or lists; `<` compares numbers and maps over lists.

```stac
'a 'b less          ;-> 1
```

### greater

`greater (a b --> bool)`

Pushes 1 if the first value sorts after the second, else 0.

Use it as `less` with its arguments the other way round.

```stac
3 2 greater         ;-> 1
```

### if

`if (A B C --> ..)`

Takes a condition and two functions. Applies the first if the condition is not 0, else the second.

Use it to choose between two computations. For a choice per item of a list, use `?` or `pick` instead.

```stac
1 \[10] \[20] if    ;-> 10
0 \[10] \[20] if    ;-> 20
```

### not

`not (A --> bool)`

Pushes 1 for 0 and 0 for anything else.

Use it to invert a test.

```stac
0 not               ;-> 1
5 not               ;-> 0
```

### try

`try (A B --> ..)`

Applies the first function. If it raises an error, applies the second and continues.

Use it to recover from a failure you expect, such as a file that may be missing. `protect` runs clean-up code and lets the error continue.

```stac
\['oops throw] \['caught] try   ;-> caught
```

### throw

`throw (a -->)`

Raises an error carrying the value. Uncaught, it prints as `error: value`.

Use it in a function to stop on bad input; `try` catches it.

```stac
\['bad-input throw] \['caught] try   ;-> caught
```

### protect

`protect (A B --> ..)`

Applies the first function, then the second, even when the first raises an error. The error then continues.

Use it for clean-up that must happen, such as freeing a buffer, whatever the outcome.

```stac
\[1] \[2] protect   ;-> 1 2
```

## form ops

### has

`has (form key --> bool)`

Pushes 1 if the form, or a parent, has the key, else 0.

Use it to give an event field a default only when it is missing.

```stac
{:a 1} 'a has       ;-> 1
{:a 1} 'z has       ;-> 0
```

### keys

`keys (form --> keys)`

Pushes a list of the form's own keys.

Use it to inspect an event or iterate over a form's fields.

```stac
{:a 1 :b 2} keys    ;-> [a b]
```

### values

`values (form --> values)`

Pushes a list of the form's own values, in key order.

Use it with `keys` to iterate over a form.

```stac
{:a 1 :b 2} values  ;-> [1 2]
```

### kv

`kv (form --> keys values)`

Pushes the form's keys, then its values, as two lists.

Use it to take a form apart in one step.

```stac
{:a 1 :b 2} kv      ;-> [a b] [1 2]
```

### local

`local (form --> local)`

Pushes the form without its parents.

Use it to see which fields a form sets itself, as opposed to those it inherits.

```stac
{:a 1} = p
{p :b 2} local      ;-> {:b 2}
```

### parent

`parent (form --> parent)`

Pushes the form's parents, the forms it inherits from.

Use it to inspect an event's defaults.

```stac
{:a 1} = p
{p :b 2} parent     ;-> {:a 1}
```

### dot

`dot (form key --> item)`

Pushes the form's value for a key, as `f,key` does, with the key as a value.

Use it when the key is computed, e.g. a field name passed to a function.

```stac
{:a 1 :b 2} 'b dot  ;-> 2
```

### pushWorkspace

`pushWorkspace (-->)`

Adds a new top layer to the workspace. Later top-level bindings go into it.

Use it with `popWorkspace` to try bindings and then forget them, as the dictionary's test runs each example.

```stac
pushWorkspace  5 = tmp  popWorkspace
```

### popWorkspace

`popWorkspace (-->)`

Removes the top layer of the workspace and forgets its bindings. The base layer cannot be removed.

Use it to discard bindings made since the matching `pushWorkspace`.

```stac
pushWorkspace  5 = tmp  popWorkspace
```

## ref ops

### get

`get (r --> a)`

Pushes the value a ref holds.

Use it to read state shared between functions or threads.

```stac
5 R = r
r get               ;-> 5
```

### set

`set (a r -->)`

Stores a value in a ref.

Use it to update state that several functions read.

```stac
5 R = r
7 r set
r get               ;-> 7
```

### R

`R (a --> r)`

Makes a ref, a mutable box, holding the value.

Use it for state that must change in place, such as a counter read by a running pattern. Most code needs no refs: bind a new value with `=`.

```stac
5 R = r
r get               ;-> 5
```

### ZR

`ZR (z --> r)`

Makes a ref that holds one real number.

Use it for a number shared with code that reads it as a signal.

```stac
5 ZR = z
z get               ;-> 5
```

### P

`P (a --> out in)`

Makes a plug pair from a value: an infinite list reading the plug, under the plug that feeds it.

Use it to change, while it plays, what a lazy list produces.

```stac
1 P = (out in)
```

### ZP

`ZP (a --> out in)`

Makes a plug pair for a signal list: an infinite signal reading the plug, under the plug that feeds it.

Use it as `P`, for signal lists.

```stac
1 ZP = (out in)
```

## function ops

### Y

`Y (funA --> funB)`

Pushes a function that calls the given one with itself as the last argument: the Y combinator. It lets an anonymous function recurse.

Use it for recursion in a function that has no name.

```stac
\x f [x 2 < \[1] \[x x -- f *] if] Y = fact
7 fact              ;-> 5040
```

### noeach

`noeach (fun --> fun)`

Marks a function so `@` arguments pass into it unmapped.

Use it for a function that handles `@` itself, such as one that forwards its arguments to another word.

```stac
\x [x] noeach = pass
```

### !

`! (... f --> ...)`

Applies the function on top of the stack to the values below it, observing `@` arguments.

Use it to call a function held as a value: one passed in, pushed with `` ` ``, or read from a form with `,`.

```stac
3 \x [x 1 +] !     ;-> 4
```

### !e

`!e (form fun --> ...)`

Applies a function, taking each argument from the form's field of the same name.

Use it to call a function with an event's fields, as `:instrument` functions get them.

```stac
{:x 3 :y 4} \x y [x y *] !e   ;-> 12
```

### compile

`compile (string --> fun)`

Compiles a string of stac source into a function.

Use it to run code built as text, e.g. read from a file. Prefer functions where the code is known.

```stac
"2 3 +" compile !   ;-> 5
```

## printing ops

### printLength

`printLength (--> length)`

Pushes how many items of a list `pr` prints before `...`. The default is 20.

Use it to save the setting before changing it with `setPrintLength`.

```stac
printLength         ;-> 20
```

### printDepth

`printDepth (--> depth)`

Pushes how many levels of nested lists `pr` prints. The default is 8.

Use it to save the setting before changing it with `setPrintDepth`.

```stac
printDepth          ;-> 8
```

### setPrintLength

`setPrintLength (length --> )`

Sets how many items of a list `pr` prints before `...`.

Use it to see more of a long list, or less of an infinite one.

```stac
printLength = n
3 setPrintLength
[1 2 3 4 5] pr cr
n setPrintLength
```

### setPrintDepth

`setPrintDepth (depth -->)`

Sets how many levels of nested lists `pr` prints.

Use it to see deep structures in full, or to keep a table's printout short.

```stac
printDepth = n
1 setPrintDepth
[[1 [2]]] pr cr
n setPrintDepth
```

### pr

`pr (A -->)`

Prints the top value and removes it, with no space or newline after it.

Use it with `cr` and `sp` to print results. The REPL prints the stack after each line on its own.

```stac
[1 2 3] pr cr
```

### prdebug

`prdebug (A -->)`

Prints the debug form of the top value and removes it.

Use it to see an object's type and identity when its plain form is ambiguous.

```stac
5 prdebug cr
```

### cr

`cr (-->)`

Prints a newline.

Use it to end a line of output.

```stac
1 pr cr
```

### sp

`sp (-->)`

Prints a space.

Use it between values printed on one line.

```stac
1 pr sp 2 pr cr
```

### tab

`tab (-->)`

Prints a tab.

Use it to align columns of printed values.

```stac
1 pr tab 2 pr cr
```

### prstk

`prstk (-->)`

Prints the whole stack, bottom first, without changing it.

Use it in the middle of a function to see what it has left so far.

```stac
1 2 prstk cr clear
```

### minfo

`minfo (-->)`

Prints the interpreter's memory statistics: objects and generators allocated and freed.

Use it to look for a leak in a long session.

```stac
minfo
```

### listdump

`listdump (list -->)`

Prints a list's internal structure: its address, its array's address and its size.

Use it when debugging stac itself, to see whether a list is packed.

```stac
[1 2 3] listdump
```

## string ops

### str

`str (x --> string)`

Converts a value to the string it prints as.

Use it to build file names or messages from numbers.

```stac
[1 2] str           ;-> [1 2]
42 str type         ;-> String
```

### debugstr

`debugstr (x --> string)`

Converts a value to its debug string, the form `prdebug` prints.

Use it to log an object's identity.

```stac
[1 2] debugstr type   ;-> String
```

### strcat

`strcat (list separator --> string)`

Joins the items of a list as strings, with the separator between each.

Use it to print a list in a chosen format, or to build a path.

```stac
[1 2 3] ", " strcat   ;-> 1, 2, 3
```

### strlines

`strlines (list --> string)`

Joins the items of a list as strings, one per line.

Use it to write a list to a text file, one value per line.

```stac
[1 2] strlines pr
```

### glob

`glob (pattern --> paths)`

Pushes a list of the file paths that match a shell pattern, such as `*.wav`.

Use it to load every sample in a folder.

```stac
"*.stac" glob size 0 >   ;-> 1
```

## sample rate ops

### sr

`sr (--> sampleRate)`

Pushes the interpreter's sample rate, 96000 unless `-r` sets another. List timing words such as `T` use it; scsynth runs at its own rate.

Use it to convert between seconds and list lengths in sapf-style code.

```stac
sr                  ;-> 96000
```

### nyq

`nyq (--> sampleRate/2)`

Pushes half the interpreter's sample rate, the Nyquist frequency.

Use it as an upper limit for a frequency.

```stac
nyq                 ;-> 48000
```

### isr

`isr (--> 1/sampleRate)`

Pushes 1 over the interpreter's sample rate: the length of one sample in seconds.

Use it to convert sample counts to seconds.

```stac
isr sr *            ;-> 1
```

### inyq

`inyq (--> 2/sampleRate)`

Pushes 2 over the interpreter's sample rate.

Use it to normalise a frequency to the Nyquist frequency.

```stac
inyq nyq *          ;-> 1
```

### rps

`rps (--> 2pi/sampleRate)`

Pushes 2 pi over the interpreter's sample rate: radians per sample at 1 Hz.

Use it to compute a phase increment for a frequency.

```stac
rps sr * 2pi /      ;-> 1
```

## help ops

### help

`help (fun -->)`

Prints the help line of a function.

Use it at the REPL with `` ` `` to read what a word takes and leaves.

```stac
`pr help
```

### helpbifs

`helpbifs (-->)`

Prints the help line of every built-in word, over 1000 lines.

Use it to search the built-ins from the terminal. `stac --words` gives the same as JSON.

### helpudfs

`helpudfs (-->)`

Prints the help line of every word the preludes and the session defined.

Use it to list the words of `std` and your own definitions.

### helpall

`helpall (-->)`

Prints `helpbifs`, then `helpudfs`.

Use it for a full listing; `stac --words` is easier to search.

### helpLine

`helpLine (string -->)`

Adds a line to the user-defined help, such as a section header `"*** name ***"`.

Use it in a library to group its words in `helpudfs` and `--words`.

```stac
"\n*** my words ***" helpLine
```

## thread ops

### go

`go (fun -->)`

Runs the function in a new thread and returns at once.

Use it for work that must not block the REPL, such as a slow file read. Patterns do not need it: `play` schedules them.

```stac
\["from a thread" pr cr] go
```

### sleep

`sleep (seconds -->)`

Pauses the current thread for the given number of seconds.

Use it inside a function run with `go`. At the REPL it blocks every following line.

```stac
.01 sleep
```

## misc

### type

`type (a --> symbol)`

Pushes the name of a value's type as a symbol: `Real`, `String`, `VList`, `ZList`, `Form`, `Fun`, `Prim`, `Sig` and the object types.

Use it to branch on what a function was given.

```stac
1 type              ;-> Real
[1] type            ;-> VList
```

### trace

`trace (bool -->)`

Turns tracing of every opcode on (1) or off (0).

Use it when debugging stac itself; it prints a line per step.

```stac
0 trace
```

## text files

### load

`load (filename -->)`

Compiles and runs a stac file in the global workspace.

Use it for a file of performance code. For a library, a module with `import` keeps its names private and reloads on change.

### prelude

`prelude (-->)`

Opens the prelude file in the default text editor.

Use it to read or edit the definitions a session starts with.

### examples

`examples (-->)`

Opens the examples file in the default text editor.

Use it to browse sapf's examples.

### logfile

`logfile (-->)`

Opens the log file in the default text editor.

Use it to review what a session evaluated; `$STAC_LOG` sets the file.

### readme

`readme (-->)`

Opens the README in the default text editor.

Use it for a quick reference while working at the REPL.

## unary math ops

### isalnum

`isalnum (x --> z)`

Pushes 1 if an ASCII code is a letter or a digit, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"a1 !" V isalnum    ;-> [1 1 0 0]
```

### isalpha

`isalpha (x --> z)`

Pushes 1 if an ASCII code is a letter, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"a1" V isalpha      ;-> [1 0]
```

### isblank

`isblank (x --> z)`

Pushes 1 if an ASCII code is a space or a tab, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
" a" V isblank      ;-> [1 0]
```

### iscntrl

`iscntrl (x --> z)`

Pushes 1 if an ASCII code is a control character, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
9 iscntrl           ;-> 1
```

### isdigit

`isdigit (x --> z)`

Pushes 1 if an ASCII code is a decimal digit, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"a1" V isdigit      ;-> [0 1]
```

### isgraph

`isgraph (x --> z)`

Pushes 1 if an ASCII code prints a visible mark, which excludes the space, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"a " V isgraph      ;-> [1 0]
```

### islower

`islower (x --> z)`

Pushes 1 if an ASCII code is a lower-case letter, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"aB" V islower      ;-> [1 0]
```

### isprint

`isprint (x --> z)`

Pushes 1 if an ASCII code is printable, the space included, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"a " V isprint      ;-> [1 1]
```

### ispunct

`ispunct (x --> z)`

Pushes 1 if an ASCII code is punctuation, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"a!" V ispunct      ;-> [0 1]
```

### isspace

`isspace (x --> z)`

Pushes 1 if an ASCII code is white space: a space, tab or newline, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
" a" V isspace      ;-> [1 0]
```

### isupper

`isupper (x --> z)`

Pushes 1 if an ASCII code is an upper-case letter, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"aB" V isupper      ;-> [0 1]
```

### isxdigit

`isxdigit (x --> z)`

Pushes 1 if an ASCII code is a hexadecimal digit, else 0. It maps over lists.

Use it to classify the characters of a string, read with `V`.

```stac
"fg" V isxdigit     ;-> [1 0]
```

### isascii

`isascii (x --> z)`

Pushes 1 if a value is an ASCII code, 0 to 127, else 0. It maps over lists.

Use it to check text before treating it as ASCII.

```stac
200 isascii         ;-> 0
```

### tolower

`tolower (x --> z)`

Converts an ASCII code to lower case. It maps over lists.

Use it with `V` and `unspell` to change a string's case.

```stac
"aB" V tolower unspell   ;-> ab
```

### toupper

`toupper (x --> z)`

Converts an ASCII code to upper case. It maps over lists.

Use it with `V` and `unspell` to change a string's case.

```stac
"aB" V toupper unspell   ;-> AB
```

### toascii

`toascii (x --> z)`

Keeps the low 7 bits of a value, making it an ASCII code. It maps over lists.

Use it to clear the high bit of bytes read from a file.

```stac
200 toascii         ;-> 72
```

### 0<=

`0<= (x --> z)`

Pushes 1 if a value is 0 or less, else 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a one-word test against zero.

```stac
[-1 0 1] 0<=        ;-> [1 1 0]
```

### 0>=

`0>= (x --> z)`

Pushes 1 if a value is 0 or more, else 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a one-word test against zero.

```stac
[-1 0 1] 0>=        ;-> [0 1 1]
```

### 0<

`0< (x --> z)`

Pushes 1 if a value is negative, else 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to find the negative half of a signal.

```stac
[-1 0 1] 0<         ;-> [1 0 0]
```

### 0>

`0> (x --> z)`

Pushes 1 if a value is positive, else 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn a bipolar signal into a gate.

```stac
[-1 0 1] 0>         ;-> [0 0 1]
```

### 0=

`0= (x --> z)`

Pushes 1 if a value is 0, else 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to find silences or empty steps.

```stac
[-1 0 1] 0=         ;-> [0 1 0]
```

### even?

`even? (x --> z)`

Pushes 1 if a value is an even integer, else 0. It maps over lists.

Use it to pick every other step of a sequence.

```stac
[1 2 3] even?       ;-> [0 1 0]
```

### odd?

`odd? (x --> z)`

Pushes 1 if a value is an odd integer, else 0. It maps over lists.

Use it to pick every other step of a sequence.

```stac
[1 2 3] odd?        ;-> [1 0 1]
```

### prime?

`prime? (x --> z)`

Pushes 1 if a value is a prime number, else 0. It maps over lists.

Use it to build sequences from primes, which never repeat in short cycles.

```stac
[7 9] prime?        ;-> [1 0]
```

### int?

`int? (x --> z)`

Pushes 1 if a value is a whole number, else 0. It maps over lists.

Use it to check an index or a count.

```stac
[2 2.5] int?        ;-> [1 0]
```

### isfinite

`isfinite (x --> z)`

Pushes 1 if a value is a finite number, else 0. It maps over lists.

Use it to catch infinities and NaN before they reach a synth.

```stac
1 isfinite          ;-> 1
```

### isinf

`isinf (x --> z)`

Pushes 1 if a value is an infinity, else 0. It maps over lists.

Use it to detect a division by zero.

```stac
1 0 / isinf         ;-> 1
```

### isnan

`isnan (x --> z)`

Pushes 1 if a value is NaN, not a number, else 0. It maps over lists.

Use it to detect an undefined result such as 0/0.

```stac
0 0 / isnan         ;-> 1
```

### isnormal

`isnormal (x --> z)`

Pushes 1 if a value is a normal floating-point number: not 0, denormal, infinite or NaN. It maps over lists.

Use it to find denormals, which slow some processors.

```stac
1 isnormal          ;-> 1
```

### signbit

`signbit (x --> z)`

Pushes 1 if a value's sign bit is set, as for negative numbers and -0, else 0. It maps over lists.

Use it to tell -0 from 0.

```stac
-1 signbit          ;-> 1
```

### abs

`abs (x --> z)`

Pushes the absolute value. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for magnitudes, e.g. to rectify a signal.

```stac
-3 abs              ;-> 3
```

### sgn

`sgn (x --> z)`

Pushes -1, 0 or 1 as a value is negative, zero or positive. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to keep only a value's direction, e.g. to square a waveform.

```stac
-3 sgn              ;-> -1
```

### ~

`~ (x --> z)`

Pushes 1 for 0 and 0 for anything else: logical negation. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert a gate.

```stac
0 ~                 ;-> 1
3 ~                 ;-> 0
```

### neg

`neg (x --> z)`

Negates a value. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert a signal's polarity.

```stac
3 neg               ;-> -3
```

### sqrt

`sqrt (x --> z)`

Pushes the square root. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for equal-power gains and RMS levels.

```stac
16 sqrt             ;-> 4
```

### cbrt

`cbrt (x --> z)`

Pushes the cube root. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert a cubic curve.

```stac
27 cbrt             ;-> 3
```

### rsqrt

`rsqrt (x --> z)`

Pushes 1 over the square root. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to normalise a vector by its length.

```stac
4 rsqrt             ;-> 0.5
```

### sq

`sq (x --> z)`

Squares a value: `x x *`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for power from amplitude and for squared curves.

```stac
3 sq                ;-> 9
```

### ssq

`ssq (x --> z)`

Squares a value and keeps its sign: `x x abs *`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to curve a bipolar control without losing its direction.

```stac
-3 ssq              ;-> -9
```

### cb

`cb (x --> z)`

Cubes a value. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a steeper curve than `sq`.

```stac
2 cb                ;-> 8
```

### ^2

`^2 (x --> z)`

Raises a value to the power 2. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as `sq`.

```stac
2 ^2                ;-> 4
```

### ^3

`^3 (x --> z)`

Raises a value to the power 3. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as `cb`.

```stac
2 ^3                ;-> 8
```

### ^4

`^4 (x --> z)`

Raises a value to the power 4. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a steep curve that stays positive.

```stac
2 ^4                ;-> 16
```

### ^5

`^5 (x --> z)`

Raises a value to the power 5. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a steep curve that keeps the sign.

```stac
2 ^5                ;-> 32
```

### ^6

`^6 (x --> z)`

Raises a value to the power 6. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a very steep positive curve.

```stac
2 ^6                ;-> 64
```

### ^7

`^7 (x --> z)`

Raises a value to the power 7. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a very steep curve that keeps the sign.

```stac
2 ^7                ;-> 128
```

### ^8

`^8 (x --> z)`

Raises a value to the power 8. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a very steep positive curve.

```stac
2 ^8                ;-> 256
```

### ^9

`^9 (x --> z)`

Raises a value to the power 9. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for a very steep curve that keeps the sign.

```stac
2 ^9                ;-> 512
```

### recip

`recip (x --> z)`

Pushes 1 over a value. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn a frequency into a period.

```stac
4 recip             ;-> 0.25
```

### 1/

`1/ (x --> z)`

Pushes 1 over a value, as `recip`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn a period into a frequency.

```stac
4 1/                ;-> 0.25
```

### exp

`exp (x --> z)`

Pushes e to the power x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for exponential curves.

```stac
1 exp               ;-> 2.71828
```

### exp2

`exp2 (x --> z)`

Pushes 2 to the power x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn octaves into a frequency ratio.

```stac
3 exp2              ;-> 8
```

### exp10

`exp10 (x --> z)`

Pushes 10 to the power x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn bels into a power ratio.

```stac
2 exp10             ;-> 100
```

### e^

`e^ (x --> z)`

Pushes e to the power x, as `exp`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for exponential curves.

```stac
1 e^                ;-> 2.71828
```

### 2^

`2^ (x --> z)`

Pushes 2 to the power x, as `exp2`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn octaves into a ratio.

```stac
3 2^                ;-> 8
```

### 10^

`10^ (x --> z)`

Pushes 10 to the power x, as `exp10`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn bels into a ratio.

```stac
2 10^               ;-> 100
```

### expm1

`expm1 (x --> z)`

Pushes e to the power x, minus 1, accurate for very small x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it where x is near 0 and `exp` would lose precision.

```stac
1e-10 expm1         ;-> 1e-10
```

### log

`log (x --> z)`

Pushes the natural logarithm. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to linearise an exponential quantity.

```stac
e log               ;-> 1
```

### log2

`log2 (x --> z)`

Pushes the base-2 logarithm. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn a frequency ratio into octaves.

```stac
8 log2              ;-> 3
```

### log10

`log10 (x --> z)`

Pushes the base-10 logarithm. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn a ratio into bels.

```stac
1000 log10          ;-> 3
```

### log1p

`log1p (x --> z)`

Pushes the natural logarithm of 1 + x, accurate for very small x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it where x is near 0 and `log` would lose precision.

```stac
1e-10 log1p         ;-> 1e-10
```

### logb

`logb (x --> z)`

Pushes the base-2 exponent of x, rounded down: `x log2 floor`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to find which octave a ratio falls in.

```stac
10 logb             ;-> 3
```

### frac

`frac (x --> z)`

Pushes the fractional part: x minus x rounded down, so it is never negative. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to wrap a phase into 0 to 1.

```stac
2.7 frac            ;-> 0.7
-2.7 frac           ;-> 0.3
```

### floor

`floor (x --> z)`

Rounds down to a whole number. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to quantise a value down to an integer step.

```stac
-2.7 floor          ;-> -3
```

### ceil

`ceil (x --> z)`

Rounds up to a whole number. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to find how many whole blocks hold a length.

```stac
2.2 ceil            ;-> 3
```

### rint

`rint (x --> z)`

Rounds to the nearest whole number, halves to even. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to quantise to integers without a bias up or down.

```stac
2.5 rint            ;-> 2
3.5 rint            ;-> 4
```

### erf

`erf (x --> z)`

Pushes the error function. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for smooth S-shaped curves and Gaussian probabilities.

```stac
0 erf               ;-> 0
```

### erfc

`erfc (x --> z)`

Pushes 1 minus the error function. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for Gaussian tail probabilities.

```stac
0 erfc              ;-> 1
```

### sinc

`sinc (x --> z)`

Pushes sin(x)/x, with 1 at x = 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to build band-limited interpolation filters.

```stac
0 sinc              ;-> 1
1 sinc              ;-> 0.841471
```

### sin

`sin (x --> z)`

Pushes the sine, x in radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for waveshaping and to compute wave tables.

```stac
pi 2 / sin          ;-> 1
```

### cos

`cos (x --> z)`

Pushes the cosine, x in radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for equal-power panning curves.

```stac
0 cos               ;-> 1
```

### sin1

`sin1 (x --> z)`

Pushes the sine of x times 2 pi: one cycle per unit. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to compute a wave table from a 0 to 1 phase.

```stac
.25 sin1            ;-> 1
```

### cos1

`cos1 (x --> z)`

Pushes the cosine of x times 2 pi. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to compute a wave table from a 0 to 1 phase.

```stac
.5 cos1             ;-> -1
```

### tan

`tan (x --> z)`

Pushes the tangent, x in radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for waveshaping with a steep curve.

```stac
pi 4 / tan          ;-> 1
```

### asin

`asin (x --> z)`

Pushes the arcsine in radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert `sin`.

```stac
1 asin              ;-> 1.5708
```

### acos

`acos (x --> z)`

Pushes the arccosine in radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert `cos`.

```stac
1 acos              ;-> 0
```

### atan

`atan (x --> z)`

Pushes the arctangent in radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a soft limiter: it never exceeds pi/2.

```stac
1 atan              ;-> 0.785398
```

### sinh

`sinh (x --> z)`

Pushes the hyperbolic sine. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it in curve formulas.

```stac
0 sinh              ;-> 0
```

### cosh

`cosh (x --> z)`

Pushes the hyperbolic cosine. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it in curve formulas.

```stac
0 cosh              ;-> 1
```

### tanh

`tanh (x --> z)`

Pushes the hyperbolic tangent. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a saturating distortion that stays within -1 to 1.

```stac
1 tanh              ;-> 0.761594
[100 saw 4 * tanh .1 *] play
```

### asinh

`asinh (x --> z)`

Pushes the inverse hyperbolic sine. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a gentle compressor curve that keeps the sign.

```stac
0 asinh             ;-> 0
```

### acosh

`acosh (x --> z)`

Pushes the inverse hyperbolic cosine, for x of 1 or more. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert `cosh`.

```stac
1 acosh             ;-> 0
```

### atanh

`atanh (x --> z)`

Pushes the inverse hyperbolic tangent, for x between -1 and 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert `tanh`.

```stac
0 atanh             ;-> 0
```

### J0

`J0 (x --> z)`

Pushes the Bessel function of the first kind of order 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for the carrier amplitude of FM at index x.

```stac
0 J0                ;-> 1
```

### J1

`J1 (x --> z)`

Pushes the Bessel function of the first kind of order 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for the first sideband amplitude of FM at index x.

```stac
0 J1                ;-> 0
```

### Y0

`Y0 (x --> z)`

Pushes the Bessel function of the second kind of order 0, for positive x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it in acoustics formulas.

```stac
1 Y0                ;-> 0.088257
```

### Y1

`Y1 (x --> z)`

Pushes the Bessel function of the second kind of order 1, for positive x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it in acoustics formulas.

```stac
1 Y1                ;-> -0.781213
```

### tgamma

`tgamma (x --> z)`

Pushes the gamma function; for a whole number n it is (n - 1) factorial. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for factorials of non-integers.

```stac
5 tgamma            ;-> 24
```

### lgamma

`lgamma (x --> z)`

Pushes the natural logarithm of the absolute value of the gamma function. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for large factorials that would overflow.

```stac
5 lgamma            ;-> 3.17805
```

### inc

`inc (x --> z)`

Adds 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to step a counter.

```stac
1 inc               ;-> 2
```

### dec

`dec (x --> z)`

Subtracts 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to step a counter down.

```stac
1 dec               ;-> 0
```

### half

`half (x --> z)`

Multiplies by .5. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to halve a level or a frequency.

```stac
3 half              ;-> 1.5
```

### twice

`twice (x --> z)`

Multiplies by 2. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to double a frequency, an octave up.

```stac
3 twice             ;-> 6
```

### ++

`++ (x --> z)`

Adds 1, as `inc`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to step a counter.

```stac
1 ++                ;-> 2
```

### --

`-- (x --> z)`

Subtracts 1, as `dec`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to step a counter down.

```stac
1 --                ;-> 0
```

### /2

`/2 (x --> z)`

Multiplies by .5, as `half`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to halve a value.

```stac
3 /2                ;-> 1.5
```

### *2

`*2 (x --> z)`

Multiplies by 2, as `twice`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to double a value.

```stac
3 *2                ;-> 6
```

### biuni

`biuni (x --> z)`

Maps -1 to 1 onto 0 to 1: `.5 * .5 +`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn an LFO into a unipolar control.

```stac
-1 biuni            ;-> 0
```

### unibi

`unibi (x --> z)`

Maps 0 to 1 onto -1 to 1: `2 * 1 -`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to turn a unipolar control into a bipolar one.

```stac
1 unibi             ;-> 1
```

### biunic

`biunic (x --> z)`

Maps -1 to 1 onto 0 to 1, clipping the input to -1 to 1 first. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it where the input may overshoot.

```stac
2 biunic            ;-> 1
```

### unibic

`unibic (x --> z)`

Maps 0 to 1 onto -1 to 1, clipping the input to 0 to 1 first. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it where the input may overshoot.

```stac
2 unibic            ;-> 1
```

### cmpl

`cmpl (x --> z)`

Pushes 1 minus x. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to invert a unipolar control, e.g. a crossfade.

```stac
.25 cmpl            ;-> 0.75
```

### ampdb

`ampdb (x --> z)`

Converts a linear amplitude to decibels. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to display a level.

```stac
.5 ampdb            ;-> -6.0206
```

### dbamp

`dbamp (x --> z)`

Converts decibels to a linear amplitude. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to set a level in decibels: `-12 dbamp`.

```stac
-6 dbamp            ;-> 0.501187
```

### ohz

`ohz (x --> z)`

Converts octaves to hertz; octave 0 is middle C, 261.626 Hz. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to write pitch as octaves, as sapf's examples do.

```stac
0 ohz               ;-> 261.626
```

### hzo

`hzo (x --> z)`

Converts hertz to octaves above middle C. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to measure an interval in octaves.

```stac
523.251 hzo         ;-> 1
```

### nnhz

`nnhz (x --> z)`

Converts MIDI note numbers to hertz; 60 is middle C, 69 is 440 Hz. SC calls it `midicps`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to play note numbers.

```stac
69 nnhz             ;-> 440
```

### hznn

`hznn (x --> z)`

Converts hertz to MIDI note numbers. SC calls it `cpsmidi`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to find the nearest note to a frequency.

```stac
440 hznn            ;-> 69
```

### centsratio

`centsratio (x --> z)`

Converts cents to a frequency ratio. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for microtonal detuning.

```stac
1200 centsratio     ;-> 2
```

### ratiocents

`ratiocents (x --> z)`

Converts a frequency ratio to cents. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to measure a just interval in cents.

```stac
2 ratiocents        ;-> 1200
```

### semiratio

`semiratio (x --> z)`

Converts semitones to a frequency ratio. SC calls it `midiratio`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to transpose a frequency.

```stac
12 semiratio        ;-> 2
```

### ratiosemi

`ratiosemi (x --> z)`

Converts a frequency ratio to semitones. SC calls it `ratiomidi`. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to measure an interval in semitones.

```stac
2 ratiosemi         ;-> 12
```

### minsec

`minsec (x --> z)`

Multiplies by 60: minutes to seconds, or beats per second to beats per minute. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to convert tempo units.

```stac
2 minsec            ;-> 120
```

### secmin

`secmin (x --> z)`

Divides by 60: seconds to minutes, or beats per minute to beats per second. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to set a clock from a BPM: `120 secmin clock.new`.

```stac
120 secmin          ;-> 2
```

### bpmsec

`bpmsec (x --> z)`

Converts beats per minute to the period of one beat in seconds. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for tempo-synced delay times.

```stac
120 bpmsec          ;-> 0.5
```

### degrad

`degrad (x --> z)`

Converts degrees to radians. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to give angles to trigonometric words.

```stac
180 degrad          ;-> 3.14159
```

### raddeg

`raddeg (x --> z)`

Converts radians to degrees. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to display an angle.

```stac
pi raddeg           ;-> 180
```

### distort

`distort (x --> z)`

Shapes a value with x/(1 + |x|), a soft sigmoid that stays within -1 to 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a gentle distortion.

```stac
1 distort           ;-> 0.5
```

### softclip

`softclip (x --> z)`

Passes values within -.5 to .5 unchanged and curves larger ones towards 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to limit peaks while leaving quiet signals clean.

```stac
.25 softclip        ;-> 0.25
1 softclip          ;-> 0.75
```

### sigm

`sigm (x --> z)`

Shapes a value with x/sqrt(1 + x squared), a sigmoid within -1 to 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it as a smooth saturation.

```stac
1 sigm              ;-> 0.707107
```

### rectWin

`rectWin (x --> z)`

A rectangular window: 1 for x from 0 to 1, else 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to compute a window table.

```stac
.5 rectWin          ;-> 1
```

### triWin

`triWin (x --> z)`

A triangular window over 0 to 1, peaking at 1 at x = .5. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to compute a grain envelope table.

```stac
.5 triWin           ;-> 1
```

### bitriWin

`bitriWin (x --> z)`

A triangular window over -1 to 1, peaking at 1 at x = 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it where the window is centred on 0.

```stac
0 bitriWin          ;-> 1
```

### hanWin

`hanWin (x --> z)`

A Hann window over 0 to 1, peaking at 1 at x = .5. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for smooth grain envelopes.

```stac
.5 hanWin           ;-> 1
```

### sinWin

`sinWin (x --> z)`

A sine window over 0 to 1, peaking at 1 at x = .5. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for equal-power crossfades and grains.

```stac
.5 sinWin           ;-> 1
```

### scurve

`scurve (x --> z)`

An S curve: 0 below 0, 3x squared minus 2x cubed between 0 and 1, 1 above 1. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it for smooth fades.

```stac
.5 scurve           ;-> 0.5
```

### zapgremlins

`zapgremlins (x --> z)`

Replaces NaN, infinities and denormals with 0. It maps over lists and builds a `UnaryOpUGen` on UGens.

Use it to protect a feedback path from values that would make it explode or stall.

```stac
0 0 / zapgremlins   ;-> 0
1 zapgremlins       ;-> 1
```

## binary math ops

### +

`+ (x y --> z)`

Adds two values. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for sums and offsets; on two UGens it mixes them.

```stac
2 3 +               ;-> 5
[1 2 3] [10 20] +   ;-> [11 22]
[300 sinosc 302 sinosc + .05 *] play
```

### +>

`+> (x y --> z)`

Adds two values; on lists it extends the shorter one with zeros, so the result is as long as the longer.

Use it to add a short list of offsets to the start of a longer one.

```stac
[1 2 3] [10 20] +>  ;-> [11 22 3]
```

### -

`- (x y --> z)`

Subtracts the top value from the one below. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for differences and to invert an offset.

```stac
7 2 -               ;-> 5
```

### *

`* (x y --> z)`

Multiplies two values. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to scale a signal by an amplitude or an envelope.

```stac
7 2 *               ;-> 14
[440 sinosc .1 *] play
```

### /

`/ (x y --> z)`

Divides the lower value by the top one, as reals. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for ratios; `idiv` gives whole-number division.

```stac
7 2 /               ;-> 3.5
```

### %

`% (x y --> z)`

Modulo, with the sign of the divisor: `-7 2 %` is 1. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to wrap a counter into a range of a length.

```stac
7 2 %               ;-> 1
-7 2 %              ;-> 1
```

### idiv

`idiv (x y --> z)`

Divides and rounds down to a whole number. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to find which octave or bar a count falls in.

```stac
7 2 idiv            ;-> 3
-7 2 idiv           ;-> -4
```

### imod

`imod (x y --> z)`

Integer modulo, the remainder after `idiv`; it is never negative for a positive divisor. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it with `idiv` to split a count into a bar and a beat.

```stac
-7 2 imod           ;-> 1
```

### remainder

`remainder (x y --> z)`

The IEEE remainder: x minus y times x/y rounded to the nearest integer. It can be negative. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it where a remainder centred on 0 is wanted, such as phase errors.

```stac
7 2 remainder       ;-> -1
```

### <

`< (x y --> z)`

Pushes 1 if the lower value is less than the top one, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for tests and, on UGens, for gates: `sig .5 <`.

```stac
1 2 <               ;-> 1
```

### <=

`<= (x y --> z)`

Pushes 1 if the lower value is less than or equal to the top one, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for inclusive range tests.

```stac
2 2 <=              ;-> 1
```

### >

`> (x y --> z)`

Pushes 1 if the lower value is greater than the top one, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for tests and triggers on thresholds.

```stac
1 2 >               ;-> 0
```

### >=

`>= (x y --> z)`

Pushes 1 if the lower value is greater than or equal to the top one, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for inclusive range tests.

```stac
2 2 >=              ;-> 1
```

### ==

`== (x y --> z)`

Pushes 1 if two numbers are equal, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to compare numbers item by item; `equals` compares whole structures.

```stac
2 2 ==              ;-> 1
```

### !=

`!= (x y --> z)`

Pushes 1 if two numbers differ, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to find the items of a list that changed.

```stac
2 3 !=              ;-> 1
```

### cmp

`cmp (x y --> z)`

Pushes -1, 0 or 1 as the lower value is less than, equal to or greater than the top one. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it as a sort key or to find the direction of a change.

```stac
1 2 cmp             ;-> -1
3 2 cmp             ;-> 1
```

### copysign

`copysign (x y --> z)`

Pushes the lower value's magnitude with the top value's sign. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to make one value follow another's direction.

```stac
3 -1 copysign       ;-> -3
```

### nextafter

`nextafter (x y --> z)`

Pushes the next representable floating-point number after x towards y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to step past a boundary by the smallest amount, e.g. to make a range exclusive.

```stac
1 2 nextafter 1 -   ;-> 2.22045e-16
```

### pow

`pow (x y --> z)`

Raises x to the power y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for exponential curves; `^` is the same.

```stac
2 10 pow            ;-> 1024
```

### ^

`^ (x y --> z)`

Raises x to the power y, as `pow`. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for exponential curves and gain laws.

```stac
2 10 ^              ;-> 1024
```

### atan2

`atan2 (x y --> z)`

Pushes the arctangent of x/y in radians, as C's `atan2(x, y)`: the angle of the point (y, x). On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to turn a pair of coordinates into an angle, e.g. for panning.

```stac
1 0 atan2           ;-> 1.5708
1 1 atan2           ;-> 0.785398
```

### Jn

`Jn (x y --> z)`

Pushes the Bessel function of the first kind of order y at x. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to compute FM sideband amplitudes.

```stac
1 0 Jn              ;-> 0.765198
```

### Yn

`Yn (x y --> z)`

Pushes the Bessel function of the second kind of order y at x. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it alongside `Jn` in acoustics formulas.

```stac
1 1 Yn              ;-> -0.781213
```

### &

`& (x y --> z)`

Pushes the smaller of two values; on 0 and 1 it is logical and. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to combine two gates, or as a minimum.

```stac
3 5 &               ;-> 3
1 0 &               ;-> 0
```

### |

`| (x y --> z)`

Pushes the larger of two values; on 0 and 1 it is logical or. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to combine two gates, or as a maximum.

```stac
3 5 |               ;-> 5
1 0 |               ;-> 1
```

### avg2

`avg2 (x y --> z)`

Pushes the mean of two values. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to find a midpoint, or to mix two signals at half level each.

```stac
3 5 avg2            ;-> 4
```

### dim

`dim (x y --> z)`

Pushes x - y when it is positive, else 0. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to measure how far a value exceeds a threshold.

```stac
5 3 dim             ;-> 2
3 5 dim             ;-> 0
```

### absdif

`absdif (x y --> z)`

Pushes the absolute difference of two values. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to measure the distance between two pitches or levels.

```stac
3 5 absdif          ;-> 2
```

### hypot

`hypot (x y --> z)`

Pushes the square root of x squared plus y squared. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for the magnitude of a vector, e.g. a stereo signal's distance from centre.

```stac
3 4 hypot           ;-> 5
```

### sumsq

`sumsq (x y --> z)`

Pushes x squared plus y squared. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for energy sums without the square root `hypot` takes.

```stac
3 4 sumsq           ;-> 25
```

### difsq

`difsq (x y --> z)`

Pushes x squared minus y squared. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it in formulas with a difference of squares.

```stac
3 4 difsq           ;-> -7
```

### sqsum

`sqsum (x y --> z)`

Pushes the square of x plus y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it as `+` followed by `sq`, in one word.

```stac
3 4 sqsum           ;-> 49
```

### sqdif

`sqdif (x y --> z)`

Pushes the square of x minus y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for squared errors.

```stac
3 4 sqdif           ;-> 1
```

### thresh

`thresh (x y --> z)`

Pushes 0 when x is less than y, else x. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it as a noise gate on a control signal.

```stac
.5 1 thresh         ;-> 0
2 1 thresh          ;-> 2
```

### absthresh

`absthresh (x y --> z)`

Pushes 0 when x's magnitude is less than y, else x. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to gate small values of either sign.

```stac
-2 1 absthresh      ;-> -2
.5 1 absthresh      ;-> 0
```

### amclip

`amclip (x y --> z)`

Pushes 0 when y is 0 or less, else x times y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for amplitude modulation that never inverts the signal.

```stac
2 0 amclip          ;-> 0
2 3 amclip          ;-> 6
```

### scaleneg

`scaleneg (x y --> z)`

Pushes x times y when x is negative, else x. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to scale only the negative half of a waveform.

```stac
-2 3 scaleneg       ;-> -6
2 3 scaleneg        ;-> 2
```

### ring1

`ring1 (x y --> z)`

Pushes x times y plus x: ring modulation with the carrier added. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for ring modulation that keeps the dry signal.

```stac
2 3 ring1           ;-> 8
```

### ring2

`ring2 (x y --> z)`

Pushes x times y plus x plus y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for ring modulation that keeps both inputs.

```stac
2 3 ring2           ;-> 11
```

### ring3

`ring3 (x y --> z)`

Pushes x squared times y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for a ring-modulation variant with more of x's harmonics.

```stac
2 3 ring3           ;-> 12
```

### ring4

`ring4 (x y --> z)`

Pushes x times y times (x minus y). On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for a ring-modulation variant with difference tones.

```stac
2 3 ring4           ;-> -6
```

### gcd

`gcd (x y --> z)`

Pushes the greatest common divisor. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to reduce a ratio, e.g. for just intonation.

```stac
12 18 gcd           ;-> 6
```

### lcm

`lcm (x y --> z)`

Pushes the least common multiple. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to find when two cycles line up again, e.g. a polyrhythm's length.

```stac
4 6 lcm             ;-> 12
```

### clip0

`clip0 (x y --> z)`

Limits x to the range 0 to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for a parameter that must stay non-negative and bounded.

```stac
12 10 clip0         ;-> 10
```

### wrap0

`wrap0 (x y --> z)`

Wraps x into the range 0 up to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for a phase or an index that cycles from 0.

```stac
12 10 wrap0         ;-> 2
```

### fold0

`fold0 (x y --> z)`

Folds x back into the range 0 to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for a value that bounces between 0 and a limit.

```stac
12 10 fold0         ;-> 8
```

### clip2

`clip2 (x y --> z)`

Limits x to the range -y to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to keep a bipolar signal within a level.

```stac
12 10 clip2         ;-> 10
```

### wrap2

`wrap2 (x y --> z)`

Wraps x into the range -y up to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it for a bipolar value that cycles.

```stac
12 10 wrap2         ;-> -8
```

### fold2

`fold2 (x y --> z)`

Folds x back into the range -y to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it as a wavefolder on a bipolar signal.

```stac
12 10 fold2         ;-> 8
```

### iwrap2

`iwrap2 (x y --> z)`

Wraps an integer x into the range -y to y, both included. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to wrap a signed index.

```stac
12 10 iwrap2        ;-> -9
```

### ifold2

`ifold2 (x y --> z)`

Folds an integer x back into the range -y to y, both included. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to bounce a signed index between limits.

```stac
12 10 ifold2        ;-> 8
```

### excess

`excess (x y --> z)`

Pushes what `clip2` removes: x minus x clipped to -y to y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to keep the part of a signal over a level, e.g. for a distortion.

```stac
12 10 excess        ;-> 2
```

### round

`round (x y --> z)`

Rounds x to the nearest multiple of y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to quantise times to a grid or pitches to a step.

```stac
7.3 .5 round        ;-> 7.5
```

### roundUp

`roundUp (x y --> z)`

Rounds x up to a multiple of y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to find the next grid line at or after a time.

```stac
7.3 .5 roundUp      ;-> 7.5
```

### trunc

`trunc (x y --> z)`

Rounds x down to a multiple of y. On lists it works item by item and stops at the shorter list; on UGens it builds a `BinaryOpUGen`.

Use it to find the grid line at or before a time.

```stac
7.3 .5 trunc        ;-> 7
```

## list conversion

### V

`V (signal --> stream)`

Converts a signal list or a string to a list of values.

Use it to apply list words to a `#[...]` or to the characters of a string.

```stac
#[1 2 3] V          ;-> [1 2 3]
"abc" V             ;-> [97 98 99]
```

### Z

`Z (series --> signal)`

Converts a list or a string to a signal list of floats.

Use it to pass a computed list to a word that takes `#[...]`, such as `buffer.load`.

```stac
[1 2 3] Z           ;-> #[1 2 3]
```

### L

`L (anything --> stream)`

Leaves a list as it is; makes anything else an infinite list of itself.

Use it so a word accepts a constant where it reads a pattern.

```stac
5 L 3 take          ;-> [5 5 5]
```

### L1

`L1 (anything --> stream)`

Leaves a list as it is; wraps anything else in a one-item list.

Use it so a word accepts one value where it reads a finite list.

```stac
5 L1                ;-> [5]
[5] L1              ;-> [5]
```

### unspell

`unspell (sequence --> string)`

Converts a list of character codes to a string.

Use it to build text from numbers, the inverse of `"text" V`.

```stac
[104 105] unspell   ;-> hi
```

## basic list operations

### size

`size (seq --> num)`

Pushes the number of items in a list. A list stac cannot tell is finite has size `inf`.

Use it to loop over a list's indices or to check a pattern's length.

```stac
[1 2 3] size        ;-> 3
ord size            ;-> inf
```

### rank

`rank (a --> n)`

Pushes the number of nested list levels: 0 for a number, 1 for a flat list. It reads only the first item at each level.

Use it to tell a list of channels from a list of chords before processing it.

```stac
5 rank              ;-> 0
[[1 2] [3 4]] rank  ;-> 2
```

### shape

`shape (a --> [n..])`

Pushes the size of each nested level, outermost first. A level stac cannot tell is finite is `inf`.

Use it to check a table's dimensions, e.g. before `flop`.

```stac
[[1 2] [3 4] [5 6]] shape   ;-> #[3 2]
```

### finite

`finite (seq --> bool)`

Pushes 1 if stac knows the list ends, else 0.

Use it before a word that must read a whole list, such as `size` or `reverse`.

```stac
[1 2 3] finite      ;-> 1
ord finite          ;-> 0
```

### empty

`empty (list --> bool)`

Pushes 1 if the list has no items, else 0.

Use it to end a recursion over a list.

```stac
[] empty            ;-> 1
[1] empty           ;-> 0
```

### nonempty

`nonempty (list --> bool)`

Pushes 1 if the list has at least one item, else 0.

Use it as the test of a loop that consumes a list.

```stac
[1] nonempty        ;-> 1
```

### head

`head (list --> item)`

Pushes the first item of a list. An empty list is an error.

Use it with `tail` to take a list apart one item at a time.

```stac
[1 2 3] head        ;-> 1
```

### tail

`tail (list --> list)`

Pushes the list without its first item. An empty list is an error.

Use it with `head`; on an infinite list it stays lazy.

```stac
[1 2 3] tail        ;-> [2 3]
```

### add

`add (list item --> list)`

Pushes a new list with the item appended.

Use it to build a list item by item. For many items, a list literal `[...]` is shorter.

```stac
[1 2] 3 add         ;-> [1 2 3]
```

### cons

`cons (list item --> list)`

Pushes a new list with the item in front.

Use it to prepend, as Lisp's `cons`; it is cheaper than `add` on a long list.

```stac
[1 2] 0 cons        ;-> [0 1 2]
```

### uncons

`uncons (list --> tail head)`

Pushes the tail, then the head on top. An empty list is an error.

Use it to take the first item and keep the rest in one step.

```stac
[1 2 3] uncons      ;-> [2 3] 1
```

### pack

`pack (list --> list)`

Pushes the list with its items computed and stored in one array.

Use it when a lazy list will be read many times, so its items are computed once.

```stac
[1 2 3] 2 * pack packed   ;-> 1
```

### packed

`packed (list --> bool)`

Pushes 1 if the list's items are already computed and stored in one array, else 0.

Use it to check whether a list is lazy, when tracking down slow code.

```stac
[1 2 3] 2 * packed        ;-> 0
```

## list generation

### ord

`ord (--> series)`

Pushes the infinite list 1, 2, 3...

An infinite list; take what you need with `take`, or let a pattern read it. Use it for harmonic numbers and step counts.

```stac
ord 4 take          ;-> [1 2 3 4]
440 ord 4 take *    ;-> [440 880 1320 1760]
```

### nat

`nat (--> series)`

Pushes the infinite list 0, 1, 2...

An infinite list; take what you need with `take`, or let a pattern read it. Use it for indices.

```stac
nat 4 take          ;-> [0 1 2 3]
```

### invs

`invs (--> series)`

Pushes the infinite list 1, 1/2, 1/3...: `ord 1/`.

An infinite list; take what you need with `take`, or let a pattern read it. Use it for the amplitudes of a sawtooth's harmonics.

```stac
invs 3 take         ;-> [1 0.5 0.333333]
```

### negs

`negs (--> series)`

Pushes the infinite list -1, -2, -3...

An infinite list; take what you need with `take`, or let a pattern read it.

```stac
negs 3 take         ;-> [-1 -2 -3]
```

### evens

`evens (--> series)`

Pushes the infinite list 0, 2, 4...

An infinite list; take what you need with `take`, or let a pattern read it.

```stac
evens 4 take        ;-> [0 2 4 6]
```

### odds

`odds (--> series)`

Pushes the infinite list 1, 3, 5...

An infinite list; take what you need with `take`, or let a pattern read it. Use it for the harmonics of a square wave.

```stac
odds 4 take         ;-> [1 3 5 7]
```

### ints

`ints (--> series)`

Pushes the infinite list 0, 1, -1, 2, -2...

An infinite list; take what you need with `take`, or let a pattern read it. Use it to search outwards from a centre.

```stac
ints 5 take         ;-> [0 1 -1 2 -2]
```

### primes

`primes (--> series)`

Pushes the primes up to 1000039. The list is finite, but stac cannot tell, so `size` is `inf`.

Use it for sequences that do not repeat in short cycles.

```stac
primes 5 take       ;-> [2 3 5 7 11]
```

### fib

`fib (a b --> series)`

Pushes the infinite Fibonacci series starting with the two numbers.

An infinite list; take what you need with `take`, or let a pattern read it. Use it for proportions near the golden ratio.

```stac
1 1 fib 7 take      ;-> [1 1 2 3 5 8 13]
```

### ordz

`ordz (--> signal)`

Pushes the infinite signal 1, 2, 3...

The signal-list form of `ord`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
ordz 3 take         ;-> #[1 2 3]
```

### natz

`natz (--> signal)`

Pushes the infinite signal 0, 1, 2...

The signal-list form of `nat`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
natz 3 take         ;-> #[0 1 2]
```

### invz

`invz (--> signal)`

Pushes the infinite signal 1, 1/2, 1/3...

The signal-list form of `invs`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
invz 3 take         ;-> #[1 0.5 0.333333]
```

### negz

`negz (--> signal)`

Pushes the infinite signal -1, -2, -3...

The signal-list form of `negs`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
negz 3 take         ;-> #[-1 -2 -3]
```

### evenz

`evenz (--> signal)`

Pushes the infinite signal 0, 2, 4...

The signal-list form of `evens`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
evenz 3 take        ;-> #[0 2 4]
```

### oddz

`oddz (--> signal)`

Pushes the infinite signal 1, 3, 5...

The signal-list form of `odds`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
oddz 3 take         ;-> #[1 3 5]
```

### intz

`intz (--> signal)`

Pushes the infinite signal 0, 1, -1, 2, -2...

The signal-list form of `ints`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
intz 5 take         ;-> #[0 1 -1 2 -2]
```

### primez

`primez (--> signal)`

Pushes the primes up to 1000039 as a signal.

The signal-list form of `primes`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
primez 3 take       ;-> #[2 3 5]
```

### fibz

`fibz (a b --> signal)`

Pushes the infinite Fibonacci signal starting with the two numbers.

The signal-list form of `fib`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
1 1 fibz 5 take     ;-> #[1 1 2 3 5]
```

### ninvs

`ninvs (n --> stream)`

Pushes the list 1, 1/2 ... 1/n.

Use it for a fixed number of harmonic amplitudes.

```stac
3 ninvs             ;-> [1 0.5 0.333333]
```

### ninvz

`ninvz (n --> signal)`

Pushes the signal 1, 1/2 ... 1/n.

The signal-list form of `ninvs`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
3 ninvz             ;-> #[1 0.5 0.333333]
```

### ever

`ever (value --> series)`

Pushes an infinite list repeating the value.

Use it to hold a pattern field constant where a list is required.

```stac
7 ever 3 take       ;-> [7 7 7]
```

### by

`by (start step --> series)`

Pushes the infinite arithmetic series start, start + step...

An infinite list; take what you need with `take`, or let a pattern read it. SC calls it `Pseries`.

```stac
5 2 by 4 take       ;-> [5 7 9 11]
```

### nby

`nby (n start step --> series)`

Pushes n terms of the arithmetic series start, start + step...

Use it for a fixed run of evenly spaced values.

```stac
4 5 2 nby           ;-> [5 7 9 11]
```

### grow

`grow (start step --> series)`

Pushes the infinite geometric series start, start * step...

An infinite list; take what you need with `take`, or let a pattern read it. SC calls it `Pgeom`.

```stac
1 2 grow 4 take     ;-> [1 2 4 8]
```

### ngrow

`ngrow (start step --> series)`

Pushes n terms of the geometric series start, start * step...

Use it for a fixed run of exponentially spaced values, e.g. octaves.

```stac
4 1 3 ngrow         ;-> [1 3 9 27]
```

### to

`to (a b --> series)`

Pushes the integers from a to b, both included, counting down when b is less than a.

Use it for a range of indices or notes.

```stac
1 4 to              ;-> [1 2 3 4]
4 1 to              ;-> [4 3 2 1]
```

### everz

`everz (value --> signal)`

Pushes an infinite signal repeating the value.

The signal-list form of `ever`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
7 everz 3 take      ;-> #[7 7 7]
```

### byz

`byz (start step --> series)`

Pushes the infinite arithmetic series as a signal.

The signal-list form of `by`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
5 2 byz 3 take      ;-> #[5 7 9]
```

### nbyz

`nbyz (n start step --> series)`

Pushes n terms of the arithmetic series as a signal.

The signal-list form of `nby`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
3 5 2 nbyz          ;-> #[5 7 9]
```

### growz

`growz (start step --> series)`

Pushes the infinite geometric series as a signal.

The signal-list form of `grow`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
1 2 growz 3 take    ;-> #[1 2 4]
```

### ngrowz

`ngrowz (n start step --> series)`

Pushes n terms of the geometric series as a signal.

The signal-list form of `ngrow`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
3 1 3 ngrowz        ;-> #[1 3 9]
```

### toz

`toz (a b --> series)`

Pushes the integers from a to b as a signal.

The signal-list form of `to`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
1 3 toz             ;-> #[1 2 3]
```

### lindiv

`lindiv (n start end --> series)`

Pushes n values evenly spaced from start to end, both included.

Use it for pan positions or levels across voices.

```stac
5 0 1 lindiv        ;-> [0 0.25 0.5 0.75 1]
```

### expdiv

`expdiv (n start end --> series)`

Pushes n values exponentially spaced from start to end, both included.

Use it for frequencies spread evenly in pitch.

```stac
3 100 1000 expdiv   ;-> [100 316.228 1000]
```

### lindivz

`lindivz (n start end --> series)`

Pushes n values evenly spaced from start to end as a signal.

The signal-list form of `lindiv`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
3 0 1 lindivz       ;-> #[0 0.5 1]
```

### expdivz

`expdivz (n start end --> series)`

Pushes n values exponentially spaced from start to end as a signal.

The signal-list form of `expdiv`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
3 100 1000 expdivz  ;-> #[100 316.228 1000]
```

### lindiv1

`lindiv1 (n start end --> series)`

Pushes n values evenly spaced from start up to, but not including, end.

Use it to divide a cycle, such as a circle of pan positions, without repeating the start.

```stac
4 0 1 lindiv1       ;-> [0 0.25 0.5 0.75]
```

### expdiv1

`expdiv1 (n start end --> series)`

Pushes n values exponentially spaced from start up to, but not including, end.

Use it to divide an octave into equal steps: `12 1 2 expdiv1`.

```stac
3 100 1000 expdiv1  ;-> [100 215.443 464.159]
```

### lindiv1z

`lindiv1z (n start end --> series)`

Pushes n values evenly spaced from start up to end, excluded, as a signal.

The signal-list form of `lindiv1`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
4 0 1 lindiv1z      ;-> #[0 0.25 0.5 0.75]
```

### expdiv1z

`expdiv1z (n start end --> series)`

Pushes n values exponentially spaced from start up to end, excluded, as a signal.

The signal-list form of `expdiv1`: its items are floats in a `#[...]`, for buffers and wave tables.

```stac
3 1 8 expdiv1z      ;-> #[1 2 4]
```

## list reduction operations

### reduce

`reduce (list value fun --> value)`

Folds a list into one value: starting from the value, it applies the function to each item and the running value, item first. It pushes the final value.

Use it to accumulate over a list with any function; for an operator, `+/` and the other `op/` forms are shorter.

```stac
[1 2 3] 0 \item acc [item acc +] reduce   ;-> 6
[1 2 3] 10 \a b [a b -] reduce          ;-> -8
```

### reduce1

`reduce1 (list fun --> value)`

As `reduce`, starting from the list's first item.

Use it where the list has no natural starting value, such as a maximum.

```stac
[3 9 4] \a b [a b |] reduce1   ;-> 9
```

### scan

`scan (list value fun --> list)`

As `reduce`, but pushes the list of running values.

Use it for running totals or a state that evolves through a list.

```stac
[1 2 3] 0 \item acc [item acc +] scan   ;-> [1 3 6]
```

### scan1

`scan1 (list fun --> list)`

As `scan`, starting from the list's first item, which the result begins with.

Use it for a running maximum or minimum.

```stac
[3 1 4 1 5] \a b [a b |] scan1   ;-> [3 3 4 4 5]
```

### iter

`iter (value fun --> list)`

Pushes the infinite list value, f(value), f(f(value))...

An infinite list; take what you need with `take`, or let a pattern read it. Use it for a recurrence, such as a chaotic map.

```stac
1 \x [x 2 *] iter 5 take   ;-> [1 2 4 8 16]
```

### itern

`itern (value fun n --> list)`

Pushes the first n items of `iter`: the value and n - 1 applications.

Use it for a fixed number of steps of a recurrence.

```stac
1 \x [x 2 *] 4 itern       ;-> [1 2 4 8]
```

### chain

`chain (value fun n --> list)`

Applies the function n times to the value and pushes the result.

Use it to repeat a transformation, e.g. a ratio applied n times.

```stac
1 \x [x 2 *] 4 chain       ;-> 16
```

## list ordering operations

### cyc

`cyc (list --> list)`

Makes a finite list repeat forever, as SC's `Pseq(list, inf)`.

Use it for a repeating pattern field.

```stac
[1 2] cyc 5 take    ;-> [1 2 1 2 1]
```

### ncyc

`ncyc (list n --> list)`

Repeats a finite list n times, as SC's `Pseq(list, n)`.

Use it for a pattern that plays a figure a fixed number of times.

```stac
[1 2] 2 ncyc        ;-> [1 2 1 2]
```

### rcyc

`rcyc (ref --> list)`

Repeats the list a ref holds, reading the ref again each time the list ends.

Use it to change a looping pattern while it plays: set the ref to a new list.

```stac
[1 2] R = r
r rcyc 5 take       ;-> [1 2 1 2 1]
```

### X

`X (value n --> stream)`

Pushes a list of n copies of a value. A function is applied to each count from 0 instead.

Use it to make n voices or n copies of a figure.

```stac
7 3 X               ;-> [7 7 7]
\i [i 10 *] 3 X     ;-> [0 10 20]
```

### XZ

`XZ (value n --> signal)`

Pushes a signal of n copies of a value.

Use it as `X` for signal lists.

```stac
1 3 XZ              ;-> #[1 1 1]
```

### mum

`mum (t --> signal)`

Pushes a signal of t seconds of silence at the interpreter's sample rate.

Use it to insert a pause into a signal list with `$z`.

```stac
.0001 mum size      ;-> 10
```

### $

`$ (listA listB --> out)`

Concatenates two lists.

Use it to join phrases into one sequence.

```stac
[1 2] [3] $         ;-> [1 2 3]
```

### $z

`$z (signalA signalB --> signal)`

Concatenates two signals.

Use it to join signal lists.

```stac
#[1] #[2] $z        ;-> #[1 2]
```

### $$

`$$ (listA listB --> out)`

Concatenates the sublists of two lists of lists, pair by pair: `a @ b @ $`.

Use it to extend each chord of a progression with notes from another.

```stac
[[1 2] [3]] [[4] [5 6]] $$   ;-> [[1 2 4] [3 5 6]]
```

### $/

`$/ (list --> out)`

Concatenates the sublists of a list into one list.

Use it to join a list of phrases.

```stac
[[1 2] [3 4]] $/    ;-> [1 2 3 4]
```

### flat

`flat (list --> list)`

Removes every level of nesting.

Use it to turn a nested structure into one sequence.

```stac
[[[1 [2]] 3]] flat  ;-> [1 2 3]
```

### flatten

`flatten (list n --> list)`

Removes n levels of nesting.

Use it to flatten only the outer levels.

```stac
[[[1 [2]] 3]] 1 flatten   ;-> [[1 [2]] 3]
```

### keep

`keep (list n --> list)`

Pushes the first n items of a list. On a UGen it is an error; `sapf.keep` ends one.

Use it to cut a list to length.

```stac
[1 2 3 4] 2 keep    ;-> [1 2]
```

### T

`T (signal t --> signal)`

Pushes the first t seconds of a signal list, at the interpreter's sample rate. On a UGen it is an error: end a synth with an envelope's `.done`, or use `sapf.T`.

Use it to cut a signal list to a duration.

```stac
ordz .0001 T size   ;-> 10
```

### T>

`T> (signal t --> signal)`

Skips the first t seconds of a signal list.

Use it to drop the start of a signal list.

```stac
ordz .0001 T> 2 take   ;-> #[11 12]
```

### N>

`N> (list n --> list)`

Skips the first n items of a list.

Use it to drop a list's start, as `skip` does.

```stac
[1 2 3 4] 2 N>      ;-> [3 4]
```

### N>>

`N>> (list hops --> listOfLists)`

Pushes a list of the list's tails, skipping the hop sizes in turn: 0, then each running total.

Use it to make overlapping windows over a list.

```stac
[1 2 3 4] [1 2] N>> ;-> [[1 2 3 4] [2 3 4] [4]]
```

### T>>

`T>> (signal hops --> listOfSignals)`

Pushes a list of the signal's tails, skipping the hop durations in turn, in seconds.

Use it to make overlapping windows over a signal list.

```stac
ordz .0001 T>> 2 take size   ;-> 2
```

### N

`N (list n --> list)`

Pushes the first n items of a list. On a UGen it is an error; `sapf.N` ends one.

Use it to cut a list to length.

```stac
[1 2 3 4] 2 N       ;-> [1 2]
```

### NZ

`NZ (signal n --> signal)`

Pushes the first n items of a signal list. On a UGen it is an error; `sapf.NZ` ends one.

Use it to cut a signal list to a number of samples.

```stac
[1 2 3 4] Z 2 NZ    ;-> #[1 2]
```

### skip

`skip (list n --> list)`

Skips the first n items of a list.

Use it to start a pattern partway through.

```stac
[1 2 3 4] 2 skip    ;-> [3 4]
```

### take

`take (list n --> list)`

Pushes the first n items of a list; a negative n takes the last items of a finite list.

Use it to read part of an infinite list, or the end of a finite one.

```stac
ord 3 take          ;-> [1 2 3]
[1 2 3 4] -2 take   ;-> [3 4]
```

### drop

`drop (list n --> list)`

Skips the first n items of a list; a negative n drops the last items of a finite list.

Use it to trim either end of a list.

```stac
[1 2 3 4] 1 drop    ;-> [2 3 4]
[1 2 3 4] -1 drop   ;-> [1 2 3]
```

### choff

`choff (channel(s) c n --> out)`

Places channels into a list of n channels starting at offset c; the other channels are 0.

Use it to route a mono or stereo signal to chosen outputs of a multichannel layout.

```stac
[1 2] 1 4 choff     ;-> [0 1 2 0]
```

### tog

`tog (a b --> series)`

Pushes the infinite list a, b, a, b...

Use it for an alternating pattern field, such as two accents.

```stac
1 2 tog 5 take      ;-> [1 2 1 2 1]
```

### togz

`togz (a b --> signal)`

Pushes the infinite signal a, b, a, b...

Use it as `tog` for signal lists.

```stac
1 2 togz 4 take     ;-> #[1 2 1 2]
```

### sel

`sel (a j --> out)`

Selects from a list of lists: item i of the result is item i of list j[i].

Use it to switch between parallel sequences step by step.

```stac
[[1 2 3] [10 20 30]] [0 1 0] sel   ;-> [1 20 3]
```

### sell

`sell (a j --> out)`

Selects lazily from a list of lists: item i is the next unread item of list j[i].

Use it to interleave independent sequences under the control of a selector.

```stac
[[1 2 3] [10 20 30]] [0 1 1 0] sell   ;-> [1 10 20 2]
```

### ?

`? (a b --> out)`

Repeats each item of a by the count in b; with counts of 0 and 1 it filters.

Use it to filter a list with a test, or to repeat notes by given counts.

```stac
[1 2 3] [1 0 1] ?   ;-> [1 3]
[1 2 3] [2 0 1] ?   ;-> [1 1 3]
```

### spread

`spread (a n --> out)`

Inserts n[i] zeros after each item a[i].

Use it to space a sequence of values out in time.

```stac
[1 2] [1 2] spread  ;-> [1 0 2 0 0]
```

### spreadz

`spreadz (a n --> signal)`

Inserts n[i] zeros after each item a[i] of a signal. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `spread` for signal lists.

```stac
#[1 2] #[1 2] spreadz   ;-> #[1 0 2 0 0]
```

### change

`change (a --> b)`

Removes items equal to the one before.

Use it to keep only the changes in a sequence.

```stac
[1 2 2 2 3 3 1] change   ;-> [1 2 3 1]
```

### changez

`changez (a --> b)`

Removes items equal to the one before, in a signal. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `change` for signal lists.

```stac
#[1 1 2] changez    ;-> #[1 2]
```

### expand

`expand (a b --> out)`

Where b is true, takes the next item of a; where b is false, writes 0.

Use it to place a sequence of values on the true steps of a rhythm.

```stac
[1 2 3] [1 0 1 1] expand   ;-> [1 0 2 3]
```

### expandz

`expandz (a b --> out)`

As `expand`, for signals. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `expand` for signal lists.

```stac
#[1 2] #[1 0 1] expandz   ;-> #[1 0 2]
```

### clump

`clump (a n --> out)`

Groups a list's items into sublists of n; a last group shorter than n is dropped.

Use it to turn a flat list into chords or rows.

```stac
[1 2 3 4 5] 2 clump ;-> [[1 2] [3 4]]
```

### hang

`hang (a --> out)`

Extends a finite list forever by repeating its last item.

Use it so a pattern field holds its last value after its sequence ends.

```stac
[1 2] hang 4 take   ;-> [1 2 2 2]
```

### hangz

`hangz (a --> out)`

Extends a finite signal forever by repeating its last item. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `hang` for signal lists.

```stac
#[1 2] hangz 3 take ;-> #[1 2 2]
```

### histo

`histo (a n --> out)`

Counts the items of a finite list in n bins: bin k counts the items equal to k.

Use it to see how often each value or index occurs.

```stac
[0 0 1 2 2 2] 3 histo   ;-> #[2 1 3]
```

### histoz

`histoz (a n --> out)`

Counts the items of a finite signal in n bins. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `histo` for signal lists.

```stac
#[0 1 1] 2 histoz   ;-> #[1 2]
```

### keepWhile

`keepWhile (a b --> out)`

Pushes items of a while the matching items of b are true; it stops at the first false.

Use it to take a list's leading run that passes a test.

```stac
[1 2 3 4] [1 1 0 1] keepWhile   ;-> [1 2]
```

### skipWhile

`skipWhile (a b --> out)`

Skips items of a while the matching items of b are true, then pushes the rest.

Use it to drop a list's leading run that passes a test.

```stac
[1 2 3 4] [1 1 0 1] skipWhile   ;-> [3 4]
```

### flop

`flop (a --> b)`

Transposes a list of lists: rows become columns. At least one dimension must be finite.

Use it to turn a list of chords into a list of voices, or the reverse.

```stac
[[1 2 3] [4 5 6]] flop   ;-> [[1 4] [2 5] [3 6]]
```

### flops

`flops (a --> b)`

As `flop`, treating signal lists as single values.

Use it to transpose a structure that holds signals without transposing the signals.

```stac
[[1 2 3] [4 5 6]] flops  ;-> [[1 4] [2 5] [3 6]]
```

### flop1

`flop1 (a --> b)`

As `flop`, wrapping an item that is not a list in a one-item list first.

Use it to transpose rows of uneven depth.

```stac
[[1 2 3] [[4 5] 6 7]] flop1   ;-> [[1 [4 5]] [2 6] [3 7]]
```

### lace

`lace (a --> b)`

Interleaves a list of lists: `flop` then concatenate.

Use it to merge parallel sequences into one, step by step.

```stac
[[1 2] [3 4]] lace  ;-> [1 3 2 4]
```

### merge

`merge (a b fun --> c)`

Merges two sorted lists into one sorted list, using a function that works like `<`.

Use it to combine two time-ordered event lists.

```stac
[1 3 5] [2 4] \a b [a b <] merge   ;-> [1 2 3 4 5]
```

### mergec

`mergec (a b fun --> c)`

Merges two sorted lists without duplicates, using a function that works like `cmp`.

Use it to combine two sorted sets.

```stac
[1 3] [1 2] \a b [a b cmp] mergec   ;-> [1 2 3]
```

### perms

`perms (a --> b)`

Pushes every permutation of a list.

Use it to enumerate the orderings of a motif.

```stac
[1 2 3] perms size  ;-> 6
```

### permz

`permz (a --> b)`

Pushes every permutation of a signal, as a list of signals. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `perms` for signal lists.

```stac
#[1 2] permz        ;-> [#[1 2] #[2 1]]
```

### permswr

`permswr (a --> b)`

Pushes every distinct permutation of a list with repeated items.

Use it to enumerate the orderings of a rhythm without repeats.

```stac
[1 1 2] permswr     ;-> [[1 1 2] [1 2 1] [2 1 1]]
```

### permzwr

`permzwr (a --> b)`

Pushes every distinct permutation of a signal with repeated items, as a list of signals. The `z` form works on signal lists, `#[...]`, and maps over a list of them.

Use it as `permswr` for signal lists.

```stac
#[1 1 2] permzwr size   ;-> 3
```

### shortas

`shortas (a b --> a')`

Cuts list a to the length of list b.

Use it to match one field's length to another's.

```stac
[1 2 3] [0 0] shortas   ;-> [1 2]
```

### longas

`longas (a b --> a')`

Extends list a to the length of list b by repeating its last item.

Use it to pad a shorter field to a longer one's length.

```stac
[1 2] [0 0 0 0] longas  ;-> [1 2 2 2]
```

### longas0

`longas0 (a b --> a')`

Extends list a to the length of list b with zeros.

Use it to pad a list of amplitudes with silence.

```stac
[1 2] [0 0 0] longas0   ;-> [1 2 0]
```

## list ops

### bub

`bub (a --> [a])`

Wraps the top value in a one-item list.

Use it where a word expects a list, or to keep a list from being mapped over: one level of nesting stops automapping.

```stac
5 bub               ;-> [5]
```

### nbub

`nbub (a n --> [[..[a]..]])`

Wraps the value in n levels of one-item lists.

Use it to protect a value from n levels of automapping.

```stac
5 2 nbub            ;-> [[5]]
```

### 2ple

`2ple (a b --> [a b])`

Collects the top 2 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 2ple          ;-> [1 2]
```

### 3ple

`3ple (a b c --> [a b c])`

Collects the top 3 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 3 3ple          ;-> [1 2 3]
```

### 4ple

`4ple (a b c d --> [a b c d])`

Collects the top 4 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 3 4 4ple          ;-> [1 2 3 4]
```

### 5ple

`5ple (a b c d e --> [a b c d e])`

Collects the top 5 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 3 4 5 5ple          ;-> [1 2 3 4 5]
```

### 6ple

`6ple (a b c d e f --> [a b c d e f])`

Collects the top 6 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 3 4 5 6 6ple          ;-> [1 2 3 4 5 6]
```

### 7ple

`7ple (a b c d e f g --> [a b c d e f g])`

Collects the top 7 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 3 4 5 6 7 7ple          ;-> [1 2 3 4 5 6 7]
```

### 8ple

`8ple (a b c d e f g h --> [a b c d e f g h])`

Collects the top 8 values into a list.

Use it to group values left on the stack, e.g. a chord's notes.

```stac
1 2 3 4 5 6 7 8 8ple          ;-> [1 2 3 4 5 6 7 8]
```

### 2ples

`2ples (a b --> [[a0 b0][a1 b1]..[aN bN]])`

Combines 2 lists into a list of 2-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 2 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] 2ples   ;-> [[1 11] [2 12]]
```

### 3ples

`3ples (a b c --> [[a0 b0 c0][a1 b1 c1]..[aN bN cN]])`

Combines 3 lists into a list of 3-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 3 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] [21 22] 3ples   ;-> [[1 11 21] [2 12 22]]
```

### 4ples

`4ples (a b c d --> seq)`

Combines 4 lists into a list of 4-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 4 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] [21 22] [31 32] 4ples   ;-> [[1 11 21 31] [2 12 22 32]]
```

### 5ples

`5ples (a b c d e --> seq)`

Combines 5 lists into a list of 5-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 5 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] [21 22] [31 32] [41 42] 5ples   ;-> [[1 11 21 31 41] [2 12 22 32 42]]
```

### 6ples

`6ples (a b c d e f--> seq)`

Combines 6 lists into a list of 6-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 6 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] [21 22] [31 32] [41 42] [51 52] 6ples   ;-> [[1 11 21 31 41 51] [2 12 22 32 42 52]]
```

### 7ples

`7ples (a b c d e f g--> seq)`

Combines 7 lists into a list of 7-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 7 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] [21 22] [31 32] [41 42] [51 52] [61 62] 7ples   ;-> [[1 11 21 31 41 51 61] [2 12 22 32 42 52 62]]
```

### 8ples

`8ples (a b c d e f g h --> seq)`

Combines 8 lists into a list of 8-item lists, item by item, stopping at the shortest.

Use it to pair up fields of 8 parallel lists, e.g. notes with durations.

```stac
[1 2] [11 12] [21 22] [31 32] [41 42] [51 52] [61 62] [71 72] 8ples   ;-> [[1 11 21 31 41 51 61 71] [2 12 22 32 42 52 62 72]]
```

### un2

`un2 ([a0 a1 .. aN-1] --> a0 a1)`

Pushes the first 2 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3] un2       ;-> 1 2
```

### un3

`un3 ([a0 a1 .. aN-1] --> a0 a1 a2)`

Pushes the first 3 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3 4] un3       ;-> 1 2 3
```

### un4

`un4 ([a0 a1 .. aN-1] --> a0 a1 a2 a3)`

Pushes the first 4 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3 4 5] un4       ;-> 1 2 3 4
```

### un5

`un5 ([a0 a1 .. aN-1] --> a0 a1 a2 a3 a4)`

Pushes the first 5 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3 4 5 6] un5       ;-> 1 2 3 4 5
```

### un6

`un6 ([a0 a1 .. aN-1] --> a0 a1 a2 .. a5)`

Pushes the first 6 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3 4 5 6 7] un6       ;-> 1 2 3 4 5 6
```

### un7

`un7 ([a0 a1 .. aN-1] --> a0 a1 a2 .. a6)`

Pushes the first 7 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3 4 5 6 7 8] un7       ;-> 1 2 3 4 5 6 7
```

### un8

`un8 ([a0 a1 .. aN-1] --> a0 a1 a2 .. a7)`

Pushes the first 8 items of a list onto the stack.

Use it to take a list apart into named arguments, as `= [a b]` does for binding.

```stac
[1 2 3 4 5 6 7 8 9] un8       ;-> 1 2 3 4 5 6 7 8
```

### reverse

`reverse (a --> b)`

Reverses a finite list.

Use it to play a sequence backwards.

```stac
[1 2 3] reverse     ;-> [3 2 1]
```

### mirror0

`mirror0 (a --> b)`

Appends the list's reverse without its ends, so the result cycles smoothly.

Use it to make a sequence go up and down when it repeats with `cyc`.

```stac
[1 2 3 4] mirror0   ;-> [1 2 3 4 3 2]
```

### mirror1

`mirror1 (a --> b)`

Appends the list's reverse without its last item.

Use it for an up-and-down sequence that ends where it started.

```stac
[1 2 3 4] mirror1   ;-> [1 2 3 4 3 2 1]
```

### mirror2

`mirror2 (a --> b)`

Appends the list's whole reverse, repeating the turning point.

Use it for an up-and-down sequence with the top value held twice.

```stac
[1 2 3 4] mirror2   ;-> [1 2 3 4 4 3 2 1]
```

### rot

`rot (seq M --> seq')`

Rotates a list by m places; a positive m moves items to the right.

Use it to shift a rhythm or a melody's start point.

```stac
[1 2 3 4] 1 rot     ;-> [4 1 2 3]
[1 2 3 4] -1 rot    ;-> [2 3 4 1]
```

### shift

`shift (seq M --> seq')`

Shifts a list by m places, filling the vacated places with 0.

Use it to delay a sequence by m steps.

```stac
[1 2 3 4] 1 shift   ;-> [0 1 2 3]
[1 2 3 4] -2 shift  ;-> [3 4 0 0]
```

### clipShift

`clipShift (seq M --> seq')`

Shifts a list by m places, filling the vacated places with the end item.

Use it to shift without introducing zeros.

```stac
[1 2 3 4] 1 clipShift   ;-> [1 1 2 3]
```

### foldShift

`foldShift (seq M --> seq')`

Shifts a list by m places, filling the vacated places from the list mirrored at its end.

Use it to shift a contour without a jump at the edge.

```stac
[1 2 3 4] 2 foldShift   ;-> [3 2 1 2]
```

### muss

`muss (a --> b)`

Puts a finite list in a random order.

Use it to shuffle a set of notes or events.

```stac
1 setseed
[1 2 3 4] muss size   ;-> 4
```

### at

`at (seq index(es) --> value(s))`

Pushes the item at an index, counting from 0; a list of indices gives a list. Out-of-range indices give 0.

Use it to look up values by position; `wrapAt` cycles instead.

```stac
[10 20 30] 1 at     ;-> 20
[10 20 30] [0 2 3] at   ;-> [10 30 0]
```

### wrapAt

`wrapAt (seq index(es) --> value(s))`

As `at`, with out-of-range indices wrapping around the list.

Use it to read a list cyclically with a counter.

```stac
[10 20 30] [3 -1 4] wrapAt   ;-> [10 30 20]
```

### foldAt

`foldAt (seq index(es) --> value(s))`

As `at`, with out-of-range indices folding back and forth over the list.

Use it to read a list up and down with a counter.

```stac
[10 20 30] [3 4] foldAt      ;-> [20 10]
```

### clipAt

`clipAt (seq index(es) --> value(s))`

As `at`, with out-of-range indices giving the item at the nearer end.

Use it where an index may overshoot and should hold the last value.

```stac
[10 20 30] [3 -1] clipAt     ;-> [30 10]
```

### degkey

`degkey (degree scale --> key)`

Converts scale degrees to keys. The scale's last item is the cycle width, so a major scale is `[0 2 4 5 7 9 11 12]`; degrees beyond it wrap into the next cycle.

Use it to play scale degrees with list words; `pbind`'s `:degree` and `:scale` do the same for events.

```stac
[0 1 2 7] [0 2 4 5 7 9 11 12] degkey   ;-> [0 2 4 12]
```

### keydeg

`keydeg (key scale --> degree)`

Converts keys to the nearest scale degrees. The scale's last item is the cycle width.

Use it to snap pitches to a scale.

```stac
[0 4 7 12] [0 2 4 5 7 9 11 12] keydeg   ;-> [0 2 4 7]
```

### sort

`sort (in --> out)`

Sorts a finite list in ascending order.

Use it to order a chord's notes or a set of values.

```stac
[3 1 2] sort        ;-> [1 2 3]
```

### sortf

`sortf (in fun --> out)`

Sorts a finite list with a function that takes two items and pushes 1 when the first comes before the second.

Use it to sort by a key of your own, e.g. events by time.

```stac
[3 1 2] \a b [a b >] sortf   ;-> [3 2 1]
```

### sort>

`sort> (in --> out)`

Sorts a finite list in descending order.

Use it to put the loudest or highest value first.

```stac
[3 1 2] sort>       ;-> [3 2 1]
```

### grade

`grade (in --> out)`

Pushes the indices that would sort a list in ascending order.

Use it to sort one list by another: `b a grade at`.

```stac
[3 1 2] grade       ;-> #[1 2 0]
```

### gradef

`gradef (in fun --> out)`

Pushes the indices that would sort a list with a compare function, as `sortf` takes.

Use it to sort parallel lists by one of them with your own order.

```stac
[3 1 2] \a b [a b >] gradef  ;-> #[0 2 1]
```

### grade>

`grade> (in --> out)`

Pushes the indices that would sort a list in descending order.

Use it to find the positions of the largest values.

```stac
[3 1 2] grade>      ;-> #[0 2 1]
```

## random number generation

### newseed

`newseed (--> seed)`

Pushes a new random seed from the clock.

Use it to start a fresh random sequence after a fixed one.

```stac
newseed type        ;-> Real
```

### setseed

`setseed (seed -->)`

Seeds the interpreter's random generator.

Use it to make random choices repeat, e.g. to reproduce a pattern exactly.

```stac
1 setseed
0 1 rand            ;-> 0.762994
1 setseed
0 1 rand            ;-> 0.762994
```

## single random numbers

### rand

`rand (a b --> r)`

Pushes a uniformly distributed random real from a to b. `sc.rand` is the UGen, drawn per synth.

Use it to vary a value when code runs; for a value that differs per note inside a SynthDef, use `sc.rand`.

```stac
1 setseed
0 1 rand            ;-> 0.762994
```

### coin

`coin (p --> r)`

Pushes 1 with probability p, else 0.

Use it to decide whether something happens.

```stac
1 setseed
.5 coin             ;-> 0
```

### rand2

`rand2 (a --> r)`

Pushes a uniformly distributed random real from -a to a.

Use it for a random offset around zero, such as detuning.

```stac
1 setseed
1 rand2             ;-> 0.525989
```

### irand

`irand (a b --> r)`

Pushes a uniformly distributed random integer from a to b, both included.

Use it to choose an index or a count.

```stac
1 setseed
1 10 irand          ;-> 8
```

### irand2

`irand2 (a --> r)`

Pushes a uniformly distributed random integer from -a to a, both included.

Use it for a random step up or down.

```stac
1 setseed
5 irand2            ;-> 3
```

### xrand

`xrand (a b --> r)`

Pushes an exponentially distributed random real from a to b; both must have the same sign.

Use it for frequencies and durations, which we hear on a ratio scale.

```stac
1 setseed
100 1000 xrand      ;-> 579.421
```

### linrand

`linrand (a b --> r)`

Pushes a random real from a to b, more likely near a: the density falls linearly to b.

Use it to favour small values, such as short gaps.

```stac
1 setseed
0 1 linrand         ;-> 0.762994
```

### ilinrand

`ilinrand (a b --> r)`

Pushes a random integer from a to b, more likely near a.

Use it to favour low indices.

```stac
1 setseed
1 10 ilinrand       ;-> 7
```

### wrand

`wrand (w --> r)`

Pushes a random index into a list of weights; the weights should sum to 1.

Use it to choose among options with given probabilities.

```stac
1 setseed
[.1 .9] wrand       ;-> 1
```

### pick

`pick (a --> r)`

Pushes a random item of a finite list.

Use it to choose a note from a scale or chord.

```stac
1 setseed
[7 8 9] pick        ;-> 9
```

### wpick

`wpick (a w --> r)`

Pushes a random item of a finite list, chosen with the weights in a second list of the same length.

Use it to favour some notes of a scale.

```stac
1 setseed
[7 8 9] [0 0 1] wpick   ;-> 9
```

## random streams

### rands

`rands (a b --> r)`

Pushes an infinite list of uniformly distributed random reals from a to b.

Use it for a random value per event: `{:amp .05 .2 rands ...} pbind`, as SC's `Pwhite`.

```stac
1 setseed
0 1 rands 3 take    ;-> [0.450725 0.182634 0.837994]
```

### coins

`coins (p --> r)`

Pushes an infinite list of 1s with probability p and 0s.

Use it for random gates, e.g. a pattern's rests.

```stac
1 setseed
.5 coins 4 take     ;-> [1 1 0 0]
```

### eprands

`eprands (a b --> r)`

Pushes an infinite list of random integers from a to b that never repeats the previous value.

Use it for melodies that should not repeat a note immediately.

```stac
1 4 eprands 6 take size   ;-> 6
```

### rand2s

`rand2s (a --> r)`

Pushes an infinite list of uniformly distributed random reals from -a to a.

Use it for per-event random offsets around zero.

```stac
1 rand2s 3 take size   ;-> 3
```

### irands

`irands (a b --> r)`

Pushes an infinite list of random integers from a to b, both included.

Use it for random indices per event, e.g. scale degrees.

```stac
1 3 irands 4 take size   ;-> 4
```

### irand2s

`irand2s (a --> r)`

Pushes an infinite list of random integers from -a to a, both included.

Use it for random steps per event, e.g. a random walk's increments.

```stac
3 irand2s 4 take size   ;-> 4
```

### xrands

`xrands (a b --> r)`

Pushes an infinite list of exponentially distributed random reals from a to b, as SC's `Pexprand`.

Use it for random frequencies or durations per event.

```stac
100 1000 xrands 3 take size   ;-> 3
```

### linrands

`linrands (a b --> r)`

Pushes an infinite list of random reals from a to b, more likely near a.

Use it to favour short values per event.

```stac
0 1 linrands 3 take size   ;-> 3
```

### ilinrands

`ilinrands (a b --> r)`

Pushes an infinite list of random integers from a to b, more likely near a.

Use it to favour low degrees per event.

```stac
1 10 ilinrands 3 take size   ;-> 3
```

### wrands

`wrands (w --> r)`

Pushes an infinite list of random indices chosen with the weights.

Use it to choose among options per event with given probabilities.

```stac
[.5 .5] wrands 4 take size   ;-> 4
```

### picks

`picks (a --> r)`

Pushes an infinite list of random items of a finite list, as SC's `Prand`.

Use it for a random note per event from a scale.

```stac
{:instrument 'ping :midinote [60 62 64 67] picks :dur .25} pbind 8 take size   ;-> 8
```

### wpicks

`wpicks (a w --> r)`

Pushes an infinite list of random items chosen with the weights, as SC's `Pwrand`.

Use it to favour some notes per event.

```stac
[7 8] [0 1] wpicks 3 take   ;-> [8 8 8]
```

## finite random streams

### nrands

`nrands (n a b --> r)`

Pushes a list of n uniformly distributed random reals from a to b.

Use it for a fixed set of random values, such as partial detunings.

```stac
3 0 1 nrands size   ;-> 3
```

### ncoins

`ncoins (n p --> r)`

Pushes a list of n 1s and 0s, each 1 with probability p.

Use it for a fixed random rhythm.

```stac
4 .5 ncoins size    ;-> 4
```

### neprands

`neprands (n a b --> r)`

Pushes a list of n random integers from a to b, none equal to the one before.

Use it for a fixed melody without repeated notes.

```stac
4 1 4 neprands size   ;-> 4
```

### nrand2s

`nrand2s (n a --> r)`

Pushes a list of n random reals from -a to a.

Use it for a fixed set of offsets, such as a chord's detunings.

```stac
3 1 nrand2s size    ;-> 3
```

### nirands

`nirands (n a b --> r)`

Pushes a list of n random integers from a to b, both included.

Use it for a fixed random sequence of degrees.

```stac
4 1 6 nirands size  ;-> 4
```

### nirand2s

`nirand2s (n a --> r)`

Pushes a list of n random integers from -a to a, both included.

Use it for a fixed sequence of random steps.

```stac
4 2 nirand2s size   ;-> 4
```

### nxrands

`nxrands (n a b --> r)`

Pushes a list of n exponentially distributed random reals from a to b.

Use it for a fixed set of random frequencies, e.g. a cluster.

```stac
5 200 2000 nxrands size   ;-> 5
```

### nlinrands

`nlinrands (n a b --> r)`

Pushes a list of n random reals from a to b, more likely near a.

Use it for a fixed set of mostly short values.

```stac
3 0 1 nlinrands size   ;-> 3
```

### nilinrands

`nilinrands (n a b --> r)`

Pushes a list of n random integers from a to b, more likely near a.

Use it for a fixed set of mostly low indices.

```stac
3 1 10 nilinrands size   ;-> 3
```

### nwrands

`nwrands (n w --> r)`

Pushes a list of n random indices chosen with the weights.

Use it for a fixed weighted choice of options.

```stac
4 [.5 .5] nwrands size   ;-> 4
```

### npicks

`npicks (n a --> r)`

Pushes a list of n random items of a finite list.

Use it for a fixed random melody from a scale.

```stac
3 [7 8 9] npicks size   ;-> 3
```

### nwpicks

`nwpicks (n a w --> r)`

Pushes a list of n random items chosen with the weights.

Use it for a fixed melody that favours some notes.

```stac
3 [7 8 9] [1 0 0] nwpicks   ;-> [7 7 7]
```

## SuperCollider UGens

### sapf

`sapf (--> namespace)`

The namespace of sapf's 126 words, in sapf's argument order and units: `440 0 sapf.sinosc`. `sapf.stac` imports them over the SC UGens.

Use it to run sapf programs, or to reach one sapf word from stac code. The dictionary's sapf chapter lists them.

```stac
440 0 sapf.sinosc .1 * play
```

## range ops

### clip

`clip (in lo hi --> out)`

Limits a value to the range lo to hi. On a UGen it builds SC's `Clip`.

Use it to keep a parameter in a safe range. `sc.clip` builds a `Clip` UGen even from numbers.

```stac
15 0 10 clip        ;-> 10
[-1 5 11] 0 10 clip ;-> [0 5 10]
```

### wrap

`wrap (in lo hi --> out)`

Wraps a value into the range lo up to hi, as a modulo. On a UGen it builds SC's `Wrap`.

Use it for values that cycle, such as a phase or an index.

```stac
12 0 10 wrap        ;-> 2
-1 0 10 wrap        ;-> 9
```

### *+

`*+ (a b c --> out)`

Multiplies a by b and adds c: `a b * c +` in one word. It maps over lists and UGens.

Use it to scale and offset a modulator, as SC's `mul` and `add` do.

```stac
2 3 4 *+            ;-> 10
```

### iwrap

`iwrap (in lo hi --> out)`

Wraps an integer into the range lo to hi, both included. Reals only.

Use it to wrap an index into a list of hi - lo + 1 items.

```stac
12 0 9 iwrap        ;-> 2
```

### ifold

`ifold (in lo hi --> out)`

Folds an integer back into the range lo to hi, both included. Reals only.

Use it to bounce an index back and forth over a list.

```stac
12 0 9 ifold        ;-> 6
```

### fold

`fold (in lo hi --> out)`

Folds a value back into the range lo to hi, reflecting at each edge. On a UGen it builds SC's `Fold`.

Use it where a value should bounce off its limits, as a wavefolder does.

```stac
12 0 10 fold        ;-> 8
-1 0 10 fold        ;-> 1
```

## play

### play

`play (channels --> node)`

Channels: compiles them into a SynthDef, sends it to scsynth and starts a synth, pushing its node. An event stream: starts one synth per event on the default clock and pushes a player. Without `$STAC_SCSYNTH`, it starts the embedded scsynth.

Use it to hear a sound at once. For a sound you start many times, define a `synthdef`; for one you replace while it plays, use `ndef`.

```stac
[440 sinosc .1 *] play = n
n.free
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
{:instrument 'ping :midinote [60 64 67] cyc :dur .25} pbind play = p
p.stop
```

### stop

`stop (-->)`

Frees every synth on the server and ends every player and `ola` voice: SC's `s.freeAll`, or Cmd-period.

Use it to silence everything at once.

```stac
[440 sinosc .1 *] play
stop
```

## namespaces

### sc

`sc (--> namespace)`

The namespace of SuperCollider 3.14.1's 374 UGens, each named by its class in lower case: `sc.sinosc`, `sc.dseq`, `sc.pv_magfreeze`. `prelude.stac` imports them with `import sc.*`.

Use the qualified name for a UGen a core word hides: `sc.clip`, `sc.rand`. The dictionary's sc chapter lists them.

```stac
440 sc.sinosc .1 * play
```

### stac

`stac (--> namespace)`

The namespace of every core word, so `stac.word` reaches a core word a binding or an import hides.

Use it after `import sc.clip`, for example, to reach the core's `clip`.

```stac
import sc.clip
15 0 10 stac.clip   ;-> 10
```

## env

### env.perc

`env.perc (attackTime releaseTime --> env)`

Makes SC's `Env.perc`: rises to 1 over attackTime and falls to 0 over releaseTime, with curve -4.

Use it for a percussive note; `envgen.kr.done` frees the synth when it ends.

```stac
[440 sinosc .01 .5 env.perc 1 envgen.kr.done * .1 *] play
```

### env.adsr

`env.adsr (attackTime decayTime sustainLevel releaseTime --> env)`

Makes SC's `Env.adsr`: attack to 1, decay to sustainLevel, hold while the gate is open, then release to 0.

Use it for a held note that a `gate` control releases.

```stac
synthdef pad freq:220 gate:1 [freq saw .01 .2 .6 1 env.adsr gate envgen.kr.done * .1 *]
{} pad = n
n.release
```

### env.asr

`env.asr (attackTime releaseTime --> env)`

Makes SC's `Env.asr` with a sustain level of 1: attack, hold while the gate is open, release.

Use it for an organ-like note with no decay.

```stac
synthdef organ freq:220 gate:1 [freq sinosc .01 .3 env.asr gate envgen.kr.done * .1 *]
{} organ = n
n.release
```

### env.linen

`env.linen (attackTime sustainTime releaseTime --> env)`

Makes SC's `Env.linen`: linear attack to 1, hold for sustainTime, linear release.

Use it for a note of fixed length that ends on its own.

```stac
[440 sinosc .01 .2 .3 env.linen 1 envgen.kr.done * .1 *] play
```

### env.new

`env.new (levels times curves --> env)`

Makes SC's `Env.new` from levels, times and curves. A curve is a number, a UGen, a shape symbol (`'lin 'exp 'sin 'wel 'sqr 'cub 'step 'hold`) or a list of them.

Use it for any envelope the other words do not shape.

```stac
[0 1 .3 0] [.01 .1 .5] 'lin env.new = e
[440 sinosc e 1 envgen.kr.done * .1 *] play
```

### env

`env (--> namespace)`

The namespace of SC's envelope shapes: `perc`, `adsr`, `asr`, `linen`, `new`. `envgen` and `ienvgen` read an `Env`.

Use it to make an `Env` for `envgen`.

```stac
env,perc type       ;-> Prim
```

## defs and nodes

### synthdef.new

`synthdef.new (fun defaults name --> synthdef)`

Builds a SynthDef from a function, a Form of defaults and a name, as SC's `SynthDef.new`. The function's arguments become controls; it pushes the SynthDef unbound and unsent.

Use it when code makes the defs, e.g. in a loop. The `synthdef` statement is shorter for one def.

```stac
\freq [freq saw .1 *] {:freq 220} 'buzz synthdef.new = buzz
{} buzz = n
n.free
```

### group.new

`group.new (placement --> group)`

Makes a server group, as SC's `Group.new`. Its Form takes `:target` and `:addAction`; `{}` puts it at the head of the root. A group is a target, and `.set`, `.release` and `.free` act on every synth in it.

Use it to control a set of synths together, or to order effects after their sources.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
{} group.new = g
{:target g} ping
{:target g} ping
{:freq 300} g.set
g.free
```

### group

`group (--> namespace)`

The namespace of `group.new`.

Use `group.new` to make a group.

```stac
group,new type      ;-> Prim
```

## bus

### bus.audio

`bus.audio (channels --> bus)`

Takes n consecutive private audio buses, from 959 down, and pushes the bus. As a UGen input or a control value, a bus is its index.

Use it to route a source into an effect: the source writes `:out bus`, the effect reads `bus n in` at the tail.

```stac
2 bus.audio = fx
synthdef verb bus mix:.4 [bus 2 in mix .8 .5 freeverb]
{:bus fx :addAction 'tail} verb = v
[440 sinosc .1 *] play = n
n.free
v.free
fx.free
```

### bus.control

`bus.control (channels --> bus)`

Takes n consecutive control buses, from 16383 down, and pushes the bus. `v b.set` sets its values.

Use it to share a control value among synths: each reads it with `bus 1 in.kr`.

```stac
1 bus.control = b
.5 b.set
b.free
```

### bus

`bus (--> namespace)`

The namespace of `bus.audio` and `bus.control`.

Use it to take buses.

```stac
bus,audio type      ;-> Prim
```

### send

`send (message -->)`

Sends an OSC message `[address args...]` to scsynth, as SC's `s.sendMsg`. Whole reals go as integers, others as floats; a bus, buffer or node goes as its index.

Use it for a server command stac has no word for.

```stac
["/n_free" 999999] send
```

## buffer

### buffer.read

`buffer.read (path --> buffer)`

Reads a WAV or AIFF file into one of the sound-file buffers, 0 to 63, as SC's `Buffer.read`, and pushes the buffer.

Use it to play a sample with `playbuf` or `bufrd`.

```stac
"tests/fixtures/ref/adsr.wav" buffer.read = b
b.free
```

### buffer.load

`buffer.load (list --> buffer)`

Loads a list of values into a new one-channel buffer, as SC's `Buffer.loadCollection`, and pushes the buffer.

Use it for wave tables, step sequences and other data a UGen reads.

```stac
[0 .5 1 .5] buffer.load = b
b.free
```

### buffer.alloc

`buffer.alloc (frames channels --> buffer)`

Allocates an empty buffer of frames by channels, as SC's `Buffer.alloc`, and pushes it.

Use it for a delay line, a recording or an FFT buffer.

```stac
48000 1 buffer.alloc = b
b.free
```

### buffer

`buffer (--> namespace)`

The namespace of `buffer.read`, `buffer.load` and `buffer.alloc`. The buffer UGens are in `sc`.

Use it to make buffers.

```stac
buffer,load type    ;-> Prim
```

## patterns

### pbind

`pbind (fields --> events)`

Makes an event stream from a Form of fields: a lazy list of events, one Form each. A list field gives its next item per event; any other field repeats; `:scale` holds its list. The stream ends with its shortest finite field. Events follow SC's default event: pitch from `:freq`, `:midinote` or `:degree` and `:scale`; `:amp` .1; `:dur` 1 beat; `:legato` .8; `'rest`; a list value plays a chord.

Use it for any sequence of notes, as SC's `Pbind`. `play` plays it; `pdef` makes it replaceable while it plays.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
{:instrument 'ping :degree [0 2 4 5 7] cyc :scale 'minor :dur .25} pbind play = p
p.stop
```

### score

`score (item seconds name -->)`

Writes `name.osc`, an NRT command file of the first seconds of an event stream or of channels, to `$STAC_SYNTHDEFS`. Events start at 1 beat per second; an event's `:tempo` changes it.

Use it to render offline with scsynth, or to give the SSP's `scsy` a score.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
{:instrument 'ping :midinote [60 64] cyc :dur .5} pbind 2 "melody" score
```

### render

`render (item seconds name -->)`

Renders the first seconds of channels or an event stream to `name.wav` in `$STAC_RECORDINGS` with the embedded scsynth. Events render to 2 channels.

Use it to record a passage to a file faster than real time.

```stac
[440 sinosc .1 *] .1 "tone" render
```

### pdef

`pdef (events name --> player)`

Plays an event stream under a name, as SC's `Pdef`. Run again with the same name, it replaces the stream on the next beat of the default clock and pushes the same player. `{:quant q} p.set` changes the beat multiple; `events name clk.pdef` uses another clock.

Use it to edit a pattern while it plays: change the line and run it again.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
{:instrument 'ping :midinote [60 62 64] cyc :dur .25} pbind 'mel pdef = p
{:instrument 'ping :midinote [67 65] cyc :dur .25} pbind 'mel pdef
p.stop
```

### ndef

`ndef (channels name --> ndef)`

Plays channels under a name, as SC's `Ndef`: on a private bus, with a monitor to bus 0. Run again, it fades the new sound in and the old one out over `fadeTime`, .02 s, and pushes the same ndef. `nd.ar` reads its bus; `.stop` and `.play` fade the monitor; `.end` fades out both; `{:fadeTime t} nd.set` sets the fade.

Use it to edit a sound while it plays without a click.

```stac
[[55 55.2] saw .1 *] 'drone ndef = d
{:fadeTime 2} d.set
[[55 55.2] saw 800 .3 rlpf .1 *] 'drone ndef
d.end
```

## clock

### clock.new

`clock.new (tempo --> clock)`

Makes a clock at tempo beats per second, as SC's `TempoClock`. `events clk.play` plays on it; `{:tempo t} clk.set` changes its tempo.

Use it to play patterns at a tempo other than the default 1 beat per second, or to change tempo live.

```stac
synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .01 .1 env.asr gate envgen.kr.done * amp *]
2 clock.new = c
{:instrument 'ping :dur 1} pbind c.play = p
{:tempo 3} c.set
p.stop
```

### clock

`clock (--> namespace)`

The namespace of `clock.new`. The default clock runs at 1 beat per second.

Use `clock.new` to make a clock.

```stac
clock,new type      ;-> Prim
```

## set operations

### S

`S (list --> set)`

Pushes the list's distinct items, in order of first appearance.

Use it to remove duplicate pitches or values.

```stac
[1 2 2 3 1] S       ;-> [1 2 3]
```

### S|

`S| (listA listB --> set)`

Pushes the union of two lists' items, without duplicates.

Use it to merge two pitch sets.

```stac
[1 2] [2 3] S|      ;-> [1 2 3]
```

### S&

`S& (listA listB --> set)`

Pushes the items both lists hold.

Use it to find the common notes of two chords.

```stac
[1 2] [2 3] S&      ;-> [2]
```

### Sx

`Sx (listA listB --> set)`

Pushes the items exactly one of the two lists holds.

Use it to find the notes that change between two chords.

```stac
[1 2] [2 3] Sx      ;-> [1 3]
```

### S-

`S- (listA listB --> set)`

Pushes the first list's items that the second does not hold.

Use it to remove a set of values from a list.

```stac
[1 2 3] [2] S-      ;-> [1 3]
```

### S=

`S= (listA listB --> set)`

Pushes 1 if the two lists hold the same items, ignoring order and repeats, else 0.

Use it to compare chords regardless of voicing order.

```stac
[1 2] [2 1 1] S=    ;-> 1
```

### subset?

`subset? (listA listB --> set)`

Pushes 1 if every item of the first list is in the second, else 0.

Use it to test whether a chord fits a scale.

```stac
[1] [1 2] subset?   ;-> 1
```

### find

`find (item(s) list --> set)`

Pushes the index of the item in a finite list, counting from 0, or -1 if it is absent.

Use it to look up a value's position, e.g. a note in a scale.

```stac
3 [5 3 7] find      ;-> 1
9 [5 3 7] find      ;-> -1
```

### Shas

`Shas (item(s) list --> set)`

Pushes 1 if a finite list holds the item, else 0.

Use it as a membership test; `find` also gives the position.

```stac
3 [5 3 7] Shas      ;-> 1
```
