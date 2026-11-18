#pragma once

#include <emscripten/val.h>
#include <string>
#include <vector>

namespace cad {

class ApiHandler;

namespace wasm {

std::vector<int> readIds(const emscripten::val& array);
std::string loadSketch(ApiHandler& api, const emscripten::val& data);

} // namespace wasm
} // namespace cad
