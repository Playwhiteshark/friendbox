#!/usr/bin/env python3
import argparse
import hashlib
import json
import re
from pathlib import Path

VERSION_RE = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)$")
REPO_RE = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--firmware", required=True)
    p.add_argument("--tag", required=True)
    p.add_argument("--repo", required=True)
    p.add_argument("--output", required=True)
    args = p.parse_args()

    match = VERSION_RE.fullmatch(args.tag)
    if not match:
        raise SystemExit("tag must be vMAJOR.MINOR.PATCH")
    if not REPO_RE.fullmatch(args.repo):
        raise SystemExit("repo must be owner/name")

    firmware = Path(args.firmware)
    data = firmware.read_bytes()
    version = ".".join(match.groups())
    tag = f"v{version}"
    manifest = {
        "schema": 1,
        "version": version,
        "url": f"https://github.com/{args.repo}/releases/download/{tag}/firmware.bin",
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": len(data),
    }
    Path(args.output).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
