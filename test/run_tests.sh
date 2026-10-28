#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${ROOT_DIR}/build"

cmake --build "$BUILD_DIR" -j$(nproc)
"$BUILD_DIR/test/mikugl_test"
"$BUILD_DIR/lib/ranmath/test/ranmath_test"
