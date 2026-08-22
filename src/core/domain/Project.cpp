#include "core/domain/Project.hpp"

#include <algorithm>
#include <utility>

namespace z3wb {

Project::Project(std::string strName)
    : m_strName(std::move(strName))
{
}

Problem* Project::addProblem(std::string strName)
{
    if (findProblem(strName) != nullptr)
    {
        return nullptr;
    }

    m_vecProblems.emplace_back(std::move(strName));
    return &m_vecProblems.back();
}

Problem* Project::findProblem(ProblemId oId)
{
    const auto itFound = std::find_if(m_vecProblems.begin(), m_vecProblems.end(),
        [oId](const Problem& oExisting)
        {
            return oExisting.id() == oId;
        });
    return itFound == m_vecProblems.end() ? nullptr : &*itFound;
}

const Problem* Project::findProblem(ProblemId oId) const
{
    const auto itFound = std::find_if(m_vecProblems.begin(), m_vecProblems.end(),
        [oId](const Problem& oExisting)
        {
            return oExisting.id() == oId;
        });
    return itFound == m_vecProblems.end() ? nullptr : &*itFound;
}

Problem* Project::findProblem(std::string_view svName)
{
    const auto itFound = std::find_if(m_vecProblems.begin(), m_vecProblems.end(),
        [svName](const Problem& oExisting)
        {
            return oExisting.name() == svName;
        });
    return itFound == m_vecProblems.end() ? nullptr : &*itFound;
}

const Problem* Project::findProblem(std::string_view svName) const
{
    const auto itFound = std::find_if(m_vecProblems.begin(), m_vecProblems.end(),
        [svName](const Problem& oExisting)
        {
            return oExisting.name() == svName;
        });
    return itFound == m_vecProblems.end() ? nullptr : &*itFound;
}

bool Project::removeProblem(ProblemId oId)
{
    const auto itFound = std::find_if(m_vecProblems.begin(), m_vecProblems.end(),
        [oId](const Problem& oExisting)
        {
            return oExisting.id() == oId;
        });
    if (itFound == m_vecProblems.end())
    {
        return false;
    }

    m_vecProblems.erase(itFound);
    return true;
}

Problem* Project::duplicateProblem(ProblemId oId, std::string strNewName)
{
    const Problem* pSource = findProblem(oId);
    if (pSource == nullptr || findProblem(strNewName) != nullptr)
    {
        return nullptr;
    }

    Problem oCopy(std::move(strNewName));

    VariableIdMap mapOldToNew;
    mapOldToNew.reserve(pSource->variableCount());
    for (const Variable& oVariable : pSource->variables())
    {
        Variable oCloned = oVariable;
        oCloned.id = makeVariableId();
        mapOldToNew.emplace(oVariable.id, oCloned.id);

        // Names were unique in the source problem, so these cannot fail.
        [[maybe_unused]] const bool bAdded = oCopy.addVariable(std::move(oCloned));
    }

    for (const Constraint& oConstraint : pSource->constraints())
    {
        Constraint oCloned = oConstraint;
        oCloned.id = makeConstraintId();
        oCloned.expr = remapVariables(oConstraint.expr, mapOldToNew);

        [[maybe_unused]] const bool bAdded = oCopy.addConstraint(std::move(oCloned));
    }

    m_vecProblems.push_back(std::move(oCopy));
    return &m_vecProblems.back();
}

} // namespace z3wb
