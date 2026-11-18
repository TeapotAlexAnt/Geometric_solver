cmake_minimum_required(VERSION 3.14)
project(GeomSolverWASM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

if(EMSCRIPTEN)
    add_executable(geom_solver src/EmscriptenBindings.cpp)
    set_target_properties(geom_solver PROPERTIES
        LINK_FLAGS "-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString'] --bind"
    )
else()
    add_executable(test_solver tests/test_solver.cpp)
endif()
