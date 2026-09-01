#!/usr/bin/env python3
"""Create an OnionHEN plugin package: manifest.json + plugin.elf in a ZIP."""
import argparse
import json
import re
import struct
import sys
import zipfile
from pathlib import Path

TID_RE = re.compile(r"^[A-Za-z]{4}[0-9]{5}$")
VERSION_RE = re.compile(r"^[0-9]+\.[0-9]{2}$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("elf", type=Path)
    parser.add_argument("--id", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--name", default=None)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--abi", type=int, default=1)
    args = parser.parse_args()

    if not TID_RE.fullmatch(args.id):
        parser.error("plugin id must contain four letters followed by five digits")
    if not VERSION_RE.fullmatch(args.version):
        parser.error("version must use the x.xx format")
    data = args.elf.read_bytes()
    if len(data) < 4 or data[:4] != b"\x7fELF":
        parser.error(f"not an ELF file: {args.elf}")

    manifest = {
        "format": 1,
        "id": args.id,
        "name": args.name or args.elf.stem,
        "version": args.version,
        "abi": args.abi,
        "entry": "plugin.elf",
        "elf_size": len(data),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", compression=zipfile.ZIP_DEFLATED) as package:
        package.writestr("manifest.json", json.dumps(manifest, indent=2) + "\n")
        package.writestr("plugin.elf", data)
    print(f"packed {args.output} ({len(data)} byte ELF)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

