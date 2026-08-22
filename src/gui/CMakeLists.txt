# -----------------------------------------------------------------------------
# Z3WorkbenchGui
#
# Owns the QML module and (in later phases) viewmodels, controllers and list
# models. QML files live in the repository-level qml/ directory.
# -----------------------------------------------------------------------------
qt_add_library(Z3WorkbenchGui STATIC)
add_library(Z3Workbench::Gui ALIAS Z3WorkbenchGui)

configure_options(Z3WorkbenchGui)
configure_warnings(Z3WorkbenchGui)

target_link_libraries(Z3WorkbenchGui PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)
