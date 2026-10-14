"""Renders tests/py/cases.py with original sapf into tests/fixtures/ref.

    SAPF_ORIG=/path/to/original/sapf uv run python scripts/make_reference.py [case names]

With case names, renders only those cases.

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
TIMEOUT = 60  # seconds per sapf run


def write_wav_float(path: Path, frames: np.ndarray) -> None:
    data = frames.astype("<f4").tobytes()
    channels = frames.shape[1]
    fmt = struct.pack("<HHIIHH", 3, channels, SR, SR * 4 * channels, 4 * channels, 32)
    path.write_bytes(
        b"RIFF" + struct.pack("<I", 4 + 8 + len(fmt) + 8 + len(data)) + b"WAVE"
        + b"fmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", len(data)) + data
    )


def source(c) -> str:
    """The sapf program that renders case `c`, one file c<i>.wav per channel."""
    frames = int(c.seconds * SR)
    lines = [f'{c.ref_expr or c.expr} = sig'] + [
        f'sig {f"{i} at " if c.channels > 1 else ""}{frames} N "c{i}" >sf' for i in range(c.channels)
    ]
    return "\n".join(lines) + "\n"


def run_sapf(sapf: str, src: Path, tmp: str) -> subprocess.CompletedProcess:
    """Runs sapf on `src`, writing into `tmp`. stdin is closed so a REPL after the file cannot wait on the terminal."""
    env = dict(os.environ, SAPF_RECORDINGS=tmp, SAPF_PRELUDE="/dev/null", SAPF_LOG=f"{tmp}/log")
    return subprocess.run([sapf, "-q", "-r", str(SR), str(src)], capture_output=True, text=True, env=env,
                          stdin=subprocess.DEVNULL, timeout=TIMEOUT)


def fail(name: str, why: str, out: subprocess.CompletedProcess | None = None) -> None:
    msg = f"{name}: {why}"
    if out is not None:
        msg += f"\n  exit {out.returncode}\n  stdout: {out.stdout.strip()}\n  stderr: {out.stderr.strip()}"
    sys.exit(msg)


def main() -> None:
    sapf = os.environ["SAPF_ORIG"]
    names = set(sys.argv[1:])
    unknown = names - {c.name for c in CASES}
    if unknown:
        sys.exit(f"no such cases: {' '.join(sorted(unknown))}")
    OUT.mkdir(parents=True, exist_ok=True)
    written = 0
    with tempfile.TemporaryDirectory() as tmp:
        for c in CASES:
            if names and c.name not in names:
                continue
            print(f"{c.name}: {c.ref_expr or c.expr}", flush=True)
            src = Path(tmp) / "ref.sapf"
            src.write_text(source(c))
            for i in range(c.channels):
                (Path(tmp) / f"c{i}.wav").unlink(missing_ok=True)
            try:
                out = run_sapf(sapf, src, tmp)
            except subprocess.TimeoutExpired:
                fail(c.name, f"sapf did not exit within {TIMEOUT} s")
            if out.returncode != 0 or "error" in out.stdout:
                fail(c.name, "sapf failed", out)
            missing = [f"c{i}.wav" for i in range(c.channels) if not (Path(tmp) / f"c{i}.wav").exists()]
            if missing:
                fail(c.name, f"sapf wrote no {' '.join(missing)}", out)
            chans = [read_wav_float(Path(tmp) / f"c{i}.wav")[:, 0] for i in range(c.channels)]
            path = OUT / f"{c.name}.wav"
            write_wav_float(path, np.stack(chans, axis=1))
            print(f"  wrote {path}", flush=True)
            written += 1
    print(f"{written} references written")


if __name__ == "__main__":
    main()
