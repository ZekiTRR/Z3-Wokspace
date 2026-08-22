// -----------------------------------------------------------------------------
// End-to-end smoke test of the Z3 integration chain:
// the MinGW-built libz3 must link, initialize, solve and produce a model.
// This validates the whole bootstrap -> find_package -> z3::libz3 path early,
// before any domain or solver code exists.
// -----------------------------------------------------------------------------
#include <doctest/doctest.h>
#include <z3++.h>

#include <cstdint>

TEST_CASE("z3 solves a basic integer problem and returns a model")
{
    z3::context ctx;
    z3::solver oSolver(ctx);

    const z3::expr x = ctx.int_const("x");
    oSolver.add(x > 10);
    oSolver.add(x < 12);

    CHECK(oSolver.check() == z3::sat);

    const z3::model oModel = oSolver.get_model();
    CHECK(oModel.eval(x).as_int64() == 11);
}

TEST_CASE("z3 reports unsat for contradictory constraints")
{
    z3::context ctx;
    z3::solver oSolver(ctx);

    const z3::expr x = ctx.int_const("x");
    oSolver.add(x > 10);
    oSolver.add(x < 5);

    CHECK(oSolver.check() == z3::unsat);
}

TEST_CASE("z3 proves unsat for a nonlinear integer system")
{
    z3::context ctx;
    z3::solver oSolver(ctx);

    const z3::expr x = ctx.int_const("x");
    const z3::expr y = ctx.int_const("y");
    oSolver.add(x * x * y * y * y == ctx.int_val(static_cast<std::int64_t>(1152921504606846976ll)));
    oSolver.add(x * y + y * y == ctx.int_val(static_cast<std::int64_t>(68719476736ll)));

    CHECK(oSolver.check() == z3::unsat);
}

TEST_CASE("z3 respects the timeout and reports unknown")
{
    z3::context ctx;
    z3::solver oSolver(ctx);

    // Integer factoring of a large semiprime is hard for the arithmetic
    // engine; a 100 ms budget is far below the required search time, so Z3
    // must abort with unknown. This also exercises the timeout mechanism the
    // solver service will rely on.
    z3::params oParams(ctx);
    oParams.set(":timeout", 100u);
    oSolver.set(oParams);

    const z3::expr x = ctx.int_const("x");
    const z3::expr y = ctx.int_const("y");
    const auto uSemiprime = static_cast<std::int64_t>(999999937ll) * 999999733ll;
    oSolver.add(x * y == ctx.int_val(uSemiprime));
    oSolver.add(x > ctx.int_val(1));
    oSolver.add(y > ctx.int_val(1));

    CHECK(oSolver.check() == z3::unknown);
}

TEST_CASE("bit-vector arithmetic matches expected two-complement behavior")
{
    z3::context ctx;
    z3::solver oSolver(ctx);

    const z3::expr x = ctx.bv_const("x", 32);
    oSolver.add((x ^ ctx.bv_val(0x1337u, 32)) + ctx.bv_val(10u, 32) == ctx.bv_val(0x4242u, 32));

    REQUIRE(oSolver.check() == z3::sat);
    const z3::model oModel = oSolver.get_model();

    // ((x ^ 0x1337) + 10) == 0x4242  =>  x == (0x4238 ^ 0x1337)
    const unsigned uExpected = (0x4242u - 10u) ^ 0x1337u;
    CHECK(static_cast<unsigned>(oModel.eval(x).get_numeral_uint()) == uExpected);
}
