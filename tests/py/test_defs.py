"""The def statement and synthdef: SCgf parity with nanosynth, control kinds and order, and errors.
Synths and nodes need a server; they are tested in test_play.py."""

import pytest

from conftest import decode_scgf, run_sapf
from nanosynth import SynthDefBuilder
from nanosynth.enums import ParameterRate
from nanosynth.envelopes import EnvGen, Envelope
from nanosynth.ugens import In, Out, SinOsc


def write(code, tmp_path, name):
    out = run_sapf(f"{code}\n`{name}.write", tmp_path)
    path = tmp_path / f"{name}.scsyndef"
    assert path.exists(), out
    return path.read_bytes()


def test_def_matches_nanosynth(sapf_bin, tmp_path):
    # kr controls in alphabetical order: nanosynth sorts them by name, stac keeps declaration order
    sapf = write("def beep amp:.1 freq:440 = [freq ug.sinosc amp *]", tmp_path, "beep")
    with SynthDefBuilder() as b:
        amp = b.add_parameter(name="amp", value=0.1)
        freq = b.add_parameter(name="freq", value=440)
        out = b.add_parameter(name="out", value=0)
        Out.ar(bus=out, source=SinOsc.ar(frequency=freq) * amp)
    assert sapf == b.build(name="beep").compile()


def test_control_kinds(sapf_bin, tmp_path):
    """t_, i_ and a_ prefixes: TrigControl, Control.ir, AudioControl, grouped ir, tr, ar, kr as in SC."""
    sapf = write("def ping freq:880 i_dur:.4 t_trig = [freq ug.sinosc .002 i_dur env.perc t_trig ug.envgen.kr *]",
                 tmp_path, "ping")
    with SynthDefBuilder() as b:
        freq = b.add_parameter(name="freq", value=880)
        dur = b.add_parameter(name="i_dur", value=0.4, rate=ParameterRate.SCALAR)
        trig = b.add_parameter(name="t_trig", value=0, rate=ParameterRate.TRIGGER)
        out = b.add_parameter(name="out", value=0)
        env = EnvGen.kr(envelope=Envelope.percussive(attack_time=0.002, release_time=dur), gate=trig)
        Out.ar(bus=out, source=SinOsc.ar(frequency=freq) * env)
    assert sapf == b.build(name="ping").compile()


def test_declaration_order(sapf_bin, tmp_path):
    """sclang's slot order: declared order within a kind, out last."""
    d = decode_scgf(write("def tone freq:440 amp:.1 = [freq ug.sinosc amp *]", tmp_path, "tone"))
    assert d["params"] == {"freq": 0, "amp": 1, "out": 2}
    assert d["defaults"] == pytest.approx([440, 0.1, 0])


def test_unused_controls_are_kept(sapf_bin, tmp_path):
    d = decode_scgf(write("def quiet freq:440 spare:3 = [freq ug.sinosc]", tmp_path, "quiet"))
    assert d["params"] == {"freq": 0, "spare": 1, "out": 2}


def test_parameter_hides_a_word(sapf_bin, tmp_path):
    """A parameter named bus, then the UGen in reading it, as the verb example."""
    sapf = write("def verb bus mix:.4 = [bus 1 ug.in mix *]", tmp_path, "verb")
    with SynthDefBuilder() as b:
        bus = b.add_parameter(name="bus", value=0)
        mix = b.add_parameter(name="mix", value=0.4)
        out = b.add_parameter(name="out", value=0)
        Out.ar(bus=out, source=In.ar(bus=bus, channel_count=1) * mix)
    assert sapf == b.build(name="verb").compile()


def test_body_writes_its_own_out(sapf_bin, tmp_path):
    d = decode_scgf(write("def own out = [300 ug.sinosc out ug.out]", tmp_path, "own"))
    assert d["params"] == {"out": 0}
    assert [u[0] for u in d["ugens"]] == ["Control", "SinOsc", "Out"]


def test_literal_defaults(sapf_bin, tmp_path):
    d = decode_scgf(write("def lits a:-3 b:5/4 c:2pi d = [a b c d + + +]", tmp_path, "lits"))
    assert d["defaults"] == pytest.approx([-3, 1.25, 6.2831853, 0, 0])


def test_synthdef_function(sapf_bin, tmp_path):
    a = write("\\freq amp [freq ug.sinosc amp *] {:freq 440 :amp .1} 'beep synthdef = beep", tmp_path, "beep")
    b = write("def beep freq:440 amp:.1 = [freq ug.sinosc amp *]", tmp_path, "beep")
    assert a == b


def test_def_print_and_warning(sapf_bin, tmp_path):
    out = run_sapf("def neg x = [x ug.sinosc]\n`neg pr cr", tmp_path)
    assert "warning: def neg hides the built-in word neg" in out
    assert "Def neg (x out)" in out


@pytest.mark.parametrize("code,message", [
    ("def = [1]", "expected a name after def"),
    ("def f freq:x = [freq]", "expected a number after ':' in def"),
    ("def f freq", "expected '=' after the parameters of def"),
    ("def f a = [a ug.sinosc]\n5 f", "f : controls"),
    ("def f a = [a ug.sinosc]\n{:b 1} f", "f : no control named b"),
    ("def f a = [a ug.sinosc]\n{:addAction 'side} f", "f : addAction"),
    ("1 {} 'x synthdef", "synthdef : fun"),
])
def test_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out
    assert "error" in out


# buses and buffers without a server

def test_bus_audio_counts_down(sapf_bin, tmp_path):
    out = run_sapf("2 bus.audio pr cr  1 bus.audio pr cr", tmp_path)
    assert "Bus 958 (2 audio)" in out and "Bus 957 (1 audio)" in out


def test_bus_is_its_index_as_an_input(sapf_bin, tmp_path):
    d = decode_scgf(write("def fxin = [2 bus.audio 2 ug.in]", tmp_path, "fxin"))
    (u,) = [u for u in d["ugens"] if u[0] == "In"]
    assert u[3] == [("c", 958.0)]


def test_bus_control_counts_down(sapf_bin, tmp_path):
    out = run_sapf("2 bus.control pr cr  1 bus.control pr cr", tmp_path)
    assert "Bus 16382 (2 control)" in out and "Bus 16381 (1 control)" in out


@pytest.mark.parametrize("code,message", [
    ("0 bus.audio", "bus.audio : channels : from 1 to 64"),
    ("100 0 buffer.alloc", "buffer.alloc : channels : from 1 to 64"),
    ("1 bus.audio = b  .5 b.set", "set : bus 959 is an audio bus"),
    ("1 bus.control = b  [1 2] b.set", "set : 2 values for a bus of 1 channels"),
    ("{:x 1} node.group", "node.group : no field x"),
    ("{:addAction 'side} node.group", "node.group : addAction"),
    ("[1 2] send", "send : address"),
    ('["/n_free" [1]] send', "send : argument"),
])
def test_resource_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out and "error" in out
