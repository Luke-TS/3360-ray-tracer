cmake_minimum_required(VERSION 3.18)
project(ray_tracer LANGUAGES CXX CUDA)

# ─── C++ Settings ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── CUDA Settings ────────────────────────────────────────────────
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_ARCHITECTURES 86)

# ─── Collect sources (NO HEADERS) ─────────────────────────────────
file(GLOB_RECURSE CPP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE CUDA_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cu"
)

add_executable(${PROJECT_NAME} 
    ${CPP_SOURCES}
    ${CUDA_SOURCES}
)

# ─── Include paths ────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/external/tinyobjloader
)

# ─── Dependencies ────────────────────────────────────────────────
add_subdirectory(external/tinyobjloader)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# ─── Link libraries ────────────────────────────────────────────────
find_package(OpenMP REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenMP::OpenMP_CXX
        tinyobjloader
        nlohmann_json::nlohmann_json
)

# ─── Warnings ─────────────────────────────────────────────────────
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic>
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall>
)
