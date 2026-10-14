"""seq and iseq read value streams when the def compiles."""

from conftest import run_sapf


def test_infinite_signal_keeps_1024_values(sapf_bin, tmp_path):
    # Z once called its output finite, and seq forced the whole infinite stream to measure it
    out = run_sapf('ord Z 1 0 impulse iseq "t" >sd', tmp_path)
    path = tmp_path / "t.scsyndef"
    assert path.exists(), out
    # 1024 values, no period: each a 4-byte constant
    assert path.stat().st_size > 1024 * 4
