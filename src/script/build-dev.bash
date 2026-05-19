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
  -DCMAKE_BUILD_TYPE=Debug \
  -DAUTOFORMATTING=True \
  -DVALIDATE_QA=True

cmake \
  --build ./build \
  --config Debug \
  --parallel "${JOBS}"

# Uncomment when the create the tests
# ctest -C Release
