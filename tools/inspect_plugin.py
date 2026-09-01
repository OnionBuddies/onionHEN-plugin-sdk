#!/usr/bin/env python3
"""Inspect an ELF descriptor section or an OnionHEN .opk package."""
import argparse
import json
import struct
import sys
import zipfile
from pathlib import Path


def inspect_package(path: Path) -> dict:
    with zipfile.ZipFile(path) as package:
        names = set(package.namelist())
        if {"manifest.json", "plugin.elf"} - names:
            raise ValueError("package must contain manifest.json and plugin.elf")
        manifest = json.loads(package.read("manifest.json"))
        elf = package.read("plugin.elf")
        if elf[:4] != b"\x7fELF":
            raise ValueError("plugin.elf is not an ELF")
        manifest["actual_elf_size"] = len(elf)
        return manifest


def inspect_elf(path: Path) -> dict:
    data = path.read_bytes()
    if data[:4] != b"\x7fELF" or data[4] != 2 or data[5] != 1:
        raise ValueError("expected a little-endian ELF64")
    # ELF64 header offsets: section table at 0x28, entry size/count/index at 0x3a.
    shoff = struct.unpack_from("<Q", data, 0x28)[0]
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x3A)
    strings = b""
    if shstrndx < shnum:
        off = shoff + shstrndx * shentsize
        _, _, _, _, section_off, section_size, *_ = struct.unpack_from("<IIQQQQIIQQ", data, off)
        strings = data[section_off:section_off + section_size]
    for index in range(shnum):
        off = shoff + index * shentsize
        fields = struct.unpack_from("<IIQQQQIIQQ", data, off)
        name_offset, _, _, _, section_off, section_size, *_ = fields
        end = strings.find(b"\0", name_offset)
        name = strings[name_offset:end].decode(errors="replace")
        if name == ".onion_plugin":
            raw = data[section_off:section_off + section_size]
            size, abi, caps, flags = struct.unpack_from("<IIII", raw, 0)
            plugin_id = raw[16:48].split(b"\0", 1)[0].decode()
            version = raw[48:64].split(b"\0", 1)[0].decode()
            plugin_name = raw[64:128].split(b"\0", 1)[0].decode()
            return {"struct_size": size, "abi": abi, "capabilities": caps,
                    "flags": flags, "id": plugin_id, "version": version,
                    "name": plugin_name}
    raise ValueError("ELF has no .onion_plugin section")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=Path)
    args = parser.parse_args()
    try:
        result = inspect_package(args.path) if args.path.suffix == ".opk" else inspect_elf(args.path)
    except (OSError, ValueError, zipfile.BadZipFile, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
