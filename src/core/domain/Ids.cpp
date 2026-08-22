#include "core/domain/Ids.hpp"

#include <atomic>

namespace z3wb {

namespace {

std::uint64_t nextSessionId()
{
    static std::atomic<std::uint64_t> s_uCounter{0};
    return s_uCounter.fetch_add(1, std::memory_order_relaxed) + 1;
}

template<typename TagT>
StrongId<TagT> makeId()
{
    return StrongId<TagT>{nextSessionId()};
}

} // namespace

VariableId makeVariableId()
{
    return makeId<VariableIdTag>();
}

ConstraintId makeConstraintId()
{
    return makeId<ConstraintIdTag>();
}

ProblemId makeProblemId()
{
    return makeId<ProblemIdTag>();
}

} // namespace z3wb
