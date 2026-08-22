#include "core/serialization/ProblemExporter.hpp"

#include "core/parser/DslPrinter.hpp"
#include "core/serialization/SmtLib2Serializer.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace z3wb {

namespace {

using Json = nlohmann::ordered_json;

std::string renderJson(const Problem& oProblem)
{
    Json oDoc;
    oDoc["version"] = k_iProjectSchemaVersion;
    oDoc["name"] = oProblem.name();
    oDoc["source"] = oProblem.sourceText();

    Json oVariables = Json::array();
    for (const Variable& oVariable : oProblem.variables())
    {
        Json oEntry;
        oEntry["name"] = oVariable.name;
        oEntry["type"] = toString(oVariable.type);
        if (oVariable.type == VariableType::BitVec)
        {
            oEntry["width"] = oVariable.params.uBitVecWidth;
        }
        oVariables.push_back(std::move(oEntry));
    }
    oDoc["variables"] = std::move(oVariables);

    Json oConstraints = Json::array();
    for (const Constraint& oConstraint : oProblem.constraints())
    {
        Json oEntry;
        oEntry["display"] = toString(oConstraint.expr);
        oEntry["enabled"] = oConstraint.enabled;
        oEntry["comment"] = oConstraint.comment;
        oEntry["line"] = oConstraint.location.iLine;
        oConstraints.push_back(std::move(oEntry));
    }
    oDoc["constraints"] = std::move(oConstraints);

    return oDoc.dump(4);
}

std::string renderTxt(const Problem& oProblem)
{
    std::string strOut;
    strOut += "Problem: " + oProblem.name() + "\n";

    strOut += "\nVariables:\n";
    for (const Variable& oVariable : oProblem.variables())
    {
        std::string strType(toString(oVariable.type));
        if (oVariable.type == VariableType::BitVec)
        {
            strType += "(" + std::to_string(oVariable.params.uBitVecWidth) + ")";
        }
        strOut += "  " + oVariable.name + " : " + strType + "\n";
    }
    if (oProblem.variables().empty())
    {
        strOut += "  (none)\n";
    }

    strOut += "\nConstraints:\n";
    for (const Constraint& oConstraint : oProblem.constraints())
    {
        strOut += oConstraint.enabled ? "  [x] " : "  [ ] ";
        strOut += toString(oConstraint.expr);
        if (!oConstraint.comment.empty())
        {
            strOut += "    // " + oConstraint.comment;
        }
        if (oConstraint.location.isValid())
        {
            strOut += "    (line " + std::to_string(oConstraint.location.iLine) + ")";
        }
        strOut += "\n";
    }
    if (oProblem.constraints().empty())
    {
        strOut += "  (none)\n";
    }

    return strOut;
}

} // namespace

std::string ProblemExporter::render(const Problem& oProblem,
    ProblemExportFormat eFormat)
{
    switch (eFormat)
    {
        case ProblemExportFormat::SmtLib2:
            return SmtLib2Serializer::serialize(oProblem);
        case ProblemExportFormat::Json:
            return renderJson(oProblem) + "\n";
        case ProblemExportFormat::Txt:
            return renderTxt(oProblem);
    }
    return {};
}

std::optional<StorageError> ProblemExporter::write(const Problem& oProblem,
    ProblemExportFormat eFormat, const std::filesystem::path& path) const
{
    std::ofstream oFile(path, std::ios::binary | std::ios::trunc);
    if (!oFile)
    {
        StorageError oError;
        oError.kind = StorageErrorKind::Io;
        oError.message = "Cannot open file for writing";
        return oError;
    }

    oFile << render(oProblem, eFormat);
    oFile.flush();
    if (!oFile)
    {
        StorageError oError;
        oError.kind = StorageErrorKind::Io;
        oError.message = "Failed while writing contents";
        return oError;
    }

    return std::nullopt;
}

} // namespace z3wb
