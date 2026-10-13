"""sapf's SCgf output must equal nanosynth's, byte for byte, for the same graph built in the same
order. sapf evaluates lists lazily, so for multichannel graphs the nanosynth side creates UGens
in the order sapf forces them."""

import math

from conftest import decode_scgf
from nanosynth import SynthDefBuilder
from nanosynth.ugens import DC, CombN, LFSaw, Out, Pan2, SinOsc


def build(name, fn, **params):
    with SynthDefBuilder(**params) as b:
        fn(b)
    return b.build(name=name).compile()


def test_sine(compile_sapf):
    def fn(b):
        Out.ar(bus=0, source=SinOsc.ar(frequency=800, phase=0) * 0.3)

    assert compile_sapf("800 0 sinosc .3 *") == build("t", fn)


def test_phase_in_cycles(compile_sapf):
    def fn(b):
        Out.ar(bus=0, source=SinOsc.ar(frequency=800, phase=0.25 * 2 * math.pi))

    assert compile_sapf("800 .25 sinosc") == build("t", fn)


def test_stereo_forced_in_channel_order(compile_sapf):
    def fn(b):
        left = SinOsc.ar(frequency=300) * 0.1
        right = SinOsc.ar(frequency=301) * 0.1
        Out.ar(bus=0, source=[left, right])

    assert compile_sapf("[300 301] 0 sinosc .1 *") == build("t", fn)


def test_parameter(compile_sapf):
    def fn(b):
        Out.ar(bus=0, source=SinOsc.ar(frequency=b["freq"]) * b["amp"])

    sapf = compile_sapf("440 'freq ctl 0 sinosc .1 'amp ctl *")
    assert sapf == build("t", fn, freq=440.0, amp=0.1)


def test_multi_output(compile_sapf):
    def fn(b):
        Out.ar(bus=0, source=Pan2.ar(source=SinOsc.ar(frequency=440), position=-0.5))

    assert compile_sapf("440 0 sinosc -.5 pan2") == build("t", fn)


def test_constant_channel(compile_sapf):
    def fn(b):
        Out.ar(bus=0, source=[SinOsc.ar(frequency=440), DC.ar(source=0.5)])

    assert compile_sapf("[440 0 sinosc .5]") == build("t", fn)


def test_analog_bubbles(compile_sapf):
    sapf = compile_sapf(".4 0 lfsaw 2 * [8 7.23] 0 lfsaw .25 * 5/3 + + ohz 0 sinosc .04 * .2 0 4 combn")

    def fn(b):
        slow = LFSaw.ar(frequency=0.4) * 2
        chans = []
        for f in (8, 7.23):
            pitch = slow + (LFSaw.ar(frequency=f) * 0.25 + 5 / 3)
            sig = SinOsc.ar(frequency=(pitch + 4).octcps()) * 0.04
            comb = CombN.ar(source=sig, maximum_delay_time=0.2, delay_time=0.2, decay_time=4)
            chans.append(comb * 0.001 ** (0.2 / 4))  # sapf's comb output includes the feedback gain
        Out.ar(bus=0, source=chans)

    assert sapf == build("t", fn)



def test_nan_constants_share_an_index(compile_sapf):
    # decode_scgf raises IndexError on an out-of-range constant index.
    graph = decode_scgf(compile_sapf("100 0 sinosc 0 0 / * 0 0 / +"))
    assert len(graph["constants"]) == 3  # 100, 0 and one NaN
