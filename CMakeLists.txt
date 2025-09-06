cmake_minimum_required(VERSION 4.0)
project(ray_tracer)

set(CMAKE_CXX_STANDARD 20)

# Add the executable first
set(SOURCE_FILES src/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Find OpenMP
find_package(OpenMP REQUIRED)

# Link OpenMP to the target
target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

