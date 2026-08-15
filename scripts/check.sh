#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

CXX="${CXX:-g++}"
"$CXX" -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror \
  -Ilib/FriendBoxCore/src \
  lib/FriendBoxCore/src/FriendBoxCore.cpp tests/host/test_core.cpp \
  -o /tmp/friendbox-core-tests
/tmp/friendbox-core-tests
"$CXX" -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror \
  -Iinclude tests/host/test_service_config.cpp \
  -o /tmp/friendbox-service-tests
/tmp/friendbox-service-tests
python3 tests/host/test_manifest.py
python3 scripts/validate_project.py
python3 scripts/validate_local_provisioning.py

if command -v pio >/dev/null 2>&1; then
  pio run -e friendbox
else
  echo "PlatformIO (pio) not found; host/repository checks passed, firmware build skipped." >&2
fi
