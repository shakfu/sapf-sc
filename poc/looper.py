"""Route 1 proof of concept: sample manipulation in a SynthDef through LocalBuf, with no buffers
allocated by the host. See docs/dev/localbuf-poc.md.

    python poc/looper.py OUTDIR    # writes looper.scsyndef and its scsy sidecar looper.txt
"""

import sys
from pathlib import Path

from nanosynth import SynthDef, SynthDefBuilder
from nanosynth.ugens import BufFrames, BufRd, In, LocalBuf, NumOutputBuses, Out, Phasor, RecordBuf, SampleRate

SECONDS = 10.0  # under the 43.6 s that scsynth's default 8 MB real-time pool holds at 48 kHz

SIDECAR = "// @p1 rate -2 2\n"


def looper(seconds: float = SECONDS) -> SynthDef:
    """Records the first `seconds` of input 1 once, then loops it to output 1 at `rate`; negative
    plays backwards. Output 2 is input 1, unchanged."""
    with SynthDefBuilder(rate=1.0) as b:
        src = In.ar(bus=NumOutputBuses.ir(), channel_count=1)
        buf = LocalBuf.ir(channel_count=1, frame_count=SampleRate.ir() * seconds)
        RecordBuf.ar(buffer_id=buf, source=src, loop=0)
        phase = Phasor.ar(rate=b["rate"], stop=BufFrames.ir(buffer_id=buf))
        Out.ar(bus=0, source=[BufRd.ar(buffer_id=buf, phase=phase, loop=1), src])
    return b.build(name="looper")


def write(outdir: Path) -> None:
    outdir.mkdir(parents=True, exist_ok=True)
    (outdir / "looper.scsyndef").write_bytes(looper().compile())
    (outdir / "looper.txt").write_text(SIDECAR)


if __name__ == "__main__":
    write(Path(sys.argv[1]))
