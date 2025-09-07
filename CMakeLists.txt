cmake_minimum_required(VERSION 4.0)
project(ray_tracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# add executable
set(SOURCE_FILES src/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# find packages
find_package(OpenMP REQUIRED)

# link to target
target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

