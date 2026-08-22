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
# Qt location for the current local environment (Qt 6.11.0, MinGW 64-bit).
# This is the project starting point; it stays overridable so other
# environments can select a different Qt installation without editing files:
#     cmake -DCMAKE_PREFIX_PATH="<other Qt>/lib/cmake" ...
# -----------------------------------------------------------------------------
if(NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "E:/Qt/6.11.0/mingw_64/lib/cmake")
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
