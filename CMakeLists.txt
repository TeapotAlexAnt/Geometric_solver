cmake_minimum_required(VERSION 3.14)
project(GeomSolverWASM LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate the compilation database for clang tooling")

add_library(cad_solver STATIC
    src/CoreAlgo.cpp
    src/Figure.cpp
    src/GeomRepository.cpp
    src/GeomSolver.cpp
    src/HelperFunctions.cpp
    src/Restrictions.cpp
    src/UseCases.cpp
)
target_include_directories(cad_solver PUBLIC include)
target_compile_features(cad_solver PUBLIC cxx_std_17)

if(EMSCRIPTEN)
    add_executable(geom_solver
        src/wasm/EmscriptenBindings.cpp
        src/wasm/SketchImport.cpp
    )
    target_link_libraries(geom_solver PRIVATE cad_solver)
    set_target_properties(geom_solver PROPERTIES SUFFIX ".mjs")
    target_compile_options(cad_solver PUBLIC
        -fexceptions
        $<$<CONFIG:Debug>:-g3>
    )
    if(GEOM_EMSCRIPTEN_SYSROOT)
        # Make the driver's implicit target and sysroot explicit in compile_commands.json.
        target_compile_options(cad_solver PUBLIC
            --target=wasm32-unknown-emscripten
            "--sysroot=${GEOM_EMSCRIPTEN_SYSROOT}"
            "SHELL:-Xclang -iwithsysroot/include/compat"
        )
    endif()
    target_link_options(geom_solver PRIVATE
        --no-entry
        --bind
        -fexceptions
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sENVIRONMENT=web
        -sALLOW_MEMORY_GROWTH=1
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']"
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:Debug>:-sASSERTIONS=2>
    )
else()
    add_executable(test_solver tests/test_solver.cpp)
    target_link_libraries(test_solver PRIVATE cad_solver)
    enable_testing()
    add_test(NAME solver COMMAND test_solver)
endif()

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format
    HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE format_sources CONFIGURE_DEPENDS
        "include/*.hpp" "src/*.cpp" "src/*.hpp" "tests/*.cpp")
    add_custom_target(format
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i ${format_sources}
        VERBATIM)
    add_custom_target(format-check
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${format_sources}
        VERBATIM)
endif()

find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy
    HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
if(CLANG_TIDY_EXECUTABLE AND CMAKE_EXPORT_COMPILE_COMMANDS)
    set(tidy_args -p "${CMAKE_BINARY_DIR}")
    if(EMSCRIPTEN)
        set(tidy_target geom_solver)
    else()
        set(tidy_target test_solver)
        if(APPLE AND NOT CMAKE_OSX_SYSROOT)
            # AppleClang finds the SDK implicitly. Clang-tidy needs the SDK path.
            execute_process(COMMAND xcrun --show-sdk-path
                OUTPUT_VARIABLE tidy_sdk OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            if(tidy_sdk)
                list(APPEND tidy_args "--extra-arg=-isysroot" "--extra-arg=${tidy_sdk}")
            endif()
        endif()
    endif()
    get_target_property(tidy_core_sources cad_solver SOURCES)
    get_target_property(tidy_entry_sources ${tidy_target} SOURCES)
    add_custom_target(tidy
        COMMAND "${CLANG_TIDY_EXECUTABLE}" ${tidy_args}
            ${tidy_core_sources} ${tidy_entry_sources}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DEPENDS ${tidy_target}
        USES_TERMINAL
        VERBATIM)
endif()
