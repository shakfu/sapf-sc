"""The def statement and synthdef: SCgf parity with nanosynth, control kinds and order, and errors.
Synths and nodes need a server; they are tested in test_play.py."""

import json
import os
import subprocess

import pytest

from conftest import SAPF, decode_scgf, run_sapf
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
    sapf = write("synthdef beep amp:.1 freq:440 [freq sc.sinosc amp *]", tmp_path, "beep")
    with SynthDefBuilder() as b:
        amp = b.add_parameter(name="amp", value=0.1)
        freq = b.add_parameter(name="freq", value=440)
        out = b.add_parameter(name="out", value=0)
        Out.ar(bus=out, source=SinOsc.ar(frequency=freq) * amp)
    assert sapf == b.build(name="beep").compile()


def test_control_kinds(sapf_bin, tmp_path):
    """t_, i_ and a_ prefixes: TrigControl, Control.ir, AudioControl, grouped ir, tr, ar, kr as in SC."""
    sapf = write("synthdef ping freq:880 i_dur:.4 t_trig [freq sc.sinosc .002 i_dur env.perc t_trig sc.envgen.kr *]",
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
    d = decode_scgf(write("synthdef tone freq:440 amp:.1 [freq sc.sinosc amp *]", tmp_path, "tone"))
    assert d["params"] == {"freq": 0, "amp": 1, "out": 2}
    assert d["defaults"] == pytest.approx([440, 0.1, 0])


def test_unused_controls_are_kept(sapf_bin, tmp_path):
    d = decode_scgf(write("synthdef quiet freq:440 spare:3 [freq sc.sinosc]", tmp_path, "quiet"))
    assert d["params"] == {"freq": 0, "spare": 1, "out": 2}


def test_parameter_hides_a_word(sapf_bin, tmp_path):
    """A parameter named bus, then the UGen in reading it, as the verb example."""
    sapf = write("synthdef verb bus mix:.4 [bus 1 sc.in mix *]", tmp_path, "verb")
    with SynthDefBuilder() as b:
        bus = b.add_parameter(name="bus", value=0)
        mix = b.add_parameter(name="mix", value=0.4)
        out = b.add_parameter(name="out", value=0)
        Out.ar(bus=out, source=In.ar(bus=bus, channel_count=1) * mix)
    assert sapf == b.build(name="verb").compile()


def test_body_writes_its_own_out(sapf_bin, tmp_path):
    d = decode_scgf(write("synthdef own out [300 sc.sinosc out sc.out]", tmp_path, "own"))
    assert d["params"] == {"out": 0}
    assert [u[0] for u in d["ugens"]] == ["Control", "SinOsc", "Out"]


def test_body_leaving_only_a_root_adds_no_out(sapf_bin, tmp_path):
    d = decode_scgf(write("synthdef own2 [0 300 sc.sinosc sc.out]", tmp_path, "own2"))
    assert d["params"] == {}
    assert [u[0] for u in d["ugens"]] == ["SinOsc", "Out"]


@pytest.mark.parametrize("code, ugens", [
    ("[1 0 sc.localin = x  x .5 * sc.localout  x 300 sc.sinosc +]",
     ["LocalIn", "BinaryOpUGen", "LocalOut", "SinOsc", "BinaryOpUGen", "Out"]),
    ("[1 sc.impulse.kr 0 1 sc.sendtrig  300 sc.sinosc]", ["Impulse", "SendTrig", "SinOsc", "Out"]),
    ("[0 300 sc.sinosc sc.out]", ["SinOsc", "Out"]),
])
def test_play_list_keeps_roots(sapf_bin, tmp_path, code, ugens):
    """A UGen with no outputs left in a plain channel list is a root of the def, not dropped."""
    out = run_sapf(f'{code} "r" >sd', tmp_path)
    assert (tmp_path / "r.scsyndef").exists(), out
    assert [u[0] for u in decode_scgf((tmp_path / "r.scsyndef").read_bytes())["ugens"]] == ugens


def test_root_is_not_a_signal(sapf_bin, tmp_path):
    out = run_sapf("0 300 sc.sinosc sc.out = r  r pr cr  r .1 *", tmp_path)
    assert "<Out>" in out and "got Root" in out


def test_literal_defaults(sapf_bin, tmp_path):
    d = decode_scgf(write("synthdef lits a:-3 b:5/4 c:2pi d [a b c d + + +]", tmp_path, "lits"))
    assert d["defaults"] == pytest.approx([-3, 1.25, 6.2831853, 0, 0])


def test_synthdef_function(sapf_bin, tmp_path):
    a = write("\\freq amp [freq sc.sinosc amp *] {:freq 440 :amp .1} 'beep synthdef.new = beep", tmp_path, "beep")
    b = write("synthdef beep freq:440 amp:.1 [freq sc.sinosc amp *]", tmp_path, "beep")
    assert a == b


def test_def_print_and_warning(sapf_bin, tmp_path):
    out = run_sapf("synthdef neg x [x sc.sinosc]\n`neg pr cr", tmp_path)
    assert "warning: synthdef neg hides the built-in word neg" in out
    assert "SynthDef neg (x out)" in out


@pytest.mark.parametrize("code,message", [
    ("synthdef = [1]", "expected a name after synthdef"),
    ("synthdef f freq:x [freq]", "expected a number after ':' in synthdef"),
    ("synthdef f freq", "expected '[' to begin the body of synthdef"),
    ("synthdef f freq = [freq]", "synthdef has no '='"),
    ("synthdef f a [a sc.sinosc]\n5 f", "f : controls"),
    ("synthdef f a [a sc.sinosc]\n{:b 1} f", "f : no control named b"),
    ("synthdef f a [a sc.sinosc]\n{:addAction 'side} f", "f : addAction"),
    ("1 {} 'x synthdef.new", "synthdef.new : fun"),
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
    d = decode_scgf(write("synthdef fxin [2 bus.audio 2 sc.in]", tmp_path, "fxin"))
    (u,) = [u for u in d["ugens"] if u[0] == "In"]
    assert u[3] == [("c", 958.0)]


def test_bus_control_counts_down(sapf_bin, tmp_path):
    out = run_sapf("2 bus.control pr cr  1 bus.control pr cr", tmp_path)
    assert "Bus 16382 (2 control)" in out and "Bus 16381 (1 control)" in out


def test_freed_buses_are_reused(sapf_bin, tmp_path):
    """SC's Bus.free: the highest free run that fits, so a freed bus above a taken one comes back."""
    out = run_sapf("2 bus.audio = a  1 bus.audio = b  a.free  1 bus.audio pr cr  2 bus.audio pr cr  "
                   "1 bus.control = c  c.free  1 bus.control pr cr", tmp_path)
    assert "error" not in out, out
    assert "Bus 959 (1 audio)" in out and "Bus 955 (2 audio)" in out and "Bus 16383 (1 control)" in out


def test_private_buses_run_out_and_free(sapf_bin, tmp_path):
    """384 private buses, 576..959, below which ola keeps 64."""
    take = "  ".join(f"64 bus.audio = b{i}" for i in range(6))
    out = run_sapf(take + "  1 bus.audio", tmp_path)
    assert "bus.audio : no 1 free private buses" in out, out
    out = run_sapf(take + "  b1.free  64 bus.audio pr cr", tmp_path)
    assert "error" not in out, out
    assert "Bus 832 (64 audio)" in out


@pytest.mark.parametrize("code,message", [
    ("1 bus.audio = b  b.free  b.free", "free : bus 959 is already free"),
    ("0 bus.audio", "bus.audio : channels : from 1 to 64"),
    ("100 0 buffer.alloc", "buffer.alloc : channels : from 1 to 64"),
    ("1 bus.audio = b  .5 b.set", "set : bus 959 is an audio bus"),
    ("1 bus.control = b  [1 2] b.set", "set : 2 values for a bus of 1 channels"),
    ("{:x 1} group.new", "group.new : no field x"),
    ("{:addAction 'side} group.new", "group.new : addAction"),
    ("[1 2] send", "send : address"),
    ('["/n_free" [1]] send', "send : argument"),
])
def test_resource_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out and "error" in out


@pytest.mark.parametrize("names,where", [
    ({"SAPF_SYNTHDEFS": "old"}, "."),                       # the old name is not read; the current directory
    ({"SAPF_SYNTHDEFS": "old", "STAC_SYNTHDEFS": "new"}, "new"),
])
def test_environment_names(sapf_bin, tmp_path, names, where):
    for d in ("old", "new"):
        (tmp_path / d).mkdir()
    src = tmp_path / "in.stac"
    src.write_text("synthdef beep [440 sc.sinosc]\n`beep.write\n")
    env = {k: v for k, v in os.environ.items() if not k.startswith("STAC_")}
    env.update({k: str(tmp_path / v) for k, v in names.items()}, STAC_PRELUDE="/dev/null")
    subprocess.run([str(SAPF), "-q", str(src)], env=env, cwd=tmp_path, capture_output=True, timeout=30, check=True)
    assert (tmp_path / where / "beep.scsyndef").exists()


# def, synthdef and const (docs/dev/stac.md, Proposal: unified definitions)

def printed(out):
    return [line for line in out.split("compiled OK.\n")[-1].split("\n") if line and not line.startswith("done")]


def test_def_is_a_function(sapf_bin, tmp_path):
    out = run_sapf('def dbl x --> num\n    "doubles x."\n    [x 2 *]\n5 dbl pr cr  [1 2] dbl pr cr', tmp_path)
    assert printed(out)[:2] == ["10", "[2 4]"]  # num takes the lists automapping makes


@pytest.mark.parametrize("code,stack", [
    ("def f x --> num num [x x 1 +]\n5 f", 2),
    ("def f x --> [x pop]\n5 f", 0),
    ("def f x --> ... [x x x]\n5 f", 3),
    ("def f x [x x]\n5 f", 2),                # undeclared: not checked
])
def test_def_results(sapf_bin, tmp_path, code, stack):
    out = run_sapf(code + "  stackDepth pr cr", tmp_path)
    assert "error" not in out, out
    assert printed(out)[0] == str(stack)


@pytest.mark.parametrize("code,message", [
    ("def f x --> num [x x]\n5 f", "f : left 2 values; it declares 1"),
    ("def f x --> num ['a]\n5 f", "f : result 1 .  expected num. got String"),
    ("def f x --> sig str [x x]\n5 f", "f : result 2 .  expected str. got Real"),
    ("def f x --> wat [x]", "unknown result type wat"),
    ("def f x:3 [x]", "def takes no defaults"),
    ("def f x = [x]", "def has no '='"),
    ("synthdef s a --> num [a]", "only def declares results"),
    ("def f x --> num ... [x]", "'...' after '-->' stands alone"),
])
def test_def_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out and "error" in out, out


def test_definitions_generate_their_stack_effects(sapf_bin, tmp_path):
    src = tmp_path / "lib.stac"
    src.write_text('def lfo2 freq lo hi --> sig "an lfo." [freq lo hi +]\ndef raw x [x]\n'
                   'synthdef pad freq:220 "a pad." [freq sc.sinosc]\nconst half "one half." [1 2 /]\n')
    env = dict(os.environ, STAC_PRELUDE="/dev/null")
    words = json.loads(subprocess.run([str(SAPF), "--words", "-p", str(src)], capture_output=True, text=True,
                                      env=env, timeout=30, check=True).stdout)
    got = {w["name"]: (w["effect"], w["help"]) for w in words if w["source"] == "prelude"}
    assert got == {"lfo2": ("(freq lo hi --> sig)", "an lfo."), "raw": ("(x --> ?)", ""),
                   "pad": ("(controls --> node)", "a pad."), "half": ("(--> const)", "one half.")}


def test_const(sapf_bin, tmp_path):
    out = run_sapf('const hrt "sqrt(2)/2." [.5 sqrt]\n\\x [x hrt *] = scale\nhrt pr cr  2 scale pr cr', tmp_path)
    assert printed(out)[:2] == ["0.707107", "1.41421"]


@pytest.mark.parametrize("code,message", [
    ("const k [1]\n2 = k", "= k : k is a constant"),
    ("const k [1]\nconst k [2]", "= k : k is a constant"),
    ("const k [1 2]", "const : the body left 2 values; a constant is 1"),
    ("const k a [a]", "const takes no parameters"),
    ("\\x [const k [1]] = f", "const is allowed only at the top level"),
])
def test_const_errors(sapf_bin, tmp_path, code, message):
    out = run_sapf(code, tmp_path)
    assert message in out and "error" in out, out


def test_const_survives_lines_but_not_a_module_reload(sapf_bin, tmp_path):
    """A REPL line cannot rebind a constant; a module reload is a new version and may change it."""
    import time
    (tmp_path / "konst.stac").write_text("const k [1]\n")
    (tmp_path / "main.stac").write_text("import konst.*\n")
    env = dict(os.environ, STAC_PRELUDE="/dev/null", STAC_LOG=str(tmp_path / "log"))
    proc = subprocess.Popen([str(SAPF), "-q", "-i", "main.stac"], cwd=tmp_path, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
    time.sleep(1.1)
    (tmp_path / "konst.stac").write_text("const k [2]\n")
    proc.stdin.write("import konst\nclear konst.k pr cr\nquit\n")
    out = proc.communicate(timeout=5)[0]
    assert "error" not in out, out
    assert "\n2\n" in out
