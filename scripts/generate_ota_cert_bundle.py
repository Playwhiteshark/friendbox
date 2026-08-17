#!/usr/bin/env python3
"""Generate the compact certificate format used by Arduino-ESP32 2.x.

The input may be one PEM bundle or a directory containing PEM certificates.
This is a maintainer tool; the generated binary is committed so normal builds
do not need the Python cryptography package.
"""
from __future__ import annotations

import argparse
import re
import struct
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization

PEM_CERTIFICATE = re.compile(
    rb"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", re.DOTALL
)


def certificate_bytes(path: Path):
    for match in PEM_CERTIFICATE.finditer(path.read_bytes()):
        yield match.group(0)


def load_certificates(source: Path) -> list[x509.Certificate]:
    paths = sorted(source.iterdir()) if source.is_dir() else [source]
    certificates: dict[bytes, x509.Certificate] = {}
    for path in paths:
        if not path.is_file():
            continue
        for pem in certificate_bytes(path):
            certificate = x509.load_pem_x509_certificate(pem)
            fingerprint = certificate.fingerprint(hashes.SHA256())
            certificates[fingerprint] = certificate
    if not certificates:
        raise ValueError(f"no PEM certificates found in {source}")
    return sorted(
        certificates.values(),
        key=lambda certificate: certificate.subject.public_bytes(),
    )


def build_bundle(certificates: list[x509.Certificate]) -> bytes:
    if len(certificates) > 0xFFFF:
        raise ValueError("too many certificates")
    output = bytearray(struct.pack(">H", len(certificates)))
    for certificate in certificates:
        subject = certificate.subject.public_bytes()
        public_key = certificate.public_key().public_bytes(
            serialization.Encoding.DER,
            serialization.PublicFormat.SubjectPublicKeyInfo,
        )
        if len(subject) > 0xFFFF or len(public_key) > 0xFFFF:
            raise ValueError("certificate entry is too large")
        output.extend(struct.pack(">HH", len(subject), len(public_key)))
        output.extend(subject)
        output.extend(public_key)
    return bytes(output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    certificates = load_certificates(args.source)
    bundle = build_bundle(certificates)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(bundle)
    print(f"wrote {len(certificates)} certificates ({len(bundle)} bytes) to {args.output}")


if __name__ == "__main__":
    main()
