import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Variables / Model panel: declared variables; values appear after SAT.
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
                text: qsTr("VARIABLES / MODEL")
                color: Theme.textDim
                font.pixelSize: Theme.headerFontSize
                font.bold: true
            }
        }

        ListView {
            id: lstVariables
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            model: workspace.variablesModel

            header: Rectangle {
                width: lstVariables.width
                height: 22
                color: Theme.panelDark

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    spacing: 0

                    Label { Layout.preferredWidth: 110; text: qsTr("Name"); color: Theme.textDim; font.pixelSize: Theme.fontSize - 1 }
                    Label { Layout.preferredWidth: 100; text: qsTr("Type"); color: Theme.textDim; font.pixelSize: Theme.fontSize - 1 }
                    Label { Layout.fillWidth: true; text: qsTr("Value"); color: Theme.textDim; font.pixelSize: Theme.fontSize - 1 }
                }
            }

            delegate: Rectangle {
                width: lstVariables.width
                height: 24
                color: index % 2 === 0 ? "transparent" : Theme.panelDark

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    spacing: 0

                    Label { Layout.preferredWidth: 110; text: vName; color: Theme.text; font.pixelSize: Theme.fontSize - 1; elide: Text.ElideRight }
                    Label { Layout.preferredWidth: 100; text: vType; color: Theme.info; font.pixelSize: Theme.fontSize - 1; elide: Text.ElideRight }
                    Label {
                        Layout.fillWidth: true
                        text: vHasValue ? vValue : "—"
                        color: vHasValue ? Theme.success : Theme.textDim
                        font.pixelSize: Theme.fontSize - 1
                        font.family: vHasValue && vType.startsWith("BitVec") ? "Consolas" : font.family
                        elide: Text.ElideMiddle
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: vHasValue
                    cursorShape: Qt.PointingHandCursor
                    onClicked: function() {
                        txtCopyBuffer.text = vValue;
                        txtCopyBuffer.selectAll();
                        txtCopyBuffer.copy();
                    }
                }
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

    // Hidden helper used to place values on the system clipboard.
    TextArea {
        id: txtCopyBuffer
        visible: false
        width: 0
        height: 0
    }
}
