#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "core/solver/SolverFactory.hpp"
#include "gui/viewmodels/WorkspaceViewModel.hpp"

int main(int argc, char* argv[])
{
    QGuiApplication oApp(argc, argv);
    QGuiApplication::setApplicationName("Z3 Workbench");
    QGuiApplication::setOrganizationName("Z3Workbench");
    QGuiApplication::setApplicationVersion(Z3WB_VERSION);

    // Deterministic cross-platform baseline; the dark IDE theme is applied in QML.
    QQuickStyle::setStyle("Basic");

    // Dependency injection: the UI layer only knows the ISolver interface.
    z3wb::gui::WorkspaceViewModel oWorkspace(z3wb::makeDefaultSolver());


    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("workspace"), &oWorkspace);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed,
        &oApp, QCoreApplication::quit,
        Qt::QueuedConnection);

    engine.loadFromModule("Z3Workbench", "Main");
    if (engine.rootObjects().isEmpty())
    {
        return 1;
    }

    return QGuiApplication::exec();
}
