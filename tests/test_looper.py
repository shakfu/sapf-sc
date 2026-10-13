import math
import struct
import wave

from nanosynth import OscMessage, Score
from nanosynth.scsynth import Options

from looper import looper, write


def write_input(path, rate, seconds):
    """A mono int16 chirp, so no two loop periods hold the same samples."""
    n = int(rate * seconds)
    frames = (int(12000 * math.sin(2 * math.pi * (110 + 200 * i / rate) * i / rate)) for i in range(n))
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(b"".join(struct.pack("<h", s) for s in frames))


def read_output(path):
    with wave.open(str(path)) as w:
        channels = w.getnchannels()
        data = w.readframes(w.getnframes())
    samples = struct.unpack(f"<{len(data) // 2}h", data)
    return [list(samples[c::channels]) for c in range(channels)]


def render(tmp_path, seconds, total, rate=48000, memory_kb=8192):
    """Plays `looper(seconds)` on a chirp for `total` seconds. Returns (loop, dry) samples up to the
    synth's end."""
    write_input(tmp_path / "in.wav", rate, total)
    score = Score()
    score.add_synthdef(0.0, looper(seconds))
    score.add_synth(0.0, "looper")
    score.add(total, OscMessage("/c_set", 0, 0))
    score.render(
        tmp_path / "out.wav",
        sample_rate=rate,
        input_path=tmp_path / "in.wav",
        input_channels=1,
        output_channels=2,
        options=Options(memory_size=memory_kb),
    )
    loop, dry = read_output(tmp_path / "out.wav")
    end = int(rate * total)
    return loop[:end], dry[:end]


def test_records_then_loops_exactly(tmp_path):
    loop, dry = render(tmp_path, seconds=1.0, total=2.5)
    n = 48000
    assert max(map(abs, dry)) > 10000
    # first pass: RecordBuf writes each sample before BufRd reads it
    assert loop[:n] == dry[:n]
    # later passes repeat the first second
    assert loop[n:] == [dry[t % n] for t in range(n, len(loop))]


def test_loop_length_follows_the_sample_rate(tmp_path):
    loop, dry = render(tmp_path, seconds=0.5, total=1.2, rate=44100)
    n = 22050
    assert loop[n:] == [dry[t % n] for t in range(n, len(loop))]


def test_default_pool_cannot_hold_a_minute(tmp_path):
    """scsynth's default 8 MB pool holds 43.6 s at 48 kHz mono. LocalBuf's allocation fails and the
    loop is silent; a 16 MB pool holds it."""
    loop, _ = render(tmp_path, seconds=60.0, total=0.5)
    assert max(map(abs, loop)) == 0
    loop, dry = render(tmp_path, seconds=60.0, total=0.5, memory_kb=16384)
    assert loop == dry


def test_writes_def_and_sidecar(tmp_path):
    write(tmp_path)
    assert (tmp_path / "looper.scsyndef").read_bytes()[:4] == b"SCgf"
    assert (tmp_path / "looper.txt").read_text().startswith("// @p1 rate")
