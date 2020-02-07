cmake_minimum_required(VERSION 3.16)
if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
            "In-source builds are not allowed.\n"
            "Please create a 'build' subfolder and use `cmake ..` inside it.\n"
            "NOTE: cmake will now create CMakeCache.txt and CMakeFiles/*.\n"
            "You must delete them, or cmake will refuse to work.")
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CheckTools)

project(dubby-dub
        VERSION 1.0.0
        LANGUAGES CXX)

if("${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

add_subdirectory(src)
