cmake_minimum_required(VERSION 3.24)

project(
    Z3Workbench
    VERSION 0.1.0
    DESCRIPTION "Desktop workbench for the Z3 SMT solver"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Qt location.
#
# NOTE: the path below is an EXAMPLE pointing to one local machine's
# installation (Qt 6.11.0, MinGW 64-bit). Replace it with the path to YOUR
# Qt installation (the directory that contains lib/cmake), e.g.:
#     Windows: C:/Qt/6.7.2/mingw_64/lib/cmake
#     Linux:   /home/user/Qt/6.7.2/gcc_64/lib/cmake
#
# Editing this file is optional: the value can also be overridden from the
# command line without touching the sources:
#     cmake -DCMAKE_PREFIX_PATH="<your Qt>/lib/cmake" ...
# -----------------------------------------------------------------------------
if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "E:/Qt/6.11.0/mingw_64/lib/cmake") # EXAMPLE PATH — replace with your own Qt location
endif()

# Single-config generators need an explicit default; multi-config generators
# select the configuration at build time via --config.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CompilerWarnings)
include(CompilerOptions)
include(Dependencies)
include(Qt)
include(Z3)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
