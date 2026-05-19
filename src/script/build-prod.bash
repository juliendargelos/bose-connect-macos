#!/usr/bin/env bash
set -ev

CURRENT_PATH=$(dirname "${0}")
cd "${CURRENT_PATH}" || exit

cd ..

rm -fR ./build

if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
else
  JOBS="$(sysctl -n hw.logicalcpu)"
fi

cmake \
  -S . \
  -B ./build \
  -DCMAKE_BUILD_TYPE=Release

cmake \
  --build ./build \
  --config Release \
  --parallel "${JOBS}"
