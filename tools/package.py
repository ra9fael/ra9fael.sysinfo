#!/usr/bin/env python3
"""Package this plugin as dist/<id>-<version>.wsplugin.

Usage:
  python tools/package.py --bin-dir . --out dist/ci
  python tools/package.py --bin-dir . --out dist/ci --tag 1.6.1

The archive holds plugin.json at the root, both READMEs, the built exe and the
licence of the embedded Phosphor font. Entry names are sorted and timestamps
fixed, so the same inputs produce the same zip. With --tag the tag must equal
plugin.json's version exactly.

Outputs in the target folder: <id>-<version>.wsplugin and its .sha256 sidecar.

Standard library only, Python 3.10+.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FIXED_TIME = (2020, 1, 1, 0, 0, 0)
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+$")
MAX_BYTES = 256 << 20  # WinShun's archive limit (src/plugin/Install.h)


def fail(msg: str) -> int:
    print(f"error: {msg}", file=sys.stderr)
    return 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tag", help="release tag; must equal the version in plugin.json")
    ap.add_argument("--out", default=str(ROOT / "dist"), help="output folder (default: dist/)")
    ap.add_argument("--bin-dir", required=True, help="folder holding the built program from runtime.program")
    args = ap.parse_args()

    manifest = json.loads((ROOT / "plugin.json").read_text(encoding="utf-8-sig"))
    pid, version = manifest.get("id", ""), manifest.get("version", "")
    if not VERSION_RE.fullmatch(version):
        return fail(f"plugin.json version {version!r} must be three numbers like 1.2.3")
    if args.tag is not None and args.tag != version:
        return fail(f"tag {args.tag!r} does not match plugin.json version {version!r}")

    program = str(manifest.get("runtime", {}).get("program", ""))
    if not program.lower().endswith(".exe"):
        return fail(f"runtime.program {program!r} is not an exe; this script packages native plugins")
    files = {
        "plugin.json": ROOT / "plugin.json",
        "README.md": ROOT / "README.md",
        "README.en.md": ROOT / "README.en.md",
        "licenses/phosphor.txt": ROOT / "fonts" / "PHOSPHOR-LICENSE",
        program: Path(args.bin_dir).resolve() / program,
    }
    missing = [str(path) for path in files.values() if not path.is_file()]
    if missing:
        return fail(f"required files are missing: {missing}")

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    target = out / f"{pid}-{version}.wsplugin"
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as archive:
        for name in sorted(files):
            info = zipfile.ZipInfo(name, FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, files[name].read_bytes())
    if target.stat().st_size > MAX_BYTES:
        target.unlink()
        return fail(f"package exceeds WinShun's {MAX_BYTES >> 20} MB limit")

    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    (out / f"{target.name}.sha256").write_text(f"{digest}  {target.name}\n", encoding="ascii")
    print(f"{target.name}: {len(files)} files, {target.stat().st_size} bytes, sha256 {digest}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
