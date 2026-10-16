"""tree-sitter-stac: the corpus passes, and the committed parser is what grammar.js generates. Needs the
tree-sitter CLI; the build itself compiles the committed parser.c."""

import shutil
import subprocess

import pytest

from conftest import ROOT

GRAMMAR = ROOT / "tree-sitter-stac"
CLI = shutil.which("tree-sitter")
pytestmark = pytest.mark.skipif(CLI is None, reason="no tree-sitter CLI")


def test_corpus():
    out = subprocess.run([CLI, "test"], cwd=GRAMMAR, capture_output=True, text=True, timeout=120)
    assert out.returncode == 0, out.stdout + out.stderr


def test_generated_parser_is_current(tmp_path):
    shutil.copy(GRAMMAR / "grammar.js", tmp_path)
    shutil.copy(GRAMMAR / "tree-sitter.json", tmp_path)
    out = subprocess.run([CLI, "generate"], cwd=tmp_path, capture_output=True, text=True, timeout=120)
    assert out.returncode == 0, out.stderr
    for name in ("grammar.json", "node-types.json", "parser.c"):
        assert (tmp_path / "src" / name).read_text() == (GRAMMAR / "src" / name).read_text(), \
            f"src/{name} is stale; run make grammar"
