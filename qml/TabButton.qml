import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

Button {
    id: tabButton

    checkable: true
    implicitHeight: 72
    implicitWidth: 160
    padding: 8

    background: Rectangle {
        color: "transparent"
        border.color: checked ? "#2196f3" : "#555555"
        border.width: 1
        radius: 6
    }

    contentItem: Label {
        text: tabButton.text
        anchors.centerIn: parent
        font.bold: tabButton.checked
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: "#ffffff"
    }
}
