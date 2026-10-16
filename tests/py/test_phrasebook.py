"""docs/phrasebook.md and README.md: every stac block runs after prelude.stac, and the phrasebook's setup."""

import sys

from conftest import ROOT

sys.path.insert(0, str(ROOT / "scripts"))
import dictionary  # noqa: E402


def test_phrases_run(sapf_bin):
    setup, blocks = dictionary.page_blocks(ROOT / "docs" / "phrasebook.md")
    assert setup and len(blocks) >= 30
    assert dictionary.run_blocks([(label, dictionary.with_setup(b, setup)) for label, b in blocks]) == []


def test_readme_runs(sapf_bin):
    """The README's stac blocks; those under "sapf programs" after sapf.stac too."""
    _, blocks = dictionary.page_blocks(ROOT / "README.md")
    sapf = [b for b in blocks if "sapf programs" in b[0]]
    stac = [b for b in blocks if b not in sapf]
    assert len(stac) >= 2 and len(sapf) == 1
    assert dictionary.run_blocks(stac) == []
    assert dictionary.run_blocks(sapf, libs=("prelude.stac", "sapf.stac")) == []
