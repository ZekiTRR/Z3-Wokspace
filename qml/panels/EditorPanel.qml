import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Problem editor: DSL text plus live diagnostics underneath.
Rectangle {
    id: pRoot

    color: Theme.panel

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: Theme.panelDark

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                text: qsTr("PROBLEM EDITOR")
                color: Theme.textDim
                font.pixelSize: Theme.headerFontSize
                font.bold: true
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 10
                text: workspace.hasCurrentProblem ? workspace.currentProblemName : qsTr("no problem selected")
                color: Theme.textDim
                font.pixelSize: Theme.headerFontSize
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                id: txtEditor
                enabled: workspace.hasCurrentProblem
                text: workspace.editorText
                color: Theme.text
                font.pixelSize: Theme.fontSize
                font.family: "Consolas"
                wrapMode: TextArea.Wrap
                selectByMouse: true
                tabStopDistance: 16
                persistentSelection: true

                background: Rectangle { color: Theme.editor }

                // Guarded two-way binding: programmatic updates (problem
                // switch) flow in, user typing flows out, no feedback loop.
                onTextChanged: function() {
                    if (text !== workspace.editorText) {
                        workspace.editorText = text;
                    }
                }

                Connections {
                    target: workspace
                    function onEditorTextChanged() {
                        if (txtEditor.text !== workspace.editorText) {
                            txtEditor.text = workspace.editorText;
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.topMargin: 2
            text: qsTr("DIAGNOSTICS")
            color: Theme.textDim
            font.pixelSize: Theme.headerFontSize
            font.bold: true
        }

        ListView {
            id: lstDiagnostics
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(110, contentHeight + 4)
            clip: true
            spacing: 1
            model: workspace.diagnosticsModel

            delegate: Label {
                width: lstDiagnostics.width
                leftPadding: 10
                rightPadding: 10
                font.pixelSize: Theme.fontSize - 1
                elide: Text.ElideRight
                color: dSeverity === "error"
                    ? Theme.error
                    : (dSeverity === "warning" ? Theme.warning : Theme.info)
                text: dLine > 0
                    ? qsTr("Line %1: %2").arg(dLine).arg(dMessage)
                    : dMessage
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}
