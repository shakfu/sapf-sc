"""kr and the rates compilation infers. Rates: 0 ir, 1 kr, 2 ar."""

from conftest import decode_scgf


def rates(compile_sapf, code):
    """The UGens after the def's out Control."""
    ugens = decode_scgf(compile_sapf(code))["ugens"]
    assert ugens[0][0] == "Control"
    return [(u[0], u[1]) for u in ugens[1:]]


def test_shared_source_stays_one_ugen(compile_sapf):
    # m feeds an audio-rate multiply too, so its kr use reads it through A2K.
    got = rates(compile_sapf, "10 lfnoise1 = m  m kr 300 * 400 + 0 sinosc m *")
    assert [n for n, _ in got].count("LFNoise1") == 1
    assert ("LFNoise1", 2) in got and ("A2K", 1) in got


def test_kr_only_source_is_lowered(compile_sapf):
    got = rates(compile_sapf, "10 lfnoise1 kr 300 * 400 + 0 sinosc")
    assert ("LFNoise1", 1) in got and "A2K" not in [n for n, _ in got]


def test_kr_channel_goes_through_k2a(compile_sapf):
    assert rates(compile_sapf, "3 0 sinosc kr") == [("SinOsc", 1), ("K2A", 2), ("Out", 2)]


def test_soundin_kr_samples_audio_bus(compile_sapf):
    # In.kr would read a control bus.
    got = rates(compile_sapf, "0 soundin kr")
    assert ("In", 2) in got and ("A2K", 1) in got


def test_ugen_rate_is_fixed(compile_sapf):
    got = rates(compile_sapf, "[3 0] 'SinOsc 'ar 1 ugen kr 0 sinosc")
    assert got[:2] == [("SinOsc", 2), ("A2K", 1)]


def test_kr_of_constant_is_constant(compile_sapf):
    assert rates(compile_sapf, ".5 kr") == [("DC", 2), ("Out", 2)]


def test_ugen_consumer_keeps_audio_input(compile_sapf):
    # Amplitude.kr reads every sample of an audio input; lowering the SinOsc would starve it.
    got = rates(compile_sapf, "[100 0 sinosc .01 .01] 'Amplitude 'kr 1 ugen")
    assert got[:2] == [("SinOsc", 2), ("Amplitude", 1)]
