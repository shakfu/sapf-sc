"""play with the embedded scsynth. Kept apart from test_play.py: scripts/server.py's PortAudio opens
the raw ALSA devices, and while it runs PipeWire cannot start the embedded server's stream."""

import os
import subprocess
import time

import pytest

from conftest import SAPF, SAPF_LIB


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
