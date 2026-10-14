"""osc against wave tables computed here from the formula sapf's code describes. Original sapf's
own tables are broken (docs/dev/upstream-sapf-bugs.md), so it is not the reference."""

import numpy as np
import pytest

from conftest import SR, render_nrt

HARMONICS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 25, 32, 40, 50, 64, 80, 101, 128, 161, 203, 256, 322,
             406, 512, 645, 812, 1024]


def tables(amps, phases, smooth=0.0):
    theta = 2 * np.pi * np.arange(4096) / 4096
    out = np.zeros((30, 4096))
    for t, n in enumerate(HARMONICS):
        n = min(len(amps), n)
        for k in range(n):
            gain = 1.0 if smooth == 0 else np.cos(np.pi / 2 / n * k) ** smooth
            out[t] += amps[k] * gain * np.sin((k + 1) * theta + 2 * np.pi * phases[k])
    return out / np.abs(out).max()


def expected(tabs, freq, frames):
    """VOsc's output, approximately: tables crossfaded by sapf's table index, read linearly."""
    h = min(24000 / freq, 1290)
    ft = min(max(h - 1 if h < 8 else 3 * np.log2(h) - 2, 0), 29 - 1e-7)
    i, frac = int(ft), ft - int(ft)
    tab = (1 - frac) * tabs[i] + frac * tabs[min(i + 1, 29)]
    pos = (np.arange(frames) * freq / SR) % 1 * 4096
    return np.interp(pos, np.arange(4097), np.append(tab, tab[0]))


SAW = tables([1 / (k + 1) for k in range(1290)], [0 if k % 2 == 0 else 0.5 for k in range(1290)])


@pytest.mark.parametrize("freq", [100, 1000, 5000])
def test_saw_table(freq, score_sapf, tmp_path):
    got = render_nrt(score_sapf(f"{freq} 0 sawTbl osc", 0.1), 0.1, tmp_path)[:, 0]
    ref = expected(SAW, freq, len(got))
    assert np.sqrt(np.mean((got - ref) ** 2)) / np.sqrt(np.mean(ref**2)) < 1e-3


def test_wavefill(score_sapf, tmp_path):
    got = render_nrt(score_sapf("300 0 #[1 .5 .25] #[0 .25 .5] 2 wavefill osc", 0.1), 0.1, tmp_path)[:, 0]
    ref = expected(tables([1, 0.5, 0.25], [0, 0.25, 0.5], 2), 300, len(got))
    assert np.abs(got - ref).max() < 1e-3


def test_osc_in_ola_voices(score_sapf, tmp_path):
    # Voices load their tables when they start.
    got = render_nrt(score_sapf("\\\\i[ 200 0 sqrTbl osc .02 T ] 1/75 1 1 ola 0 at", 0.05), 0.05, tmp_path)[:, 0]
    assert np.abs(got[:900]).max() > 0.5
