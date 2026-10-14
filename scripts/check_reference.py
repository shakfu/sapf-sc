"""Checks why make_reference.py would not update references, without writing any.

    SAPF_ORIG=/path/to/original/sapf uv run python scripts/check_reference.py [case names]

Reports, for the named cases (default: the op1_log cases): the checkout, which cases match, each
reference's mtime and git status, the sapf binary, and one render per case in a temp folder
compared with its reference.
"""

import os
import platform
import subprocess
import sys
import tempfile
import time
from datetime import datetime
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_reference import CASES, OUT, ROOT, TIMEOUT, read_wav_float, run_sapf, source  # noqa: E402

DEFAULT = ["op1_log", "op1_log2", "op1_log10"]


def sh(*cmd: str) -> str:
    r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    return (r.stdout + r.stderr).strip()


def main() -> None:
    names = sys.argv[1:] or DEFAULT
    print(f"== checkout {ROOT}")
    print(f"HEAD {sh('git', 'log', '-1', '--format=%h %s')}")
    print(f"changes: {sh('git', 'status', '--short') or 'none'}")

    print("\n== cases")
    cases = {c.name: c for c in CASES}
    for n in names:
        print(f"{n}: {cases[n].ref_expr or cases[n].expr}" if n in cases else f"{n}: NO SUCH CASE")

    print("\n== references")
    for n in names:
        p = OUT / f"{n}.wav"
        if p.exists():
            mtime = datetime.fromtimestamp(p.stat().st_mtime).isoformat(" ", "seconds")
            git = sh("git", "status", "--short", str(p)) or "unchanged in git"
            print(f"{p.name}: {p.stat().st_size} bytes, mtime {mtime}, {git}")
        else:
            print(f"{p.name}: MISSING")

    print("\n== sapf")
    sapf = os.environ.get("SAPF_ORIG", "")
    print(f"SAPF_ORIG={sapf or '(unset)'}")
    if not sapf or not os.access(sapf, os.X_OK):
        sys.exit("SAPF_ORIG is not an executable file")
    print(sh("file", sapf))
    if platform.system() == "Darwin":
        print(f"xattr: {sh('xattr', sapf) or 'none'}")

    print("\n== renders (temp folder; references untouched)")
    for n in names:
        if n not in cases:
            continue
        c = cases[n]
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "ref.sapf"
            src.write_text(source(c))
            print(f"-- {n}\n{source(c).rstrip()}")
            t = time.monotonic()
            try:
                out = run_sapf(sapf, src, tmp)
            except subprocess.TimeoutExpired:
                print(f"TIMEOUT after {TIMEOUT} s")
                continue
            print(f"exit {out.returncode} after {time.monotonic() - t:.1f} s")
            print(f"stdout: {out.stdout.strip() or '(empty)'}")
            print(f"stderr: {out.stderr.strip() or '(empty)'}")
            print(f"files: {' '.join(sorted(f.name for f in Path(tmp).iterdir()))}")
            log = Path(tmp) / "log"
            if log.exists():
                print(f"log: {log.read_text().strip() or '(empty)'}")
            got = Path(tmp) / "c0.wav"
            if not got.exists():
                continue
            frames = read_wav_float(got)
            print(f"c0.wav: {frames.shape[0]} frames, range {frames.min():.4f} .. {frames.max():.4f}")
            ref = OUT / f"{n}.wav"
            if ref.exists():
                r = read_wav_float(ref)
                if r.shape[0] != frames.shape[0]:
                    print(f"vs reference: {r.shape[0]} frames there")
                else:
                    print(f"vs reference: max difference {np.nanmax(np.abs(frames[:, :1] - r[:, :1])):.3g}")


if __name__ == "__main__":
    main()
