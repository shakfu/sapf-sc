"""Runs nanosynth's embedded scsynth on a UDP port for sapf's play and stop.

    uv run python scripts/server.py [--port 57110]
"""

import argparse
import signal
import threading

from nanosynth import Options, Server


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--port", type=int, default=57110)
    args = parser.parse_args()
    stop = threading.Event()
    signal.signal(signal.SIGINT, lambda *_: stop.set())
    signal.signal(signal.SIGTERM, lambda *_: stop.set())
    with Server(Options(port=args.port)):
        print(f"scsynth on 127.0.0.1:{args.port}; Ctrl-C quits", flush=True)
        stop.wait()


if __name__ == "__main__":
    main()
