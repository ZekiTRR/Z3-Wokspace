#pragma once

#include "core/domain/SourceLocation.hpp"
#include "core/domain/Values.hpp"
#include "core/domain/Variable.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace z3wb {

enum class SolverStatus
{
    Sat,
    Unsat,
    Unknown,
    Error,
};

struct ModelEntry
{
    std::string name;
    VariableType type = VariableType::Int;
    // Int -> int64; Real -> decimal string; Bool -> bool; BitVec -> bits;
    // String -> UTF-8. Mirrors Constant's data model on purpose: both are
    // solver-sorted values rendered by the same UI formatting code.
    std::variant<bool, std::int64_t, std::string, BitVecValue> value{std::int64_t{0}};
};

struct Model
{
    std::vector<ModelEntry> entries;

    [[nodiscard]] const ModelEntry* find(std::string_view svName) const;
};

enum class DiagnosticSeverity
{
    Info,
    Warning,
    Error,
};

struct Diagnostic
{
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string message;
    SourceLocation location{};
};

// Flat key/value statistics as reported by the backend; keys are backend
// specific ("conflicts", "decisions", ...) and shown in the advanced panel.
struct SolverStatistics
{
    std::vector<std::pair<std::string, std::string>> entries;
};

struct SolverResult
{
    SolverStatus status = SolverStatus::Unknown;
    // Present only when status == Sat.
    std::optional<Model> model;
    std::vector<Diagnostic> diagnostics;
    SolverStatistics statistics;
    std::chrono::milliseconds solveTime{0};

    [[nodiscard]] static SolverResult makeError(std::string strMessage);
};

} // namespace z3wb
