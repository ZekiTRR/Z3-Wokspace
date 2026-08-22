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

set(Z3WB_APP_QML_FILES
    "${Z3WB_APP_QML_DIR}/Main.qml"
    "${Z3WB_APP_QML_DIR}/Theme.qml"
    "${Z3WB_APP_QML_DIR}/components/InputPopup.qml"
    "${Z3WB_APP_QML_DIR}/panels/ConsolePanel.qml"
    "${Z3WB_APP_QML_DIR}/panels/EditorPanel.qml"
    "${Z3WB_APP_QML_DIR}/panels/ProjectExplorerPanel.qml"
    "${Z3WB_APP_QML_DIR}/panels/VariablesPanel.qml"
)

# QML files live outside the target directory, so each resource alias must be
# relative to the qml/ root explicitly.
foreach(strQmlFile IN LISTS Z3WB_APP_QML_FILES)
    file(RELATIVE_PATH strAlias "${Z3WB_APP_QML_DIR}" "${strQmlFile}")
    set_source_files_properties("${strQmlFile}" PROPERTIES QT_RESOURCE_ALIAS "${strAlias}")
endforeach()

set_source_files_properties("${Z3WB_APP_QML_DIR}/Theme.qml" PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(Z3WorkbenchApp
    URI Z3Workbench
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Z3Workbench"
    QML_FILES ${Z3WB_APP_QML_FILES}
)

target_link_libraries(Z3WorkbenchApp PRIVATE
    Z3Workbench::Gui
)
