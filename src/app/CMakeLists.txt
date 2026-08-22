# -----------------------------------------------------------------------------
# Z3WorkbenchApp — the application executable.
# Kept deliberately thin: engine bootstrap only; all logic lives in Gui/Core.
# -----------------------------------------------------------------------------
qt_add_executable(Z3WorkbenchApp
    main.cpp
)

add_executable(Z3Workbench::App ALIAS Z3WorkbenchApp)

configure_options(Z3WorkbenchApp)
configure_warnings(Z3WorkbenchApp)

set_target_properties(Z3WorkbenchApp PROPERTIES
    OUTPUT_NAME "Z3Workbench"
)

target_compile_definitions(Z3WorkbenchApp PRIVATE
    Z3WB_VERSION="${PROJECT_VERSION}"
)

# The QML module is attached to the executable: resource initializers of
# modules declared in static libraries are dropped by the linker unless the
# plugin target is force-linked, so the executable owns the module.
set(Z3WB_APP_QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../qml")

# QML files live outside the target directory, so the resource alias must be
# relative to the module root explicitly.
set_source_files_properties("${Z3WB_APP_QML_DIR}/Main.qml" PROPERTIES
    QT_RESOURCE_ALIAS "Main.qml"
)

qt_add_qml_module(Z3WorkbenchApp
    URI Z3Workbench
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Z3Workbench"
    QML_FILES
        "${Z3WB_APP_QML_DIR}/Main.qml"
)

target_link_libraries(Z3WorkbenchApp PRIVATE
    Z3Workbench::Gui
)
