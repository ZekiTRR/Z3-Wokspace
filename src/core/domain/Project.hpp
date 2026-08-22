#pragma once

#include "core/domain/Problem.hpp"

#include <string>
#include <vector>

namespace z3wb {

// Top-level document: a named container of problems.
// Note: like Problem, this type lives in a std::vector — pointers/references
// into problems() are invalidated by any mutating call on Project.
class Project
{
public:
    explicit Project(std::string strName);

    [[nodiscard]] const std::string& name() const noexcept { return m_strName; }
    void setName(std::string strName) { m_strName = std::move(strName); }

    // Creates an empty problem with a fresh id. Returns nullptr when the
    // name is already taken (problem names act as user-visible identifiers).
    [[nodiscard]] Problem* addProblem(std::string strName);

    [[nodiscard]] Problem* findProblem(ProblemId oId);
    [[nodiscard]] const Problem* findProblem(ProblemId oId) const;
    [[nodiscard]] Problem* findProblem(std::string_view svName);
    [[nodiscard]] const Problem* findProblem(std::string_view svName) const;

    [[nodiscard]] bool removeProblem(ProblemId oId);

    // Mutable access by index for management operations (rename); the same
    // pointer-invalidation caveat as problems() applies.
    [[nodiscard]] Problem* problemAt(std::size_t stIndex);
    [[nodiscard]] const Problem* problemAt(std::size_t stIndex) const;

    // Deep copy with fresh ids for the problem and all of its variables,
    // constraints, including references inside expressions.
    [[nodiscard]] Problem* duplicateProblem(ProblemId oId, std::string strNewName);

    // Moves an externally built problem into the project (used by the
    // project loader). Returns nullptr when the name is already taken.
    [[nodiscard]] Problem* adoptProblem(Problem oProblem);

    [[nodiscard]] const std::vector<Problem>& problems() const noexcept { return m_vecProblems; }

private:
    std::string m_strName;
    std::vector<Problem> m_vecProblems;
};

} // namespace z3wb
