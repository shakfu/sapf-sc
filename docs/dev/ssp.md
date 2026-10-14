# ssp: state and tasks

State as of 2026-10-14, on the Linux machine, ../ssp branch `sapf` (`c1290dd`).

## State

- `scsy`, in ../ssp `plugins/scsynth`, plays sapf-sc's `>nrt` scores (`.osc`) and loads `// @buf` sidecar lines. Committed as `0301b27`.

- `make test` passes on Linux: 169 tests. The scsy tests compile the plugin sources with their own flags (`plugins/scsynth/tests/test_scsynth.py`), not through `plugins/scsynth/CMakeLists.txt`. They run `scripts/build_deps.sh host` only when `build/deps/host/lib/libscsynth.a` is missing.

- `make ssp` passes. It is the first build to use the `sndfile.h` include path in `plugins/scsynth/CMakeLists.txt`. Output: `build.cmake.ssp/plugins/scsynth/SCSYNTH_artefacts/Release/VST3/scsy.vst3/Contents/armv7l-linux/scsy.so`.

- World lifetime: `ScWorld::close` calls `World_Cleanup(world_, false)` and unloads the UGens only when the last open World closes (`plugins/scsynth/Source/ScWorld.cpp`). Scores run in the realtime World: a worker thread sends their bundles ahead of its clock. No NRT World runs, so sapf-sc's `Graph_Calc` segfault cannot occur. Inferred from the code; no test opens two Worlds.

- Patching: ../ssp does not use FetchContent. `scripts/build_deps.sh` applies each patch to `build/deps/src` once, skipping it when a reverse dry run succeeds. An edited patch fails that check, then fails to apply over the old one, which stops the script. Delete the patched source directory to re-apply. Inferred from the script; not run.

- Device test (2026-10-14): three files play on the SSP in scsy built from `c1290dd`. `sapftex.osc` (`ola` klank texture, 20 s), `sapfosc.osc` (`sawTbl osc`, 30 wave-table buffers, 1.2 MB), and `sapfloop.scsyndef` with sidecar `// @buf 0 loop.wav`.

- `>nrt` writes a score when values are left on the stack, without a warning. A typo dropped `osc` from a test score this way.

## Tasks

1. A `sapf` SSP plugin for live coding: the interpreter with its own `ScWorld`, ttyp's keyboard input, a multi-line editor, the prelude loaded at start. Design: ../ssp docs/dev/sapf-plugin.md. Step 1 (the interpreter on the SSP) passed 2026-10-14. Its sapf-sc changes: `SAPF_REPLXX` (CMake option, default ON), and `Array::setSize`/`addSize` taking `int64_t`: as `size_t`, `Gen::produce`'s negative delta became 2^32 - k on 32-bit ARM and crashed every list operation. Steps 2 and 4 still need `ScsynthServer::now()`, a `post` hook and an interrupt flag.

2. Compiler-dependent graphs: with the same seed, gcc and clang builds write different defs for six examples (e.g. `rocks on rails`): UGen order and random draws follow function argument evaluation order, which C++ leaves unspecified. That cause is inferred from gcc vs clang; the code is not located yet.

## Known differences from sapf

- `log`, `log2` and `log10` of a negative input: sapf gives NaN. On Linux x86, scsynth's UnaryOpUGen takes the `log_nova` SIMD path when `NOVA_SIMD` is on (`server/plugins/UnaryOpUGens.cpp:1058`) and returns finite values. On the SSP (ARM, also built with `NOVA_SIMD`) all three give NaN, as sapf does (../ssp `plugins/sapf/tests/run_server_on_ssp.sh`, 2026-10-14). Why ARM differs is not traced; nova-simd falling back to scalar `std::log` without an ARM vector type is a guess.

## Related sapf-sc work

- `>sd` writing `@p` sidecar lines for `ctl` controls: item 4 in next-steps.md, format in TODO.md.
