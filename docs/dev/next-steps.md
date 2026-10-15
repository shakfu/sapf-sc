# Next steps

State as of 2026-10-14. sapf-sc is committed up to dd66917. ../ssp's state and tasks are in docs/dev/ssp.md.

## State

- `make test` passes on Linux: 262 ctest, 340 pytest. macOS passed before the `op1_log*` references and the `>sfo` tests changed, and has not been rerun since.

- `build/stac` embeds scsynth (`cmake/EmbeddedScsynth.cmake`): `play` starts it on first use; `STAC_SCSYNTH` selects a UDP server instead. `>sf` and `>sfo` render in-process in NRT mode.

- SuperCollider 3.14.1 and libsndfile 1.2.2 are fetched as pinned tarballs. `cmake/patch-supercollider.sh` makes SC build as a subdirectory and keeps the UGens loaded after an NRT render.

- `examples/sapf.stac` keeps 216 of sapf's 312 examples; 160 play or render. `prelude.stac` keeps 280 of 285 definitions.

- Polyphony: `ola` voices are synths on private buses 512..959, scheduled by a thread for `play`, or written into an NRT score by `>nrt`.

- Wave tables (`wavefill`, `osc`, `sawTbl`...) use buffers 512..1023; samples (`sf>`, `playbuf`) use buffers 0..63.

## Open work

4. `>sd` writes `@p` lines for `ctl` controls into the scsy sidecar, as it writes `@buf` lines. Needs decisions: where a control's min, max and curve come from (`ctl` takes only a default and a name), and slot order (`>sd` sorts parameters by name). See TODO.md.

5. `lpcombc` (10 examples): its low pass sits inside the feedback loop. Options: an approximation, or a plugin UGen.

6. Unmapped words by examples blocked: `violet` 5, `klang` 3, `pdiv` 3, `par`, `octenv`, `quadenv`, `vsaw`, `gray`, and about a dozen with one or two uses. The mouse controls are excluded on purpose.

7. Live patching (`zplug`, `set`): needs `/n_set` on running synths.

8. `ola` voices start on the 64-frame block holding their time. `OffsetOut` would place them on the frame but crashed the NRT renderer; not investigated.

9. Buffers from sapf lists (`#[...] buf`); samples load only from WAV or AIFF files.

10. Six bugs in original sapf are logged in docs/dev/upstream-sapf-bugs.md; reporting them upstream is undecided.

11. tests/fixtures/ref (3.6 MB of WAV) could be packed into one `.npz`; kept as files for now.

## Linux check (2026-10-14)

- Sound from the embedded server, heard from the REPL. The tests boot scsynth on its default device, an HDMI output on the test machine; their synths are silent, so the device does not matter. If `play` is silent, set `STAC_DEVICE`.

- `>sfo` opens the file with `xdg-open`. It used `system("xdg-open ... &")`, which always returns 0, so a missing opener or handler failed silently, and a `"` or `$(...)` in the name reached the shell. It now uses `posix_spawnp` (no shell), and a thread reports a non-zero exit, so the REPL never waits for the opener; the Windows branch (`start`) is untested.

- Bug: a `>sf` render while `play` ran segfaulted in `Graph_Calc`, because the NRT World `dlclose`d plugins the realtime World was running. The fix in `patch-supercollider.sh` (`World_Cleanup(world, false)`) was added after `build/` had run its patch step. FetchContent re-runs `PATCH_COMMAND` only when the command string changes. `cmake/EmbeddedScsynth.cmake` now passes the script's SHA-256 as an ignored argument and adds the script to `CMAKE_CONFIGURE_DEPENDS`. The patch step re-runs on an already-patched tree, so the script must stay idempotent.

- Bug: the `test_play.py` fixture skipped every test because ALSA warnings came before "scsynth on" in `server.py`'s output. It now reads until that line or EOF.

- Bug: the embedded-server test in `test_play.py` hung until its 30 s timeout. nanosynth's PortAudio does not list the `pipewire` or `default` ALSA PCMs, so `server.py` opens raw `hw:` devices, capture included. PipeWire then cannot start its input node, and the embedded `World_New` waited forever in `snd_pcm_prepare`. Two fixes: `STAC_INPUTS` now defaults to 0, so `play` opens no input stream; and the test moved to `test_play_embedded.py`, so `server.py` has exited before it runs. An output-device clash would still hang, hence the split as well.

- `op1_log`, `op1_log2` and `op1_log10` failed on Linux. For a negative input sapf gives NaN, but nanosynth's scsynth takes the `log_nova` SIMD path (`UnaryOpUGens.cpp:1058`) and returns finite values. The cases now take `POS`, a positive input; their references were re-rendered on macOS with the sapf v0.1.22 release.

## Rebuilding things that lived in the old session's scratchpad

- Reference renders: `SAPF_ORIG=<sapf> uv run python scripts/make_reference.py [case names]`. The macOS release of sapf v0.1.22 (https://github.com/lfnoise/sapf/releases/tag/v0.1.22) works: it has no `-q` and runs no file argument, so each case runs as the prelude (`-p`) and `quit` goes to stdin. The `op1_log*` references come from that release; the other 194 came from an earlier ../sapf build that is gone. Before regenerating, `scripts/check_reference.py <case names>` compares fresh renders with the stored references without writing them.

- The example check ran each block of sapf's examples with `play` redefined as `\c [c 1 "ex" >nrt] = play` and `>sfo` as `\c n [c n >sf] = >sfo`, after loading the definitions that passed. The script is gone; it is about 60 lines of Python.
