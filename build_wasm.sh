#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mode="${1:-release}"
case "$mode" in
  debug) build_type=Debug ;;
  release) build_type=Release ;;
  *) echo "Usage: $0 [debug|release]" >&2; exit 2 ;;
esac

if ! command -v emcmake >/dev/null; then
  echo "Emscripten (emcmake) is required on PATH." >&2
  exit 1
fi

if ! command -v cmake >/dev/null; then
  echo "CMake is required on PATH." >&2
  exit 1
fi

build_dir="$root/.build/wasm/$mode"
export EM_CACHE="${EM_CACHE:-$root/.build/emscripten-cache}"

emcmake cmake -S "$root" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DGEOM_EMSCRIPTEN_SYSROOT="$(em-config CACHE)/sysroot"
cmake --build "$build_dir" --target geom_solver
