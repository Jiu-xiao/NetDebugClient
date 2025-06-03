import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: 800
    height: 600

    property int currentIndex: 0
    property var terminalOutputs: ["", "", ""]
    property var backends: [backend0, backend1, backend2]
    property var configs: [
        { baudrate: "115200", parity: "None", stopBits: "1", dataBits: "8" },
        { baudrate: "115200", parity: "None", stopBits: "1", dataBits: "8" },
        { baudrate: "115200", parity: "None", stopBits: "1", dataBits: "8" }
    ]

    ListModel {
        id: terminalModel
        ListElement { name: "MiniPC" }
        ListElement { name: "USART1" }
        ListElement { name: "USART2" }
    }

    function copyConfig(cfg) {
        return {
            baudrate: cfg.baudrate,
            parity: cfg.parity,
            stopBits: cfg.stopBits,
            dataBits: cfg.dataBits
        }
    }

    Component.onCompleted: {
        for (var i = 0; i < backends.length; ++i) {
            var cfg = backends[i].defaultConfig()
            if (cfg) configs[i] = copyConfig(cfg)
        }
        configPanel.config = copyConfig(configs[currentIndex])
    }

    onCurrentIndexChanged: {
        configPanel.config = copyConfig(configs[currentIndex])
    }

    Keys.onPressed: function(event) {
        const backend = backends[currentIndex]
        if (backend && event.text.length > 0)
            backend.sendCommand(currentIndex, event.text)
        event.accepted = true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // 顶部按钮
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "#1e1e1e"
            RowLayout {
                anchors.fill: parent
                spacing: 0
                Repeater {
                    model: terminalModel
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: currentIndex === index ? "#2979FF" : "#2c2c2c"
                        border.color: "#444444"
                        border.width: 1
                        MouseArea {
                            anchors.fill: parent
                            onClicked: currentIndex = index
                        }
                        Text {
                            anchors.centerIn: parent
                            text: model.name
                            color: currentIndex === index ? "white" : "#aaaaaa"
                            font.pixelSize: 16
                            font.bold: currentIndex === index
                        }
                    }
                }
            }
        }

        SerialConfigPanel {
            id: configPanel
            index: currentIndex
            Layout.fillWidth: true
            onUserConfigUpdated: function(newConfig) {
                configs[currentIndex] = copyConfig(newConfig)
                var backend = backends[currentIndex]
                if (backend) {
                    backend.setBaudrate(newConfig.baudrate)
                    backend.setParity(newConfig.parity)
                    backend.setStopBits(newConfig.stopBits)
                    backend.setDataBits(newConfig.dataBits)
                }
            }
        }

        TextArea {
            textFormat: TextEdit.RichText
            Layout.fillWidth: true
            Layout.fillHeight: true
            readOnly: true
            wrapMode: TextArea.WrapAnywhere
            font.family: "monospace"
            font.pixelSize: 14
            color: "#dddddd"
            background: Rectangle { color: "#1e1e1e" }
            text: terminalOutputs[currentIndex]
            onTextChanged: cursorPosition = length
        }

        StatusIndicators {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            backendConnected: true
            miniPCOnline: true
        }
    }

    // 后端连接输出
    Connections { target: backend0; function onOutputReceived(i, o) { if (i === 0) { terminalOutputs[0] += o; if (currentIndex === 0) terminalOutputs = terminalOutputs } } }
    Connections { target: backend1; function onOutputReceived(i, o) { if (i === 1) { terminalOutputs[1] += o; if (currentIndex === 1) terminalOutputs = terminalOutputs } } }
    Connections { target: backend2; function onOutputReceived(i, o) { if (i === 2) { terminalOutputs[2] += o; if (currentIndex === 2) terminalOutputs = terminalOutputs } } }
}
