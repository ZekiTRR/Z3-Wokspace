import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Modal single-line input used for New/Rename problem actions.
Popup {
    id: pRoot

    property string strTitle: ""
    property string strValue: ""
    signal accepted(string strText)

    modal: true
    anchors.centerIn: parent
    width: 320
    padding: 12
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.panel
        border.color: Theme.border
        radius: 4
    }

    onOpened: function() {
        fldName.text = strValue;
        fldName.selectAll();
        fldName.forceActiveFocus();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Label {
            text: pRoot.strTitle
            color: Theme.textDim
            font.pixelSize: Theme.fontSize
        }

        TextField {
            id: fldName
            Layout.fillWidth: true
            font.pixelSize: Theme.fontSize
            selectByMouse: true
            color: Theme.text

            background: Rectangle {
                color: Theme.editor
                border.color: fldName.activeFocus ? Theme.accent : Theme.border
                radius: 3
            }

            onAccepted: function() { btnOk.clicked(); }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 8

            Button {
                text: qsTr("Cancel")
                flat: true
                onClicked: function() { pRoot.close(); }
            }

            Button {
                id: btnOk
                text: qsTr("OK")
                highlighted: true
                onClicked: function() {
                    if (fldName.text.length > 0) {
                        pRoot.accepted(fldName.text);
                    }
                    pRoot.close();
                }
            }
        }
    }
}
