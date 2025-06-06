import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

Frame {
    id: configPanel

    Layout.fillWidth: true
    Layout.minimumHeight: 80
    Material.theme: Material.Dark
    Material.accent: Material.Teal

    // 当前终端索引（0 = MiniPC，禁用配置）
    property int index: 0

    // 当前配置项（由外部传入并绑定）
    property var config: {
        "baudrate": "115200",
        "parity": "None",
        "stopBits": "1",
        "dataBits": "8"
    }

    // 配置更改信号，供外部同步更新
    signal userConfigUpdated(var config)

    padding: 12
    background: Rectangle {
        color: "#2e2e2e"
        radius: 8
    }

    // 更新配置项字段值（避免重复触发）
    function updateConfigField(field, value) {
        if (config && config[field] !== value) {
            config[field] = value
            userConfigUpdated(config)
        }
    }

    // 当配置变更时更新各 ComboBox 的选项索引
    onConfigChanged: {
        if (!config || typeof config !== "object") return
        baudrateBox.currentIndex = baudrateBox.model.indexOf(config.baudrate)
        parityBox.currentIndex   = parityBox.model.indexOf(config.parity)
        stopBitsBox.currentIndex = stopBitsBox.model.indexOf(config.stopBits)
        dataBitsBox.currentIndex = dataBitsBox.model.indexOf(config.dataBits)
    }

    Flow {
        spacing: 16
        Layout.fillWidth: true
        width: parent.width

        // Baudrate 配置
        Item {
            width: 120; height: 40
            opacity: index !== 0 ? 1.0 : 0.4
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Column {
                anchors.fill: parent
                spacing: 2
                Label { text: "Baudrate:"; color: "#dddddd"; font.pixelSize: 12 }
                ComboBox {
                    id: baudrateBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600", "1000000", "2000000"]
                    onCurrentIndexChanged: updateConfigField("baudrate", model[currentIndex])
                }
            }
        }

        // Parity 校验位
        Item {
            width: 100; height: 40
            opacity: index !== 0 ? 1.0 : 0.4
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Column {
                anchors.fill: parent
                spacing: 2
                Label { text: "Parity:"; color: "#dddddd"; font.pixelSize: 12 }
                ComboBox {
                    id: parityBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["None", "Even", "Odd"]
                    onCurrentIndexChanged: updateConfigField("parity", model[currentIndex])
                }
            }
        }

        // StopBits 停止位
        Item {
            width: 70; height: 40
            opacity: index !== 0 ? 1.0 : 0.4
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Column {
                anchors.fill: parent
                spacing: 2
                Label { text: "StopBits:"; color: "#dddddd"; font.pixelSize: 12 }
                ComboBox {
                    id: stopBitsBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["1", "2"]
                    onCurrentIndexChanged: updateConfigField("stopBits", model[currentIndex])
                }
            }
        }

        // DataBits 数据位
        Item {
            width: 70; height: 40
            opacity: index !== 0 ? 1.0 : 0.4
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Column {
                anchors.fill: parent
                spacing: 2
                Label { text: "DataBits:"; color: "#dddddd"; font.pixelSize: 12 }
                ComboBox {
                    id: dataBitsBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["5", "6", "7", "8"]
                    onCurrentIndexChanged: updateConfigField("dataBits", model[currentIndex])
                }
            }
        }

        // MiniPC 专用：重启按钮
        Item {
            width: 90; height: 40
            opacity: index === 0 ? 1.0 : 0.4
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Column {
                anchors.fill: parent
                spacing: 2
                Label { text: " " }
                Button {
                    text: "Restart"
                    icon.name: "refresh"
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    enabled: index === 0
                    onClicked: {
                        console.log("重启MiniPC")
                        device_manager.RestartMiniPC()
                    }
                }
            }
        }
    }
}
