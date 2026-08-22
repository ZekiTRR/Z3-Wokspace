#pragma once

#include "core/domain/Problem.hpp"
#include "core/domain/Project.hpp"
#include "core/domain/SolverResult.hpp"
#include "core/solver/AtomicCancellation.hpp"
#include "core/solver/ISolver.hpp"
#include "core/solver/SolverConfig.hpp"
#include "gui/models/ConsoleLogModel.hpp"
#include "gui/models/DiagnosticsModel.hpp"
#include "gui/models/ProblemsModel.hpp"
#include "gui/models/VariablesModel.hpp"
#include "gui/threading/SolverWorker.hpp"

#include <QObject>
#include <QString>
#include <QThread>

#include <memory>
#include <optional>

namespace z3wb::gui {

// Central UI state: owns the Project, the editor text of the current problem,
// and the last solver run. QML talks only to this object (context property
// "workspace"); all domain work happens here on the GUI thread in Phase 5.
class WorkspaceViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString projectName READ projectName CONSTANT)
    Q_PROPERTY(QString currentProblemName READ currentProblemName NOTIFY currentProblemChanged)
    Q_PROPERTY(bool hasCurrentProblem READ hasCurrentProblem NOTIFY currentProblemChanged)
    Q_PROPERTY(QString editorText READ editorText WRITE setEditorText NOTIFY editorTextChanged)
    Q_PROPERTY(QAbstractListModel* problemsModel READ problemsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* variablesModel READ variablesModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* diagnosticsModel READ diagnosticsModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* consoleModel READ consoleModel CONSTANT)
    Q_PROPERTY(QString resultStatus READ resultStatus NOTIFY resultChanged)
    Q_PROPERTY(qint64 solveTimeMs READ solveTimeMs NOTIFY resultChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit WorkspaceViewModel(std::shared_ptr<ISolver> spSolver,
        QObject* pParent = nullptr);
    ~WorkspaceViewModel() override;

    [[nodiscard]] QString projectName() const;
    [[nodiscard]] QString currentProblemName() const;
    [[nodiscard]] bool hasCurrentProblem() const;
    [[nodiscard]] QString editorText() const;
    void setEditorText(const QString& strText);

    [[nodiscard]] QAbstractListModel* problemsModel();
    [[nodiscard]] QAbstractListModel* variablesModel();
    [[nodiscard]] QAbstractListModel* diagnosticsModel();
    [[nodiscard]] QAbstractListModel* consoleModel();

    [[nodiscard]] QString resultStatus() const;
    [[nodiscard]] qint64 solveTimeMs() const;
    [[nodiscard]] bool busy() const;

    // --- QML invokables ------------------------------------------------------
    Q_INVOKABLE void createProblem();
    Q_INVOKABLE void selectProblem(int iRow);
    Q_INVOKABLE void removeProblem(int iRow);
    Q_INVOKABLE void renameProblem(int iRow, const QString& strNewName);
    Q_INVOKABLE void duplicateProblem(int iRow);
    Q_INVOKABLE void solve();
    Q_INVOKABLE void stop();
    Q_INVOKABLE QString smtLib2Text();

signals:
    void currentProblemChanged();
    void editorTextChanged();
    void resultChanged();
    void busyChanged();
    void solveRequested(z3wb::gui::SolveJob oJob);

private slots:
    void onSolveFinished(z3wb::gui::SolveJobResult oResult);

private:
    struct EditorState
    {
        bool bValid = false;                       // last parse succeeded
        Problem oProblem;                    // last valid contents
        std::vector<Diagnostic> vecDiags;    // last parse diagnostics
    };

    [[nodiscard]] Problem* currentProblem();
    void selectById(ProblemId oId);
    void reparseEditor();                 // editor text -> diagnostics + variables
    void refreshProblemsModel();
    void refreshVariablesModel(const std::optional<Model>& oModel);
    void refreshDiagnosticsModel(const std::vector<Diagnostic>& vecDiags);
    void applyResult(const SolverResult& oResult);
    void setBusy(bool bBusy);
    void logInfo(const QString& strText);
    void logWarning(const QString& strText);
    void logError(const QString& strText);
    void log(DiagnosticSeverity eSeverity, const QString& strText);
    [[nodiscard]] static QString formatModelValue(const ModelEntry& oEntry);

    std::shared_ptr<ISolver> m_spSolver;
    Project m_oProject;

    // Worker thread plumbing. The worker has no parent: it lives in the
    // dedicated thread and is deleted through QThread::finished.
    QThread m_oWorkerThread;
    SolverWorker* m_pWorker = nullptr;

    quint64 m_uNextRequestId = 0;
    bool m_bBusy = false;
    // Concrete type: stop() must be able to arm the flag.
    std::shared_ptr<AtomicCancellation> m_spCancellation;

    ProblemId m_oCurrentId;               // invalid = no selection
    EditorState m_oEditor;

    ProblemsModel m_oProblemsModel;
    VariablesModel m_oVariablesModel;
    DiagnosticsModel m_oDiagnosticsModel;
    ConsoleLogModel m_oConsoleModel;

    SolverConfig m_oSolverConfig;
    QString m_strResultStatus;            // "" | SAT | UNSAT | UNKNOWN | ERROR | INVALID
    qint64 m_iSolveTimeMs = 0;
};

} // namespace z3wb::gui
