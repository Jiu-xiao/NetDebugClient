// qml/StatusIndicators.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

RowLayout {
    property bool backendConnected: true
    property bool miniPCOnline: true

    spacing: 24
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignLeft
    Layout.margins: 6

    // Backend 状态灯
    RowLayout {
        spacing: 6
        Rectangle {
            width: 12; height: 12; radius: 6
            color: backendConnected ? "#4caf50" : "#888888"
            border.color: "#333333"
        }
        Label {
            text: "Connected"
            color: "#cccccc"
            font.pixelSize: 13
        }
    }

    // MiniPC 状态灯
    RowLayout {
        spacing: 6
        Rectangle {
            width: 12; height: 12; radius: 6
            color: miniPCOnline ? "#4caf50" : "#888888"
            border.color: "#333333"
        }
        Label {
            text: "MiniPC Online"
            color: "#cccccc"
            font.pixelSize: 13
        }
    }
}
