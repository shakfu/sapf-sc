"""The env namespace and the Env type: SCgf parity with nanosynth's Envelope through EnvGen, and
IEnvGen's interpolation format checked by decoding (nanosynth has no IEnvGen)."""

import pytest

from conftest import decode_scgf, run_sapf
from nanosynth import SynthDefBuilder
from nanosynth.envelopes import EnvGen, Envelope
from nanosynth.ugens import K2A, Out, SinOsc


def build(fn):
    with SynthDefBuilder() as b:
        Out.ar(bus=b.add_parameter(name="out", value=0), source=fn())
    return b.build(name="t").compile()


@pytest.mark.parametrize("expr,env", [
    (".01 1 env.perc", lambda: Envelope.percussive(attack_time=0.01, release_time=1)),
    (".01 .2 .6 1.5 env.adsr", lambda: Envelope.adsr(attack_time=0.01, decay_time=0.2, sustain=0.6, release_time=1.5)),
    (".05 .5 env.asr", lambda: Envelope.asr(attack_time=0.05, release_time=0.5)),
    # nanosynth's Envelope.linen reads curve=1 as curvature 1; SC's Env.linen is \\lin, shape 1
    (".01 .5 .25 env.linen",
     lambda: Envelope(amplitudes=[0, 1, 1, 0], durations=[0.01, 0.5, 0.25], curves=["linear"])),
    ("[0 1 .5 0] [.1 .2 .3] ['exp 'sin 3] env.new",
     lambda: Envelope(amplitudes=[0, 1, 0.5, 0], durations=[0.1, 0.2, 0.3], curves=["exponential", "sine", 3])),
    ("[0 1 0] [.1 .2] 'lin env.new", lambda: Envelope(amplitudes=[0, 1, 0], durations=[0.1, 0.2], curves=["linear"])),
], ids=["perc", "adsr", "asr", "linen", "new", "new-one-curve"])
def test_envgen(compile_sapf, expr, env):
    sapf = compile_sapf(f"{expr} .5 ug.envgen")
    assert sapf == build(lambda: EnvGen.ar(envelope=env(), gate=0.5))


@pytest.mark.parametrize("methods", [".kr.done", ".done.kr"])
def test_envgen_kr_done(compile_sapf, methods):
    sapf = compile_sapf(f".002 .4 env.perc 1 ug.envgen{methods}")
    expect = build(lambda: K2A.ar(source=EnvGen.kr(envelope=Envelope.percussive(attack_time=0.002, release_time=0.4),
                                                   done_action=2)))
    assert sapf == expect


def test_envelope_times_from_a_ugen(compile_sapf):
    sapf = compile_sapf(".01 300 0 sinosc env.perc 1 ug.envgen")
    expect = build(lambda: EnvGen.ar(envelope=Envelope.percussive(attack_time=0.01,
                                                                  release_time=SinOsc.ar(frequency=300))))
    assert sapf == expect


def test_ienvgen_interpolation_format(compile_sapf):
    """SC's asArrayForInterpolation: offset, level0, segments, total time, then time shape curve level."""
    ugens = decode_scgf(compile_sapf("[0 1 .5] [.1 .3] ['lin -2] env.new .2 ug.ienvgen"))["ugens"]
    (u,) = [u for u in ugens if u[0] == "IEnvGen"]
    assert [v for _, v in u[3]] == pytest.approx([.2, 0, 0, 2, .4, .1, 1, 0, 1, .3, 5, -2, .5])


def test_raw_envelope_list(compile_sapf):
    """A list in SC's envelope format passes through, as sclang allows."""
    assert compile_sapf("[0 2 -99 -99 1 .1 1 0 0 .2 1 0] .5 ug.envgen") == \
        compile_sapf("[0 1 0] [.1 .2] 'lin env.new .5 ug.envgen")


def test_multichannel_levels(compile_sapf):
    ugens = decode_scgf(compile_sapf("[0 [1 .5] 0] [.1 .2] 'lin env.new 1 ug.envgen"))["ugens"]
    peaks = [[v for _, v in u[3]][9] for u in ugens if u[0] == "EnvGen"]
    assert peaks == [1, .5]


@pytest.mark.parametrize("expr,message", [
    ("[0 1] 1 'foo env.new", "env.new : curves : no envelope shape named 'foo"),
    (".01 1 env.perc ug.sinosc", "sinosc : freq"),
    ("[0] 1 'lin env.new", "env.new : needs 2 or more levels"),
])
def test_errors(sapf_bin, tmp_path, expr, message):
    out = run_sapf(f"{expr} pr cr", tmp_path)
    assert message in out
    assert "error" in out


def test_print(sapf_bin, tmp_path):
    out = run_sapf(".01 .2 .6 1 env.adsr pr cr  env pr cr", tmp_path)
    assert "Env([0 1 0.6 0] [0.01 0.2 1] [-4])" in out
    assert "Namespace env (5 words)" in out
