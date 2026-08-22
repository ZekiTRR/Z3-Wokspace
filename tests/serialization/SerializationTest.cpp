#include <doctest/doctest.h>

#include "core/parser/ProblemParser.hpp"
#include "core/serialization/JsonProjectStorage.hpp"

#include <filesystem>
#include <fstream>

namespace {

struct TempFile
{
    std::filesystem::path path;

    explicit TempFile(const char* szName)
        : path(std::filesystem::temp_directory_path() / szName)
    {
        std::error_code oError;
        std::filesystem::remove(path, oError);
    }

    ~TempFile()
    {
        std::error_code oError;
        std::filesystem::remove(path, oError);
    }
};

z3wb::Project makeSampleProject()
{
    z3wb::Project oProject("Demo");

    z3wb::Problem oArithmetic("basic_arithmetic");
    {
        std::vector<z3wb::Diagnostic> vecDiags;
        REQUIRE(z3wb::rebuildProblemFromSource(oArithmetic,
            "var x: Int\n"
            "constraint x >= 10\n"
            "constraint x <= 100\n", vecDiags));
    }

    z3wb::Problem oCrackme("crackme_01");
    {
        std::vector<z3wb::Diagnostic> vecDiags;
        REQUIRE(z3wb::rebuildProblemFromSource(oCrackme,
            "var x: BitVec(32)\n"
            "constraint ((x ^ 0x1337) + 10) == 0x4242\n", vecDiags));
    }

    REQUIRE(oProject.adoptProblem(std::move(oArithmetic)) != nullptr);
    REQUIRE(oProject.adoptProblem(std::move(oCrackme)) != nullptr);
    return oProject;
}

} // namespace

TEST_CASE("project save -> load round-trip preserves names and sources")
{
    using namespace z3wb;

    const Project oOriginal = makeSampleProject();
    const TempFile oFile("z3wb_roundtrip.z3w");

    const JsonProjectStorage oStorage;

    const StorageOutcome oSaved = oStorage.save(oOriginal, oFile.path);
    REQUIRE_FALSE(oSaved.oError.has_value());

    const StorageOutcome oLoaded = oStorage.load(oFile.path);
    REQUIRE_FALSE(oLoaded.oError.has_value());
    REQUIRE(oLoaded.oProject.has_value());

    const Project& oRestored = *oLoaded.oProject;
    CHECK(oRestored.name() == "Demo");
    REQUIRE(oRestored.problems().size() == 2);

    const Problem* pCrackme = oRestored.findProblem("crackme_01");
    REQUIRE(pCrackme != nullptr);
    CHECK(pCrackme->sourceText().find("0x1337") != std::string::npos);
    CHECK(pCrackme->variableCount() == 1);
    CHECK(pCrackme->constraintCount() == 1);

    // The rebuilt problem is fully solvable again.
    CHECK(pCrackme->enabledConstraintCount() == 1);

    const Problem* pArithmetic = oRestored.findProblem("basic_arithmetic");
    REQUIRE(pArithmetic != nullptr);
    CHECK(pArithmetic->variableCount() == 1);
    CHECK(pArithmetic->constraintCount() == 2);
}

TEST_CASE("invalid JSON reports a format error")
{
    using namespace z3wb;

    const TempFile oFile("z3wb_broken.z3w");
    {
        std::ofstream oStream(oFile.path, std::ios::binary);
        oStream << "{ this is not json";
    }

    const JsonProjectStorage oStorage;
    const StorageOutcome oOutcome = oStorage.load(oFile.path);

    REQUIRE(oOutcome.oError.has_value());
    CHECK(oOutcome.oError->kind == StorageErrorKind::Format);
    CHECK_FALSE(oOutcome.oProject.has_value());
}

TEST_CASE("newer schema versions are rejected with a clear message")
{
    using namespace z3wb;

    const TempFile oFile("z3wb_future.z3w");
    {
        std::ofstream oStream(oFile.path, std::ios::binary);
        oStream << R"({"version": 999, "name": "future", "problems": []})";
    }

    const JsonProjectStorage oStorage;
    const StorageOutcome oOutcome = oStorage.load(oFile.path);

    REQUIRE(oOutcome.oError.has_value());
    CHECK(oOutcome.oError->kind == StorageErrorKind::Version);
    CHECK(oOutcome.oError->message.find("999") != std::string::npos);
}

TEST_CASE("stored sources that no longer parse are reported per problem")
{
    using namespace z3wb;

    const TempFile oFile("z3wb_stale_source.z3w");
    {
        std::ofstream oStream(oFile.path, std::ios::binary);
        // Valid JSON, but the DSL source contains a type error.
        oStream << "{\"version\":1,\"name\":\"p\",\"problems\":[{"
                << "\"name\":\"bad\",\"source\":\"var x: Int\\nconstraint x == true\"}]}";
    }

    const JsonProjectStorage oStorage;
    const StorageOutcome oOutcome = oStorage.load(oFile.path);

    REQUIRE(oOutcome.oError.has_value());
    CHECK(oOutcome.oError->kind == StorageErrorKind::Format);
    CHECK(oOutcome.oError->message.find("\"bad\"") != std::string::npos);
}

TEST_CASE("missing files and unwritable paths produce io errors")
{
    using namespace z3wb;

    const JsonProjectStorage oStorage;

    SUBCASE("missing file")
    {
        const auto path = std::filesystem::temp_directory_path()
            / "z3wb_does_not_exist.z3w";
        std::error_code oError;
        std::filesystem::remove(path, oError);

        const StorageOutcome oOutcome = oStorage.load(path);
        REQUIRE(oOutcome.oError.has_value());
        CHECK(oOutcome.oError->kind == StorageErrorKind::Io);
    }

    SUBCASE("unwritable directory")
    {
#ifdef _WIN32
        const std::filesystem::path path = "Q:\\definitely\\missing\\dir\\proj.z3w";
#else
        const std::filesystem::path path = "/proc/nonexistent-dir/proj.z3w";
#endif
        const StorageOutcome oOutcome = oStorage.save(makeSampleProject(), path);
        REQUIRE(oOutcome.oError.has_value());
        CHECK(oOutcome.oError->kind == StorageErrorKind::Io);
    }
}
