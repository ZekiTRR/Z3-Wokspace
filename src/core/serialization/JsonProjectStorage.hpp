#pragma once

#include "core/domain/Project.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace z3wb {

enum class StorageErrorKind
{
    Io,      // file could not be read/written
    Format,  // content is not valid project JSON / failed validation
    Version, // written by an incompatible (newer) schema version
};

struct StorageError
{
    StorageErrorKind kind = StorageErrorKind::Io;
    std::string message;
};

struct StorageOutcome
{
    std::optional<Project> oProject;     // set after a successful load
    std::optional<StorageError> oError;  // set on failure
};

// Highest schema version this build writes. Loaders accept any older version
// through the migration chain and reject newer ones with a clear message.
inline constexpr int k_iProjectSchemaVersion = 1;

// Reads and writes the JSON-based .z3w project format.
//
// Schema v1 keeps only problem names and their DSL source texts: the parsed
// variables/constraints are rebuilt on load through the real parser, so the
// file can never drift from the language implementation. Expression dumps
// were deliberately left out of the format.
class JsonProjectStorage
{
public:
    [[nodiscard]] StorageOutcome save(const Project& oProject,
        const std::filesystem::path& path) const;

    [[nodiscard]] StorageOutcome load(const std::filesystem::path& path) const;
};

} // namespace z3wb
