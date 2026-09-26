# Not a mod anyone installs: a build whose only product is the compiler's record layouts for the
# classes gdsolver reads by raw offset. tools/layout-probe/dump_layouts.py re-runs probe.cpp's
# compile command with -fdump-record-layouts, once per platform, so a Windows 2.2081 offset can
# be traced to the member that holds it and from there to the same member on iOS.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

project(layout_probe VERSION 1.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} SHARED probe.cpp)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
