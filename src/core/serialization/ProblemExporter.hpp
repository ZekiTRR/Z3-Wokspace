#pragma once

#include "core/domain/Problem.hpp"
#include "core/serialization/JsonProjectStorage.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace z3wb {

enum class ProblemExportFormat
{
    SmtLib2, // portable SMT-LIB2 document (declarations + asserts + commands)
    Json,    // structured dump: name, source, variables, constraint displays
    Txt,     // human-readable listing
};

// Writes a single problem to a file in the requested format.
class ProblemExporter
{
public:
    [[nodiscard]] std::optional<StorageError> write(const Problem& oProblem,
        ProblemExportFormat eFormat, const std::filesystem::path& path) const;

    [[nodiscard]] static std::string render(const Problem& oProblem,
        ProblemExportFormat eFormat);
};

} // namespace z3wb
