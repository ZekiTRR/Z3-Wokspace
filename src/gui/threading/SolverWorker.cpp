#include "gui/threading/SolverWorker.hpp"

namespace z3wb::gui {

SolverWorker::SolverWorker(QObject* pParent)
    : QObject(pParent)
{
}

void SolverWorker::doSolve(SolveJob oJob)
{
    // Runs in the worker thread; the job owns everything this call needs.
    SolveJobResult oResult;
    oResult.uRequestId = oJob.uRequestId;
    oResult.oProblemId = oJob.oProblem.id();
    if (oJob.spSolver != nullptr)
    {
        oResult.oResult = oJob.spSolver->solve(oJob.oProblem, oJob.oConfig, oJob.spCancellation);
    }
    else
    {
        oResult.oResult = SolverResult::makeError("Internal error: no solver backend");
    }

    emit solveFinished(std::move(oResult));
}

} // namespace z3wb::gui
