#!/usr/bin/env python3
"""Copy a plugin ELF or package to an OnionHEN plugin directory."""
import argparse
import shutil
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("plugin", type=Path)
    parser.add_argument("--root", type=Path, default=Path("/data/OnionHEN/plugins"))
    args = parser.parse_args()
    if not args.plugin.is_file():
        parser.error(f"plugin does not exist: {args.plugin}")
    args.root.mkdir(parents=True, exist_ok=True)
    destination = args.root / args.plugin.name
    shutil.copy2(args.plugin, destination)
    print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

