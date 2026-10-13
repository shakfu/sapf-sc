"""play and stop against scripts/server.py, nanosynth's embedded scsynth in its own process. An
in-process server stops answering UDP after the NRT renders other tests run. It needs an audio
device; every synth here is silent."""

import socket
import subprocess
import sys
import time

import pytest
from nanosynth.osc import OscMessage

from conftest import ROOT, run_sapf

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
