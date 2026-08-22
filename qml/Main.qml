import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: wndRoot

    visible: true
    width: 1280
    height: 800
    minimumWidth: 1000
    minimumHeight: 640
    title: qsTr("Z3 Workbench — %1 [%2]").arg(workspace.projectName).arg(workspace.currentProblemName)
    color: Theme.background

    // --- actions -------------------------------------------------------------
    Action {
        id: actNewProblem
        shortcut: "Ctrl+N"
        text: qsTr("New Problem")
        onTriggered: function() { workspace.createProblem(); }
    }
    Action {
        id: actSolve
        shortcut: "F5"
        text: qsTr("Solve")
        enabled: workspace.hasCurrentProblem && !workspace.busy
        onTriggered: function() { workspace.solve(); }
    }
    Action {
        id: actStop
        shortcut: "Shift+F5"
        text: qsTr("Stop")
        enabled: workspace.busy
        onTriggered: function() { workspace.stop(); }
    }
    Action {
        id: actSolveFromEditor
        shortcut: "Ctrl+Return"
        text: qsTr("Solve from editor")
        enabled: workspace.hasCurrentProblem && !workspace.busy
        onTriggered: function() { workspace.solve(); }
    }
    Action {
        id: actSmtLib2
        shortcut: "Ctrl+M"
        text: qsTr("View SMT-LIB2")
        onTriggered: function() {
            txtSmtLib2.text = workspace.smtLib2Text();
            pSmtLib2.open();
        }
    }

    menuBar: MenuBar {
        background: Rectangle { color: Theme.panelDark }

        Menu {
            title: qsTr("File")
            MenuItem { action: actNewProblem }
            MenuSeparator {}
            MenuItem { action: actSmtLib2 }
            MenuSeparator {}
            Action {
                text: qsTr("Quit")
                shortcut: "Ctrl+Q"
                onTriggered: function() { Qt.quit(); }
            }
        }
        Menu {
            title: qsTr("Solver")
            MenuItem { action: actSolve }
            MenuItem { action: actStop }
        }
        Menu {
            title: qsTr("Help")
            Action {
                text: qsTr("About")
                onTriggered: function() { pAbout.open(); }
            }
        }
    }

    header: ToolBar {
        background: Rectangle { color: Theme.panelDark }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 6
            spacing: 4

            ToolButton {
                text: qsTr("New")
                icon.width: 0
                onClicked: function() { workspace.createProblem(); }
            }
            ToolSeparator {}
            ToolButton {
                text: qsTr("Solve (F5)")
                enabled: workspace.hasCurrentProblem && !workspace.busy
                onClicked: function() { workspace.solve(); }

                contentItem: Label {
                    text: parent.text
                    color: parent.enabled ? Theme.success : Theme.textDim
                    font.pixelSize: Theme.fontSize
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                    rightPadding: 8
                }
            }
            ToolButton {
                text: qsTr("Stop (Shift+F5)")
                enabled: workspace.busy
                onClicked: function() { workspace.stop(); }

                contentItem: Label {
                    text: parent.text
                    color: parent.enabled ? Theme.error : Theme.textDim
                    font.pixelSize: Theme.fontSize
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 8
                    rightPadding: 8
                }
            }
            ToolSeparator {}
            ToolButton {
                text: qsTr("SMT-LIB2")
                onClicked: function() { actSmtLib2.trigger(); }
            }

            Item { Layout.fillWidth: true }
        }
    }

    // --- main layout ---------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        SplitView {
            id: splMain
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            ProjectExplorerPanel {
                SplitView.preferredWidth: 230
                SplitView.minimumWidth: 160
            }

            EditorPanel {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 320
            }

            VariablesPanel {
                SplitView.preferredWidth: 330
                SplitView.minimumWidth: 240
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
        }

        ConsolePanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            Layout.minimumHeight: 80
        }

        // --- status bar ------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: Theme.panelDark

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 16

                Label {
                    text: {
                        if (workspace.busy) {
                            return workspace.resultStatus === "CANCELLING"
                                ? qsTr("Cancelling…")
                                : qsTr("Solving…");
                        }
                        return workspace.resultStatus.length > 0
                            ? qsTr("Status: %1").arg(workspace.resultStatus)
                            : qsTr("Ready");
                    }
                    color: {
                        if (workspace.busy) return Theme.accent;
                        if (workspace.resultStatus === "SAT") return Theme.success;
                        if (workspace.resultStatus === "UNSAT") return Theme.warning;
                        if (workspace.resultStatus === "UNKNOWN") return Theme.info;
                        if (workspace.resultStatus === "ERROR" || workspace.resultStatus === "INVALID") return Theme.error;
                        return Theme.textDim;
                    }
                    font.pixelSize: Theme.fontSize
                    font.bold: true
                }

                Label {
                    visible: workspace.solveTimeMs > 0
                    text: qsTr("Time: %1 ms").arg(workspace.solveTimeMs)
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSize
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: qsTr("Z3 Workbench %1").arg("0.1.0")
                    color: Theme.textDim
                    font.pixelSize: Theme.fontSize - 1
                }
            }
        }
    }

    // --- dialogs -------------------------------------------------------------
    Popup {
        id: pSmtLib2
        modal: true
        anchors.centerIn: parent
        width: Math.min(760, wndRoot.width - 80)
        height: Math.min(560, wndRoot.height - 80)
        padding: 10
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle { color: Theme.panel; border.color: Theme.border; radius: 4 }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: qsTr("SMT-LIB2")
                    color: Theme.textDim
                    font.pixelSize: Theme.headerFontSize
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Copy")
                    flat: true
                    onClicked: function() {
                        txtSmtLib2.selectAll();
                        txtSmtLib2.copy();
                    }
                }
                Button {
                    text: qsTr("Close")
                    flat: true
                    onClicked: function() { pSmtLib2.close(); }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true

                TextArea {
                    id: txtSmtLib2
                    readOnly: true
                    color: Theme.text
                    font.pixelSize: Theme.fontSize - 1
                    font.family: "Consolas"
                    wrapMode: TextArea.Wrap
                    selectByMouse: true

                    background: Rectangle { color: Theme.editor; border.color: Theme.border }
                }
            }
        }
    }

    Popup {
        id: pAbout
        modal: true
        anchors.centerIn: parent
        width: 360
        padding: 14
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle { color: Theme.panel; border.color: Theme.border; radius: 4 }

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            Label { text: qsTr("Z3 Workbench 0.1.0"); color: Theme.text; font.bold: true }
            Label {
                Layout.fillWidth: true
                text: qsTr("A desktop workbench for the Z3 SMT solver.\nPhase 5: IDE layout, editor with live diagnostics, synchronous solving.")
                color: Theme.textDim
                font.pixelSize: Theme.fontSize - 1
                wrapMode: Text.WordWrap
            }
        }
    }
}
