#pragma once

#include <string>

namespace z3wb {

// Position in user-authored source text. Kept in domain entities so
// diagnostics can always point back to what the user wrote.
struct SourceLocation
{
    int iLine = 0;   // 1-based; 0 means "not from source"
    int iColumn = 0; // 1-based; 0 means "unknown column"
    std::string strFile;

    [[nodiscard]] bool isValid() const noexcept { return iLine > 0; }
};

} // namespace z3wb
