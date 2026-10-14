"""sf> and playbuf: sound files from scsynth buffers, in NRT renders of >nrt scores, and the
sidecar >sd writes for scsy."""

import struct

import numpy as np
import pytest

from conftest import SR, render_nrt, run_sapf


def write_wav(path, frames):
    """16-bit WAV of frames, shape (n, channels), in -1..1."""
    data = (np.asarray(frames) * 32768).round().clip(-32768, 32767).astype("<i2").tobytes()
    ch = frames.shape[1]
    fmt = struct.pack("<HHIIHH", 1, ch, SR, SR * 2 * ch, 2 * ch, 16)
    path.write_bytes(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVE" + b"fmt " + struct.pack("<I", 16) + fmt
                     + b"data" + struct.pack("<I", len(data)) + data)


@pytest.fixture
def tone(tmp_path):
    """0.05 s stereo: a ramp on the left, a sine on the right."""
    n = 2400
    frames = np.stack([np.arange(n) / n - 0.5, 0.5 * np.sin(2 * np.pi * 100 * np.arange(n) / SR)], axis=1)
    path = tmp_path / "tone.wav"
    write_wav(path, frames)
    return path, (np.asarray(frames) * 32768).round() / 32768


def test_sf_plays_the_file_once(tone, score_sapf, tmp_path):
    path, frames = tone
    got = render_nrt(score_sapf(f'"{path}" sf>', 0.1), 0.1, tmp_path, channels=2)
    assert np.abs(got[: len(frames)] - frames).max() < 1e-6
    assert np.all(got[len(frames) :] == 0), "the signal ends with the file"


def test_playbuf_loops(tone, score_sapf, tmp_path):
    path, frames = tone
    got = render_nrt(score_sapf(f'"{path}" 1 1 playbuf', 0.12), 0.12, tmp_path, channels=2)
    looped = np.concatenate([frames, frames, frames])[: len(got)]
    assert np.abs(got - looped).max() < 1e-6


def test_playbuf_rate(tone, score_sapf, tmp_path):
    path, frames = tone
    got = render_nrt(score_sapf(f'"{path}" 2 0 playbuf', 0.05), 0.05, tmp_path, channels=2)
    assert np.abs(got[:1200] - frames[::2][:1200]).max() < 1e-6
    assert np.all(got[1200:] == 0)


def test_sd_names_the_file_in_the_sidecar(tone, sapf_bin, tmp_path):
    path, _ = tone
    (tmp_path / "s.txt").write_text("// @p1 amp\n// @buf 9 old.wav\n")
    out = run_sapf(f'"{path}" sf> 0 at .5 \'amp ctl * "s" >sd', tmp_path)
    assert "error" not in out, out
    assert (tmp_path / "s.txt").read_text() == f"// @p1 amp\n// @buf 0 {path}\n"


def test_sd_without_samples_leaves_a_sidecar_alone(sapf_bin, tmp_path):
    (tmp_path / "t.txt").write_text("// @p1 amp")  # no final newline
    run_sapf("100 0 sinosc .5 'amp ctl * \"t\" >sd", tmp_path)
    assert (tmp_path / "t.txt").read_text() == "// @p1 amp"


@pytest.mark.parametrize("code, message", [
    ('"nope.wav" sf>', "cannot read 'nope.wav'"),
    ('"{path}" 0 0 playbuf', "rate must be positive"),
])
def test_errors(code, message, tone, sapf_bin, tmp_path):
    out = run_sapf(code.format(path=tone[0]) + ' "x" >sd', tmp_path)
    assert "error" in out and message in out, out
