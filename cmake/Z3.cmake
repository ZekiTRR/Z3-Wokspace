# -----------------------------------------------------------------------------
# Z3 integration.
#
# Preferred layout is a project-local installation produced by
# scripts/bootstrap_z3.ps1 / scripts/bootstrap_z3.sh:
#
#     ThirdParty/Z3/source   pinned sources (git clone)
#     ThirdParty/Z3/build    CMake build tree
#     ThirdParty/Z3/install  include/ lib/ bin/
#
# The installed package provides the z3::libz3 target. All Z3-specific
# CMake knowledge is intentionally kept in this one file.
# -----------------------------------------------------------------------------
set(Z3WORKBENCH_Z3_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Z3/install"
    CACHE PATH "Project-local Z3 installation root")

list(APPEND CMAKE_PREFIX_PATH "${Z3WORKBENCH_Z3_ROOT}")

find_package(Z3 CONFIG QUIET)

if(NOT Z3_FOUND)
    message(FATAL_ERROR
        "\n"
        "Z3 was not found.\n"
        "Expected a project-local installation at:\n"
        "    ${Z3WORKBENCH_Z3_ROOT}\n"
        "\n"
        "Bootstrap it first:\n"
        "    PowerShell: scripts/bootstrap_z3.ps1\n"
        "    Bash:       scripts/bootstrap_z3.sh\n"
        "Or point to an existing installation:\n"
        "    cmake -DZ3WORKBENCH_Z3_ROOT=<existing Z3 install prefix>\n")
endif()

message(STATUS "Z3 ${Z3_VERSION_STRING}: ${Z3_DIR}")
