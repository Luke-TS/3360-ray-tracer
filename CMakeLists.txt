cmake_minimum_required(VERSION 4.0)
project(ray_tracer)

set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES main.cpp color.h vec3.h ray.h)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
