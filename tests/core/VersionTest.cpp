#include <doctest/doctest.h>

#include "core/utils/Version.hpp"

TEST_CASE("application version follows the project version")
{
    CHECK(z3wb::k_strVersion == "0.1.0");
    CHECK(z3wb::version() == z3wb::k_strVersion);
    CHECK(z3wb::appName() == "Z3 Workbench");
}
