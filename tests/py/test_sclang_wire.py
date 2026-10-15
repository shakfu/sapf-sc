"""scripts/sclang_wire.py: wire orders read from SC's class library source."""

import json
import sys

import pytest

from conftest import ROOT

sys.path.insert(0, str(ROOT / "scripts"))
import sclang_wire as sw  # noqa: E402

WIRE = json.loads((ROOT / "spec" / "sclang-wire.json").read_text())["ugens"]


@pytest.mark.parametrize("body,names,wire,meta", [
    ("arg dur = 1, level = 1; ^this.multiNew('audio', dur, reset, doneAction, level)",
     ["dur", "reset", "level", "doneAction"], ["dur", "reset", "doneAction", "level"], {}),
    ("arg list, repeats = 1; ^this.multiNewList(['demand', repeats] ++ list)",
     ["list", "repeats"], ["repeats", "*list"], {}),
    ("^this.multiNewList(['control', trig, reset] ++ demandUGens.asArray)",
     ["trig", "reset", "demandUGens"], ["trig", "reset", "*demandUGens"], {}),
    ("^this.multiNew('audio', in.asAudioRateInput(this), maxdelaytime, delaytime)",
     ["in", "maxdelaytime", "delaytime"], ["in", "maxdelaytime", "delaytime"], {"audio": ["in"]}),
    ("^this.multiNew('audio', trig, resetval ? min, 1)",
     ["trig", "resetval", "min"], ["trig", "resetval", "=1"], {"fallback": {"resetval": "min"}}),
    ("^this.multiNewList(['scalar', buf, values.size] ++ values)",
     ["buf", "values"], ["buf", "#values", "*values"], {}),
    ("channelsArray = this.foo(channelsArray); this.multiNewList(['audio', bus] ++ channelsArray); ^0.0",
     ["bus", "channelsArray"], ["bus", "*channelsArray"], {}),
    ("^this.multiNew('audio', in, rect.left)", ["in", "rect"], ["in", "?rect.left"], {}),
])
def test_tokens(body, names, wire, meta):
    assert sw.tokens(body, names) == (wire, meta)


def test_tokens_without_call():
    assert sw.tokens("^Ddup(n, in)", ["n", "in"]) is None


def test_comments_are_blanked():
    src = 'a // b {\n/* c /* d */ } */ $" "x // y"'
    assert sw.strip_comments(src) == 'a \n  $" "x // y"'


@pytest.mark.parametrize("cls,rate,wire", [
    ("Duty", "ar", ["dur", "reset", "doneAction", "level"]),
    ("Dseq", "new", ["repeats", "*list"]),
    ("Dwhite", "new", ["length", "lo", "hi"]),
    ("SinOsc", "ar", ["freq", "phase"]),
    ("Pan2", "ar", ["in", "pos", "level"]),
])
def test_known_orders(cls, rate, wire):
    row = WIRE[cls][rate]
    assert row["wire"] == wire
    assert row["status"] in ("verified", "consistent")


def test_num_channels_sets_outputs():
    row = WIRE["PlayBuf"]["ar"]
    assert row["outputs"] == "numChannels"
    assert row["wire"][0] == "bufnum"
    assert row["status"] in ("verified", "consistent")


def test_coverage():
    """Trusted maps for at least 90% of the 635 rate methods; the rest need hand entries."""
    rows = [r for methods in WIRE.values() for r in methods.values()]
    trusted = [r for r in rows if r["status"] in ("verified", "consistent")]
    assert len(rows) == 635
    assert len(trusted) >= 0.9 * len(rows)


@pytest.mark.skipif(not sw.LIB.is_dir(), reason="needs the SuperCollider source from make build")
def test_wire_file_is_fresh():
    assert sw.extract(sw.LIB) == json.loads((ROOT / "spec" / "sclang-wire.json").read_text())
