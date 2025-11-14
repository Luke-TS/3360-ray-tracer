cmake_minimum_required(VERSION 3.18)

project(ray_tracer LANGUAGES CXX CUDA)


# ───────────────── Compiler Settings ──
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_ARCHITECTURES 86)

# Always export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ───────────────── Source Collection ─
file(GLOB_RECURSE CPP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
)

file(GLOB_RECURSE CUDA_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cu"
)

add_executable(${PROJECT_NAME}
    ${CPP_SOURCES}
    ${CUDA_SOURCES}
)

# ───────────────── Includes ─────
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/geom
        ${CMAKE_SOURCE_DIR}/src/integrator
        ${CMAKE_SOURCE_DIR}/src/material
        ${CMAKE_SOURCE_DIR}/src/renderer
        ${CMAKE_SOURCE_DIR}/src/scene
        ${CMAKE_SOURCE_DIR}/src/third-party
        ${CMAKE_SOURCE_DIR}/external/tinyobjloader
)

# ───────────────── Dependencies ────
add_subdirectory(external/tinyobjloader)

# JSON lib
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

find_package(OpenMP REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenMP::OpenMP_CXX
        tinyobjloader
        nlohmann_json::nlohmann_json
)

# ───────────────── Warning Levels ─
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall>
)

# ────────── Multi-Config Output Layout ──
# This puts executables in build/debug or build/release
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/relwithdebinfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/minsizerel"
)

# Also make sure compile_commands.json appears in each build directory
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(copy-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# ─────── Asset Directory Macro ──────────
target_compile_definitions(${PROJECT_NAME} PUBLIC
    IMAGE_DIR="${CMAKE_SOURCE_DIR}/textures"
)
