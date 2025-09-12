cmake_minimum_required(VERSION 4.0)
project(ray_tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/tinyobjloader)

add_definitions(-DPROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# add executable
set(SOURCE_FILES src/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# find packages
find_package(OpenMP REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/external/tinyobjloader)

# link to target
target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX tinyobjloader)

