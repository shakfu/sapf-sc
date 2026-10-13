"""Every mapped word under kr: the def's UGens run at control rate where nanosynth says the UGen
supports it, scsynth loads and runs the def, and the output is finite and not silent. Fidelity at
control rate is not compared with sapf; block quantization would dominate the difference."""

import numpy as np
import pytest
import nanosynth.envelopes
import nanosynth.ugens

from conftest import decode_scgf, render

S = "100 0 sinosc"
S2 = "130 .2 sinosc"

# word -> (arguments, SC UGen the word builds)
WORDS = {
    "sinosc": ("30 0", "SinOsc"),
    "lfsaw": ("30 0", "LFSaw"),
    "lftri": ("30 0", "LFTri"),
    "lfpulse": ("30 0 .5", "LFPulse"),
    "lfpulseb": ("30 0 .5", "LFPulse"),
    "lfsquare": ("30 0", "LFPulse"),
    "impulse": ("30 0", "Impulse"),
    "white": (".5", "WhiteNoise"),
    "pink": (".5", "PinkNoise"),
    "brown": (".5", "BrownNoise"),
    "dust": ("300 .5", "Dust"),
    "dust2": ("300 .5", "Dust2"),
    "lfnoise0": ("30", "LFNoise0"),
    "lfnoise1": ("30", "LFNoise1"),
    "lag": (f"{S} .01", "Lag"),
    "lagud": (f"{S} .01 .02", "LagUD"),
    "lpf": (f"{S} 1000", "LPF"),
    "hpf": (f"{S} 10", "HPF"),
    "rlpf": (f"{S} 1000 .3", "RLPF"),
    "rhpf": (f"{S} 10 .3", "RHPF"),
    "bpf": (f"{S} 100 1", "BPF"),
    "bsf": (f"{S} 1000 1", "BRF"),
    "resonz": (f"{S} 100 .2", "Resonz"),
    "ringz": (f"{S} 100 .05", "Ringz"),
    "formlet": (f"{S} 100 .01 .05", "Formlet"),
    "leakdc": (f"{S} .995", "LeakDC"),
    "decay": (f"{S} .01", "Decay"),
    "decay2": (f"{S} .001 .01", "Decay2"),
    "delayn": (f"{S} .01 0", "DelayN"),
    "delayl": (f"{S} .01 0", "DelayL"),
    "delayc": (f"{S} .01 0", "DelayC"),
    "combn": (f"{S} .01 0 .1", "CombN"),
    "combl": (f"{S} .01 0 .1", "CombL"),
    "combc": (f"{S} .01 0 .1", "CombC"),
    "alpasn": (f"{S} .01 0 .1", "AllpassN"),
    "alpasl": (f"{S} .01 0 .1", "AllpassL"),
    "alpasc": (f"{S} .01 0 .1", "AllpassC"),
    "pan2": (f"{S} .3", "Pan2"),
    "bal2": (f"{S} {S2} .3", "Balance2"),
    "rot2": (f"{S} {S2} .3", "Rotate2"),
    "fade2": (f"{S} {S2} .3", "XFade2"),
    "sah": (f"{S} 30 0 impulse", "Latch"),
    "clip": (f"{S} -.5 .5", "Clip"),
    "wrap": (f"{S} -.5 .5", "Wrap"),
    "fold": (f"{S} -.5 .5", "Fold"),
    "fadein": (f"{S} .01", "EnvGen"),
    "fadeout": (f"{S} .05 .04", "EnvGen"),
}

ERRORS = ("exception", "ERROR", "FAILURE", "not installed")

# UGens nanosynth does not define; the render below is the only check of their control rate.
NOT_IN_NANOSYNTH = {"Resonz"}


def sc_class(name):
    return getattr(nanosynth.ugens, name, None) or getattr(nanosynth.envelopes, name, None)


@pytest.mark.parametrize("word", WORDS)
def test_word_at_control_rate(word, compile_sapf, tmp_path, capfd):
    args, ugen = WORDS[word]
    cls = sc_class(ugen)
    assert (cls is None) == (ugen in NOT_IN_NANOSYNTH)
    assert cls is None or hasattr(cls, "kr"), f"{ugen} has no control-rate form"
    defbytes = compile_sapf(f"{args} {word} kr")
    graph = decode_scgf(defbytes)
    assert [rate for name, rate, *_ in graph["ugens"] if name == ugen] == [1]
    # The inputs feed only control-rate UGens, so they are lowered too.
    assert all(rate != 2 for name, rate, *_ in graph["ugens"] if name not in ("K2A", "Out"))

    channels = 2 if word in ("pan2", "bal2", "rot2") else 1
    capfd.readouterr()
    out = render(defbytes, "t", 0.1, tmp_path, channels)
    log = capfd.readouterr().out
    assert not any(e in log for e in ERRORS), log
    assert np.all(np.isfinite(out)) and np.any(out != 0)


def test_soundin_kr_reads_audio_input(compile_sapf):
    names = [(n, r) for n, r, *_ in decode_scgf(compile_sapf("0 soundin kr"))["ugens"]]
    assert ("In", 2) in names and ("A2K", 1) in names


def test_control_rate_triggers_fire(compile_sapf, tmp_path, capfd):
    # Impulse.kr fires once per period for one control block; Decay.kr shapes each into a peak.
    out = render(compile_sapf("30 0 impulse kr .005 decay kr"), "t", 1.0, tmp_path)[:, 0]
    assert not any(e in capfd.readouterr().out for e in ERRORS)
    onsets = np.count_nonzero((out[1:] > 0.5) & (out[:-1] <= 0.5))
    assert 29 <= onsets <= 31, onsets


def test_scsynth_errors_are_seen(compile_sapf, tmp_path, capfd):
    # Guards the ERRORS checks above against scsynth output not reaching capfd.
    capfd.readouterr()
    render(compile_sapf("[1] 'NoSuchUGen 'kr 1 ugen"), "t", 0.01, tmp_path)
    assert any(e in capfd.readouterr().out for e in ERRORS)
