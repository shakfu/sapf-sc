"""pbind and event streams, through score: the NRT command file holds each event's messages at its
time, so timing, pitch and releases are checked without a server. Clocks and pdef are checked
against a recorder that answers /sync and keeps each bundle's time tag. play is in test_play.py."""

import os
import socket
import struct
import subprocess
import threading
import time

import pytest
from nanosynth.osc import OscMessage

import numpy as np

from conftest import SAPF, decode_scgf, read_wav_float, run_sapf

SR = 96000  # sapf's default rate; run_sapf passes no -r

PING = "def ping freq:440 amp:.5 gate:1 = [freq ug.sinosc .01 .1 env.asr gate ug.envgen.kr.done * amp *]\n"
DRUM = "def drum amp:.5 = [.001 .1 env.perc 1 ug.envgen.kr.done ug.whitenoise * amp *]\n"


def unbundle(bundle):
    """(seconds, address, contents) for each message in an OSC bundle; seconds from its time tag."""
    t, i, out = struct.unpack(">Q", bundle[8:16])[0] / 2 ** 32, 16, []
    while i < len(bundle):
        m = struct.unpack(">I", bundle[i:i + 4])[0]
        msg = OscMessage.from_datagram(bundle[i + 4:i + 4 + m])
        out.append((t, msg.address, list(msg.contents)))
        i += 4 + m
    return out


def scgf_blobs(data):
    """The SCgf defs in OSC data, raw: nanosynth decodes an SCgf blob as a message."""
    out, i = [], data.find(b"SCgf")
    while i >= 0:
        out.append(data[i:i + struct.unpack(">I", data[i - 4:i])[0]])
        i = data.find(b"SCgf", i + 4)
    return out


def messages(path):
    """(seconds, address, contents) for each message in an NRT command file."""
    data, pos, out = path.read_bytes(), 0, []
    while pos < len(data):
        n = struct.unpack(">I", data[pos:pos + 4])[0]
        out += unbundle(data[pos + 4:pos + 4 + n])
        pos += 4 + n
    return out


def score(code, tmp_path, seconds=4):
    out = run_sapf(f'{code} {seconds} "s" score', tmp_path)
    assert (tmp_path / "s.osc").exists(), out
    return messages(tmp_path / "s.osc")


def news(msgs):
    """(seconds, controls) of each /s_new."""
    return [(t, dict(zip(c[4::2], c[5::2]))) for t, a, c in msgs if a == "/s_new"]


def midicps(n):
    return 440 * 2 ** ((n - 69) / 12)


def test_pbind_fields(sapf_bin, tmp_path):
    out = run_sapf("{:a [1 2 3] :b 5} pbind 3 take pr cr", tmp_path)
    assert "[{:a 1  :b 5} {:a 2  :b 5} {:a 3  :b 5}]" in out


def test_pbind_holds_scale(sapf_bin, tmp_path):
    out = run_sapf("{:degree [1 2] :scale [0 3 7]} pbind 2 take pr cr", tmp_path)
    assert "[{:degree 1  :scale [0 3 7]} {:degree 2  :scale [0 3 7]}]" in out


def test_times_and_pitch(sapf_bin, tmp_path):
    msgs = score(PING + "{:instrument 'ping :midinote [60 62 64] :dur [.25 .5 1]} pbind", tmp_path)
    assert [t for t, _ in news(msgs)] == pytest.approx([0, .25, .75])
    assert [c["freq"] for _, c in news(msgs)] == pytest.approx([midicps(60), midicps(62), midicps(64)], rel=1e-6)
    assert msgs[0][1] == "/d_recv"


def test_release_after_dur_times_legato(sapf_bin, tmp_path):
    msgs = score(PING + "{:instrument 'ping :midinote 60 :dur .5 :legato .5} pbind", tmp_path, 1)
    assert [t for t, a, _ in msgs if a == "/n_set"] == pytest.approx([.25, .75])


def test_no_release_without_gate(sapf_bin, tmp_path):
    msgs = score(DRUM + "{:instrument 'drum :dur .5} pbind", tmp_path, 1)
    assert [a for _, a, _ in msgs].count("/n_set") == 0
    assert [c for _, c in news(msgs)] == [{"amp": pytest.approx(.1)}] * 2


def test_defaults_and_given_controls(sapf_bin, tmp_path):
    """SC's default event: amp .1 when absent; keys that are not controls are not sent."""
    msgs = score(PING + "{:instrument 'ping :amp .3 :cutoff 900 :dur 1} pbind", tmp_path, 1)
    (_, c), = news(msgs)
    assert c == {"amp": pytest.approx(.3), "freq": pytest.approx(midicps(60), rel=1e-6)}


@pytest.mark.parametrize("fields,notes", [
    (":degree [0 1 2 7 -1]", [60, 62, 64, 72, 59]),
    (":degree [0 2] :scale 'minor", [60, 63]),
    (":degree 0 :octave 4 :root 2", [50]),
    (":degree 2.1", [65]),  # .1 is a semitone up, as in SC
    (":degree [1 2] :scale [0 3 7]", [63, 67]),
    (":freq 300", None),
])
def test_pitch_chain(sapf_bin, tmp_path, fields, notes):
    events = len(notes) if notes else 1  # constant fields repeat: one event per second
    msgs = score(PING + f"{{:instrument 'ping {fields} :dur 1}} pbind", tmp_path, events)
    freqs = [c["freq"] for _, c in news(msgs)]
    expect = [300] if notes is None else [midicps(n) for n in notes]
    assert freqs == pytest.approx(expect, rel=1e-6)


def test_rest_and_chord(sapf_bin, tmp_path):
    msgs = score(PING + "{:instrument 'ping :midinote [60 'rest [64 67]] :dur [.25 .5 1]} pbind", tmp_path, 2)
    assert [(t, c["freq"]) for t, c in news(msgs)] == [
        (0, pytest.approx(midicps(60), rel=1e-6)),
        (pytest.approx(.75), pytest.approx(midicps(64), rel=1e-6)),
        (pytest.approx(.75), pytest.approx(midicps(67), rel=1e-6))]


def test_infinite_stream_stops_at_seconds(sapf_bin, tmp_path):
    msgs = score(PING + "{:instrument 'ping :midinote [60 64] cyc :dur .25} pbind", tmp_path, 1)
    assert len(news(msgs)) == 4
    assert msgs[-1][:2] == (1.0, "/c_set")


def test_instrument_as_a_def(sapf_bin, tmp_path):
    msgs = score(PING + "{:instrument `ping :dur 1} pbind", tmp_path, 1)
    assert len(news(msgs)) == 1


TONE_FN = "\\freq amp sustain [freq 0 sinosc amp * sustain T]"


def test_function_instrument_shares_alike_defs(sapf_bin, tmp_path):
    """Each note compiles; the two 60s compile alike and share a def, so 3 defs serve 4 synths."""
    msgs = score(f"{{:instrument {TONE_FN} :midinote [60 [64 67] 60] :dur .5}} pbind", tmp_path, 1.5)
    recvs = [c for t, a, c in msgs if a == "/d_recv"]
    starts = [(t, c[0]) for t, a, c in msgs if a == "/s_new"]
    assert len(recvs) == 3 and msgs[0][0] == 0
    assert [t for t, _ in starts] == pytest.approx([0, .5, .5, 1])
    assert starts[0][1] == starts[3][1] and len({n for _, n in starts}) == 3


def test_function_instrument_arguments(sapf_bin, tmp_path):
    """freq from the pitch chain, amp .1 by default, sustain dur * legato, other names from fields."""
    out = run_sapf("{:instrument \\freq amp sustain dur pan [[freq amp sustain dur pan] pr cr  freq 0 sinosc .1 T] "
                   ":degree 1 :dur 2 :pan -1} pbind 1 \"s\" score", tmp_path)
    assert f"[{midicps(62):g} 0.1 1.6 2 -1]" in out, out


def test_function_instrument_out(sapf_bin, tmp_path):
    """:out sets the bus the synth writes to; a Bus is its index."""
    score(f"2 bus.audio = fx  {{:instrument {TONE_FN} :out [0 fx 3] :dur 1}} pbind", tmp_path, 3)
    defs = [decode_scgf(b) for b in scgf_blobs((tmp_path / "s.osc").read_bytes())]
    outs = [[v for k, v in u[3][:1]] for d in defs for u in d["ugens"] if u[0] == "Out"]
    assert outs == [[0], [958], [3]]


def test_render_function_instrument(sapf_bin, render):
    """The synth frees itself after sustain, a quarter second here."""
    wav = render(f"{{:instrument {TONE_FN} :midinote 69 :dur .5 :legato .5}} pbind", 1)[:, 0]
    sec = lambda a, b: wav[int(a * SR):int(b * SR)]
    assert np.abs(sec(.01, .24)).max() == pytest.approx(.1, abs=.005)
    assert np.abs(sec(.26, .5)).max() == 0
    assert np.abs(sec(.51, .74)).max() == pytest.approx(.1, abs=.005)


def test_play_function_instrument_loads_its_defs_first(sapf_bin, tmp_path, recorder):
    news = live(recorder, tmp_path, f"{{:instrument {TONE_FN} :midinote [60 64] cyc :dur .25}} pbind play\n", 1)
    assert len(recorder.defs) == 2
    assert len(news) >= 4
    assert gaps([t for _, t, _ in news]) == pytest.approx([.25] * (len(news) - 1), abs=1e-3)


def test_score_of_channels_is_nrt(sapf_bin, tmp_path):
    msgs = score("440 0 sinosc .1 *", tmp_path, 1)
    assert [a for _, a, _ in msgs] == ["/d_recv", "/s_new", "/c_set"]


@pytest.fixture
def render(tmp_path, monkeypatch):
    monkeypatch.setenv("SAPF_RECORDINGS", str(tmp_path))
    monkeypatch.delenv("SAPF_SCSYNTH", raising=False)

    def render_(code, seconds):
        out = run_sapf(f'{code} {seconds} "r" render', tmp_path)
        assert "error" not in out, out
        return read_wav_float(tmp_path / "r.wav")
    return render_


def test_render_events(sapf_bin, render):
    """Two notes .5 s apart, released at .25 s and silent .1 s later; events render to 2 channels."""
    wav = render(PING + "{:instrument 'ping :midinote 69 :dur .5 :legato .5} pbind", 1)
    assert wav.shape == (SR, 2)
    left, right = wav[:, 0], wav[:, 1]
    sec = lambda a, b: left[int(a * SR):int(b * SR)]
    assert np.abs(sec(.02, .25)).max() == pytest.approx(.1, abs=.005)  # amp .1, SC's default
    assert np.abs(sec(.37, .5)).max() < 1e-4
    assert np.abs(sec(.52, .75)).max() == pytest.approx(.1, abs=.005)
    assert np.abs(right).max() == 0


def test_render_channels_matches_sf(sapf_bin, render, tmp_path):
    chans = f"[300 0 sinosc .1 * {SR // 2} N  400 0 sinosc .1 * {SR // 2} N]"
    wav = render(chans, .5)
    assert wav.shape == (SR // 2, 2)
    out = run_sapf(f'{chans} "s" >sf', tmp_path)
    assert "error" not in out, out
    assert np.array_equal(wav, read_wav_float(tmp_path / "s.wav"))


@pytest.mark.parametrize("code,message", [
    ("{:freq 300 0 sinosc} pbind", "pbind : freq is a UGen"),
    ("{:dur 1} pbind 1 \"s\" score", "event : needs an :instrument"),
    ("{:instrument 'nope :dur 1} pbind 1 \"s\" score", "event : no def named nope"),
    (PING + "{:instrument 'ping :degree 0 :scale 'nope} pbind 1 \"s\" score", "event : no scale named 'nope"),
    ("{:dur 1} pbind 0 \"s\" render", "render : seconds : above 0"),
    ("{:instrument \\x [x 0 sinosc] :dur 1} pbind 1 \"s\" score", "the instrument's argument x has no field"),
    ("{:instrument \\freq [freq] :dur 1} pbind 1 \"s\" score", "event : instrument's result"),
])
def test_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out and "error" in out, out


# clocks and pdef, against a recorder

NTP_OFFSET = 2208988800


class Recorder:
    """A UDP endpoint standing in for scsynth: answers /sync, keeps (arrival, time tag, address, args) of
    each bundled message, and the other messages in plain."""

    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("127.0.0.1", 0))
        self.sock.settimeout(.1)
        self.port = self.sock.getsockname()[1]
        self.bundles, self.plain, self.defs, self.done = [], [], [], False
        self.thread = threading.Thread(target=self.serve, daemon=True)
        self.thread.start()

    def serve(self):
        while not self.done:
            try:
                data, addr = self.sock.recvfrom(65536)
            except socket.timeout:
                continue
            now = time.time()
            if data.startswith(b"#bundle"):
                self.bundles += [(now, t - NTP_OFFSET, a, c) for t, a, c in unbundle(data)]
            elif (msg := OscMessage.from_datagram(data)).address == "/sync":
                self.sock.sendto(OscMessage("/synced", 7301).to_datagram(), addr)
            else:
                self.plain.append((msg.address, list(msg.contents)))
                if msg.address == "/d_recv":
                    self.defs += scgf_blobs(data)

    def news(self):
        return [(arrived, t, dict(zip(c[4::2], c[5::2]))) for arrived, t, a, c in self.bundles if a == "/s_new"]

    def close(self):
        self.done = True
        self.thread.join()
        self.sock.close()


@pytest.fixture
def recorder():
    r = Recorder()
    yield r
    r.close()


def live(recorder, tmp_path, code, *steps):
    """Runs code in sapf -i against the recorder; steps are seconds to wait or lines to send."""
    src = tmp_path / "live.sapf"
    src.write_text(code)
    env = dict(os.environ, SAPF_SCSYNTH=f"127.0.0.1:{recorder.port}", SAPF_PRELUDE="/dev/null")
    proc = subprocess.Popen([str(SAPF), "-q", "-i", str(src)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, env=env)
    try:
        for step in steps:
            if isinstance(step, str):
                proc.stdin.write(step + "\n")
                proc.stdin.flush()
            else:
                time.sleep(step)
        proc.stdin.write("stop\n")
    finally:
        out = proc.communicate(timeout=5)[0]
    assert "error" not in out, out
    return recorder.news()


def gaps(times):
    return [b - a for a, b in zip(times, times[1:])]


def test_play_sends_each_event_ahead_of_its_time(sapf_bin, tmp_path, recorder):
    """dur 1 at 1 beat per second; each bundle arrives about .2 s before its time tag."""
    news = live(recorder, tmp_path, PING + "{:instrument 'ping :dur 1} pbind play\n", 2.5)
    assert len(news) == 3
    assert gaps([t for _, t, _ in news]) == pytest.approx([1, 1], abs=1e-3)
    assert all(0 < t - arrived < .3 for arrived, t, _ in news)


def test_clock_tempo(sapf_bin, tmp_path, recorder):
    news = live(recorder, tmp_path, PING + "4 clock.new = c\n{:instrument 'ping :dur 1} pbind c.play\n", 1.1)
    assert 4 <= len(news) <= 6
    assert gaps([t for _, t, _ in news]) == pytest.approx([.25] * (len(news) - 1), abs=1e-3)


def test_clock_set_tempo_while_playing(sapf_bin, tmp_path, recorder):
    """Releases are timed in beats, so they follow the tempo too."""
    news = live(recorder, tmp_path, PING + "2 clock.new = c\n{:instrument 'ping :dur 1} pbind c.play\n",
                1.2, "{:tempo 8} c.set", 1)
    ts = [t for _, t, _ in news]
    assert gaps(ts)[:2] == pytest.approx([.5, .5], abs=1e-3)
    assert gaps(ts)[-3:] == pytest.approx([.125] * 3, abs=1e-3)
    sets = [t for _, t, a, _ in recorder.bundles if a == "/n_set"]
    assert sets[-1] - ts[-1] == pytest.approx(.8 / 8, abs=1e-3)


def test_pdef_replaces_at_the_next_beat(sapf_bin, tmp_path, recorder):
    code = PING + "{:instrument 'ping :midinote 60 :dur .25} pbind 'mel pdef = p\n"
    news = live(recorder, tmp_path, code, 1.3,
                "{:instrument 'ping :midinote 72 :dur .5} pbind 'mel pdef", 1.6)
    low = [t for _, t, c in news if c["freq"] < 300]
    high = [t for _, t, c in news if c["freq"] > 300]
    assert len(low) >= 4 and len(high) >= 2
    assert max(low) < min(high)
    # both start on whole beats of the default clock, so they are whole seconds apart
    apart = min(high) - min(low)
    assert apart == pytest.approx(round(apart), abs=1e-3)
    assert min(high) - max(low) == pytest.approx(.25, abs=1e-3)
    assert gaps(high) == pytest.approx([.5] * (len(high) - 1), abs=1e-3)


def test_pdef_player_stops_the_current_stream(sapf_bin, tmp_path, recorder):
    code = PING + "{:instrument 'ping :dur .25} pbind 'mel pdef = p\n"
    news = live(recorder, tmp_path, code, 1.2, "{:instrument 'ping :dur .25} pbind 'mel pdef", 1.2, "p.stop", .3)
    before = len(news)
    time.sleep(.6)
    assert len(recorder.news()) == before


def test_ndef_def_is_sc_proxy_fade(sapf_bin, tmp_path, recorder, monkeypatch):
    """SC's ProxySynthDef: channels times EnvGate(i_level: 0, curve: 'sin'), released by gate."""
    monkeypatch.setenv("SAPF_SCSYNTH", f"127.0.0.1:{recorder.port}")
    out = run_sapf("[300 400] 0 sinosc .1 * 'drone ndef pr cr", tmp_path)
    assert "error" not in out and "Ndef drone (node" in out, out
    time.sleep(.1)
    (blob,) = recorder.defs
    d = decode_scgf(blob)
    assert d["name"] == "sapf-ndef-drone-1"
    assert d["params"] == {"gate": 0, "fadeTime": 1, "out": 2}
    assert d["defaults"] == pytest.approx([1, .02, 0])
    (env,) = [u for u in d["ugens"] if u[0] == "EnvGen"]
    consts = [v for k, v in env[3] if k == "c"]
    # levelScale 1, levelBias 0, doneAction 2; then Env([0, 1, 0], [1, 1], 'sin', releaseNode: 1)
    assert consts == pytest.approx([1, 0, 2, 0, 2, 1, -99, 1, 1, 3, 0, 0, 1, 3, 0])
    names = [u[0] for u in d["ugens"]]
    (out,) = [u for u in d["ugens"] if u[0] == "Out"]
    for src, _ in out[3][1:]:  # each channel is a product with the EnvGen
        assert d["ugens"][src][0] == "BinaryOpUGen" and (names.index("EnvGen"), 0) in d["ugens"][src][3]


def test_ndef_replaces_with_a_crossfade(sapf_bin, tmp_path, recorder):
    """The new synth starts in the bundle that releases the old one; set changes the fade of both."""
    live(recorder, tmp_path, "300 0 sinosc .1 * 'drone ndef = d\n", .3,
         "{:fadeTime 2} d.set", "400 0 sinosc .1 * 'drone ndef", .3, "d.stop", .2)
    starts = [(t, c) for _, t, a, c in recorder.bundles if a == "/s_new"]
    assert [c[0] for _, c in starts] == ["sapf-ndef-drone-1", "sapf-ndef-drone-2"]
    first, second = starts[0][1][1], starts[1][1][1]
    assert starts[0][1][4:] == ["fadeTime", pytest.approx(.02)]
    assert starts[1][1][4:] == ["fadeTime", pytest.approx(2)]
    (released,) = [(t, c) for _, t, a, c in recorder.bundles if a == "/n_set"]
    assert released == (starts[1][0], [first, "gate", 0])
    assert (("/n_set", [first, "fadeTime", pytest.approx(2)]) in recorder.plain)
    assert ("/n_set", [second, "gate", 0]) in recorder.plain


@pytest.mark.parametrize("code,message", [
    ("0 clock.new", "clock.new : tempo : above 0"),
    ("1 clock.new = c {:speed 2} c.set", "set : a clock has no field speed"),
    ("1 clock.new = c 440 0 sinosc c.play", "play : events"),
    ("440 0 sinosc 'x pdef", "pdef : events"),
    ("{:dur 1} pbind 'x ndef", "ndef : channels"),
])
def test_clock_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out and "error" in out, out
