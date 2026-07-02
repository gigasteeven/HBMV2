cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(HBMV2 VERSION 1.0.0)

file(GLOB_RECURSE SOURCES
    src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable pointing to Geode root.")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# custom-keybinds (InvokeBindEvent) — pulled by geode-sdk installer as a mod dep,
# but we also want the headers at build time.
CPMAddPackage("gh:geode-sdk/custom-keybinds#v1.7.0")

setup_geode_mod(${PROJECT_NAME} EXTERNALS geode.custom-keybinds:1.7.0)
