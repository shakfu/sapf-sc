"""Helpers: compile sapf expressions to SCgf, and render SynthDefs with nanosynth's NRT scsynth."""

import ctypes
import os
import struct
import subprocess
from pathlib import Path

import numpy as np
import pytest

ROOT = Path(__file__).resolve().parents[2]
SAPF = Path(os.environ.get("SAPF_BIN", ROOT / "build" / "sapf"))
SR = 48000
_libc = ctypes.CDLL(None)


@pytest.fixture(scope="session")
def sapf_bin() -> Path:
    if not SAPF.exists():
        pytest.fail(f"{SAPF} not built; run make build")
    return SAPF


def run_sapf(code: str, workdir: Path) -> str:
    """Runs sapf `code` from a file with SAPF_SYNTHDEFS=workdir. Returns its output."""
    src = workdir / "input.sapf"
    src.write_text(code + "\n")
    env = dict(os.environ, SAPF_SYNTHDEFS=str(workdir), SAPF_PRELUDE="/dev/null", SAPF_LOG=str(workdir / "log"))
    out = subprocess.run([str(SAPF), "-q", str(src)], capture_output=True, text=True, env=env, timeout=30, check=True)
    return out.stdout + out.stderr


@pytest.fixture
def compile_sapf(sapf_bin, tmp_path):
    def compile_(expr: str, name: str = "t") -> bytes:
        out = run_sapf(f'{expr} "{name}" >sd', tmp_path)
        path = tmp_path / f"{name}.scsyndef"
        assert path.exists(), out
        return path.read_bytes()

    return compile_


def read_wav_float(path: Path) -> np.ndarray:
    """Reads a 32-bit float WAV into an array of shape (frames, channels)."""
    data = path.read_bytes()
    assert data[:4] == b"RIFF" and data[8:12] == b"WAVE"
    pos, channels, samples = 12, 0, None
    while pos < len(data):
        cid, size = data[pos : pos + 4], struct.unpack("<I", data[pos + 4 : pos + 8])[0]
        body = data[pos + 8 : pos + 8 + size]
        if cid == b"fmt ":
            fmt, channels = struct.unpack("<HH", body[:4])
            bits = struct.unpack("<H", body[14:16])[0]
            assert bits == 32 and fmt in (3, 0xFFFE), (fmt, bits)
        elif cid == b"data":
            samples = np.frombuffer(body, dtype="<f4")
        pos += 8 + size + (size & 1)
    assert samples is not None
    return samples.reshape(-1, channels)


def render(defbytes: bytes, name: str, seconds: float, workdir: Path, channels: int = 1, **params) -> np.ndarray:
    """Renders one synth of the def from time 0 for `seconds` with scsynth NRT."""
    from nanosynth.osc import OscMessage
    from nanosynth.score import Score

    score = Score()
    score.add(0.0, OscMessage("/d_recv", defbytes))
    score.add_synth(0.0, name, **params)
    score.add(seconds, OscMessage("/c_set", 0, 0))
    out = workdir / f"{name}.wav"
    score.render(out, sample_rate=SR, sample_format="float", output_channels=channels)
    _libc.fflush(None)  # scsynth logs through C stdio; flush so capfd sees its errors
    return read_wav_float(out)[: int(seconds * SR)]


def decode_scgf(data: bytes) -> dict:
    """Decodes a version 2 SCgf file holding one def into constants, params and a UGen list.
    Each UGen is (name, rate, special, inputs, outputs); an input is ("c", value) or (ugen, output)."""
    pos = 0

    def take(fmt):
        nonlocal pos
        vals = struct.unpack_from(">" + fmt, data, pos)
        pos += struct.calcsize(">" + fmt)
        return vals if len(vals) > 1 else vals[0]

    def pstr():
        nonlocal pos
        n = data[pos]
        s = data[pos + 1 : pos + 1 + n].decode("ascii")
        pos += 1 + n
        return s

    assert data[:4] == b"SCgf"
    pos = 4
    assert take("I") == 2 and take("H") == 1
    name = pstr()
    constants = [take("f") for _ in range(take("I"))]
    defaults = [take("f") for _ in range(take("I"))]
    params = {pstr(): take("I") for _ in range(take("I"))}
    ugens = []
    for _ in range(take("I")):
        uname, rate, nin, nout, special = pstr(), take("B"), take("I"), take("I"), take("H")
        inputs = []
        for _ in range(nin):
            u, i = take("I"), take("I")
            inputs.append(("c", constants[i]) if u == 0xFFFFFFFF else (u, i))
        outs = [take("B") for _ in range(nout)]
        ugens.append((uname, rate, special, inputs, outs))
    assert take("H") == 0 and pos == len(data)
    return {"name": name, "constants": constants, "defaults": defaults, "params": params, "ugens": ugens}
