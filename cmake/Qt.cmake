# -----------------------------------------------------------------------------
# Qt discovery.
#
# The Qt prefix default lives in the root CMakeLists.txt and can be replaced
# on the command line (-DCMAKE_PREFIX_PATH or -DQt6_DIR), which also covers a
# project-local Qt under ThirdParty/Qt and a future MSVC-compatible Qt.
# -----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

qt_standard_project_setup()

# ':/qt/qml/' is the default resource prefix for QML modules (Qt 6.5+).
qt_policy(SET QTP0001 NEW)
# qmldir files for extra QML subdirectories (panels/, components/) are
# generated automatically.
qt_policy(SET QTP0004 NEW)
