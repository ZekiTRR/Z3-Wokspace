#include "core/domain/Problem.hpp"

#include <algorithm>

namespace z3wb {

Problem::Problem(std::string strName)
    : m_id(makeProblemId())
    , m_strName(std::move(strName))
{
}

bool Problem::addVariable(Variable oVariable)
{
    if (!isValidVariableName(oVariable.name))
    {
        return false;
    }

    const auto itFound = std::find_if(m_vecVariables.begin(), m_vecVariables.end(),
        [&oVariable](const Variable& oExisting)
        {
            return oExisting.name == oVariable.name;
        });
    if (itFound != m_vecVariables.end())
    {
        return false;
    }

    if (!oVariable.id.isValid())
    {
        oVariable.id = makeVariableId();
    }

    // BitVec without an explicit width is meaningless; default to 32 so the
    // entity stays solvable even if the UI forgot the width field.
    if (oVariable.type == VariableType::BitVec && oVariable.params.uBitVecWidth == 0)
    {
        oVariable.params.uBitVecWidth = 32;
    }

    m_vecVariables.push_back(std::move(oVariable));
    return true;
}

bool Problem::removeVariable(VariableId oId)
{
    const auto itFound = std::find_if(m_vecVariables.begin(), m_vecVariables.end(),
        [oId](const Variable& oExisting)
        {
            return oExisting.id == oId;
        });
    if (itFound == m_vecVariables.end())
    {
        return false;
    }

    m_vecVariables.erase(itFound);
    // Constraints referencing the removed variable become invalid; that is
    // reported by validation instead of being silently rewritten here.
    return true;
}

const Variable* Problem::findVariable(std::string_view svName) const
{
    const auto itFound = std::find_if(m_vecVariables.begin(), m_vecVariables.end(),
        [svName](const Variable& oExisting)
        {
            return oExisting.name == svName;
        });
    return itFound == m_vecVariables.end() ? nullptr : &*itFound;
}

const Variable* Problem::findVariable(VariableId oId) const
{
    const auto itFound = std::find_if(m_vecVariables.begin(), m_vecVariables.end(),
        [oId](const Variable& oExisting)
        {
            return oExisting.id == oId;
        });
    return itFound == m_vecVariables.end() ? nullptr : &*itFound;
}

bool Problem::addConstraint(Constraint oConstraint)
{
    if (!oConstraint.id.isValid())
    {
        oConstraint.id = makeConstraintId();
    }

    m_vecConstraints.push_back(std::move(oConstraint));
    return true;
}

bool Problem::removeConstraint(ConstraintId oId)
{
    const auto itFound = std::find_if(m_vecConstraints.begin(), m_vecConstraints.end(),
        [oId](const Constraint& oExisting)
        {
            return oExisting.id == oId;
        });
    if (itFound == m_vecConstraints.end())
    {
        return false;
    }

    m_vecConstraints.erase(itFound);
    return true;
}

bool Problem::setConstraintEnabled(ConstraintId oId, bool bEnabled)
{
    const auto itFound = std::find_if(m_vecConstraints.begin(), m_vecConstraints.end(),
        [oId](const Constraint& oExisting)
        {
            return oExisting.id == oId;
        });
    if (itFound == m_vecConstraints.end())
    {
        return false;
    }

    itFound->enabled = bEnabled;
    return true;
}

const Constraint* Problem::findConstraint(ConstraintId oId) const
{
    const auto itFound = std::find_if(m_vecConstraints.begin(), m_vecConstraints.end(),
        [oId](const Constraint& oExisting)
        {
            return oExisting.id == oId;
        });
    return itFound == m_vecConstraints.end() ? nullptr : &*itFound;
}

std::size_t Problem::enabledConstraintCount() const
{
    return static_cast<std::size_t>(std::count_if(m_vecConstraints.begin(), m_vecConstraints.end(),
        [](const Constraint& oExisting)
        {
            return oExisting.enabled;
        }));
}

} // namespace z3wb
