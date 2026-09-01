import json
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def test_pack_and_inspect() -> None:
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        elf = directory / "sample.elf"
        package = directory / "sample.opk"
        elf.write_bytes(b"\x7fELF" + b"test")
        subprocess.run(
            [sys.executable, str(ROOT / "tools/pack_plugin.py"), str(elf),
             "--id", "TEST00001", "--version", "1.00", "--output", str(package)],
            check=True,
        )
        with zipfile.ZipFile(package) as archive:
            assert set(archive.namelist()) == {"manifest.json", "plugin.elf"}
            assert archive.read("plugin.elf") == elf.read_bytes()
            assert json.loads(archive.read("manifest.json"))["id"] == "TEST00001"

