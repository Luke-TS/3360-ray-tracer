cmake_minimum_required(VERSION 3.18) # CUDA requires ≥ 3.18 for modern language support
project(ray_tracer LANGUAGES CXX CUDA)

# ─── Compiler settings ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CUDA settings (optional but recommended)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)  # allows linking device code across translation units
set(CMAKE_CUDA_ARCHITECTURES 86)          # adjust for your GPU (75=Turing, 86=Ampere, 89=Ada)

# ─── Dependencies ────────────────────────────────────────────────────
add_subdirectory(external/tinyobjloader)

# ─── Source files ────────────────────────────────────────────────
# Automatically collect all source files (.cpp, .cu, .h, etc.) under src/
file(GLOB_RECURSE SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cu"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# ─── OpenMP ──────────────────────────────────────────────────────────
find_package(OpenMP REQUIRED)

# ─── nlohmann/json (via FetchContent) ────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# ─── Includes ────────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/external/tinyobjloader
)

# ─── Link libraries ──────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenMP::OpenMP_CXX
        tinyobjloader
        nlohmann_json::nlohmann_json
)

# ─── Optional compile warnings ───────────────────────────────────────
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall>
)
