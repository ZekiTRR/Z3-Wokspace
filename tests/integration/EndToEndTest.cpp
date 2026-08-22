#include <doctest/doctest.h>

#include "core/domain/SolverResult.hpp"
#include "core/parser/ProblemParser.hpp"
#include "core/solver/AtomicCancellation.hpp"
#include "core/solver/ISolver.hpp"
#include "core/solver/SolverConfig.hpp"
#include "core/solver/z3/Z3Solver.hpp"

namespace {

z3wb::SolverResult solveText(std::string_view svSource,
    const z3wb::SolverConfig& oConfig = z3wb::SolverConfig{})
{
    const z3wb::ParseResult oParsed = z3wb::parseProblem(svSource, "integration");
    REQUIRE(oParsed.ok());

    const z3wb::Z3Solver oSolver;
    return oSolver.solve(*oParsed.problem, oConfig, std::make_shared<z3wb::NoCancellation>());
}

std::int64_t intValue(const z3wb::Model& oModel, const char* szName)
{
    const z3wb::ModelEntry* pEntry = oModel.find(szName);
    REQUIRE(pEntry != nullptr);
    return std::get<std::int64_t>(pEntry->value);
}

} // namespace

TEST_CASE("end-to-end: arithmetic problem solves with a consistent model")
{
    using namespace z3wb;

    const SolverResult oResult = solveText(
        "var x: Int\n"
        "var y: Int\n"
        "var flag: Bool\n"
        "constraint x >= 10\n"
        "constraint x <= 100\n"
        "constraint y == x + 20\n"
        "constraint flag == true\n");

    REQUIRE(oResult.status == SolverStatus::Sat);
    REQUIRE(oResult.model.has_value());

    CHECK(intValue(*oResult.model, "y") == intValue(*oResult.model, "x") + 20);

    const ModelEntry* pFlag = oResult.model->find("flag");
    REQUIRE(pFlag != nullptr);
    CHECK(std::get<bool>(pFlag->value));
}

TEST_CASE("end-to-end: contradictory constraints report unsat")
{
    using namespace z3wb;

    const SolverResult oResult = solveText(
        "var x: Int\n"
        "constraint x > 10\n"
        "constraint x < 5\n");

    CHECK(oResult.status == SolverStatus::Unsat);
    CHECK_FALSE(oResult.model.has_value());
}

TEST_CASE("end-to-end: timeout produces unknown with a reason")
{
    using namespace z3wb;

    SolverConfig oConfig;
    oConfig.timeout = std::chrono::milliseconds{100};

    // Integer factoring of a large semiprime does not finish in 100 ms.
    const SolverResult oResult = solveText(
        "var f1: Int\n"
        "var f2: Int\n"
        "constraint f1 > 1\n"
        "constraint f2 > 1\n"
        "constraint f1 * f2 == 999999670000016821\n",
        oConfig);

    CHECK(oResult.status == SolverStatus::Unknown);
    CHECK(oResult.solveTime.count() < 5000);
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("Reason:") != std::string::npos);
}

TEST_CASE("end-to-end: RE crackme finds the exact bit-vector value")
{
    using namespace z3wb;

    const SolverResult oResult = solveText(
        "var x: BitVec(32)\n"
        "constraint ((x ^ 0x1337) + 10) == 0x4242\n");

    REQUIRE(oResult.status == SolverStatus::Sat);
    REQUIRE(oResult.model.has_value());

    const ModelEntry* pX = oResult.model->find("x");
    REQUIRE(pX != nullptr);
    const BitVecValue oBits = std::get<BitVecValue>(pX->value);
    CHECK(oBits.uWidth == 32);

    // (0x4242 - 10) ^ 0x1337
    const std::uint64_t uExpected = (0x4242ull - 10ull) ^ 0x1337ull; // 0x510F
    CHECK(oBits.uBits == uExpected);
}

TEST_CASE("end-to-end: disabled constraints are excluded from solving")
{
    using namespace z3wb;

    z3wb::ParseResult oParsed = z3wb::parseProblem(
        "var x: Int\n"
        "constraint x > 10\n"
        "constraint x < 5\n", "toggle");

    REQUIRE(oParsed.ok());
    Problem& oProblem = *oParsed.problem;
    REQUIRE(oProblem.constraintCount() == 2);

    const z3wb::Z3Solver oSolver;
    const z3wb::SolverConfig oConfig;

    // Toggle through the id-based API.
    const ConstraintId oUpperBoundId = oProblem.constraints()[1].id;
    REQUIRE(oProblem.setConstraintEnabled(oUpperBoundId, false));

    const SolverResult oSat = oSolver.solve(oProblem, oConfig, std::make_shared<NoCancellation>());
    CHECK(oSat.status == SolverStatus::Sat);

    REQUIRE(oProblem.setConstraintEnabled(oUpperBoundId, true));

    const SolverResult oUnsat = oSolver.solve(oProblem, oConfig, std::make_shared<NoCancellation>());
    CHECK(oUnsat.status == SolverStatus::Unsat);
}

TEST_CASE("end-to-end: unsupported sort yields an error result")
{
    using namespace z3wb;

    Problem oProblem("arrays");
    Variable oArray;
    oArray.name = "a";
    oArray.type = VariableType::Array;
    REQUIRE(oProblem.addVariable(oArray));

    const z3wb::Z3Solver oSolver;
    const SolverResult oResult = oSolver.solve(oProblem, SolverConfig{},
        std::make_shared<NoCancellation>());

    CHECK(oResult.status == SolverStatus::Error);
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("not supported") != std::string::npos);
}

TEST_CASE("end-to-end: pre-cancelled request returns unknown without solving")
{
    using namespace z3wb;

    const z3wb::ParseResult oParsed = z3wb::parseProblem(
        "var x: Int\n"
        "constraint x > 10\n", "cancelled");

    REQUIRE(oParsed.ok());

    auto spCancel = std::make_shared<AtomicCancellation>();
    spCancel->cancel();

    const Z3Solver oSolver;
    const SolverResult oResult = oSolver.solve(*oParsed.problem, SolverConfig{}, spCancel);

    CHECK(oResult.status == SolverStatus::Unknown);
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("cancelled") != std::string::npos);
}

TEST_CASE("smt-lib2 rendering is complete and runnable")
{
    using namespace z3wb;

    const z3wb::ParseResult oParsed = z3wb::parseProblem(
        "var x: Int\n"
        "constraint x > 10\n"
        "constraint x < 20\n", "smt");

    REQUIRE(oParsed.ok());

    const z3wb::Z3Solver oSolver;
    const std::string strSmt = oSolver.toSmtLib2(*oParsed.problem, SolverConfig{});

    CHECK(strSmt.find("x") != std::string::npos);
    CHECK(strSmt.find("(assert") != std::string::npos);
    CHECK(strSmt.find("(check-sat)") != std::string::npos);
    CHECK(strSmt.find("(get-model)") != std::string::npos);
}

TEST_CASE("solver reports statistics after a run")
{
    using namespace z3wb;

    const SolverResult oResult = solveText(
        "var x: Int\n"
        "constraint x > 10\n");

    REQUIRE(oResult.status == SolverStatus::Sat);
    CHECK(oResult.solveTime.count() >= 0);
    // Statistics content is backend-dependent; just verify the plumbing.
    CHECK(oResult.statistics.entries.size() > 0);
}
