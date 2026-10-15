"""stac's generated UGen words (phase 2 slice: the demand namespace). SCgf must equal nanosynth's
for the same graph; classes nanosynth lacks are checked by decoding."""

import math
import subprocess
import sys

import pytest

from conftest import ROOT, decode_scgf, run_sapf
from nanosynth import SynthDefBuilder
from nanosynth.ugens import (BufCombC, BufFrames, BufRd, BufWr, Dbrown, Dbufrd, Demand, DemandEnvGen, Dgeom, Dseq,
                             Dseries, DecodeB2, Duty, Dwhite, Dwrand, FFT, GrainBuf, IFFT, Impulse, K2A, Klank, LocalIn, MFCC, Out, PlayBuf, Poll, RecordBuf, SinOsc,
                             Warp1)

INF = "1 0 /"  # sapf-sc has no inf word without the prelude


def build(fn):
    with SynthDefBuilder() as b:
        Out.ar(bus=b.add_parameter(name="out", value=0), source=fn())
    return b.build(name="t").compile()


def test_table_is_fresh():
    r = subprocess.run([sys.executable, str(ROOT / "scripts" / "gen_ugens.py"), "--check"], capture_output=True,
                       text=True)
    assert r.returncode == 0, r.stderr


def test_dseq_in_demand(compile_sapf):
    sapf = compile_sapf(f"8 0 impulse 0 [1 2 3] {INF} demand.dseq demand.demand")
    expect = build(lambda: Demand.ar(trigger=Impulse.ar(frequency=8), reset=0,
                                     source=Dseq.dr(repeats=math.inf, sequence=[1, 2, 3])))
    assert sapf == expect


def test_keyword_call(compile_sapf):
    sapf = compile_sapf("{:trig 8 0 impulse :reset 0 :demandUGens {:list [1 2] :repeats 3} demand.dseq} demand.demand")
    expect = build(lambda: Demand.ar(trigger=Impulse.ar(frequency=8), reset=0,
                                     source=Dseq.dr(repeats=3, sequence=[1, 2])))
    assert sapf == expect


def test_two_sources_give_two_outputs(compile_sapf):
    sapf = compile_sapf(f"8 0 impulse 0 [[1 2] {INF} demand.dseq  2 5 {INF} demand.dwhite] demand.demand")
    expect = build(lambda: Demand.ar(trigger=Impulse.ar(frequency=8), reset=0, source=[
        Dseq.dr(repeats=math.inf, sequence=[1, 2]), Dwhite.dr(length=math.inf, minimum=2, maximum=5)]))
    assert sapf == expect


@pytest.mark.parametrize("expr,ugen", [
    ("2 5 7 demand.dwhite", lambda: Dwhite.dr(length=7, minimum=2, maximum=5)),
    ("2 5 .5 7 demand.dbrown", lambda: Dbrown.dr(length=7, minimum=2, maximum=5, step=0.5)),
    ("3 2 7 demand.dgeom", lambda: Dgeom.dr(length=7, start=3, grow=2)),
    ("3 2 7 demand.dseries", lambda: Dseries.dr(length=7, start=3, step=2)),
    ("[4 5 6] [.5 .5] 3 demand.dwrand", lambda: Dwrand.dr(repeats=3, sequence=[4, 5, 6], weights=[0.5, 0.5, 0])),
], ids=["dwhite", "dbrown", "dgeom", "dseries", "dwrand-pads-weights"])
def test_wire_order(compile_sapf, expr, ugen):
    """Each argument gets a distinct value, so a swapped wire slot changes the bytes."""
    sapf = compile_sapf(f"8 0 impulse 0 {expr} demand.demand")
    assert sapf == build(lambda: Demand.ar(trigger=Impulse.ar(frequency=8), reset=0, source=ugen()))


def test_duty_wire_order_and_done(compile_sapf):
    sapf = compile_sapf(f".25 1 [1 2] {INF} demand.dseq demand.duty.done")
    expect = build(lambda: Duty.ar(duration=0.25, reset=1, done_action=2,
                                   level=Dseq.dr(repeats=math.inf, sequence=[1, 2])))
    assert sapf == expect


def test_tail_parameters_take_defaults(compile_sapf):
    sapf = compile_sapf(f"[1 2] {INF} demand.dseq [.1 .2] {INF} demand.dseq 1 0 1 demand.demandenvgen")
    expect = build(lambda: DemandEnvGen.ar(level=Dseq.dr(repeats=math.inf, sequence=[1, 2]),
                                           duration=Dseq.dr(repeats=math.inf, sequence=[0.1, 0.2]),
                                           shape=1, curve=0, gate=1))
    assert sapf == expect


def test_kr(compile_sapf):
    ugens = decode_scgf(compile_sapf(f"8 0 impulse.kr 0 [1 2] {INF} demand.dseq demand.demand.kr"))["ugens"]
    assert [(u[0], u[1]) for u in ugens if u[0] in ("Impulse", "Demand")] == [("Impulse", 1), ("Demand", 1)]


@pytest.mark.parametrize("expr,cls,inputs", [
    (".25 1 3 demand.tduty", "TDuty", [.25, 1, 0, 3, 0]),
    ("{:dur .25 :level 3 :gapFirst 1} demand.tduty.done", "TDuty", [.25, 0, 2, 3, 1]),
    ("10 3 .01 demand.dconst", "Dconst", [10, 3, .01]),
    ("2 7 demand.ddup", "Ddup", [2, 7]),
    ("7 4 demand.dreset", "Dreset", [7, 4]),
    ("[4 5] 3 demand.dswitch1", "Dswitch1", [3, 4, 5]),
])
def test_classes_nanosynth_lacks(compile_sapf, expr, cls, inputs):
    """Wire order from SCClassLibrary/Common/Audio/Demand.sc."""
    ugens = decode_scgf(compile_sapf(f"8 0 impulse 0 {expr} demand.demand" if cls[0] == "D" else expr))["ugens"]
    (u,) = [u for u in ugens if u[0] == cls]
    assert [v for _, v in u[3]] == pytest.approx(inputs)


def test_multichannel_expansion(compile_sapf):
    ugens = decode_scgf(compile_sapf("8 0 impulse 0 [[1 2] 3] 2 demand.dseq demand.demand"))["ugens"]
    seqs = [[v for _, v in u[3]] for u in ugens if u[0] == "Dseq"]
    assert seqs == [[2, 1, 3], [2, 2, 3]]
    assert [len(u[4]) for u in ugens if u[0] == "Demand"] == [2]


def test_list_of_forms_expands(sapf_bin, tmp_path):
    assert "[Dwhite.dr Dwhite.dr]" in run_sapf("[{:lo 1} {:hi 9}] demand.dwhite pr cr", tmp_path)


def test_namespaces(sapf_bin, tmp_path):
    out = run_sapf("demand pr cr buffer pr cr ug pr cr ug,dseq pr cr demand,dseq pr cr", tmp_path)
    assert "Namespace demand (21 words)" in out
    assert "Namespace buffer (39 words)" in out
    assert "Namespace ug (374 words)" in out
    assert out.count("{Prim, dseq}") == 2


def test_help_lists_positional_arguments(sapf_bin, tmp_path):
    out = run_sapf("helpall", tmp_path)
    assert "demand.duty (dur reset level --> out) Duty. Keyword only: doneAction." in out
    assert "demand (--> namespace)" in out


@pytest.mark.parametrize("expr,message", [
    ("{:lo 1 :hie 2} demand.dwhite", "demand.dwhite : no argument named hie"),
    ("{:repeats 2} demand.dseq", "demand.dseq : list : required"),
    ("1 2 3 demand.dwhite.kr", "demand.dwhite has no kr"),
    ("1 2 3 demand.dwhite.done", "Dwhite has no doneAction"),
    ("demand.nope", "nope"),
    ("[] 5 7 demand.dwhite", "demand.dwhite : empty list"),
])
def test_errors(sapf_bin, tmp_path, expr, message):
    out = run_sapf(f"{expr} pr cr", tmp_path)
    assert message in out
    assert "error" in out


# buffer namespace

@pytest.mark.parametrize("expr,ugen", [
    ("2 3 .5 4 5 1 buffer.playbuf",
     lambda: PlayBuf.ar(channel_count=2, buffer_id=3, rate=0.5, trigger=4, start_position=5, loop=1)),
    ("{:numChannels 2 :bufnum 3 :rate .5} buffer.playbuf.done",
     lambda: PlayBuf.ar(channel_count=2, buffer_id=3, rate=0.5, done_action=2)),
    ("1 3 300 0 sinosc 0 4 buffer.bufrd",
     lambda: BufRd.ar(channel_count=1, buffer_id=3, phase=SinOsc.ar(frequency=300), loop=0, interpolation=4)),
    ("3 300 0 sinosc .25 .75 buffer.bufcombc",
     lambda: BufCombC.ar(buffer_id=3, source=SinOsc.ar(frequency=300), delay_time=0.25, decay_time=0.75)),
    ("[300 0 sinosc 400 0 sinosc] 3 5 .5 .25 7 0 buffer.recordbuf",
     lambda: RecordBuf.ar(source=[SinOsc.ar(frequency=300), SinOsc.ar(frequency=400)], buffer_id=3, offset=5,
                          record_level=0.5, preexisting_level=0.25, run=7, loop=0)),
    ("300 0 sinosc 3 7 0 buffer.bufwr", lambda: BufWr.ar(source=SinOsc.ar(frequency=300), buffer_id=3, phase=7,
                                                         loop=0)),
    ("2 3 .5 4 5 6 7 8 9 10 buffer.grainbuf",
     lambda: GrainBuf.ar(channel_count=2, trigger=3, duration=0.5, buffer_id=4, rate=5, position=6, interpolate=7,
                         pan=8, envelope_buffer_id=9, maximum_overlap=10)),
    ("2 3 .5 4 5 6 7 8 9 buffer.warp1",
     lambda: Warp1.ar(channel_count=2, buffer_id=3, pointer=0.5, frequency_scaling=4, window_size=5,
                      envelope_buffer_id=6, overlaps=7, window_rand_ratio=8, interpolation=9)),
    ("3 buffer.bufframes", lambda: K2A.ar(source=BufFrames.kr(buffer_id=3))),  # sapf-sc raises kr channels
], ids=["playbuf", "playbuf-done", "bufrd", "bufcombc", "recordbuf", "bufwr", "grainbuf", "warp1", "bufframes"])
def test_buffer(compile_sapf, expr, ugen):
    assert compile_sapf(expr) == build(ugen)


def test_dbufrd(compile_sapf):
    sapf = compile_sapf("8 0 impulse 0 3 7 0 buffer.dbufrd demand.demand")
    expect = build(lambda: Demand.ar(trigger=Impulse.ar(frequency=8), reset=0,
                                     source=Dbufrd.dr(buffer_id=3, phase=7, loop=0)))
    assert sapf == expect


@pytest.mark.parametrize("expr,first", [
    ("3 .5 .25 .75 buffer.bufcombc", ("DC", 2, [.5])),
    ("3 300 0 sinosc.kr .25 .75 buffer.bufcombc", ("K2A", 2, None)),
])
def test_ar_inputs_raised_to_audio_rate(compile_sapf, expr, first):
    """sclang's asAudioRateInput: a constant becomes DC.ar, a control-rate input K2A.ar."""
    ugens = decode_scgf(compile_sapf(expr))["ugens"]
    comb = next(u for u in ugens if u[0] == "BufCombC")
    raised = ugens[comb[3][1][0]]
    assert (raised[0], raised[1]) == first[:2]
    if first[2] is not None:
        assert [v for _, v in raised[3]] == first[2]


def test_kr_method_keeps_control_rate_inputs(compile_sapf):
    ugens = decode_scgf(compile_sapf("[3 .5 .25 buffer.bufdelayn.kr]"))["ugens"]
    assert [u[0] for u in ugens if u[0] in ("DC", "K2A")] == ["K2A"]  # K2A only for Out


def test_num_channels_must_be_whole(sapf_bin, tmp_path):
    out = run_sapf("{:numChannels 2.5 :bufnum 0} buffer.playbuf pr cr", tmp_path)
    assert "buffer.playbuf : numChannels" in out and "whole number" in out


def test_setbuf_sends_size_then_values(compile_sapf):
    ugens = decode_scgf(compile_sapf("[3 [4 5 6] 2 buffer.setbuf]"))["ugens"]
    (u,) = [u for u in ugens if u[0] == "SetBuf"]
    assert [v for _, v in u[3]] == [3, 2, 3, 4, 5, 6]


def test_empty_form_takes_every_default(sapf_bin, tmp_path):
    """{} once evaluated to a Form with no table and segfaulted (docs/dev/upstream-sapf-bugs.md)."""
    out = run_sapf("{} pr cr {} ug.whitenoise pr cr", tmp_path)
    assert "{}" in out and "WhiteNoise.ar" in out


# specification arrays

def test_klank(compile_sapf):
    sapf = compile_sapf("[[800 1200] [1 .5] [1 2]] 2 0 impulse ug.klank")
    expect = build(lambda: Klank.ar(source=Impulse.ar(frequency=2), frequencies=[800, 1200], amplitudes=[1, 0.5],
                                    decay_times=[1, 2]))
    assert sapf == expect


@pytest.mark.parametrize("expr,cls,inputs", [
    ("{:specificationsArrayRef [[800 1200] .5] :input 3 :freqscale 2 :decayscale 4} ug.klank", "Klank",
     [3, 2, 0, 4, 800, .5, 1, 1200, .5, 1]),
    ("[[800 1200]] ug.klang", "Klang", [1, 0, 800, 1, 0, 1200, 1, 0]),
    ("[[800 1200] [.25] [.5 .75]] ug.klang", "Klang", [1, 0, 800, .25, .5, 1200, .25, .75]),
])
def test_specification_rows(compile_sapf, expr, cls, inputs):
    """sclang's [freqs, amps, times].flop.flat: omitted rows take Klank's 1 and 1, Klang's 1 and 0;
    a real or a shorter row repeats."""
    ugens = decode_scgf(compile_sapf(expr))["ugens"]
    (u,) = [u for u in ugens if u[0] == cls]
    assert [v for _, v in u[3]] == pytest.approx(inputs)


def test_specification_rows_limit(sapf_bin, tmp_path):
    out = run_sapf("[1 2 3 4] 0 ug.klank pr cr", tmp_path)
    assert "klank : specificationsArrayRef : expected a list of 1 to 3 rows" in out


# classes the overlay or the generator's rules handle

CHAIN = "{:buffer 0 :in 300 0 sinosc} fft.fft"


def chain():
    return FFT.kr(buffer_id=0, source=SinOsc.ar(frequency=300))


def test_mfcc_outputs_follow_numcoeff(compile_sapf):
    """MFCC sends numcoeff and makes that many outputs."""
    sapf = compile_sapf(f"{{:chain {CHAIN} :numcoeff 4}} analysis.mfcc")
    assert sapf == build(lambda: K2A.ar(source=MFCC.kr(pv_chain=chain(), coeff_count=4)))


def test_ifft_drops_new(compile_sapf):
    """IFFT.new only calls IFFT.ar; the word builds at ar."""
    sapf = compile_sapf(f"{CHAIN} 1 2 fft.ifft")
    assert sapf == build(lambda: IFFT.ar(pv_chain=chain(), window_type=1, window_size=2))


def test_localin_wraps_default(compile_sapf):
    sapf = compile_sapf("{:numChannels 3 :default [.5 .25]} io.localin")
    assert sapf == build(lambda: LocalIn.ar(default=[0.5, 0.25, 0.5], channel_count=3))


def test_decodeb2_sends_orientation(compile_sapf):
    """sclang's DecodeB2 always sends 0.5; stac sends the argument."""
    sapf = compile_sapf("{:numChans 4 :w .1 :x .2 :y .3 :orientation .4} ambisonics.decodeb2")
    assert sapf == build(lambda: DecodeB2.ar(w=0.1, x=0.2, y=0.3, orientation=0.4, channel_count=4))


@pytest.mark.parametrize("expr,cls,slot,value", [
    (f"{{:chain {CHAIN}}} analysis.onsets", "Onsets", 2, 3),  # \\rcomplex
    ("{:freq 300} analysis.ampcomp", "AmpComp", 1, 261.6255653),  # 60.midicps
    ("{:busindex 0 :numfeatures 2} analysis.beattrack2", "BeatTrack2", 5, -2.1),  # weightingscheme ? (-2.1)
])
def test_expression_defaults(compile_sapf, expr, cls, slot, value):
    ugens = decode_scgf(compile_sapf(expr))["ugens"]
    (u,) = [u for u in ugens if u[0] == cls]
    assert u[3][slot][1] == pytest.approx(value)


# .ir and .poll

def test_ir(compile_sapf):
    sapf = compile_sapf("3 buffer.bufframes.ir 0 sinosc")
    with SynthDefBuilder() as b:
        Out.ar(bus=b.add_parameter(name="out", value=0), source=SinOsc.ar(frequency=BufFrames.ir(buffer_id=3)))
    assert sapf == b.build(name="t").compile()


def test_ir_of_a_ugen_also_used_at_its_own_rate(compile_sapf):
    """ir has no conversion from another rate, so the ir copy is a node of its own."""
    ugens = decode_scgf(compile_sapf("3 buffer.bufframes = f  [f 0 sinosc  f.ir 0 sinosc]"))["ugens"]
    assert sorted(u[1] for u in ugens if u[0] == "BufFrames") == [0, 1]


def test_poll_matches_sclang(compile_sapf):
    """sclang's label, UGen(SinOsc); nanosynth's default is SinOsc, so it is given here."""
    sapf = compile_sapf("2 ug.sinosc.kr.poll 0 sinosc")
    with SynthDefBuilder() as b:
        lfo = SinOsc.kr(frequency=2)
        Poll.kr(source=lfo, trigger=10, label="UGen(SinOsc)")
        Out.ar(bus=b.add_parameter(name="out", value=0), source=SinOsc.ar(frequency=lfo))
    assert sapf == b.build(name="t").compile()


def test_poll_on_a_kr_copy_of_an_audio_node(compile_sapf):
    """The Poll rides on the node, so a later .kr of it still compiles the Poll."""
    ugens = decode_scgf(compile_sapf("2 ug.sinosc.poll.kr 0 sinosc"))["ugens"]
    (poll,) = [u for u in ugens if u[0] == "Poll"]
    assert ugens[poll[3][1][0]][0] == "SinOsc"


def test_poll_in_a_def(sapf_bin, tmp_path):
    out = run_sapf("def p freq:2 = [freq ug.lfnoise1.kr.poll 0 sinosc]\n`p.write", tmp_path)
    d = decode_scgf((tmp_path / "p.scsyndef").read_bytes())
    assert [u[0] for u in d["ugens"]].count("Poll") == 1, out


@pytest.mark.parametrize("expr,message", [
    ("440 ug.sinosc.ir", "sinosc has no ir"),
    ("440 0 sinosc.ir", "SinOsc has no ir"),
])
def test_ir_errors(sapf_bin, tmp_path, expr, message):
    out = run_sapf(expr, tmp_path)
    assert message in out and "error" in out, out


# bare UGens, without sapf.stac (stac.md, Phases 5)

def core_def(code, tmp_path, name):
    run_sapf(f'\\ [{code}] {{}} "{name}" synthdef .write', tmp_path, lib=[])
    return (tmp_path / f"{name}.scsyndef").read_bytes()


@pytest.mark.parametrize("bare,ug", [
    ("440 sinosc", "440 ug.sinosc"),
    ("440 sinosc .1 2 ramp", "440 ug.sinosc .1 2 ug.ramp"),
    ("440 sinosc -.5 .5 clip", "440 ug.sinosc -.5 .5 ug.clip"),
], ids=["sinosc", "ramp-is-sc", "clip-on-sig"])
def test_bare_word_is_the_ugen(bare, ug, sapf_bin, tmp_path):
    assert core_def(bare, tmp_path, "t") == core_def(ug, tmp_path, "t")


def test_core_keeps_real_valued_words(sapf_bin, tmp_path):
    out = run_sapf("15 0 10 clip pr cr  12 0 10 wrap pr cr  12 0 10 fold pr cr  .5 sapf.ramp pr cr  2 3 4 *+ pr cr",
                   tmp_path, lib=[])
    assert out.split("compiled OK.\n")[-1].split()[:5] == ["10", "2", "8", "0.5", "10"]


@pytest.mark.parametrize("code,word", [("440 sinosc 1 T", "T"), ("440 sinosc 9 N", "N"), ("440 sinosc 9 keep", "keep")])
def test_core_length_words_reject_a_ugen(code, word, sapf_bin, tmp_path):
    out = run_sapf(code, tmp_path, lib=[])
    assert f"{word} : a UGen has no length; sapf.{word} ends one" in out and "error" in out


def test_core_length_words_take_lists(sapf_bin, tmp_path):
    out = run_sapf("[1 2 3] 2 N pr cr  [1 2 3] 1 keep pr cr", tmp_path, lib=[])
    assert out.split("compiled OK.\n")[-1].split("\n")[:2] == ["[1 2]", "[1]"]


def test_sapf_t_ends_a_ugen(sapf_bin, tmp_path):
    ugens = decode_scgf(core_def("440 sinosc 1 sapf.T", tmp_path, "t"))["ugens"]
    assert "EnvGen" in [u[0] for u in ugens]
