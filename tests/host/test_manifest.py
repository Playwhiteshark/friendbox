#!/usr/bin/env python3
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as td:
    td = Path(td)
    fw = td / "firmware.bin"
    fw.write_bytes(b"friendbox-test-firmware")
    out = td / "manifest.json"
    subprocess.run([
        "python3", str(ROOT / "scripts/generate_manifest.py"),
        "--firmware", str(fw), "--tag", "v1.2.3",
        "--repo", "owner/friendbox", "--output", str(out)
    ], check=True)
    data = json.loads(out.read_text())
    assert data["schema"] == 1
    assert data["version"] == "1.2.3"
    assert data["size"] == fw.stat().st_size
    assert data["sha256"] == hashlib.sha256(fw.read_bytes()).hexdigest()
    assert data["url"].endswith("/releases/download/v1.2.3/firmware.bin")
print("manifest tests passed")
