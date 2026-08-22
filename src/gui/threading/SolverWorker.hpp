#pragma once

#include "core/domain/Problem.hpp"
#include "core/domain/SolverResult.hpp"
#include "core/solver/ISolver.hpp"
#include "core/solver/SolverConfig.hpp"

#include <QObject>
#include <QString>

#include <memory>

namespace z3wb::gui {

// One solve request, passed by value into the worker thread so the worker
// never touches viewmodel state.
struct SolveJob
{
    quint64 uRequestId = 0;
    Problem oProblem;
    SolverConfig oConfig;
    std::shared_ptr<ISolver> spSolver;
    std::shared_ptr<ICancellation> spCancellation;
};

// One finished request, delivered back to the GUI thread via a queued signal.
struct SolveJobResult
{
    quint64 uRequestId = 0;
    ProblemId oProblemId;
    SolverResult oResult;
};

} // namespace z3wb::gui

Q_DECLARE_METATYPE(z3wb::gui::SolveJob)
Q_DECLARE_METATYPE(z3wb::gui::SolveJobResult)

namespace z3wb::gui {

// Lives inside a dedicated QThread. Owns nothing but the injected solver
// interface; every call gets fresh backend state (see Z3Solver).
class SolverWorker : public QObject
{
    Q_OBJECT

public:
    explicit SolverWorker(QObject* pParent = nullptr);

public slots:
    void doSolve(SolveJob oJob);

signals:
    void solveFinished(z3wb::gui::SolveJobResult oResult);
};

} // namespace z3wb::gui
