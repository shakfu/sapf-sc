"""play with the embedded scsynth. Kept apart from test_play.py: scripts/server.py's PortAudio opens
the raw ALSA devices, and while it runs PipeWire cannot start the embedded server's stream."""

import os
import subprocess
import time

import pytest

from conftest import ROOT, SAPF, SAPF_LIB


def test_embedded_server_plays_a_file_to_its_end(tmp_path):
    # Without STAC_SCSYNTH, play starts scsynth in the sapf process, and a file waits for its sound.
    src = tmp_path / "e.sapf"
    src.write_text("440 0 sinosc 0 * .5 T play\n")
    env = {k: v for k, v in os.environ.items() if k != "STAC_SCSYNTH"}
    t0 = time.monotonic()
    r = subprocess.run([str(SAPF), "-q", *SAPF_LIB, str(src)], capture_output=True, text=True, env=env, timeout=30)
    elapsed = time.monotonic() - t0
    if "scsynth did not start" in r.stdout:
        pytest.skip("no audio device")
    assert r.returncode == 0 and "error" not in r.stdout, r.stdout
    assert 0.45 < elapsed < 10, elapsed


PING = "synthdef ping freq:440 amp:.1 gate:1 [freq sinosc .005 .05 env.asr gate envgen.kr.done * amp * 2 X]\n"


@pytest.mark.parametrize("code", [
    "[110 saw .001 *] play\n",
    PING + "{:instrument 'ping :amp .001 :dur .01} pbind play\n",
], ids=["synth", "pattern"])
def test_quit_while_sound_plays_exits_cleanly(code):
    # quit unloaded SC's plugins while the audio thread still ran a synth, and segfaulted
    env = {k: v for k, v in os.environ.items() if k != "STAC_SCSYNTH"}
    proc = subprocess.Popen([str(SAPF), "-q", "-p", str(ROOT / "prelude.stac")], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
    proc.stdin.write(code)
    proc.stdin.flush()
    time.sleep(.5)
    out, _ = proc.communicate("quit\n", timeout=30)
    if "scsynth did not start" in out:
        pytest.skip("no audio device")
    assert proc.returncode == 0 and "error" not in out and "failed" not in out, out
