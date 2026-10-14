"""play and stop against scripts/server.py, nanosynth's embedded scsynth in its own process. An
in-process server stops answering UDP after the NRT renders other tests run. It needs an audio
device; every synth here is silent."""

import os
import socket
import subprocess
import sys
import time

import pytest
from nanosynth.osc import OscMessage

from conftest import ROOT, SAPF, run_sapf

PORT = 57193


def status() -> OscMessage:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1)
    sock.sendto(OscMessage("/status").to_datagram(), ("127.0.0.1", PORT))
    return OscMessage.from_datagram(sock.recv(4096))


@pytest.fixture(scope="module")
def server():
    proc = subprocess.Popen(
        [sys.executable, str(ROOT / "scripts" / "server.py"), "--port", str(PORT)],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    line = proc.stdout.readline()
    if "scsynth on" not in line:
        proc.kill()
        pytest.skip(f"cannot boot scsynth: {line}")
    yield proc
    proc.terminate()
    proc.wait(timeout=10)


@pytest.fixture
def sapf(server, tmp_path, monkeypatch):
    monkeypatch.setenv("SAPF_SCSYNTH", f"127.0.0.1:{PORT}")
    run_sapf("stop", tmp_path)
    return lambda code: run_sapf(code, tmp_path)


def synths(server):
    return status().contents[2]


def test_play_starts_a_synth_and_stop_frees_it(server, sapf):
    out = sapf("440 0 sinosc 0 * play")
    assert "error" not in out, out
    assert synths(server) == 1
    sapf("stop")
    time.sleep(0.2)
    assert synths(server) == 0


def test_finite_signal_frees_itself(server, sapf):
    sapf("440 0 sinosc 0 * .2 T play")
    assert synths(server) == 1
    time.sleep(0.5)
    assert synths(server) == 0


def test_play_reports_a_missing_server(tmp_path, monkeypatch):
    monkeypatch.setenv("SAPF_SCSYNTH", "127.0.0.1:57199")
    out = run_sapf("440 0 sinosc play", tmp_path)
    assert "no reply" in out and "error" in out, out


def test_ola_voices_start_and_stop(server, sapf, tmp_path):
    # -i keeps sapf running after the file, so its scheduler keeps starting voices.
    src = tmp_path / "ola.sapf"
    src.write_text("\\i[ 0 0 sinosc .3 T ] .1 1 1 ola 0 at play\n")
    env = dict(os.environ, SAPF_SCSYNTH=f"127.0.0.1:{PORT}", SAPF_PRELUDE="/dev/null")
    proc = subprocess.Popen([str(SAPF), "-q", "-i", str(src)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, env=env)
    try:
        time.sleep(1)
        # The main synth and the voices of the last .3 s, .1 s apart.
        assert 3 <= synths(server) <= 5
        defs = status().contents[4]
        time.sleep(0.5)
        assert status().contents[4] <= defs + 1, "voice defs are not freed"
        proc.stdin.write("stop\n")
        proc.stdin.flush()
        time.sleep(0.5)
        assert synths(server) == 0
    finally:
        out = proc.communicate(timeout=5)[0]
    assert "error" not in out, out
