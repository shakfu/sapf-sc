# Next steps

State as of 2026-10-14. Nothing below is committed in sapf-sc or in ../ssp.

## State

- `make test` passes on macOS and Linux: 262 ctest, 337 pytest.
- `build/sapf` embeds scsynth (`cmake/EmbeddedScsynth.cmake`): `play` starts it on first use; `SAPF_SCSYNTH` selects a UDP server instead. `>sf` and `>sfo` render in-process in NRT mode.
- SuperCollider 3.14.1 and libsndfile 1.2.2 are fetched as pinned tarballs. `cmake/patch-supercollider.sh` makes SC build as a subdirectory and keeps the UGens loaded after an NRT render.
- `sapf-examples.txt` keeps 216 of sapf's 312 examples; 160 play or render. `sapf-prelude.txt` keeps 280 of 285 definitions.
- Polyphony: `ola` voices are synths on private buses 512..959, scheduled by a thread for `play`, or written into an NRT score by `>nrt`.
- Wave tables (`wavefill`, `osc`, `sawTbl`...) use buffers 512..1023; samples (`sf>`, `playbuf`) use buffers 0..63.
- ../ssp `scsy` (uncommitted, in `plugins/scsynth`): plays `>nrt` scores (`.osc`) and loads `// @buf` sidecar lines. Its tests were run on macOS against a scratch build of libscsynth; see Testing scsy below.

## Unverified

1. Sound from the embedded server on Linux. `uv run pytest -q -rs tests/py/test_play.py tests/py/test_sf.py` shows whether the audio tests ran or were skipped. By hand: `build/sapf`, then `440 0 sinosc .1 * 1 T play`. If it prints "scsynth did not start", set `SAPF_DEVICE` to a device from scsynth's list. When it works, say in the README that Linux is tested.
2. ../ssp on Linux: `make test` (runs the scsy tests natively), and the SSP cross build. `plugins/scsynth/CMakeLists.txt` gained an include path for `ScsynthEngine.cpp` (`sndfile.h`) that no build has used yet.
3. `>sfo`'s open step (`open` on macOS, `xdg-open` on Linux).

## Open work

4. `>sd` writes `@p` lines for `ctl` controls into the scsy sidecar, as it writes `@buf` lines. Needs decisions: where a control's min, max and curve come from (`ctl` takes only a default and a name), and slot order (`>sd` sorts parameters by name). See TODO.md.
5. `lpcombc` (10 examples): its low pass sits inside the feedback loop. Options: an approximation, or a plugin UGen.
6. Unmapped words by examples blocked: `violet` 5, `klang` 3, `pdiv` 3, `par`, `octenv`, `quadenv`, `vsaw`, `gray`, and about a dozen with one or two uses. The mouse controls are excluded on purpose.
7. Live patching (`zplug`, `set`): needs `/n_set` on running synths.
8. `ola` voices start on the 64-frame block holding their time. `OffsetOut` would place them on the frame but crashed the NRT renderer; not investigated.
9. Buffers from sapf lists (`#[...] buf`); samples load only from WAV or AIFF files.
10. Six bugs in original sapf are logged in docs/dev/upstream-sapf-bugs.md; reporting them upstream is undecided.
11. tests/fixtures/ref (3.6 MB of WAV) could be packed into one `.npz`; kept as files for now.

## Rebuilding things that lived in the old session's scratchpad

- Reference renders (`make reference SAPF_ORIG=...`) need original sapf built from ../sapf. The old build is gone; build ../sapf with its own CMake and pass the binary's path. The references in tests/fixtures/ref came from that macOS build (Accelerate); a Linux build of sapf may differ in the last bits, so compare before regenerating.
- The example check ran each block of sapf's examples with `play` redefined as `\c [c 1 "ex" >nrt] = play` and `>sfo` as `\c n [c n >sf] = >sfo`, after loading the definitions that passed. The script is gone; it is about 60 lines of Python.

## Testing scsy

../ssp's `make test` builds `scripts/build_deps.sh host` (Linux only) and runs `plugins/scsynth/tests`. On macOS the old session built libsndfile and libscsynth by hand (without `--exclude-libs`, with `-undefined dynamic_lookup` for the plugins, and with `SC_Filesystem_unix.cpp` compiled for macOS), then built `scsynth_test.cpp` and `scsy_test.cpp` against them. On Linux, `make test` should do all of that itself.
