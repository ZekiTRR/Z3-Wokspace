import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Project Explorer: problems list plus management actions.
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
                text: qsTr("PROJECT EXPLORER")
                color: Theme.textDim
                font.pixelSize: Theme.headerFontSize
                font.bold: true
            }
        }

        ListView {
            id: lstProblems
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: workspace.problemsModel

            delegate: ItemDelegate {
                width: lstProblems.width
                height: 26

                contentItem: Label {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    text: pName
                    color: lstProblems.currentIndex === index ? Theme.text : Theme.textDim
                    font.pixelSize: Theme.fontSize
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: lstProblems.currentIndex === index
                        ? Theme.selection
                        : (hovered ? Theme.panelDark : "transparent")
                }

                onClicked: function() {
                    lstProblems.currentIndex = index;
                    workspace.selectProblem(index);
                }
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.border
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 6
            spacing: 4

            ToolButton {
                text: qsTr("New")
                onClicked: function() { workspace.createProblem(); }
            }
            ToolButton {
                text: qsTr("Duplicate")
                enabled: lstProblems.currentIndex >= 0
                onClicked: function() { workspace.duplicateProblem(lstProblems.currentIndex); }
            }
            ToolButton {
                text: qsTr("Rename")
                enabled: lstProblems.currentIndex >= 0
                onClicked: function() {
                    pRenamePopup.strValue = workspace.currentProblemName;
                    pRenamePopup.open();
                }
            }
            ToolButton {
                text: qsTr("Delete")
                enabled: lstProblems.currentIndex >= 0
                onClicked: function() { workspace.removeProblem(lstProblems.currentIndex); }
            }

            Item { Layout.fillWidth: true }
        }
    }

    InputPopup {
        id: pRenamePopup
        strTitle: qsTr("Rename problem")
        onAccepted: function(strText) {
            workspace.renameProblem(lstProblems.currentIndex, strText);
        }
    }
}
