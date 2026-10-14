# ssp: state and tasks

State as of 2026-10-14. ../ssp is not checked out on the Linux machine used for sapf-sc's Linux check (see next-steps.md), so nothing below has run there.

## State

- `scsy`, in ../ssp `plugins/scsynth`, plays sapf-sc's `>nrt` scores (`.osc`) and loads `// @buf` sidecar lines. The work is uncommitted in the macOS checkout of ../ssp.
- Its tests ran on macOS only, against a scratch build of libscsynth. The old session built libsndfile and libscsynth by hand: without `--exclude-libs`, with `-undefined dynamic_lookup` for the plugins, and with `SC_Filesystem_unix.cpp` compiled for macOS. It then built `scsynth_test.cpp` and `scsy_test.cpp` against them. That build is gone.
- `plugins/scsynth/CMakeLists.txt` gained an include path for `ScsynthEngine.cpp` (`sndfile.h`). No build has used it yet.

## Tasks

1. Commit the scsy work in the macOS checkout, so a Linux checkout can get it.
2. On Linux, run ../ssp's `make test`. It runs `scripts/build_deps.sh host`, then the tests in `plugins/scsynth/tests`. This is the first build that uses the new `sndfile.h` include path.
3. Run the SSP cross build.
4. Check that scsy's libscsynth does not unload plugins when a World ends. In sapf-sc, an NRT World ending called `World_Cleanup(world, true)` and `dlclose`d the UGen plugins under a running realtime World, which segfaulted in `Graph_Calc`. `cmake/patch-supercollider.sh` changes that call to `World_Cleanup(world, false)`. This matters only if scsy creates or ends more than one World per process, or ends one while another runs. Not checked.
5. If ../ssp patches SuperCollider's source through FetchContent's `PATCH_COMMAND`, check whether editing the patch re-runs it. FetchContent compares only the command string; sapf-sc puts the script's SHA-256 in the command (`cmake/EmbeddedScsynth.cmake`). Not checked.
6. Play a sapf-sc score on the SSP: `>nrt` writes `name.osc`, and `>sd` writes the sidecar `name.txt`; copy any `@buf` files beside them.

## Known differences from sapf

- `log`, `log2` and `log10` of a negative input: sapf gives NaN. On Linux x86, scsynth's UnaryOpUGen takes the `log_nova` SIMD path when `NOVA_SIMD` is on (`server/plugins/UnaryOpUGens.cpp:1058`) and returns finite values. Whether the SSP's ARM build takes that path is not known.

## Related sapf-sc work

- `>sd` writing `@p` sidecar lines for `ctl` controls: item 4 in next-steps.md, format in TODO.md.
