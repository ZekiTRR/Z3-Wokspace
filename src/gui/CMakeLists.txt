# -----------------------------------------------------------------------------
# Z3WorkbenchGui
#
# ViewModels and list models exposed to QML. QML files live in the
# repository-level qml/ directory and belong to the executable's QML module.
# -----------------------------------------------------------------------------
set(Z3WB_GUI_SOURCES
    models/ConsoleLogModel.cpp
    models/DiagnosticsModel.cpp
    models/ProblemsModel.cpp
    models/VariablesModel.cpp
    threading/SolverWorker.cpp
    viewmodels/WorkspaceViewModel.cpp
)

set(Z3WB_GUI_HEADERS
    models/ConsoleLogModel.hpp
    models/DiagnosticsModel.hpp
    models/ProblemsModel.hpp
    models/VariablesModel.hpp
    threading/SolverWorker.hpp
    viewmodels/WorkspaceViewModel.hpp
)

qt_add_library(Z3WorkbenchGui STATIC ${Z3WB_GUI_SOURCES} ${Z3WB_GUI_HEADERS})
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

# ViewModels consume core services; the dependency is public so the
# executable sees core headers through the GUI layer.
target_link_libraries(Z3WorkbenchGui PUBLIC
    Z3Workbench::Core
)
