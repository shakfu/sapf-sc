""">sf: offline renders by the embedded scsynth, against original sapf's renders in tests/fixtures/ref."""

import os
import subprocess
import time
from pathlib import Path

import numpy as np
import pytest

from conftest import SAPF, SAPF_LIB, SR, read_wav_float

REF = Path(__file__).resolve().parents[1] / "fixtures" / "ref"


def sf(code, tmp_path, rate=SR):
    src = tmp_path / "in.sapf"
    src.write_text(code + "\n")
    env = dict(os.environ, STAC_RECORDINGS=str(tmp_path), STAC_LOG=str(tmp_path / "log"))
    env.pop("STAC_SCSYNTH", None)
    r = subprocess.run([str(SAPF), "-q", *SAPF_LIB, "-r", str(rate), str(src)], capture_output=True, text=True, env=env,
                       timeout=60)
    return r.stdout + r.stderr


@pytest.mark.parametrize("name, expr, tol", [
    ("sinosc", "100 0 sinosc", 1e-4),
    ("lines", "#[0 1 -.5 .2] #[.02 .03 .01] 1 lines", 1e-4),
    ("adsr_early", "[.03 .02 .5 .02] .8 .02 1 adsr", 1e-3),
    ("ola", "\\i[ 0 0 sinosc i 1 + + .02 T ] 1/75 1 1 ola 0 at", None),
])
def test_matches_sapf(name, expr, tol, sapf_bin, tmp_path):
    ref = read_wav_float(REF / f"{name}.wav")
    out = sf(f'{expr} {len(ref)} N "x" >sf', tmp_path)
    assert "error" not in out, out
    got = read_wav_float(tmp_path / "x.wav")
    assert got.shape == ref.shape, (got.shape, ref.shape)
    bad = np.abs(got - ref) > (tol or 1e-4)
    if tol is None:  # as test_conformance's ola case: sapf's beat drift moves voice edges by a frame
        assert bad.sum() <= 12
    else:
        assert not bad.any(), np.abs(got - ref).max()


def test_length_is_the_first_channel_to_end(sapf_bin, tmp_path):
    out = sf('[100 0 sinosc 1000 N  200 0 sinosc 2000 N  300 0 sinosc] "x" >sf', tmp_path)
    assert "error" not in out, out
    got = read_wav_float(tmp_path / "x.wav")
    assert got.shape == (1000, 3)


@pytest.mark.parametrize("code, message", [
    ('100 0 sinosc "x" >sf', "indefinite number of frames"),
    ('#[1 2] 40 0 impulse seq "x" >sf', "length is unknown"),
])
def test_errors(code, message, sapf_bin, tmp_path):
    out = sf(code, tmp_path)
    assert "error" in out and message in out, out
    assert not (tmp_path / "x.wav").exists()


def test_render_while_playing(sapf_bin, tmp_path):
    # A render's World ends without unloading the UGens the realtime one is using.
    out = sf('440 0 sinosc 0 * .3 T play\n100 0 sinosc 480 N "a" >sf\n100 0 sinosc 480 N "b" >sf', tmp_path)
    if "scsynth did not start" in out:
        pytest.skip("no audio device")
    assert "error" not in out, out
    assert read_wav_float(tmp_path / "b.wav").shape == (480, 1)


def sfo(name, opener, tmp_path):
    """Runs `>sfo` in the REPL with `opener` (a shell script body, or None for no opener) as
    xdg-open and open, and quits after the opener has been reaped. Returns sapf's output."""
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    if opener is not None:
        for cmd in ("xdg-open", "open"):
            (bin_dir / cmd).write_text(f"#!/bin/sh\n{opener}\n")
            (bin_dir / cmd).chmod(0o755)
    env = dict(os.environ, STAC_RECORDINGS=str(tmp_path), STAC_LOG=str(tmp_path / "log"),
               PATH=f"{bin_dir}:/usr/bin:/bin" if opener is not None else str(bin_dir))
    env.pop("STAC_SCSYNTH", None)
    p = subprocess.Popen([str(SAPF), *SAPF_LIB], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         text=True, env=env)
    p.stdin.write(f'100 0 sinosc 480 N "{name}" >sfo\n')
    p.stdin.flush()
    time.sleep(1)
    return p.communicate("quit\n", timeout=30)[0]


def test_sfo_opens_the_path_unquoted(sapf_bin, tmp_path):
    # The path reaches the opener as one argument; no shell expands it.
    out = sfo("a$(touch pwned)", f'echo "$1" > {tmp_path}/opened', tmp_path)
    assert "error" not in out and ">sfo :" not in out, out
    assert (tmp_path / "opened").read_text().strip() == str(tmp_path / "a$(touch pwned).wav")
    assert not (tmp_path / "pwned").exists() and not Path("pwned").exists()


def test_sfo_reports_a_failed_open(sapf_bin, tmp_path):
    out = sfo("b", "exit 4", tmp_path)
    assert ">sfo :" in out and "could not open" in out and "b.wav" in out, out


def test_sfo_reports_a_missing_opener(sapf_bin, tmp_path):
    out = sfo("c", None, tmp_path)
    assert ">sfo : could not run" in out, out
    assert (tmp_path / "c.wav").exists()
