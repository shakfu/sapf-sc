"""The REPL's tab completion and hints, through a pseudo-terminal. Completion.cpp's rules are
unit-tested in tests/unit/test_completion.cpp; this checks they reach replxx."""

import os
import re
import select
import subprocess
import sys
import time

import pytest

from conftest import SAPF

pty = pytest.importorskip("pty")


@pytest.fixture
def repl(sapf_bin, tmp_path):
    # openpty and Popen, not pty.fork: forking a multi-threaded pytest can deadlock the child
    fd, tty = pty.openpty()
    env = dict(os.environ, STAC_PRELUDE="/dev/null", STAC_HISTORY=str(tmp_path / "history"),
               STAC_LOG=str(tmp_path / "log"), TERM="xterm")
    proc = subprocess.Popen([str(SAPF), "-q"], stdin=tty, stdout=tty, stderr=tty, env=env, start_new_session=True)
    os.close(tty)

    def read(seconds=1.0):
        out, end = b"", time.time() + seconds
        while time.time() < end:
            if select.select([fd], [], [], .05)[0]:
                try:
                    out += os.read(fd, 65536)
                except OSError:
                    break
        return re.sub(r"\x1b\[[0-9;?]*[A-Za-z]", "", out.decode(errors="replace"))

    def type_(keys):
        os.write(fd, b"\x15" + keys)  # ctrl-u clears the line first
        return read()

    read(2)
    yield type_
    os.write(fd, b"\x15quit\r")
    read(.5)
    proc.wait(timeout=5)
    os.close(fd)


@pytest.mark.skipif(sys.platform == "win32", reason="needs a pty")
@pytest.mark.parametrize("keys,shown", [
    (b"env.pe\t", "env.perc  (attackTime releaseTime --> env)"),  # completes, then hints
    (b"sc.sinosc", "sc.sinosc  (freq --> out)"),                   # hint for a whole word
    (b"pv_magf\t", "sc.pv_magfreeze"),                           # a plain prefix finds its namespace
])
def test_completion_and_hints(repl, keys, shown):
    assert shown in repl(keys)
