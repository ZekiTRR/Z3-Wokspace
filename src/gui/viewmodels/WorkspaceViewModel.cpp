#include "gui/viewmodels/WorkspaceViewModel.hpp"

#include "core/parser/ProblemParser.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QSettings>
#include <fstream>
#include <sstream>
#include <utility>

namespace z3wb::gui {

namespace {

constexpr int k_iMaxRecentProjects = 10;

} // namespace

WorkspaceViewModel::WorkspaceViewModel(std::shared_ptr<ISolver> spSolver,
    QObject* pParent)
    : QObject(pParent)
    , m_spSolver(std::move(spSolver))
    , m_oProject(QStringLiteral("Untitled").toStdString())
{
    // Queued-connection argument types must be registered before any job
    // crosses the thread boundary.
    qRegisterMetaType<SolveJob>("z3wb::gui::SolveJob");
    qRegisterMetaType<SolveJobResult>("z3wb::gui::SolveJobResult");

    // Solver thread: one long-lived worker; each request creates fresh
    // backend state inside solve(), so no per-request worker churn is needed.
    m_pWorker = new SolverWorker(); // no parent: owned by the thread affinity
    m_pWorker->moveToThread(&m_oWorkerThread);

    connect(&m_oWorkerThread, &QThread::finished,
        m_pWorker, &QObject::deleteLater);
    connect(this, &WorkspaceViewModel::solveRequested,
        m_pWorker, &SolverWorker::doSolve, Qt::QueuedConnection);
    connect(m_pWorker, &SolverWorker::solveFinished,
        this, &WorkspaceViewModel::onSolveFinished, Qt::QueuedConnection);

    m_oWorkerThread.setObjectName(QStringLiteral("solver-worker"));
    m_oWorkerThread.start();

    // Start with one problem so the workbench is immediately usable.
    createProblem();
}

WorkspaceViewModel::~WorkspaceViewModel()
{
    m_oWorkerThread.quit();
    m_oWorkerThread.wait();
}

QString WorkspaceViewModel::projectName() const
{
    return QString::fromStdString(m_oProject.name());
}

bool WorkspaceViewModel::hasCurrentProblem() const
{
    return m_oCurrentId.isValid();
}

QString WorkspaceViewModel::currentProblemName() const
{
    const Problem* pProblem = m_oProject.findProblem(m_oCurrentId);
    return pProblem != nullptr ? QString::fromStdString(pProblem->name()) : QString();
}

QString WorkspaceViewModel::editorText() const
{
    const Problem* pProblem = m_oProject.findProblem(m_oCurrentId);
    if (pProblem == nullptr)
    {
        return QString();
    }
    // The editor shows the live text; the last valid copy lives in oEditor.
    return QString::fromStdString(m_oEditor.oProblem.sourceText());
}

void WorkspaceViewModel::setEditorText(const QString& strText)
{
    Problem* pProblem = currentProblem();
    if (pProblem == nullptr || strText.toStdString() == m_oEditor.oProblem.sourceText())
    {
        return;
    }

    pProblem->setSourceText(strText.toStdString());
    reparseEditor();
    markDirty();
    emit editorTextChanged();
}

QAbstractListModel* WorkspaceViewModel::problemsModel()
{
    return &m_oProblemsModel;
}

QAbstractListModel* WorkspaceViewModel::variablesModel()
{
    return &m_oVariablesModel;
}

QAbstractListModel* WorkspaceViewModel::diagnosticsModel()
{
    return &m_oDiagnosticsModel;
}

QAbstractListModel* WorkspaceViewModel::consoleModel()
{
    return &m_oConsoleModel;
}

QString WorkspaceViewModel::resultStatus() const
{
    return m_strResultStatus;
}

qint64 WorkspaceViewModel::solveTimeMs() const
{
    return m_iSolveTimeMs;
}

bool WorkspaceViewModel::busy() const
{
    return m_bBusy;
}

void WorkspaceViewModel::setBusy(bool bBusy)
{
    if (m_bBusy == bBusy)
    {
        return;
    }
    m_bBusy = bBusy;
    emit busyChanged();
}

bool WorkspaceViewModel::hasUnsavedChanges() const
{
    return m_bDirty;
}

void WorkspaceViewModel::markDirty()
{
    if (!m_bDirty)
    {
        m_bDirty = true;
        emit dirtyChanged();
    }
}

std::filesystem::path WorkspaceViewModel::toNativePath(const QString& strPath)
{
#ifdef _WIN32
    return std::filesystem::path(strPath.toStdWString());
#else
    return std::filesystem::path(strPath.toStdString());
#endif
}

void WorkspaceViewModel::rememberRecentProject(const QString& strPath)
{
    if (strPath.isEmpty())
    {
        return;
    }

    QSettings oSettings;
    QStringList vecRecents = loadRecentProjects();
    vecRecents.removeAll(strPath);
    vecRecents.prepend(strPath);
    while (vecRecents.size() > k_iMaxRecentProjects)
    {
        vecRecents.removeLast();
    }
    oSettings.setValue(QStringLiteral("recentProjects"), vecRecents);
}

QStringList WorkspaceViewModel::loadRecentProjects()
{
    QSettings oSettings;
    return oSettings.value(QStringLiteral("recentProjects")).toStringList();
}

QStringList WorkspaceViewModel::recentProjects() const
{
    return loadRecentProjects();
}

void WorkspaceViewModel::openProjectPath(const QString& strPath)
{
    loadFromPath(strPath);
}

void WorkspaceViewModel::removeRecentProject(const QString& strPath)
{
    QSettings oSettings;
    QStringList vecRecents = loadRecentProjects();
    vecRecents.removeAll(strPath);
    oSettings.setValue(QStringLiteral("recentProjects"), vecRecents);
}

QVariantMap WorkspaceViewModel::restoreWindowGeometry() const
{
    QSettings oSettings;
    QVariantMap oGeometry;
    oGeometry[QStringLiteral("x")] =
        oSettings.value(QStringLiteral("window/x"), -1).toInt();
    oGeometry[QStringLiteral("y")] =
        oSettings.value(QStringLiteral("window/y"), -1).toInt();
    oGeometry[QStringLiteral("width")] =
        oSettings.value(QStringLiteral("window/width"), 1280).toInt();
    oGeometry[QStringLiteral("height")] =
        oSettings.value(QStringLiteral("window/height"), 800).toInt();
    return oGeometry;
}

void WorkspaceViewModel::saveWindowGeometry(int iX, int iY, int iWidth, int iHeight)
{
    QSettings oSettings;
    oSettings.setValue(QStringLiteral("window/x"), iX);
    oSettings.setValue(QStringLiteral("window/y"), iY);
    oSettings.setValue(QStringLiteral("window/width"), iWidth);
    oSettings.setValue(QStringLiteral("window/height"), iHeight);
}

void WorkspaceViewModel::loadFromPath(const QString& strPath)
{
    if (m_bBusy)
    {
        logWarning(QStringLiteral("Cannot open a project while solving"));
        return;
    }

    const StorageOutcome oOutcome = m_oStorage.load(toNativePath(strPath));
    if (oOutcome.oError.has_value())
    {
        logError(QStringLiteral("Open failed: %1")
            .arg(QString::fromStdString(oOutcome.oError->message)));
        return;
    }

    m_oProject = std::move(*oOutcome.oProject);
    m_strProjectPath = strPath;
    m_bDirty = false;
    emit dirtyChanged();

    rememberRecentProject(strPath);

    logInfo(QStringLiteral("Opened %1 (%2 problems)")
        .arg(strPath).arg(m_oProject.problems().size()));
}

void WorkspaceViewModel::saveToPath(const QString& strPath)
{
    if (m_bBusy)
    {
        logWarning(QStringLiteral("Cannot save while solving"));
        return;
    }

    const StorageOutcome oOutcome = m_oStorage.save(m_oProject, toNativePath(strPath));
    if (oOutcome.oError.has_value())
    {
        logError(QStringLiteral("Save failed: %1")
            .arg(QString::fromStdString(oOutcome.oError->message)));
        return;
    }

    m_strProjectPath = strPath;
    m_bDirty = false;
    emit dirtyChanged();
    rememberRecentProject(strPath);
    logInfo(QStringLiteral("Project saved to %1").arg(strPath));
}

void WorkspaceViewModel::openProject(const QUrl& oFile)
{
    const QString strPath = oFile.toLocalFile();
    if (strPath.isEmpty())
    {
        return;
    }

    loadFromPath(strPath);
}

void WorkspaceViewModel::saveProject()
{
    if (m_strProjectPath.isEmpty())
    {
        // The UI shows the save dialog and calls back via saveProjectAs().
        emit requestSaveDialog();
        return;
    }

    saveToPath(m_strProjectPath);
}

void WorkspaceViewModel::saveProjectAs(const QUrl& oFile)
{
    QString strPath = oFile.toLocalFile();
    if (strPath.isEmpty())
    {
        return;
    }

    // Normalize the extension so files stay openable by double-click.
    if (!strPath.endsWith(QStringLiteral(".z3w"), Qt::CaseInsensitive))
    {
        strPath += QStringLiteral(".z3w");
    }

    saveToPath(strPath);
}

void WorkspaceViewModel::resetSelectionToFirst()
{
    if (!m_oProject.problems().empty())
    {
        selectById(m_oProject.problems().front().id());
        return;
    }

    // Empty project: clear the editor state completely.
    m_oCurrentId = ProblemId{};
    m_oEditor = EditorState{};
    emit currentProblemChanged();
    emit editorTextChanged();
    refreshDiagnosticsModel({});
    refreshVariablesModel(std::nullopt);
    m_strResultStatus.clear();
    m_iSolveTimeMs = 0;
    emit resultChanged();
}

void WorkspaceViewModel::exportProblem(const QUrl& oFile, const QString& strFormat)
{
    Problem* pProblem = currentProblem();
    if (pProblem == nullptr)
    {
        logWarning(QStringLiteral("Nothing to export — no problem selected"));
        return;
    }
    if (m_bBusy)
    {
        logWarning(QStringLiteral("Cannot export while solving"));
        return;
    }

    ProblemExportFormat eFormat = ProblemExportFormat::Txt;
    if (strFormat == QLatin1String("smt2"))
    {
        eFormat = ProblemExportFormat::SmtLib2;
    }
    else if (strFormat == QLatin1String("json"))
    {
        eFormat = ProblemExportFormat::Json;
    }

    // Export always reflects what the user sees right now; if the current
    // text has not parsed yet, fall back to the last valid contents.
    reparseEditor();
    Problem* pSource = m_oEditor.bValid ? &m_oEditor.oProblem : pProblem;

    const std::optional<StorageError> oError =
        m_oExporter.write(*pSource, eFormat, toNativePath(oFile.toLocalFile()));
    if (oError.has_value())
    {
        logError(QStringLiteral("Export failed: %1")
            .arg(QString::fromStdString(oError->message)));
        return;
    }

    logInfo(QStringLiteral("Exported %1 as %2")
        .arg(QString::fromStdString(pSource->name()), strFormat));
}

void WorkspaceViewModel::importSmtLib2(const QUrl& oFile)
{
    const QString strPath = oFile.toLocalFile();
    if (strPath.isEmpty() || m_bBusy)
    {
        return;
    }

    std::ifstream oStream(toNativePath(strPath), std::ios::binary);
    if (!oStream)
    {
        logError(QStringLiteral("Cannot open SMT-LIB2 file for reading"));
        return;
    }
    std::ostringstream oBuffer;
    oBuffer << oStream.rdbuf();

    // Derive a unique problem name from the file name.
    const QFileInfo oInfo(strPath);
    std::string strName = oInfo.completeBaseName().toStdString();

    std::vector<std::string> vecNames;
    vecNames.reserve(m_oProject.problems().size());
    for (const Problem& oExisting : m_oProject.problems())
    {
        vecNames.push_back(oExisting.name());
    }
    strName = Problem::makeUniqueName(strName, vecNames);

    StorageError oError;
    std::optional<Problem> oImported = m_oReader.read(oBuffer.str(), strName, &oError);
    if (!oImported.has_value())
    {
        logError(QStringLiteral("Import failed: %1")
            .arg(QString::fromStdString(oError.message)));
        return;
    }

    // Turn the imported expressions into editable DSL source and rebuild so
    // the problem becomes identical to a hand-typed one.
    const std::string strSource = DslPrinter::printProblem(*oImported);
    std::vector<Diagnostic> vecDiags;
    if (!rebuildProblemFromSource(*oImported, strSource, vecDiags))
    {
        logError(QStringLiteral("Imported problem does not validate"));
        return;
    }

    Problem* pAdopted = m_oProject.adoptProblem(std::move(*oImported));
    if (pAdopted == nullptr)
    {
        logWarning(QStringLiteral("Import failed: duplicate problem name"));
        return;
    }
    pAdopted->setSourceText(strSource);

    refreshProblemsModel();
    markDirty();
    selectById(pAdopted->id());

    logInfo(QStringLiteral("Imported %1 (%2 constraints)")
        .arg(QString::fromStdString(strName))
        .arg(pAdopted->constraintCount()));
}

Problem* WorkspaceViewModel::currentProblem()
{
    return m_oProject.findProblem(m_oCurrentId);
}

void WorkspaceViewModel::reparseEditor()
{
    Problem* pProblem = currentProblem();
    if (pProblem == nullptr)
    {
        return;
    }

    // Rebuild into the editor state; the project problem keeps its last valid
    // contents until parsing succeeds again.
    EditorState oNext;
    oNext.oProblem = *pProblem;
    oNext.bValid = rebuildProblemFromSource(oNext.oProblem,
        pProblem->sourceText(), oNext.vecDiags);

    m_oEditor = std::move(oNext);
    refreshDiagnosticsModel(m_oEditor.vecDiags);

    m_strResultStatus = m_oEditor.bValid ? QString() : QStringLiteral("INVALID");
    refreshVariablesModel(std::nullopt);
    emit resultChanged();
}

void WorkspaceViewModel::refreshProblemsModel()
{
    std::vector<ProblemsModel::ProblemRow> vecRows;
    vecRows.reserve(m_oProject.problems().size());
    for (const Problem& oProblem : m_oProject.problems())
    {
        vecRows.push_back({QString::fromStdString(oProblem.name()), oProblem.id().value()});
    }
    m_oProblemsModel.resetWith(std::move(vecRows));
}

void WorkspaceViewModel::refreshVariablesModel(const std::optional<Model>& oModel)
{
    const Problem* pSource = m_oEditor.bValid
        ? &m_oEditor.oProblem
        : currentProblem();

    std::vector<VariablesModel::Row> vecRows;
    if (pSource == nullptr)
    {
        m_oVariablesModel.resetWith(std::move(vecRows));
        return;
    }

    vecRows.reserve(pSource->variableCount());
    for (const Variable& oVariable : pSource->variables())
    {
        VariablesModel::Row oRow;
        oRow.strName = QString::fromStdString(oVariable.name);

        QString strType = QString::fromUtf8(toString(oVariable.type));
        if (oVariable.type == VariableType::BitVec)
        {
            strType += QStringLiteral("(%1)").arg(oVariable.params.uBitVecWidth);
        }
        oRow.strType = strType;

        if (oModel.has_value())
        {
            const ModelEntry* pEntry = oModel->find(oVariable.name);
            if (pEntry != nullptr)
            {
                oRow.strValue = formatModelValue(*pEntry);
            }
        }
        vecRows.push_back(std::move(oRow));
    }
    m_oVariablesModel.resetWith(std::move(vecRows));
}

void WorkspaceViewModel::refreshDiagnosticsModel(
    const std::vector<Diagnostic>& vecDiags)
{
    std::vector<DiagnosticsModel::Row> vecRows;
    vecRows.reserve(vecDiags.size());
    for (const Diagnostic& oDiagnostic : vecDiags)
    {
        DiagnosticsModel::Row oRow;
        switch (oDiagnostic.severity)
        {
            case DiagnosticSeverity::Error:
                oRow.strSeverity = QStringLiteral("error");
                break;
            case DiagnosticSeverity::Warning:
                oRow.strSeverity = QStringLiteral("warning");
                break;
            case DiagnosticSeverity::Info:
                oRow.strSeverity = QStringLiteral("info");
                break;
        }
        oRow.strMessage = QString::fromStdString(oDiagnostic.message);
        oRow.iLine = oDiagnostic.location.iLine;
        oRow.iColumn = oDiagnostic.location.iColumn;
        vecRows.push_back(std::move(oRow));
    }
    m_oDiagnosticsModel.resetWith(std::move(vecRows));
}

void WorkspaceViewModel::log(DiagnosticSeverity eSeverity, const QString& strText)
{
    ConsoleLogModel::Row oRow;
    oRow.strTime = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    switch (eSeverity)
    {
        case DiagnosticSeverity::Info:
            oRow.strLevel = QStringLiteral("INFO");
            break;
        case DiagnosticSeverity::Warning:
            oRow.strLevel = QStringLiteral("WARNING");
            break;
        case DiagnosticSeverity::Error:
            oRow.strLevel = QStringLiteral("ERROR");
            break;
    }
    oRow.strText = strText;
    m_oConsoleModel.append(std::move(oRow));
}

void WorkspaceViewModel::logInfo(const QString& strText)
{
    log(DiagnosticSeverity::Info, strText);
}

void WorkspaceViewModel::logWarning(const QString& strText)
{
    log(DiagnosticSeverity::Warning, strText);
}

void WorkspaceViewModel::logError(const QString& strText)
{
    log(DiagnosticSeverity::Error, strText);
}

QString WorkspaceViewModel::formatModelValue(const ModelEntry& oEntry)
{
    struct Visitor
    {
        QString operator()(bool bValue) const
        {
            return bValue ? QStringLiteral("true") : QStringLiteral("false");
        }

        QString operator()(std::int64_t iValue) const
        {
            return QString::number(iValue);
        }

        QString operator()(const std::string& strValue) const
        {
            return QString::fromStdString(strValue);
        }

        QString operator()(const BitVecValue& oBits) const
        {
            // RE-oriented display: hex first, decimal in parentheses.
            const int iNibbles = static_cast<int>((oBits.uWidth + 3) / 4);
            return QStringLiteral("0x%1 (%2)")
                .arg(QString::number(oBits.uBits, 16).rightJustified(iNibbles, QLatin1Char('0')),
                    QString::number(oBits.uBits));
        }
    };

    return std::visit(Visitor{}, oEntry.value);
}

void WorkspaceViewModel::createProblem()
{
    std::vector<std::string> vecNames;
    vecNames.reserve(m_oProject.problems().size());
    for (const Problem& oProblem : m_oProject.problems())
    {
        vecNames.push_back(oProblem.name());
    }

    const std::string strName = Problem::makeUniqueName("problem", vecNames);
    Problem* pProblem = m_oProject.addProblem(strName);
    if (pProblem == nullptr)
    {
        logError(QStringLiteral("Failed to create problem"));
        return;
    }

    constexpr auto k_szStarter =
        "var x: Int\n"
        "\n"
        "constraint x > 0\n";
    pProblem->setSourceText(k_szStarter);

    refreshProblemsModel();

    m_oCurrentId = pProblem->id();
    emit currentProblemChanged();
    setEditorText(QString::fromUtf8(k_szStarter));

    logInfo(QStringLiteral("Created %1").arg(QString::fromStdString(strName)));
    markDirty();
}

void WorkspaceViewModel::selectProblem(int iRow)
{
    if (iRow < 0 || iRow >= static_cast<int>(m_oProject.problems().size()))
    {
        return;
    }
    selectById(m_oProject.problems()[static_cast<std::size_t>(iRow)].id());
}

void WorkspaceViewModel::selectById(ProblemId oId)
{
    m_oCurrentId = oId;
    emit currentProblemChanged();

    const Problem* pProblem = m_oProject.findProblem(m_oCurrentId);
    if (pProblem == nullptr)
    {
        return;
    }

    m_oEditor = EditorState{};
    m_oEditor.oProblem = *pProblem;
    reparseEditor();
    emit editorTextChanged(); // make the editor load the new source
}

void WorkspaceViewModel::removeProblem(int iRow)
{
    if (iRow < 0 || iRow >= static_cast<int>(m_oProject.problems().size()))
    {
        return;
    }

    const ProblemId oRemovedId = m_oProject.problems()[static_cast<std::size_t>(iRow)].id();
    const QString strName = QString::fromStdString(m_oProject.findProblem(oRemovedId)->name());

    if (!m_oProject.removeProblem(oRemovedId))
    {
        return;
    }

    refreshProblemsModel();
    logInfo(QStringLiteral("Deleted %1").arg(strName));
    markDirty();

    if (m_oCurrentId == oRemovedId)
    {
        m_oCurrentId = ProblemId{};
        m_oEditor = EditorState{};
        emit currentProblemChanged();
        emit editorTextChanged();
        refreshDiagnosticsModel({});
        refreshVariablesModel(std::nullopt);
        m_strResultStatus.clear();
        emit resultChanged();
    }
}

void WorkspaceViewModel::renameProblem(int iRow, const QString& strNewName)
{
    const std::string strName = strNewName.toStdString();
    if (iRow < 0 || iRow >= static_cast<int>(m_oProject.problems().size()))
    {
        return;
    }

    Problem* pProblem = m_oProject.problemAt(static_cast<std::size_t>(iRow));
    const bool bIsCurrent = pProblem != nullptr && pProblem->id() == m_oCurrentId;

    if (!isValidVariableName(strName))
    {
        logWarning(QStringLiteral("Invalid name \"%1\"").arg(strNewName));
        return;
    }
    if (m_oProject.findProblem(strName) != nullptr && pProblem->name() != strName)
    {
        logWarning(QStringLiteral("Name \"%1\" is already taken").arg(strNewName));
        return;
    }

    pProblem->setName(strName);
    refreshProblemsModel();
    markDirty();
    if (bIsCurrent)
    {
        emit currentProblemChanged();
    }
}

void WorkspaceViewModel::duplicateProblem(int iRow)
{
    if (iRow < 0 || iRow >= static_cast<int>(m_oProject.problems().size()))
    {
        return;
    }

    const Problem& oSource = m_oProject.problems()[static_cast<std::size_t>(iRow)];

    std::vector<std::string> vecNames;
    vecNames.reserve(m_oProject.problems().size());
    for (const Problem& oProblem : m_oProject.problems())
    {
        vecNames.push_back(oProblem.name());
    }
    const std::string strCopyName = Problem::makeUniqueName(oSource.name() + "_copy", vecNames);

    Problem* pCopy = m_oProject.duplicateProblem(oSource.id(), strCopyName);
    if (pCopy == nullptr)
    {
        logWarning(QStringLiteral("Cannot duplicate %1").arg(QString::fromStdString(oSource.name())));
        return;
    }

    refreshProblemsModel();
    logInfo(QStringLiteral("Duplicated %1 -> %2")
        .arg(QString::fromStdString(oSource.name()), QString::fromStdString(strCopyName)));
    markDirty();
}

void WorkspaceViewModel::solve()
{
    if (!hasCurrentProblem() || m_bBusy)
    {
        return;
    }

    // Re-parse to be sure the job always runs on the newest editor text.
    reparseEditor();

    if (!m_oEditor.bValid)
    {
        logError(QStringLiteral("Problem has validation errors — fix them before solving"));
        return;
    }

    SolveJob oJob;
    oJob.uRequestId = ++m_uNextRequestId;
    oJob.oProblem = m_oEditor.oProblem; // immutable snapshot for the worker
    oJob.oConfig = m_oSolverConfig;
    oJob.spSolver = m_spSolver;
    m_spCancellation = std::make_shared<AtomicCancellation>();
    oJob.spCancellation = m_spCancellation;

    setBusy(true);
    m_strResultStatus = QStringLiteral("SOLVING");
    emit resultChanged();

    logInfo(QStringLiteral("Parsing problem... done"));
    logInfo(QStringLiteral("Variables: %1, Constraints: %2 (of %3 enabled)")
        .arg(m_oEditor.oProblem.variableCount())
        .arg(m_oEditor.oProblem.enabledConstraintCount())
        .arg(m_oEditor.oProblem.constraintCount()));
    logInfo(QStringLiteral("Starting %1...").arg(QString::fromUtf8(m_spSolver->name())));

    emit solveRequested(std::move(oJob));
}

void WorkspaceViewModel::onSolveFinished(SolveJobResult oResult)
{
    setBusy(false);
    m_spCancellation.reset();

    // The user may have switched problems while the job was running.
    if (oResult.oProblemId != m_oCurrentId)
    {
        logInfo(QStringLiteral("Result discarded — the selected problem changed"));
        return;
    }

    applyResult(oResult.oResult);
}

void WorkspaceViewModel::applyResult(const SolverResult& oResult)
{
    for (const Diagnostic& oDiagnostic : oResult.diagnostics)
    {
        log(oDiagnostic.severity, QString::fromStdString(oDiagnostic.message));
    }

    m_iSolveTimeMs = static_cast<qint64>(oResult.solveTime.count());

    switch (oResult.status)
    {
        case SolverStatus::Sat:
            m_strResultStatus = QStringLiteral("SAT");
            logInfo(QStringLiteral("Result: SAT"));
            logInfo(QStringLiteral("Solver time: %1 ms").arg(m_iSolveTimeMs));
            refreshVariablesModel(oResult.model);
            break;
        case SolverStatus::Unsat:
            m_strResultStatus = QStringLiteral("UNSAT");
            logInfo(QStringLiteral("Result: UNSAT — no solution exists"));
            logInfo(QStringLiteral("Solver time: %1 ms").arg(m_iSolveTimeMs));
            refreshVariablesModel(std::nullopt);
            break;
        case SolverStatus::Unknown:
            m_strResultStatus = QStringLiteral("UNKNOWN");
            logWarning(QStringLiteral("Result: UNKNOWN"));
            refreshVariablesModel(std::nullopt);
            break;
        case SolverStatus::Error:
            m_strResultStatus = QStringLiteral("ERROR");
            logError(QStringLiteral("Solver failed"));
            refreshVariablesModel(std::nullopt);
            break;
    }

    emit resultChanged();
}

void WorkspaceViewModel::stop()
{
    if (!m_bBusy || m_spCancellation == nullptr)
    {
        return;
    }

    // Cooperative cancellation: the backend cannot be interrupted mid-check,
    // so Stop arms the flag and the configured timeout bounds the wait.
    m_spCancellation->cancel();
    m_strResultStatus = QStringLiteral("CANCELLING");
    emit resultChanged();
    logWarning(QStringLiteral("Stop requested — waiting for the backend to finish"));
}

QString WorkspaceViewModel::smtLib2Text()
{
    if (!m_oEditor.bValid)
    {
        return QStringLiteral("; fix validation errors to see SMT-LIB2");
    }
    return QString::fromStdString(
        m_spSolver->toSmtLib2(m_oEditor.oProblem, m_oSolverConfig));
}

} // namespace z3wb::gui
