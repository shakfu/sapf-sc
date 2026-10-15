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

from conftest import ROOT, SAPF, SAPF_LIB, run_sapf

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
    lines = []  # ALSA prints probe warnings on Linux before the server is up
    for line in proc.stdout:
        lines.append(line)
        if "scsynth on" in line:
            break
    else:
        proc.kill()
        pytest.skip(f"cannot boot scsynth: {''.join(lines[-5:])}")
    yield proc
    proc.terminate()
    proc.wait(timeout=10)


@pytest.fixture
def sapf(server, tmp_path, monkeypatch):
    monkeypatch.setenv("STAC_SCSYNTH", f"127.0.0.1:{PORT}")
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
    monkeypatch.setenv("STAC_SCSYNTH", "127.0.0.1:57199")
    out = run_sapf("440 0 sinosc play", tmp_path)
    assert "no reply" in out and "error" in out, out


def test_ola_voices_start_and_stop(server, sapf, tmp_path):
    # -i keeps sapf running after the file, so its scheduler keeps starting voices.
    src = tmp_path / "ola.sapf"
    src.write_text("\\i[ 0 0 sinosc .3 T ] .1 1 1 ola 0 at play\n")
    env = dict(os.environ, STAC_SCSYNTH=f"127.0.0.1:{PORT}")
    proc = subprocess.Popen([str(SAPF), "-q", *SAPF_LIB, "-i", str(src)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
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


def test_ola_stops_with_its_main_synth(server, sapf, tmp_path):
    # The main synth ends after .5 s; its scheduler then starts no more voices.
    src = tmp_path / "f.sapf"
    src.write_text("\\i[ 0 0 sinosc .2 T ] .1 1 1 ola 0 at .5 T play\n")
    env = dict(os.environ, STAC_SCSYNTH=f"127.0.0.1:{PORT}")
    proc = subprocess.Popen([str(SAPF), "-q", *SAPF_LIB, "-i", str(src)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, env=env)
    try:
        time.sleep(1.5)
        assert synths(server) == 0
    finally:
        proc.stdin.close()
        proc.communicate(timeout=5)


# defs and nodes

def node_of(out):
    """The ID in the first 'Node N' sapf printed."""
    return int(out.split("Node ", 1)[1].split()[0])


def control(node, name):
    """The value of a running synth's control, by /s_get."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1)
    sock.sendto(OscMessage("/s_get", node, name).to_datagram(), ("127.0.0.1", PORT))
    reply = OscMessage.from_datagram(sock.recv(4096))
    assert reply.address == "/n_set", reply
    return reply.contents[2]


TONE = "def tone amp:0 freq:440 = [freq ug.sinosc amp *]\n"
GATED = "def gated amp:0 gate:1 = [300 ug.sinosc .01 .3 env.asr gate ug.envgen.kr.done * amp *]\n"


def test_def_starts_a_synth_with_controls(server, sapf):
    out = sapf(TONE + "{:freq 660} tone = x  x pr cr")
    assert "error" not in out, out
    assert synths(server) == 1
    assert control(node_of(out), "freq") == pytest.approx(660)


def test_set(server, sapf):
    out = sapf(TONE + "{} tone = x  {:freq 880 :amp 0} x.set  x pr cr")
    assert "error" not in out, out
    assert control(node_of(out), "freq") == pytest.approx(880)


def test_set_leaves_nothing_on_the_stack(server, sapf):
    out = sapf(TONE + "{} tone = x  {:freq 880} x.set  x.free  stackDepth pr cr")
    assert "\n0\n" in out, out


def test_free(server, sapf):
    out = sapf(TONE + "{} tone = x  {} tone = y  x.free")
    assert "error" not in out, out
    assert synths(server) == 1


def test_release_ends_a_gated_synth(server, sapf):
    out = sapf(GATED + "{} gated = x  x.release")
    assert "error" not in out, out
    assert synths(server) == 1
    time.sleep(0.6)
    assert synths(server) == 0


def test_release_in(server, sapf):
    """SC's release(t): the envelope releases over t seconds, not its own .3."""
    out = sapf(GATED + "{} gated = x  1.5 x.releaseIn")
    assert "error" not in out, out
    time.sleep(0.6)
    assert synths(server) == 1
    time.sleep(1.4)
    assert synths(server) == 0


def test_def_is_sent_once_and_resent_when_redefined(server, sapf):
    out = sapf(TONE + "{} tone  {} tone  " + TONE.replace("440", "220") + "{} tone pr cr")
    assert "error" not in out, out
    assert synths(server) == 3
    assert control(node_of(out), "freq") == pytest.approx(220)


def test_add_action_and_target(server, sapf):
    out = sapf(TONE + "{} tone = a  {:target a :addAction 'after} tone = b  {} tone = c  a pr cr  b pr cr  c pr cr")
    assert "error" not in out, out
    a, b, c = [int(part.split()[0]) for part in out.split("Node ")[1:4]]
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1)
    sock.sendto(OscMessage("/g_queryTree", 0, 0).to_datagram(), ("127.0.0.1", PORT))
    tree = OscMessage.from_datagram(sock.recv(65536)).contents
    order = [x for x in tree[3:] if x in (a, b, c)]
    assert order == [c, a, b]  # c went to the head; b right after a


def test_synth_reports_a_missing_server(tmp_path, monkeypatch):
    monkeypatch.setenv("STAC_SCSYNTH", "127.0.0.1:57199")
    out = run_sapf(TONE + "{} tone", tmp_path)
    assert "no reply" in out and "error" in out, out


# buses and buffers

def query(address, *args):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(1)
    sock.sendto(OscMessage(address, *args).to_datagram(), ("127.0.0.1", PORT))
    return OscMessage.from_datagram(sock.recv(65536)).contents


def buffer_of(out):
    return int(out.split("Buffer ", 1)[1].split()[0])


def test_buffer_read(server, sapf, tmp_path):
    import numpy as np
    from test_samples import write_wav
    path = tmp_path / "tone.wav"
    write_wav(path, np.array([0.0, 0.25, 0.5, 0.75] * 100).reshape(-1, 1))
    out = sapf(f'"{path}" buffer.read pr cr')
    assert "error" not in out, out
    assert "(1 x 400)" in out
    buf = buffer_of(out)
    assert 0 <= buf < 64
    assert tuple(query("/b_query", buf)[1:3]) == (400, 1)


def test_buffer_load(server, sapf):
    out = sapf("[.5 .25 -1] buffer.load pr cr")
    assert "error" not in out, out
    buf = buffer_of(out)
    assert 64 <= buf < 512
    assert query("/b_getn", buf, 0, 3)[3:] == pytest.approx([0.5, 0.25, -1])


def test_buffer_alloc(server, sapf):
    out = sapf("100 2 buffer.alloc pr cr")
    assert "error" not in out, out
    assert tuple(query("/b_query", buffer_of(out))[1:3]) == (100, 2)


def test_effect_on_a_bus(server, sapf):
    """P12: the effect reads a private bus at the tail; the source writes to it."""
    out = sapf(TONE + "def verb bus mix:.4 = [bus 1 ug.in mix *]\n"
               "1 bus.audio = fx  {:bus fx :addAction 'tail} verb = v  {:out fx} tone = t  v pr cr  t pr cr")
    assert "error" not in out, out
    v, t = [int(part.split()[0]) for part in out.split("Node ")[1:3]]
    assert control(v, "bus") == 959
    assert control(t, "out") == 959


def group_of(out):
    return int(out.split("Group ", 1)[1].split()[0])


def test_group_holds_synths_and_send_frees_it(server, sapf):
    out = sapf(TONE + "{} node.group = g  {:target g} tone = a  {:target g :addAction 'tail} tone = b  "
               "{:freq 300} g.set  g pr cr a pr cr b pr cr")
    assert "error" not in out, out
    g = group_of(out)
    a, b = [int(part.split()[0]) for part in out.split("Node ")[1:3]]
    assert list(query("/g_queryTree", g, 0)[1:]) == [g, 2, a, -1, "tone", b, -1, "tone"]
    assert control(a, "freq") == control(b, "freq") == 300
    out = sapf(f'["/n_free" {g}] send')  # a whole real goes as an int, as SC's sendMsg
    assert "error" not in out, out
    assert synths(server) == 0


def test_control_bus_and_send(server, sapf):
    out = sapf("2 bus.control = b  [.5 .25] b.set  b pr cr")
    assert "Bus 16382 (2 control)" in out and "error" not in out, out
    assert list(query("/c_getn", 16382, 2)) == [16382, 2, pytest.approx(.5), pytest.approx(.25)]
    sapf('["/c_set" 100 .75] send')
    assert list(query("/c_get", 100)) == [100, pytest.approx(.75)]


# event streams

def interactive(code, tmp_path):
    """sapf -i on a file, which keeps it running after the file so its players keep scheduling."""
    src = tmp_path / "events.sapf"
    src.write_text(code)
    env = dict(os.environ, STAC_SCSYNTH=f"127.0.0.1:{PORT}")
    return subprocess.Popen([str(SAPF), "-q", *SAPF_LIB, "-i", str(src)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, env=env)


def test_events_play_and_stop(server, sapf, tmp_path):
    """A synth per event, .2 s apart; player.stop ends the stream; stop frees the synths."""
    proc = interactive(TONE + "{:instrument 'tone :dur .2} pbind play = pl\n", tmp_path)
    try:
        time.sleep(1.1)
        assert 4 <= synths(server) <= 7
        proc.stdin.write("pl.stop\n")
        proc.stdin.flush()
        time.sleep(0.5)
        held = synths(server)
        time.sleep(0.5)
        assert synths(server) == held
        proc.stdin.write("stop\n")
        proc.stdin.flush()
        time.sleep(0.5)
        assert synths(server) == 0
    finally:
        out = proc.communicate(timeout=5)[0]
    assert "error" not in out, out


def test_events_release_their_gate(server, sapf, tmp_path):
    """Each note is released after dur * legato and frees itself, so synths do not pile up."""
    proc = interactive(GATED + "{:instrument 'gated :dur .1 :legato .5} pbind play\n", tmp_path)
    try:
        time.sleep(1.5)
        assert 2 <= synths(server) <= 6  # 15 started; each lasts about .35 s
        proc.stdin.write("stop\n")
        proc.stdin.flush()
        time.sleep(0.5)
        assert synths(server) == 0
    finally:
        out = proc.communicate(timeout=5)[0]
    assert "error" not in out, out


def test_ndef_crossfades_and_stops(server, sapf, tmp_path):
    """The old synth frees itself after its fade; nd.stop fades the last one out; after stop, ndef starts anew."""
    proc = interactive("300 0 sinosc 0 * 'drone ndef = d\n", tmp_path)
    send = lambda line: (proc.stdin.write(line + "\n"), proc.stdin.flush())
    try:
        time.sleep(.5)
        assert synths(server) == 1
        send("{:fadeTime 1} d.set  400 0 sinosc 0 * 'drone ndef")
        time.sleep(.6)
        assert synths(server) == 2
        time.sleep(1)
        assert synths(server) == 1
        send("d.stop")
        time.sleep(.5)
        assert synths(server) == 1  # fading out over 1 s
        time.sleep(1)
        assert synths(server) == 0
        send("500 0 sinosc 0 * 'drone ndef  stop  600 0 sinosc 0 * 'drone ndef")
        time.sleep(.5)
        assert synths(server) == 1
        send("stop")
        time.sleep(.3)
        assert synths(server) == 0
    finally:
        out = proc.communicate(timeout=5)[0]
    assert "error" not in out, out
