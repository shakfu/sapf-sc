# stac CLAP plugin

A stereo CLAP instrument that runs stac code, with scsynth inside the plugin. It is built by default (`STAC_BUILD_CLAP=ON`, with the embedded scsynth) as `build/examples/clap/stac.clap`. Beside it, the folder `stac/` holds the UGen plugins and the bundled `prelude.stac`, `std.stac` and `sapf.stac`.

Install by copying both to the CLAP folder:

| Platform | Folder | Copy |
|-|-|-|
| Linux | `~/.clap/` | `stac.clap` and `stac/` |
| macOS | `~/Library/Audio/Plug-Ins/CLAP/` | `stac.clap`, a bundle holding its resources in `Contents/Resources` |

To try it without a DAW, `make editor` opens the editor in `stac_dev_host` (`dev_host.cpp`). This minimal host plays to the default audio output.

## Input

**Editor.** Dear ImGui in a pugl OpenGL view (`thirdparty/imgui`, `thirdparty/pugl`).

| Button | Key | Action |
|-|-|-|
| Eval Block | Cmd+Return (macOS), Ctrl+Return | Evaluate the selection; else the current line, or the whole multi-line `( )`, `[ ]` or `{ }` group it is part of |
| Eval All | Cmd+Shift+Return (macOS), Ctrl+Shift+Return | Evaluate the whole editor as one piece |
| Stop | Ctrl+. | What `stop` does: end patterns and textures, free every synth. Cmd+. works only if the host passes it |

Output and errors appear in the log pane. **Clear** empties the editor and the log; it asks first when the editor has text. **Wrap** (on by default) wraps long lines in both panes. On Linux the editor needs the host's CLAP timer support to redraw.

**UDP.** Each datagram to `127.0.0.1:57150` is evaluated; the reply is the output, or `ok`:

```
$ nc -u 127.0.0.1 57150
1 2 +
3
```

UDP is off by default; toggle it in the editor. `STAC_CLAP_UDP_PORT` turns it on at load and sets the port. It binds to loopback only, but any local process can send it code.

The editor text and the Wrap and UDP settings are saved with the host project.

## Environment

A DAW does not inherit the shell's environment, so the plugin needs none:

- It loads the bundled `prelude.stac`, or `STAC_PRELUDE` if set.
- It appends its resource folder to `STAC_PATH`, so `import sapf.*` finds the bundled module. This changes the host process's environment.
- It loads UGens from the bundled `plugins/`, or `STAC_PLUGINS` if set.

`STAC_SCSYNTH` still sends to a server over UDP instead; the plugin's output is then silent.

## Audio

The host's audio callback renders scsynth one 64-frame block at a time, through `src/server/HostAudioDriver.cpp`. scsynth opens no device. Its clock counts the frames rendered, so `play` and patterns schedule against the host's audio, not the wall clock. Activating the plugin sets scsynth's sample rate; deactivating it frees every synth and stops scsynth, which starts again on the next `play`.

## Limits

- One instance per process: the interpreter and scsynth are process-wide. A second instance fails to load.
- Output only; no audio input or MIDI from the host.
- scsynth handles commands only while the host calls `process()`. With processing stopped, `play` fails with `no reply` after 1 s.
- Offline rendering much faster than real time can play patterns late. Players check the clock every 50 ms of real time; at 4x or more, that exceeds their .2 s lookahead in rendered time.
- Evaluation runs on its own thread. Code that never returns blocks removing the plugin.
- macOS: untested. The bundle layout and resource lookup are written but have not run.

`test_clap_host.cpp` loads the built plugin headlessly and runs as ctest `example_clap_plugin`. It checks playback level, pattern timing against rendered frames at 1x and 2x real time, state, and a second instance after the first is destroyed. `test_editor_logic.cpp` covers which code an eval request selects.
