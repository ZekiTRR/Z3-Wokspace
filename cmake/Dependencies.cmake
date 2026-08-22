# -----------------------------------------------------------------------------
# Third-party dependencies fetched at configure time.
#
# All external versions are pinned and recorded in DEPENDENCIES.md.
# When updating a pin here, update DEPENDENCIES.md in the same change.
# -----------------------------------------------------------------------------
include(FetchContent)

# Lightweight C++ testing framework for unit and integration tests.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(doctest)
