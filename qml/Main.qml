import QtQuick 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
visible: true
width: 600
height: 400
title: "Material Terminal"
Material.theme: Material.Dark
Material.accent: Material.Blue

ColumnLayout {
anchors.fill: parent
spacing: 8
anchors.margins: 8

TextArea {
id: output
readOnly: true
wrapMode: TextArea.Wrap
Layout.fillWidth: true
Layout.fillHeight: true
}

RowLayout {
spacing: 6
Layout.fillWidth: true

TextField {
id: input
Layout.fillWidth: true
placeholderText: "Enter command"
onAccepted: {
backend.sendCommand(input.text)
input.text = ""
}
}

Button {
text: "Send"
onClicked: {
backend.sendCommand(input.text)
input.text = ""
}
}
}
}

Connections {
target: backend
onOutputReceived: output.text += output + "\n"
}
}
