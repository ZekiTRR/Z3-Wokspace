# -----------------------------------------------------------------------------
# Warning policy.
#
# Compiler-specific flags live only here (and in CompilerOptions.cmake), so a
# future toolchain (e.g. MSVC) can be added without touching target files.
# Goal: zero warnings in project code.
# -----------------------------------------------------------------------------
option(Z3WORKBENCH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

function(configure_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
        if(Z3WORKBENCH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    else()
        # Other compilers are intentionally not configured yet.
        # Future MSVC support adds its block here only.
    endif()
endfunction()
