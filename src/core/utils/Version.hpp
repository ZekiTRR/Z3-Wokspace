#pragma once

#include <string_view>

namespace z3wb {

inline constexpr std::string_view k_strAppName = "Z3 Workbench";
inline constexpr std::string_view k_strVersion = "0.1.0";

std::string_view appName();
std::string_view version();

} // namespace z3wb
