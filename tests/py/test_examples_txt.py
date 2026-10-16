"""examples.txt: every line, typed into the REPL in order, runs without an error."""

import os
import subprocess
import sys
import tempfile

from conftest import ROOT, SAPF

sys.path.insert(0, str(ROOT / "scripts"))
import dictionary  # noqa: E402


def test_examples_run_in_the_repl(sapf_bin):
    lines = [line for line in (ROOT / "examples.txt").read_text().splitlines()
             if line.strip() and not line.lstrip().startswith(";")]
    server = dictionary.StandInServer()
    try:
        with tempfile.TemporaryDirectory() as tmp:
            env = dict(os.environ, STAC_PRELUDE="/dev/null", STAC_SCSYNTH=f"127.0.0.1:{server.port}",
                       STAC_SYNTHDEFS=tmp, STAC_RECORDINGS=tmp, STAC_LOG=f"{tmp}/log", STAC_HISTORY=f"{tmp}/history")
            out = subprocess.run([str(SAPF), "-q", "-p", str(ROOT / "prelude.stac")], input="\n".join(lines) + "\nquit\n",
                                 capture_output=True, text=True, env=env, timeout=120).stdout
    finally:
        server.close()
    errors = [line for line in out.splitlines() if line.startswith(("error", "syntax error")) or "undefined word" in line]
    assert errors == [], out[-3000:]
    assert "good bye" in out
