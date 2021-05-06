cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(EU4TimeNotifi VERSION 0.0.0)

find_package(OpenGL REQUIRED)

add_subdirectory(External)
add_subdirectory(EU4_CPP_Time)
add_subdirectory(EU4_CPP_Time/test)
