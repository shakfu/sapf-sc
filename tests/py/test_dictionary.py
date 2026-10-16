"""docs/dictionary: an entry for every word, generated lines current, and every example run."""

import sys

from conftest import ROOT

sys.path.insert(0, str(ROOT / "scripts"))
import dictionary  # noqa: E402


def test_every_word_has_an_entry(sapf_bin):
    ws, chs = dictionary.words(), dictionary.chapters()
    probs = dictionary.problems(ws, chs)
    assert probs["missing"] == [] and probs["unknown"] == [], probs
    stale = [w.name for w in ws if (e := chs[w.chapter()].entries().get(w.name)) and dictionary.set_line(e, w.line())]
    assert stale == [], "run scripts/dictionary.py sync"


def test_examples_run(sapf_bin):
    """Each example runs without an error and leaves what its ;-> lines say; play goes to a stand-in server."""
    assert dictionary.run_examples(dictionary.chapters()) == []
