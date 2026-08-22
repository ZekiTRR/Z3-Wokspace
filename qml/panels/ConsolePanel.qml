import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Console / solver output log with automatic scrolling.
Rectangle {
    id: pRoot

    color: Theme.panelDark

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: Theme.panel

            Label {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                text: qsTr("CONSOLE")
                color: Theme.textDim
                font.pixelSize: Theme.headerFontSize
                font.bold: true
            }
        }

        ListView {
            id: lstConsole
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 1
            model: workspace.consoleModel

            onCountChanged: function() {
                positionViewAtEnd();
            }

            delegate: RowLayout {
                width: lstConsole.width
                height: 18
                spacing: 8

                Label { text: "[" + cTime + "]"; color: Theme.textDim; font.pixelSize: Theme.fontSize - 2; font.family: "Consolas" }
                Label {
                    Layout.preferredWidth: 64
                    text: cLevel
                    font.pixelSize: Theme.fontSize - 2
                    font.family: "Consolas"
                    color: cLevel === "ERROR" ? Theme.error : (cLevel === "WARNING" ? Theme.warning : Theme.info)
                }
                Label { Layout.fillWidth: true; text: cText; color: Theme.text; font.pixelSize: Theme.fontSize - 1; elide: Text.ElideRight }
            }

            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }
}
