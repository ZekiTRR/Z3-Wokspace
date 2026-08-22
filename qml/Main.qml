import QtQuick
import QtQuick.Controls.Basic

// Phase 1 skeleton window: proves QML module loading and provides the dark
// IDE-style base for the panels added in later phases.
ApplicationWindow {
    id: wndRoot

    visible: true
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    title: qsTr("Z3 Workbench")
    color: "#1e1f24"

    Column {
        anchors.centerIn: parent
        spacing: 8

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Z3 Workbench")
            font.pixelSize: 28
            font.bold: true
            color: "#d4d4d4"
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Phase 1 skeleton — build system, Qt discovery, Z3 integration")
            font.pixelSize: 13
            color: "#9a9a9a"
        }
    }
}
