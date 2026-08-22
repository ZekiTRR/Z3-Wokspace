#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char* argv[])
{
    QGuiApplication oApp(argc, argv);
    QGuiApplication::setApplicationName("Z3 Workbench");
    QGuiApplication::setOrganizationName("Z3Workbench");
    QGuiApplication::setApplicationVersion(Z3WB_VERSION);

    // Deterministic cross-platform baseline; the dark IDE theme is applied in QML.
    QQuickStyle::setStyle("Basic");

    QQmlApplicationEngine engine;

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
