"""stac --words: every word after the preludes, as JSON on stdout."""

import json
import os
import subprocess

from conftest import SAPF, SAPF_LIB


def words(*args):
    env = dict(os.environ, STAC_PRELUDE="/dev/null")
    out = subprocess.run([str(SAPF), "--words", *args], capture_output=True, text=True, env=env, timeout=30,
                         check=True)
    return json.loads(out.stdout)


def test_words_lists_builtins_and_preludes(sapf_bin):
    core = {w["name"]: w for w in words()}
    assert core["ndef"]["effect"] == "(channels name --> ndef)" and core["ndef"]["section"] == "patterns"
    assert core["bus.audio"]["source"] == "builtin" and core["sc.pv_magfreeze"]["section"] == "sc: fft"
    assert core["sapf.sinosc"]["automap"] == "zz"
    assert "lfo" not in core
    lib = {w["name"]: w for w in words(*SAPF_LIB)}
    assert lib["lfo"]["source"] == "prelude" and lib["lfo"]["effect"] == "(freq phase lo hi --> out)"


def test_words_entry_fields(sapf_bin):
    w = next(w for w in words() if w["name"] == "clear")
    assert w == {"name": "clear", "section": "stack ops", "effect": "(... -->)",
                 "help": "clears everything off the stack.", "automap": "", "source": "builtin"}


def test_words_hides_private_module_words(sapf_bin):
    names = {w["name"] for w in words(*SAPF_LIB)}
    assert "std.padz" in names
    assert not [n for n in names if n.startswith("_") or "._" in n]
