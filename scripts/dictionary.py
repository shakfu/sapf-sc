"""The stac dictionary: one entry per word, in docs/dictionary/*.md.

Each entry is a `### name` heading, a generated line with the word's canonical name and stack effect,
then hand-written text: what the word does, why to use it, and an example in a ```stac block. The
words come from `stac --words`, so the dictionary cannot miss one.

    uv run python scripts/dictionary.py sync     # add stubs for new words, refresh generated lines
    uv run python scripts/dictionary.py check    # report missing, stale and unwritten entries
    uv run python scripts/dictionary.py examples # run the examples
    ... --chapter NAME limits any command to one chapter; sync then writes only that file.

In an example, `code ;-> values` asserts what the line leaves: it runs as `[code]` and must print
`[values]`. tests/py/test_dictionary.py runs every example.

Chapters: core.md (core words and interpreter namespaces), syntax.md (statements, syntax and methods,
which are not words), std.md, sc.md, sapf.md.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import socket
import subprocess
import sys
import tempfile
import threading
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DICT = ROOT / 'docs' / 'dictionary'
STAC = Path(os.environ.get('STAC_BIN', ROOT / 'build' / 'stac'))
STUB = 'TODO'
CHAPTERS = {
    'core': 'Core words: the language, its lists and streams, patterns, and server objects.',
    'syntax': 'Statements, syntax and methods: what stac reads but does not bind as words.',
    'std': "The module std, stac's standard library; prelude.stac imports it.",
    'sc': "SuperCollider's UGens, in the namespace sc; prelude.stac imports them.",
    'sapf': "sapf's words, in the namespace sapf, and sapf.stac's definitions over them.",
}
# Not words, so not in --words: each needs an entry in syntax.md.
NON_WORDS = {
    'statements': ['def', 'synthdef', 'const', 'import'],
    'syntax': ['=', '\\', '[ ]', '{ }', '( )', '#[ ]', "'", '`', ',', '.', '@', ':', ';', '"', 'op/ op\\ op^'],
    'methods': ['.add', '.ar', '.done', '.end', '.free', '.ir', '.kr', '.pdef', '.play', '.poll', '.release',
                '.releaseIn', '.set', '.stop', '.write'],
}


@dataclass
class Word:
    name: str
    section: str
    effect: str
    help: str
    source: str

    def chapter(self) -> str:
        if self.name in NON_WORDS['statements']:  # synthdef is also the namespace of synthdef.new
            return 'syntax'
        if self.name.startswith('sc.'):
            return 'sc'
        if self.name.startswith('sapf.') or self.source == 'prelude':
            return 'sapf'
        if self.source == 'module':
            return self.name.split('.')[0]
        return 'core'

    def section_title(self) -> str:
        if self.name in NON_WORDS['statements']:
            return 'statements'
        if self.name.startswith('sc.'):
            return self.section.removeprefix('sc: ') if self.section != 'sc' else 'uncategorised'
        return self.section or 'other'

    def line(self) -> str:
        """The generated line under the heading."""
        return f'`{self.name}{" " + self.effect if self.effect else ""}`'


def words(stac: Path = STAC) -> list[Word]:
    env = dict(os.environ, STAC_PRELUDE='/dev/null')
    out = subprocess.run([str(stac), '--words', '-p', str(ROOT / 'prelude.stac'), '-p', str(ROOT / 'sapf.stac')],
                         capture_output=True, text=True, env=env, timeout=60, check=True).stdout
    return [Word(w['name'], w['section'], w['effect'], w['help'], w['source']) for w in json.loads(out)]


@dataclass
class Entry:
    name: str
    lines: list[str] = field(default_factory=list)  # after the heading, up to the next heading

    def todo(self) -> bool:
        return any(STUB in line for line in self.lines)

    def examples(self) -> list[str]:
        blocks, block = [], None
        for line in self.lines:
            if line.strip() == '```stac':
                block = []
            elif line.strip() == '```' and block is not None:
                blocks.append('\n'.join(block))
                block = None
            elif block is not None:
                block.append(line)
        return blocks


@dataclass
class Chapter:
    path: Path
    head: list[str] = field(default_factory=list)          # before the first section
    sections: list[tuple[str, list[Entry]]] = field(default_factory=list)

    @classmethod
    def read(cls, path: Path) -> Chapter:
        ch = cls(path)
        if not path.exists():
            name = path.stem
            ch.head = [f'# {name}', '', CHAPTERS[name], '']
            return ch
        entries: list[Entry] | None = None
        for line in path.read_text().split('\n'):
            if line.startswith('## '):
                entries = []
                ch.sections.append((line[3:].strip(), entries))
            elif line.startswith('### ') and entries is not None:
                entries.append(Entry(line[4:].strip()))
            elif entries:
                entries[-1].lines.append(line)
            elif entries is None:
                ch.head.append(line)
        return ch

    def entries(self) -> dict[str, Entry]:
        return {e.name: e for _, es in self.sections for e in es}

    def section(self, title: str) -> list[Entry]:
        for t, es in self.sections:
            if t == title:
                return es
        self.sections.append((title, []))
        return self.sections[-1][1]

    def text(self) -> str:
        out = list(self.head)
        while out and not out[-1].strip():
            out.pop()
        out.append('')
        for title, es in self.sections:
            out += [f'## {title}', '']
            for e in es:
                body = list(e.lines)
                while body and not body[-1].strip():
                    body.pop()
                out += [f'### {e.name}'] + body + ['']
        return '\n'.join(out).rstrip('\n') + '\n'


def stub(line: str) -> list[str]:
    return ['', line, '', f'{STUB}: what it does.', '', f'{STUB}: why and when to use it.', '',
            f'{STUB}: an example.']


def set_line(e: Entry, line: str) -> bool:
    """Puts the generated line first under the heading; True when it changed."""
    for i, text in enumerate(e.lines):
        if text.strip():
            if text.startswith('`') and text.strip() != line and _same_word(text, e.name):
                e.lines[i] = line
                return True
            if text.startswith('`') and _same_word(text, e.name):
                return False
            break
    e.lines[:0] = ['', line]
    return True


def _same_word(text: str, name: str) -> bool:
    return text.startswith(f'`{name}`') or text.startswith(f'`{name} ')


def chapters() -> dict[str, Chapter]:
    return {name: Chapter.read(DICT / f'{name}.md') for name in CHAPTERS}


def sync(ws: list[Word], chs: dict[str, Chapter]) -> tuple[int, int]:
    added = changed = 0
    for w in ws:
        ch = chs.setdefault(w.chapter(), Chapter.read(DICT / f'{w.chapter()}.md'))
        e = ch.entries().get(w.name)
        if e is None:
            ch.section(w.section_title()).append(Entry(w.name, stub(w.line())))
            added += 1
        elif set_line(e, w.line()):
            changed += 1
    syn = chs['syntax']
    for title, names in NON_WORDS.items():
        have = syn.entries()
        for n in names:
            if n not in have:
                syn.section(title).append(Entry(n, stub(f'`{n}`')))
                added += 1
    return added, changed


def problems(ws: list[Word], chs: dict[str, Chapter]) -> dict[str, list[str]]:
    """missing: words with no entry; unknown: entries for no word; todo: entries not yet written."""
    known = {w.name for w in ws} | {n for names in NON_WORDS.values() for n in names}
    entries = {n: e for ch in chs.values() for n, e in ch.entries().items()}
    return {
        'missing': sorted(known - entries.keys()),
        'unknown': sorted(entries.keys() - known),
        'todo': sorted(n for n, e in entries.items() if e.todo()),
    }


EXPECT = re.compile(r'^(.*?)\s*;->(.*)$')


def program(block: str) -> tuple[str, list[str]]:
    """The block as stac source: each `code ;-> values` line runs inside [ ] and prints after a marker."""
    lines, expected = [], []
    for line in block.split('\n'):
        m = EXPECT.match(line)
        if m and m.group(1).strip():
            lines.append(f'"@@{len(expected)}" pr cr [{m.group(1)}] pr cr')
            expected.append(m.group(2).strip())
        else:
            lines.append(line)
    return '\n'.join(lines), expected


def sc_summaries() -> dict[str, str]:
    """Each SC class's one-line summary from its help file, by lower-case class name."""
    out = {}
    for f in (ROOT / 'build' / '_deps' / 'supercollider-src' / 'HelpSource' / 'Classes').glob('*.schelp'):
        m = re.search(r'^summary::\s*(.*)$', f.read_text(errors='replace'), flags=re.M)
        if m:
            out[f.stem.lower()] = m.group(1).strip()
    return out


class StandInServer:
    """A UDP endpoint in place of scsynth: answers /sync and drops everything else, so examples that
    play run without sound and without waiting on a real server."""

    def __init__(self):
        from nanosynth.osc import OscMessage
        self.OscMessage = OscMessage
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 0))
        self.sock.settimeout(.1)
        self.port = self.sock.getsockname()[1]
        self.done = False
        self.thread = threading.Thread(target=self.serve, daemon=True)
        self.thread.start()

    def serve(self):
        while not self.done:
            try:
                data, addr = self.sock.recvfrom(65536)
            except socket.timeout:
                continue
            if not data.startswith(b'#bundle') and self.OscMessage.from_datagram(data).address == '/sync':
                self.sock.sendto(self.OscMessage('/synced', 1).to_datagram(), addr)

    def close(self):
        self.done = True
        self.thread.join()
        self.sock.close()


def run_examples(chs: dict[str, Chapter], env: dict | None = None, stac: Path = STAC) -> list[str]:
    """Runs every written example: the sapf chapter after sapf.stac, the others after prelude.stac only.
    Each runs in its own workspace layer, so its bindings and imports do not reach the next; files it
    writes go to a temporary directory. Returns one message per failure. With STAC_SCSYNTH in env,
    each example starts after `stop`."""
    return _run_examples(chs, dict(env or os.environ), stac)


def _run_examples(chs: dict[str, Chapter], env: dict, stac: Path) -> list[str]:
    failures = []
    for libs, names in ((['prelude.stac'], [n for n in chs if n != 'sapf']),
                        (['prelude.stac', 'sapf.stac'], [n for n in chs if n == 'sapf'])):
        blocks = [(f'{name}.md {word}', block) for name in names for word, e in chs[name].entries().items()
                  for block in e.examples() if STUB not in block]
        failures += run_blocks(blocks, libs, env, stac)
    return failures


def page_blocks(path: Path) -> tuple[str, list[tuple[str, str]]]:
    """A page's ```stac-setup code, and its ```stac blocks labelled by the heading above each."""
    setup, blocks, label, block, kind = [], [], path.name, None, None
    for line in path.read_text().split('\n'):
        if line.startswith('#'):
            label = f'{path.name} {line.lstrip("#").strip()}'
        elif line.strip() in ('```stac', '```stac-setup') and block is None:
            block, kind = [], line.strip()
        elif line.strip() == '```' and block is not None:
            (setup.extend(block) if kind == '```stac-setup' else blocks.append((label, '\n'.join(block))))
            block = None
        elif block is not None:
            block.append(line)
    return '\n'.join(setup), blocks


def with_setup(block: str, setup: str) -> str:
    """The block with the setup after its imports, which must come first."""
    lines = block.split('\n')
    n = 0
    while n < len(lines) and (lines[n].startswith('import ') or not lines[n].strip()):
        n += 1
    return '\n'.join(lines[:n] + ([setup] if setup else []) + lines[n:])


def run_blocks(blocks: list[tuple[str, str]], libs=('prelude.stac',), env: dict | None = None,
               stac: Path = STAC) -> list[str]:
    """Runs each (label, code) block as run_examples does; returns one message per failure."""
    if env is None or 'STAC_SYNTHDEFS' not in env:
        out_dir = tempfile.TemporaryDirectory()
        env = dict(env or os.environ, STAC_PRELUDE='/dev/null', STAC_SYNTHDEFS=out_dir.name,
                   STAC_RECORDINGS=out_dir.name)
        server = None
        if 'STAC_SCSYNTH' not in env:
            server = StandInServer()
            env['STAC_SCSYNTH'] = f'127.0.0.1:{server.port}'
        try:
            return run_blocks(blocks, libs, env, stac)
        finally:
            if server:
                server.close()
            out_dir.cleanup()
    failures = []
    if blocks:
        plan = []
        with tempfile.TemporaryDirectory() as tmp:
            driver = []
            for label, block in blocks:
                src, expected = program(block)
                f = Path(tmp) / f'ex{len(plan)}.stac'
                f.write_text(src + '\n')
                reset = 'stop ' if 'STAC_SCSYNTH' in env else ''
                driver.append(f'"@@block {len(plan)}" pr cr clear {reset}pushWorkspace "{f}" load popWorkspace')
                plan.append((label, expected))
            (Path(tmp) / 'driver.stac').write_text('\n'.join(driver) + '\n')
            args = [str(stac), '-q'] + [a for lib in libs for a in ('-p', str(ROOT / lib))] + [str(Path(tmp) / 'driver.stac')]
            out = subprocess.run(args, capture_output=True, text=True, env=env, timeout=300).stdout
        parts = re.split(r'^@@block (\d+)\n', out, flags=re.M)
        seen = {int(parts[i]): parts[i + 1] for i in range(1, len(parts) - 1, 2)}
        for i, (label, expected) in enumerate(plan):
            text = seen.get(i)
            if text is None:
                failures.append(f'{label}: did not run')
                continue
            for line in text.split('\n'):
                if line.startswith('error') or line.startswith('syntax error'):
                    failures.append(f'{label}: {line}')
            for k, want in enumerate(expected):
                m = re.search(rf'^@@{k}\n(.*)$', text, flags=re.M)
                got = m.group(1) if m else '(nothing)'
                if got != f'[{want}]':
                    failures.append(f'{label}: expected [{want}], got {got}')
    return failures


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('command', choices=['sync', 'check', 'examples'])
    ap.add_argument('--chapter', choices=list(CHAPTERS))
    args = ap.parse_args(argv)
    ws = words()
    chs = chapters()
    if args.chapter:
        ws = [w for w in ws if w.chapter() == args.chapter]
    if args.command == 'examples':
        failures = run_examples({args.chapter: chs[args.chapter]} if args.chapter else chs)
        print('\n'.join(failures) or 'all examples pass')
        return 1 if failures else 0
    if args.command == 'sync':
        added, changed = sync(ws, chs)
        DICT.mkdir(parents=True, exist_ok=True)
        for name, ch in chs.items():
            if not args.chapter or name == args.chapter:
                ch.path.write_text(ch.text())
        print(f'{added} stubs added, {changed} generated lines refreshed')
        return 0
    if args.chapter:
        chs = {args.chapter: chs[args.chapter], **{n: c for n, c in chs.items() if n != args.chapter}}
    probs = problems(ws, chs)
    if args.chapter:
        probs['unknown'] = [n for n in probs['unknown'] if n in chs[args.chapter].entries()]
        probs['todo'] = [n for n in probs['todo'] if n in chs[args.chapter].entries()]
    stale = sum(1 for w in ws if (e := chs[w.chapter()].entries().get(w.name)) and set_line(e, w.line()))
    for kind, names in probs.items():
        print(f'{kind}: {len(names)}' + (f' ({", ".join(names[:10])}{"..." if len(names) > 10 else ""})' if names else ''))
    print(f'stale generated lines: {stale}')
    return 1 if probs['missing'] or probs['unknown'] or stale else 0


if __name__ == '__main__':
    sys.exit(main())
