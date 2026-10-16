"""Modules: a file imported in its own scope (docs/dev/stac.md, Proposal: modules)."""

import os
import subprocess
import time

from conftest import ROOT, SAPF, run_sapf

GEO = "\\x [x 2 *] = _double\n\\x [x _double 1 +] = geoexp\n"


def results(out):
    return [line for line in out.split("compiled OK.\n")[-1].split("\n") if line and not line.startswith("done")]


def test_module_words(sapf_bin, tmp_path):
    (tmp_path / "geo.stac").write_text(GEO + "2 = two\n")
    out = run_sapf("import geo.*\n5 geoexp pr cr  two pr cr", tmp_path, lib=[])
    assert results(out)[:2] == ["11", "2"], out
    assert f"loading module geo from '{tmp_path / 'geo.stac'}'" in out


def test_private_names(sapf_bin, tmp_path):
    (tmp_path / "geo.stac").write_text(GEO)
    assert '"_double" is an undefined word' in run_sapf("import geo.*\n5 _double", tmp_path, lib=[])
    assert results(run_sapf("import geo._double\n5 _double pr cr", tmp_path, lib=[]))[0] == "10"
    assert results(run_sapf("import geo\n5 geo.geoexp pr cr", tmp_path, lib=[]))[0] == "11"


def test_a_module_starts_empty(sapf_bin, tmp_path):
    """It sees core words and its imports, nothing the importer bound."""
    (tmp_path / "leak.stac").write_text("\\x [x g +] = useg\n")
    pre = tmp_path / "pre.stac"
    pre.write_text("7 = g\n")
    out = run_sapf("import leak.*", tmp_path, lib=["-p", str(pre)])
    assert '"g" is an undefined word' in out and "error" in out


def test_a_modules_imports_stay_private(sapf_bin, tmp_path):
    (tmp_path / "tone.stac").write_text("import sc.*\n\\f [f sinosc] = tone\n")
    assert results(run_sapf("import tone.*\n440 tone pr cr", tmp_path, lib=[]))[0] == "SinOsc.ar"
    assert '"sinosc" is an undefined word' in run_sapf("import tone.*\n`sinosc", tmp_path, lib=[])


def test_def_binds_into_the_module(sapf_bin, tmp_path):
    (tmp_path / "defs.stac").write_text("def beep freq:440 = [freq sc.sinosc]\n")
    out = run_sapf("import defs.*\n`beep.write", tmp_path, lib=[])
    assert (tmp_path / "beep.scsyndef").exists(), out


def test_stac_path(sapf_bin, tmp_path, monkeypatch):
    lib = tmp_path / "lib"
    lib.mkdir()
    (lib / "geo.stac").write_text(GEO)
    work = tmp_path / "work"
    work.mkdir()
    assert "no namespace or module named geo" in run_sapf("import geo.*", work, lib=[])
    monkeypatch.setenv("STAC_PATH", f"/nonexistent:{lib}")
    assert results(run_sapf("import geo.*\n5 geoexp pr cr", work, lib=[]))[0] == "11"


def test_import_cycle(sapf_bin, tmp_path):
    (tmp_path / "a.stac").write_text("import b.*\n1 = x\n")
    (tmp_path / "b.stac").write_text("import a.*\n2 = y\n")
    out = run_sapf("import a.*", tmp_path, lib=[])
    assert "import cycle: a -> b -> a" in out and "error" in out


def test_a_failing_module_fails_the_import(sapf_bin, tmp_path):
    (tmp_path / "bad.stac").write_text("1 = x\nimport sc.*\n")
    out = run_sapf("import bad.*", tmp_path, lib=[])
    assert "import must come before" in out and "error" in out


def test_prelude_imports_std(sapf_bin, tmp_path):
    out = run_sapf("[1 2] prall  phi pr cr  440 sinosc pr cr", tmp_path, lib=["-p", str(ROOT / "prelude.stac")])
    assert results(out)[:4] == ["1", "2", "1.61803", "SinOsc.ar"], out


# reload, through the REPL as in a set: edit the file, import again

class Repl:
    def __init__(self, tmp_path, main):
        (tmp_path / "main.stac").write_text(main)
        env = dict(os.environ, STAC_PRELUDE="/dev/null", STAC_LOG=str(tmp_path / "log"))
        self.proc = subprocess.Popen([str(SAPF), "-q", "-i", "main.stac"], cwd=tmp_path, stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, env=env)
        time.sleep(1.1)  # file times have a 1 s resolution on some file systems

    def send(self, line):
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()
        time.sleep(.3)

    def finish(self):
        self.send("quit")
        return self.proc.communicate(timeout=5)[0]


def printed(out):
    """The numbers the session printed, in order."""
    return [line for line in out.splitlines() if line.lstrip("-").replace(".", "").isdigit()]


def test_reload_on_import_after_a_change(sapf_bin, tmp_path):
    (tmp_path / "geo.stac").write_text(GEO)
    repl = Repl(tmp_path, "import geo.*\n")
    repl.send("clear 5 geoexp pr cr")
    (tmp_path / "geo.stac").write_text(GEO.replace("2 *", "3 *"))
    repl.send("clear 5 geoexp pr cr")   # not imported again: geo's current version is still the old one
    repl.send("import geo")
    repl.send("clear 5 geoexp pr cr")   # the old binding's helper is looked up late, in geo's new version
    out = repl.finish()
    assert printed(out) == ["11", "11", "16"], out
    assert out.count("loading module geo") == 2


def test_reload_follows_a_changed_dependency(sapf_bin, tmp_path):
    (tmp_path / "base.stac").write_text("\\x [x 10 +] = bshift\n")
    (tmp_path / "mid.stac").write_text("import base.*\n\\x [x bshift] = useshift\n")
    repl = Repl(tmp_path, "import mid.*\n")
    repl.send("clear 1 useshift pr cr")
    (tmp_path / "base.stac").write_text("\\x [x 100 +] = bshift\n")
    repl.send("import mid.*")
    repl.send("clear 1 useshift pr cr")
    out = repl.finish()
    assert printed(out) == ["11", "101"], out
    assert out.count("loading module mid") == 2


def test_a_failed_reload_keeps_the_loaded_version(sapf_bin, tmp_path):
    (tmp_path / "geo.stac").write_text(GEO)
    repl = Repl(tmp_path, "import geo.*\n")
    (tmp_path / "geo.stac").write_text(GEO + "nope\n")
    repl.send("import geo.*")
    repl.send("clear 5 geoexp pr cr")
    out = repl.finish()
    assert '"nope" is an undefined word' in out
    assert printed(out) == ["11"], out
