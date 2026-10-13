"""Renders each case's SynthDef with scsynth and compares it with original sapf's output."""

from pathlib import Path

import numpy as np
import pytest

from cases import CASES
from conftest import read_wav_float, render

REF = Path(__file__).resolve().parents[1] / "fixtures" / "ref"


@pytest.mark.parametrize("case", CASES, ids=lambda c: f"{c.name}: {c.expr}")
def test_matches_sapf(case, compile_sapf, tmp_path):
    ref = read_wav_float(REF / f"{case.name}.wav")
    got = render(compile_sapf(case.expr, case.name), case.name, case.seconds, tmp_path, case.channels)
    assert got.shape[1] == ref.shape[1]
    # A finite sapf signal ends; its synth frees itself by the end of that block.
    tail = got[len(ref) :]
    assert np.abs(tail).max(initial=0) < 1e-4 and np.all(tail[64:] == 0), "synth continues after the signal ends"
    got = got[: len(ref)]
    diff = np.abs(ref - got)
    if case.mode == "rms":
        ratio = np.sqrt(np.mean(diff**2)) / np.sqrt(np.mean(ref**2))
        assert ratio <= case.tol, f"RMS of difference is {ratio:.3g} of the reference's"
    else:
        scale = np.maximum(1.0, np.abs(ref))
        nan_mismatch = np.isnan(ref) != np.isnan(got)
        bad = np.nonzero(((diff / scale) > case.tol).any(axis=1) | nan_mismatch.any(axis=1))[0]
        assert len(bad) <= case.max_bad, (
            f"{len(bad)} frames differ by more than {case.tol}, max {diff.max():.3g}; "
            f"first at {bad[0]}: ref {ref[bad[0]]} got {got[bad[0]]}"
        )
