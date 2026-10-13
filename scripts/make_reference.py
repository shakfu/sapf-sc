"""Renders tests/py/cases.py with original sapf into tests/fixtures/ref.

    SAPF_ORIG=/path/to/original/sapf uv run python scripts/make_reference.py

Original sapf's multichannel >sf drops frames, so each channel is rendered alone and interleaved here.
"""

import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests" / "py"))
from cases import CASES  # noqa: E402
from conftest import SR, read_wav_float  # noqa: E402

OUT = ROOT / "tests" / "fixtures" / "ref"


def write_wav_float(path: Path, frames: np.ndarray) -> None:
    data = frames.astype("<f4").tobytes()
    channels = frames.shape[1]
    fmt = struct.pack("<HHIIHH", 3, channels, SR, SR * 4 * channels, 4 * channels, 32)
    path.write_bytes(
        b"RIFF" + struct.pack("<I", 4 + 8 + len(fmt) + 8 + len(data)) + b"WAVE"
        + b"fmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", len(data)) + data
    )


def main() -> None:
    sapf = os.environ["SAPF_ORIG"]
    OUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        env = dict(os.environ, SAPF_RECORDINGS=tmp, SAPF_PRELUDE="/dev/null", SAPF_LOG=f"{tmp}/log")
        for c in CASES:
            expr = c.ref_expr or c.expr
            frames = int(c.seconds * SR)
            lines = [f'{expr} = sig'] + [
                f'sig {f"{i} at " if c.channels > 1 else ""}{frames} N "c{i}" >sf' for i in range(c.channels)
            ]
            src = Path(tmp) / "ref.sapf"
            src.write_text("\n".join(lines) + "\n")
            out = subprocess.run([sapf, "-q", "-r", str(SR), str(src)], capture_output=True, text=True, env=env)
            if "error" in out.stdout:
                sys.exit(f"{c.name}: {out.stdout}")
            chans = [read_wav_float(Path(tmp) / f"c{i}.wav")[:, 0] for i in range(c.channels)]
            write_wav_float(OUT / f"{c.name}.wav", np.stack(chans, axis=1))


if __name__ == "__main__":
    main()
