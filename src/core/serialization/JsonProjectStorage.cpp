#include "core/serialization/JsonProjectStorage.hpp"

#include "core/parser/ProblemParser.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <utility>

namespace z3wb {

namespace {

using Json = nlohmann::ordered_json;

StorageOutcome makeError(StorageErrorKind eKind, std::string strMessage)
{
    StorageOutcome oOutcome;
    oOutcome.oError = StorageError{eKind, std::move(strMessage)};
    return oOutcome;
}

Json encodeProject(const Project& oProject)
{
    Json oDoc;
    oDoc["version"] = k_iProjectSchemaVersion;
    oDoc["name"] = oProject.name();

    Json oProblems = Json::array();
    for (const Problem& oProblem : oProject.problems())
    {
        Json oEntry;
        oEntry["name"] = oProblem.name();
        oEntry["source"] = oProblem.sourceText();
        oProblems.push_back(std::move(oEntry));
    }
    oDoc["problems"] = std::move(oProblems);
    return oDoc;
}

// Runs one-step migrations until the document reaches the current schema.
// Extension pattern for a future version bump:
//
//   while (iVersion < k_iProjectSchemaVersion) {
//       switch (iVersion) {
//           case 1: /* v1 -> v2 */ break;
//           default: return std::nullopt; // unknown step: refuse
//       }
//       ++iVersion;
//   }
//   doc["version"] = iVersion;
[[nodiscard]] std::optional<StorageError> migrateDocument(Json&, int iVersion)
{
    if (iVersion > k_iProjectSchemaVersion)
    {
        StorageError oError;
        oError.kind = StorageErrorKind::Version;
        oError.message = "Project was written by a newer version of Z3 Workbench"
            " (schema " + std::to_string(iVersion) + ", supported up to "
            + std::to_string(k_iProjectSchemaVersion) + ")";
        return oError;
    }

    // Schema v1 is the current version: no migrations yet.
    return std::nullopt;
}

StorageOutcome loadImpl(const std::filesystem::path& path)
{
    std::ifstream oFile(path, std::ios::binary);
    if (!oFile)
    {
        return makeError(StorageErrorKind::Io,
            "Cannot open project file for reading");
    }

    std::ostringstream oBuffer;
    oBuffer << oFile.rdbuf();
    const std::string strContent = oBuffer.str();

    const Json oDoc = Json::parse(strContent, nullptr, false);
    if (oDoc.is_discarded())
    {
        return makeError(StorageErrorKind::Format,
            "Project file is not valid JSON");
    }
    if (!oDoc.is_object())
    {
        return makeError(StorageErrorKind::Format,
            "Project root must be a JSON object");
    }

    if (!oDoc.contains("version") || !oDoc["version"].is_number_integer())
    {
        return makeError(StorageErrorKind::Format,
            "Project file has no valid \"version\" field");
    }
    const int iVersion = oDoc["version"].get<int>();

    // Copy for migrations; today the chain is a pass-through.
    Json oMigrated = oDoc;
    if (const std::optional<StorageError> oError = migrateDocument(oMigrated, iVersion))
    {
        StorageOutcome oOutcome;
        oOutcome.oError = *oError;
        return oOutcome;
    }

    Project oProject(oMigrated.value("name", std::string("Untitled")));

    if (!oMigrated.contains("problems") || !oMigrated["problems"].is_array())
    {
        return makeError(StorageErrorKind::Format,
            "Project file has no \"problems\" array");
    }

    for (const Json& oEntry : oMigrated["problems"])
    {
        if (!oEntry.is_object() || !oEntry.contains("name")
            || !oEntry["name"].is_string() || !oEntry.contains("source")
            || !oEntry["source"].is_string())
        {
            return makeError(StorageErrorKind::Format,
                "Malformed problem entry (expected \"name\" and \"source\")");
        }

        Problem oProblem(oEntry["name"].get<std::string>());

        std::vector<Diagnostic> vecDiags;
        if (!rebuildProblemFromSource(
                oProblem, oEntry["source"].get<std::string>(), vecDiags))
        {
            std::string strReason = "source does not parse";
            if (!vecDiags.empty())
            {
                strReason = vecDiags.front().message;
            }
            return makeError(StorageErrorKind::Format,
                "Problem \"" + oProblem.name() + "\": " + strReason);
        }

        if (oProject.adoptProblem(std::move(oProblem)) == nullptr)
        {
            return makeError(StorageErrorKind::Format,
                "Duplicate problem name in project file");
        }
    }

    StorageOutcome oOutcome;
    oOutcome.oProject = std::move(oProject);
    return oOutcome;
}

} // namespace

StorageOutcome JsonProjectStorage::save(const Project& oProject,
    const std::filesystem::path& path) const
{
    std::ofstream oFile(path, std::ios::binary | std::ios::trunc);
    if (!oFile)
    {
        return makeError(StorageErrorKind::Io,
            "Cannot open project file for writing");
    }

    const std::string strContent = encodeProject(oProject).dump(4);
    oFile << strContent << "\n";
    oFile.flush();
    if (!oFile)
    {
        return makeError(StorageErrorKind::Io,
            "Failed while writing project contents");
    }

    return {};
}

StorageOutcome JsonProjectStorage::load(const std::filesystem::path& path) const
{
    return loadImpl(path);
}

} // namespace z3wb
