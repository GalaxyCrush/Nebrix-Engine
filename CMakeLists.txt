cmake_minimum_required(VERSION 3.24)
project(Nebrix VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(NEBRIX_BUILD_SANDBOX "Build the sandbox demo application" ON)

find_package(glfw3 3.4 REQUIRED)

add_subdirectory(engine)

if(NEBRIX_BUILD_SANDBOX)
    add_subdirectory(sandbox)
endif()
