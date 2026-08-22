#include <doctest/doctest.h>

#include "core/parser/DslPrinter.hpp"
#include "core/parser/ProblemParser.hpp"
#include "core/serialization/ProblemExporter.hpp"
#include "core/serialization/SmtLib2Reader.hpp"
#include "core/serialization/SmtLib2Serializer.hpp"

#include <filesystem>
#include <fstream>

namespace {

z3wb::Problem parseOrDie(std::string_view svSource, const char* szName)
{
    z3wb::Problem oProblem(szName);
    std::vector<z3wb::Diagnostic> vecDiags;
    const bool bOk = z3wb::rebuildProblemFromSource(oProblem, svSource, vecDiags);
    REQUIRE(bOk);
    return oProblem;
}

std::string canonicalConstraints(const z3wb::Problem& oProblem)
{
    std::string strOut;
    for (const z3wb::Constraint& oConstraint : oProblem.constraints())
    {
        strOut += toString(oConstraint.expr);
        strOut += "\n";
    }
    return strOut;
}

std::string varSignature(const z3wb::Problem& oProblem)
{
    std::string strOut;
    for (const z3wb::Variable& oVariable : oProblem.variables())
    {
        strOut += oVariable.name + ":" + std::string(toString(oVariable.type));
        if (oVariable.type == z3wb::VariableType::BitVec)
        {
            strOut += "(" + std::to_string(oVariable.params.uBitVecWidth) + ")";
        }
        strOut += ";";
    }
    return strOut;
}

} // namespace

TEST_CASE("SMT-LIB2 export produces a complete document")
{
    using namespace z3wb;

    const Problem oProblem = parseOrDie(
        "var x: BitVec(32)\n"
        "var flag: Bool\n"
        "\n"
        "constraint (x ^ 0x1337) + 10 == 0x4242\n"
        "constraint flag == true\n", "crackme");

    const std::string strSmt = SmtLib2Serializer::serialize(oProblem);

    CHECK(strSmt.find("(declare-const x (_ BitVec 32))") != std::string::npos);
    CHECK(strSmt.find("(declare-const flag Bool)") != std::string::npos);
    // Unsigned bv ops and indexed literals.
    CHECK(strSmt.find("(bvxor x (_ bv4919 32))") != std::string::npos);
    CHECK(strSmt.find("(= (+ (bvxor x (_ bv4919 32)) (_ bv10 32)) (_ bv16962 32))")
        != std::string::npos);
    CHECK(strSmt.find("(check-sat)") != std::string::npos);
}

TEST_CASE("export -> import round-trip restores structure")
{
    using namespace z3wb;

    const Problem oOriginal = parseOrDie(
        "var x: BitVec(32)\n"
        "var y: Int\n"
        "var r: Real\n"
        "var s: String\n"
        "\n"
        "constraint ((x ^ 0x1337) + 10) == 0x4242\n"
        "constraint y * 2 - 1 >= -5\n"
        "constraint r / 2.5 <= 4.0\n"
        "constraint s != \"a\\\"b\"\n", "roundtrip");

    const std::string strSmt = SmtLib2Serializer::serialize(oOriginal);

    SmtLib2Reader oReader;
    std::optional<Problem> oRestored = oReader.read(strSmt, "restored", nullptr);
    REQUIRE(oRestored.has_value());

    CHECK(varSignature(*oRestored) == varSignature(oOriginal));

    // Constraint expressions must be structurally identical after the trip.
    // The imported problem carries unresolved refs; resolve both through the
    // DSL pipeline and compare canonical forms.
    const std::string strDsl = DslPrinter::printProblem(*oRestored);
    const Problem oReparsed = parseOrDie(strDsl, "reparsed");

    CHECK(canonicalConstraints(oReparsed) == canonicalConstraints(oOriginal));
}

TEST_CASE("DSL printer output re-parses to identical expressions")
{
    using namespace z3wb;

    const std::string strSource =
        "var a: Int\n"
        "var b: BitVec(8)\n"
        "\n"
        "constraint (a + 3) * -2 < 10\n"
        "constraint (b << 2) > b\n"
        "constraint (b & 0xF) != 0\n";

    const Problem oFirst = parseOrDie(strSource, "p1");
    const std::string strPrinted = DslPrinter::printProblem(oFirst);
    const Problem oSecond = parseOrDie(strPrinted, "p2");

    CHECK(canonicalConstraints(oSecond) == canonicalConstraints(oFirst));
    CHECK(varSignature(oSecond) == varSignature(oFirst));
}

TEST_CASE("reader reports unsupported operators with positions")
{
    using namespace z3wb;

    SmtLib2Reader oReader;
    StorageError oError;
    const std::optional<Problem> oResult = oReader.read(
        "(declare-const x Int)\n"
        "(assert (forall ((y Int)) (> y 0)))\n", "q", &oError);

    CHECK_FALSE(oResult.has_value());
    CHECK(oError.kind == StorageErrorKind::Format);
    CHECK(oError.message.find("forall") != std::string::npos);
    CHECK(oError.message.find("Line 2") != std::string::npos);
}

TEST_CASE("problem exporter writes all three formats")
{
    using namespace z3wb;

    const Problem oProblem = parseOrDie(
        "var x: Int\n"
        "constraint x >= 10\n"
        "constraint x <= 100\n", "export_me");

    const auto basePath = std::filesystem::temp_directory_path();

    SUBCASE("smt2 file")
    {
        const auto path = basePath / "z3wb_export.smt2";

        const ProblemExporter oExporter;
        CHECK_FALSE(oExporter.write(oProblem, ProblemExportFormat::SmtLib2, path).has_value());

        std::ifstream oStream(path, std::ios::binary);
        const std::string strContent((std::istreambuf_iterator<char>(oStream)),
            std::istreambuf_iterator<char>());
        CHECK(strContent.find("(assert (>= x 10))") != std::string::npos);
        std::error_code oError;
        std::filesystem::remove(path, oError);
    }

    SUBCASE("json file")
    {
        const auto path = basePath / "z3wb_export.json";

        const ProblemExporter oExporter;
        CHECK_FALSE(oExporter.write(oProblem, ProblemExportFormat::Json, path).has_value());

        std::ifstream oStream(path, std::ios::binary);
        const std::string strContent((std::istreambuf_iterator<char>(oStream)),
            std::istreambuf_iterator<char>());
        CHECK(strContent.find("\"name\": \"export_me\"") != std::string::npos);
        CHECK(strContent.find("(x >= 10)") != std::string::npos);
        std::error_code oError;
        std::filesystem::remove(path, oError);
    }

    SUBCASE("txt file")
    {
        const auto path = basePath / "z3wb_export.txt";

        const ProblemExporter oExporter;
        CHECK_FALSE(oExporter.write(oProblem, ProblemExportFormat::Txt, path).has_value());

        std::ifstream oStream(path, std::ios::binary);
        const std::string strContent((std::istreambuf_iterator<char>(oStream)),
            std::istreambuf_iterator<char>());
        CHECK(strContent.find("Problem: export_me") != std::string::npos);
        CHECK(strContent.find("[x] (x >= 10)") != std::string::npos);
        std::error_code oError;
        std::filesystem::remove(path, oError);
    }
}
