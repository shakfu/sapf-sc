"""The REPL colours its input line on a terminal, with tree-sitter-stac, unless NO_COLOR is set."""

import os
import select
import subprocess
import sys
import time

import pytest

from conftest import SAPF

pytestmark = pytest.mark.skipif(sys.platform == "win32", reason="needs a pty")


def type_line(line: str, **env_extra) -> str:
    """Types line into the REPL on a pty, a character at a time, then quits. Returns everything shown."""
    import pty
    master, slave = pty.openpty()
    env = {k: v for k, v in os.environ.items() if k != "NO_COLOR"}
    env.update(STAC_PRELUDE=os.devnull, STAC_LOG=os.devnull, STAC_HISTORY=os.devnull, TERM="xterm-256color")
    env.update(env_extra)
    proc = subprocess.Popen([str(SAPF), "-q"], stdin=slave, stdout=slave, stderr=slave, env=env, close_fds=True)
    os.close(slave)
    out = b""

    def pump(seconds):
        nonlocal out
        end = time.time() + seconds
        while time.time() < end:
            if select.select([master], [], [], 0.05)[0]:
                try:
                    out += os.read(master, 65536)
                except OSError:
                    return

    try:
        pump(1.5)
        for ch in line:
            os.write(master, ch.encode())
            pump(0.02)
        os.write(master, b"\r")
        pump(0.5)
        os.write(master, b"quit\r")
        pump(1)
    finally:
        proc.kill()
        proc.wait()
        os.close(master)
    return out.decode(errors="replace")


def test_repl_colours_by_syntax_and_binding(sapf_bin):
    out = type_line('12 "s" clear hlNowhere ; c')
    assert "\x1b[0;22;36m12" in out          # number: cyan
    assert '\x1b[0;22;32m"s"' in out         # string: green
    assert "\x1b[0;22;31mhlNowhere" in out   # unbound word: red
    assert "\x1b[0;90m; c" in out            # comment: grey
    assert "\x1b[0;22;31mquit" not in out    # quit is a command, not an unbound word


def test_library_words_are_blue_and_session_words_cyan(sapf_bin, tmp_path):
    prelude = tmp_path / "prelude.stac"
    prelude.write_text("1 = hlLib\n")
    out = type_line("hlLib clear 2 = hlMine hlMine", STAC_PRELUDE=str(prelude))
    assert "\x1b[0;94mhlLib" in out            # bound by the prelude: bright blue
    assert "\x1b[0;94mclear" in out            # builtin: bright blue
    assert "\x1b[0;96mhlMine" in out           # bound in the line: bright cyan


def test_ugens_are_bright_magenta(sapf_bin, tmp_path):
    prelude = tmp_path / "prelude.stac"
    prelude.write_text("import sc.*\n")
    out = type_line("440 sinosc play", STAC_PRELUDE=str(prelude))
    assert "\x1b[0;95msinosc" in out           # UGen: bright magenta
    assert "\x1b[0;94mplay" in out             # function: bright blue


def test_word_at_the_cursor_is_not_red(sapf_bin):
    out = type_line("12 hlNow")
    assert "\x1b[0;22;36m12" in out
    assert "\x1b[0;22;31mh" not in out      # each prefix was at the cursor
    out = type_line("hlNow 1")                 # the cursor has left the word
    assert "\x1b[0;22;31mhlNow" in out


def test_stac_colors_overrides_kinds(sapf_bin):
    out = type_line("12 clear", STAC_COLORS="number=91:library=01;4;32:nokind=31:string=bad")
    assert "\x1b[0;91m12" in out                 # bright red
    assert "\x1b[0;22;32;4;1mclear" in out        # underlined bold green
    assert "ignored 'nokind=31'" in out and "ignored 'string=bad'" in out


def test_no_color_turns_highlighting_off(sapf_bin):
    out = type_line('12 "s"', NO_COLOR="1")
    assert "\x1b[0;22;36m" not in out and "\x1b[0;22;32m" not in out
