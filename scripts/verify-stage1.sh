#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build/cmake
cmake --build build/cmake
ctest --test-dir build/cmake --output-on-failure
