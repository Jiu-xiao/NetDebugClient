import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: "XRobot Net Debug Tools"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    MainTerminalView {
        anchors.fill: parent
    }
}
