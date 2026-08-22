#include <doctest/doctest.h>

#include "core/domain/SolverResult.hpp"

TEST_CASE("model lookup by variable name")
{
    using namespace z3wb;

    Model oModel;
    oModel.entries.push_back({"x", VariableType::Int, std::int64_t{42}});
    oModel.entries.push_back({"flag", VariableType::Bool, true});
    oModel.entries.push_back({"eax", VariableType::BitVec, BitVecValue{32, 0x12345678}});

    const ModelEntry* pX = oModel.find("x");
    REQUIRE(pX != nullptr);
    CHECK(std::get<std::int64_t>(pX->value) == 42);

    const ModelEntry* pEax = oModel.find("eax");
    REQUIRE(pEax != nullptr);
    CHECK(std::get<BitVecValue>(pEax->value) == BitVecValue{32, 0x12345678});

    CHECK(oModel.find("missing") == nullptr);
}

TEST_CASE("solver result factories carry diagnostics")
{
    using namespace z3wb;

    SolverResult oResult = SolverResult::makeError("Z3 failed to initialize");
    CHECK(oResult.status == SolverStatus::Error);
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics.front().severity == DiagnosticSeverity::Error);
    CHECK_FALSE(oResult.model.has_value());

    SolverResult oSat;
    oSat.status = SolverStatus::Sat;
    oSat.solveTime = std::chrono::milliseconds{12};
    oSat.model = Model{};
    CHECK(oSat.solveTime.count() == 12);
}
