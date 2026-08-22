#pragma once

#include "core/domain/Problem.hpp"
#include "core/domain/Project.hpp"
#include "core/domain/SolverResult.hpp"
#include "core/parser/DslPrinter.hpp"
#include "core/serialization/JsonProjectStorage.hpp"
#include "core/serialization/ProblemExporter.hpp"
#include "core/serialization/SmtLib2Reader.hpp"
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
#include <QUrl>

#include <filesystem>
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
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY dirtyChanged)

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
    [[nodiscard]] bool hasUnsavedChanges() const;

    // --- QML invokables ------------------------------------------------------
    Q_INVOKABLE void createProblem();
    Q_INVOKABLE void selectProblem(int iRow);
    Q_INVOKABLE void removeProblem(int iRow);
    Q_INVOKABLE void renameProblem(int iRow, const QString& strNewName);
    Q_INVOKABLE void duplicateProblem(int iRow);
    Q_INVOKABLE void solve();
    Q_INVOKABLE void stop();
    Q_INVOKABLE QString smtLib2Text();

    // Persistence. save() uses the remembered path or asks the UI for one
    // (requestSaveDialog) when the project was never saved.
    Q_INVOKABLE void openProject(const QUrl& oFile);
    Q_INVOKABLE void saveProject();
    Q_INVOKABLE void saveProjectAs(const QUrl& oFile);

    // strFormat is "smt2" | "json" | "txt" (matched in exportProblem).
    Q_INVOKABLE void exportProblem(const QUrl& oFile, const QString& strFormat);
    Q_INVOKABLE void importSmtLib2(const QUrl& oFile);

    // Recents + window layout (QSettings-backed, org/app from main()).
    Q_INVOKABLE QStringList recentProjects() const;
    Q_INVOKABLE void openProjectPath(const QString& strPath);
    Q_INVOKABLE void removeRecentProject(const QString& strPath);
    Q_INVOKABLE QVariantMap restoreWindowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int iX, int iY, int iWidth, int iHeight);

signals:
    void currentProblemChanged();
    void editorTextChanged();
    void resultChanged();
    void busyChanged();
    void dirtyChanged();
    void solveRequested(z3wb::gui::SolveJob oJob);
    void requestSaveDialog();

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
    void markDirty();
    void resetSelectionToFirst();
    [[nodiscard]] static std::filesystem::path toNativePath(const QString& strPath);

    // Persistence internals; callers must ensure the worker is idle.
    void loadFromPath(const QString& strPath);
    void saveToPath(const QString& strPath);
    void rememberRecentProject(const QString& strPath);
    [[nodiscard]] static QStringList loadRecentProjects();
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

    QString m_strProjectPath;             // empty until first save
    bool m_bDirty = false;
    JsonProjectStorage m_oStorage;
    ProblemExporter m_oExporter;
    SmtLib2Reader m_oReader;
};

} // namespace z3wb::gui
