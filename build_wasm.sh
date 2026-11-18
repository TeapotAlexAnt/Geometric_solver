#!/usr/bin/env bash
set -e
echo "Сборка C++ ядра в WebAssembly через Emscripten (emcc)..."
mkdir -p web/dist
emcc -std=c++17 -O3 src/EmscriptenBindings.cpp \
  -Iinclude \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString']" \
  -o web/dist/geom_solver.js
echo "Готово: web/dist/geom_solver.js и web/dist/geom_solver.wasm"
