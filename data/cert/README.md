# OTA certificate bundle

`x509_crt_bundle.bin` contains the compact public-root format consumed by
Arduino-ESP32 2.x. PlatformIO embeds it directly in the application image and
`OtaUpdater::begin()` installs it before the first GitHub HTTPS request.

The committed bundle was generated from the 121 public Mozilla roots shipped
in Ubuntu's `ca-certificates` package `20260601~24.04.1`. Local/private trust
anchors are intentionally excluded. Its source directory was
`/usr/share/ca-certificates/mozilla` and the generated file contains no private
keys or credentials. The current generated binary is 55,762 bytes with SHA-256
`224317342c3d11abc38f2a4778aadb9d1d66dbfdd27dc8c72b931722ae77728a`.

To refresh it after reviewing an updated public root store:

```bash
python3 scripts/generate_ota_cert_bundle.py \
  /usr/share/ca-certificates/mozilla \
  data/cert/x509_crt_bundle.bin
./scripts/check.sh
```

The generated binary is committed so ordinary PlatformIO and GitHub Actions
builds do not require Python's `cryptography` package.
