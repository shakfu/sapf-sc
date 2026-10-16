# stac names: prefix stripping and short namespace names

Decision record for open questions 5 and 6 in `stac.md`. Decided 2026-10-15: both are done together, as the last stage. Status: superseded 2026-10-16 by `stac.md`, Namespaces, Proposal: canonical namespaces and import. Neither stripping nor short names is planned; this record keeps the counts.

Counts below come from `helpall` in `build/stac` and `spec/sclang-reference.json`. 384 namespaced words exist; 155 are in `ug`.

## Question 5: strip SC prefixes inside a namespace

Five namespaces hold a shared SC prefix. A mechanical rule strips 84 words, with no collision inside any namespace.

| Namespace | Words | Rule | Stripped | Chars saved |
|-|-|-|-|-|
| `fft` | 48 | drop `PV_` | 34 | 102 |
| `buffer` | 39 | drop leading `Buf`, or trailing `Buf` | 22 | 66 |
| `demand` | 21 | drop `D` before a lowercase letter; not `Demand`, `DemandEnvGen`, `Duty` | 17 | 17 |
| `beqsuite` | 8 | drop `B` before a capital | 8 | 8 |
| `granular` | 3 | drop `Grain` | 3 | 15 |

The others have no shared prefix: `ambisonics`, `analysis`, `bela`, `chaos`, `info`, `io`, `node`, `random`, `ui`.

Full mapping:

- `fft`: `pv_add`->`add`, `pv_binscramble`->`binscramble`, `pv_binshift`->`binshift`, `pv_binwipe`->`binwipe`, `pv_brickwall`->`brickwall`, `pv_conformalmap`->`conformalmap`, `pv_conj`->`conj`, `pv_copy`->`copy`, `pv_copyphase`->`copyphase`, `pv_diffuser`->`diffuser`, `pv_div`->`div`, `pv_hainsworthfoote`->`hainsworthfoote`, `pv_jensenandersen`->`jensenandersen`, `pv_localmax`->`localmax`, `pv_magabove`->`magabove`, `pv_magbelow`->`magbelow`, `pv_magclip`->`magclip`, `pv_magdiv`->`magdiv`, `pv_magfreeze`->`magfreeze`, `pv_magmul`->`magmul`, `pv_magnoise`->`magnoise`, `pv_magshift`->`magshift`, `pv_magsmear`->`magsmear`, `pv_magsquared`->`magsquared`, `pv_max`->`max`, `pv_min`->`min`, `pv_mul`->`mul`, `pv_phaseshift`->`phaseshift`, `pv_phaseshift270`->`phaseshift270`, `pv_phaseshift90`->`phaseshift90`, `pv_randcomb`->`randcomb`, `pv_randwipe`->`randwipe`, `pv_rectcomb`->`rectcomb`, `pv_rectcomb2`->`rectcomb2`.
- `buffer`: `bufallpassc/l/n`->`allpassc/l/n`, `bufchannels`->`channels`, `bufcombc/l/n`->`combc/l/n`, `bufdelayc/l/n`->`delayc/l/n`, `bufdur`->`dur`, `bufframes`->`frames`, `bufratescale`->`ratescale`, `bufrd`->`rd`, `bufsamplerate`->`samplerate`, `bufsamples`->`samples`, `bufwr`->`wr`, `clearbuf`->`clear`, `grainbuf`->`grain`, `playbuf`->`play`, `recordbuf`->`record`, `setbuf`->`set`.
- `demand`: `dbrown`->`brown`, `dconst`->`const`, `ddup`->`dup`, `dgeom`->`geom`, `dibrown`->`ibrown`, `diwhite`->`iwhite`, `drand`->`rand`, `dreset`->`reset`, `dseq`->`seq`, `dser`->`ser`, `dseries`->`series`, `dshuf`->`shuf`, `dswitch`->`switch`, `dswitch1`->`switch1`, `dwhite`->`white`, `dwrand`->`wrand`, `dxrand`->`xrand`.
- `beqsuite`: `ballpass`->`allpass`, `bbandpass`->`bandpass`, `bbandstop`->`bandstop`, `bhipass`->`hipass`, `bhishelf`->`hishelf`, `blowpass`->`lowpass`, `blowshelf`->`lowshelf`, `bpeakeq`->`peakeq`.
- `granular`: `grainfm`->`fm`, `grainin`->`in`, `grainsin`->`sin`.

Hazards:

- **Near-duplicates in `buffer`.** `buffer.rd` is the BufRd UGen; `buffer.read` loads a file. They differ by 2 letters and do unrelated things. Exception candidate: keep `bufrd` and `bufwr`, or rename `read` to `file`.
- **`buffer.set` reads as a method.** `.set` is a method on Node, Clock and Ndef. SetBuf is rare; exception candidate: keep `setbuf`.
- **SC lookup.** A stripped word no longer spells its SC class. Help text names the class. The planned completion searches by SC name (`stac.md`, Completion), so `playb<TAB>` could still offer `buf.play`.
- **Misplaced classes.** `Dbufrd` and `Dbufwr` are demand-rate but sit in `buffer`, by their first SC category. Moving them to `demand` as `bufrd`/`bufwr` fits the stripping rule.

## Question 6: short namespace names

A namespace name is a global word. A user binding with the same name hides it for the rest of the program; the linter flags this as W2.

| Now | Candidates | Taken or risky |
|-|-|-|
| `buffer` | `buf` | `buf` is a common variable name for a buffer |
| `demand` | `dem` | `d` is bound in `prelude.stac` |
| `analysis` | `ana` | `an` is used in `examples/sapf.stac` |
| `random` | `rnd` | `rand` is a sapf word |
| `ambisonics` | `amb`, `ambi` | none |
| `granular` | `gran`, `grain` | none |
| `beqsuite` | `beq` | none |

Kept as they are: `fft`, `io`, `ui`, `info`, `node`, `chaos`, `bela`, plus the interpreter namespaces `env`, `bus`, `clock`. `env` already has the hiding problem: `examples/sapf.stac` binds `= env` at lines 250, 351 and 358.

## Combined

Stripping and short names stack; neither substitutes for the other.

| SC | Now | Strip only | Short only | Both |
|-|-|-|-|-|
| `PlayBuf` | `buffer.playbuf` (14) | `buffer.play` (11) | `buf.playbuf` (11) | `buf.play` (8) |
| `Dseq` | `demand.dseq` (11) | `demand.seq` (10) | `dem.dseq` (8) | `dem.seq` (7) |
| `PV_MagFreeze` | `fft.pv_magfreeze` (16) | `fft.magfreeze` (13) | same as now | `fft.magfreeze` (13) |
| `BLowPass` | `beqsuite.blowpass` (17) | `beqsuite.lowpass` (16) | `beq.blowpass` (12) | `beq.lowpass` (11) |

## Recommendation

1. Strip by the 5 rules, with 2 exceptions: keep `bufrd`/`bufwr`, keep `setbuf`. 81 words change.
2. Move `Dbufrd`/`Dbufwr` to `demand` as `bufrd`/`bufwr`.
3. Short names: `buf`, `dem`, `ana`, `rnd`, `amb`, `gran`, `beq`. Accept the `buf` hiding risk; W2 already reports it.

Alternatives:

- **Keep `buffer`.** It avoids the hiding problem and costs 3 characters per use.
- **Fold `granular` (3 words), `ui` (4) and `ambisonics` (5) into `ug`.** This removes 3 namespaces. Bare names would then include `grainsin` and `mousex`; `ug` would hold 167 words.
- **Aliases, long and short.** Rejected: every namespace is listed twice in completion and help.

## Work once decided

- The overlay `spec/stac-ugens.json` gains a `strip` rule per namespace and a `rename` map; `gen_ugens.py` applies them, and `--check` tests the output.
- 146 references in 7 files under `tests/`, `docs/`, `scripts/` and `src/` change.
- The linter's "undefined; did you mean" hint maps SC class names to the new words.
