#pragma once

#include "core/domain/Constraint.hpp"
#include "core/domain/Ids.hpp"
#include "core/domain/Variable.hpp"

#include <string>
#include <vector>

namespace z3wb {

// A solvable unit: a named set of variables and constraints.
// Invariants: variable names are unique; constraint expressions only reference
// variables of this problem (enforced later by the semantic analyzer).
class Problem
{
public:
    // Default construction yields a placeholder with an invalid id; real
    // problems are created through Project::addProblem or this constructor.
    Problem() = default;
    explicit Problem(std::string strName);

    [[nodiscard]] ProblemId id() const noexcept { return m_id; }
    [[nodiscard]] const std::string& name() const noexcept { return m_strName; }
    void setName(std::string strName) { m_strName = std::move(strName); }

    // Adds a variable; assigns a fresh id when unset. Returns false when the
    // name is missing, invalid, or already taken — the caller surfaces that
    // as a validation message instead of an exception path.
    [[nodiscard]] bool addVariable(Variable oVariable);
    [[nodiscard]] bool removeVariable(VariableId oId);

    [[nodiscard]] const Variable* findVariable(std::string_view svName) const;
    [[nodiscard]] const Variable* findVariable(VariableId oId) const;

    [[nodiscard]] const std::vector<Variable>& variables() const noexcept { return m_vecVariables; }
    [[nodiscard]] std::size_t variableCount() const noexcept { return m_vecVariables.size(); }

    // Adds a constraint; assigns a fresh id when unset.
    [[nodiscard]] bool addConstraint(Constraint oConstraint);
    [[nodiscard]] bool removeConstraint(ConstraintId oId);
    [[nodiscard]] bool setConstraintEnabled(ConstraintId oId, bool bEnabled);

    [[nodiscard]] const Constraint* findConstraint(ConstraintId oId) const;
    [[nodiscard]] const std::vector<Constraint>& constraints() const noexcept { return m_vecConstraints; }
    [[nodiscard]] std::size_t constraintCount() const noexcept { return m_vecConstraints.size(); }
    [[nodiscard]] std::size_t enabledConstraintCount() const;

private:
    ProblemId m_id;
    std::string m_strName;
    std::vector<Variable> m_vecVariables;
    std::vector<Constraint> m_vecConstraints;
};

} // namespace z3wb
