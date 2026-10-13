"""Operations a SynthDef cannot express must fail with an error and write no file."""

import pytest

from conftest import run_sapf

SIG = "100 0 sinosc"


@pytest.mark.parametrize(
    "code, message",
    [
        (f"{SIG} reverse", "expected List. got Sig"),
        (f"{SIG} 1 at", "expected List. got Sig"),
        (f"{SIG} sort", "expected List. got Sig"),
        (f"{SIG} 1 skip", "expected List. got Sig"),
        (f"{SIG} +/", "got Sig"),
        (f"{SIG} #[1 2 3] +", "got ZList"),
        (f"{SIG} erf", "no SuperCollider equivalent"),
        (f"{SIG} 4 rand", "got Sig"),
        (f"{SIG} {SIG} 0 delayn", "maxdelay is zero and delay is a UGen"),
        (f"1 'f ctl 2 'f ctl +", "parameter 'f' is defined more than once"),
        (f"[{SIG}] 'Foo 'zr 1 ugen", "expected 'ar, 'kr or 'ir"),
        ("[1 2 3] ever", "indefinite"),
    ],
)
def test_error(code, message, sapf_bin, tmp_path):
    out = run_sapf(f'{code} "t" >sd', tmp_path)
    assert "error" in out and message in out, out
    assert not (tmp_path / "t.scsyndef").exists()


def test_writes_to_synthdefs_dir(sapf_bin, tmp_path):
    out = run_sapf(f'{SIG} "named" >sd', tmp_path)
    assert (tmp_path / "named.scsyndef").read_bytes()[:4] == b"SCgf", out
