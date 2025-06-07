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

    // 当前终端索引（0=MiniPC，配置不可编辑）
    property int index: 0

    property bool updating: false

    // 当前配置
    property var config: ({})

    onConfigChanged: {
        updateWidgets();
    }

    property var backend: (index === 0 ? backend0Obj : index === 1 ? backend1Obj : index === 2 ? backend2Obj : null)

    // 所有终端配置模型（应由外部初始化）
    ListModel {
        id: configModel
        ListElement {
            baudrate: "115200"
            parity: "None"
            stopBits: "1"
            dataBits: "8"
            hexOutput: false
            saveToFile: false
        }
        ListElement {
            baudrate: "115200"
            parity: "None"
            stopBits: "1"
            dataBits: "8"
            hexOutput: false
            saveToFile: false
        }
        ListElement {
            baudrate: "115200"
            parity: "None"
            stopBits: "1"
            dataBits: "8"
            hexOutput: false
            saveToFile: false
        }
        ListElement {
            baudrate: "115200"
            parity: "None"
            stopBits: "1"
            dataBits: "8"
            hexOutput: false
            saveToFile: false
        }
    }

    // 向 C++ 通知当前终端配置变更
    signal userConfigUpdated(int index, var config)

    padding: 12
    background: Rectangle {
        color: "#2e2e2e"
        radius: 8
    }

    // 切换 index 时载入配置
    onIndexChanged: {
        if (index >= 0 && index < configModel.count) {
            updateWidgets();
        }
    }

    function findIndex(model, value) {
        for (var i = 0; i < model.length; ++i) {
            if (model[i] === value) {
                return i;
            }
        }
        return -1;
    }

    // 更新控件显示状态
    function updateWidgets() {
        if (!config || typeof config !== "object")
            return;
        updating = true;

        baudrateBox.currentIndex = findIndex(baudrateBox.model, config.baudrate);
        parityBox.currentIndex = findIndex(parityBox.model, config.parity);
        stopBitsBox.currentIndex = findIndex(stopBitsBox.model, config.stopBits);
        dataBitsBox.currentIndex = findIndex(dataBitsBox.model, config.dataBits);

        hexOutputBox.checked = !!config.hexOutput;
        saveToFileBox.checked = !!config.saveToFile;

        updating = false;
    }

    // 更新字段：config + configModel
    function updateConfigField(field, value) {
        if (!config || config[field] === value)
            return;
        config[field] = value;
        var obj = {};
        obj[field] = value;
        configModel.set(index, obj);
        if (!updating)
            userConfigUpdated(index, config);
    }

    Flow {
        spacing: 16
        Layout.fillWidth: true
        width: parent.width

        // Baudrate
        Item {
            width: 120
            height: 40
            opacity: index !== 0 ? 1 : 0.4
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: "Baudrate:"
                    color: "#dddddd"
                    font.pixelSize: 12
                }
                ComboBox {
                    id: baudrateBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600", "1000000", "2000000"]
                    onCurrentIndexChanged: {
                        if (updating)
                            return;
                        updateConfigField("baudrate", model[currentIndex]);
                        if (backend)
                            backend.setBaudrate(model[currentIndex]);
                    }
                }
            }
        }

        // Parity
        Item {
            width: 100
            height: 40
            opacity: index !== 0 ? 1 : 0.4
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: "Parity:"
                    color: "#dddddd"
                    font.pixelSize: 12
                }
                ComboBox {
                    id: parityBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["None", "Even", "Odd"]
                    onCurrentIndexChanged: {
                        if (updating)
                            return;
                        updateConfigField("parity", model[currentIndex]);
                        if (backend)
                            backend.setParity(model[currentIndex]);
                    }
                }
            }
        }

        // StopBits
        Item {
            width: 70
            height: 40
            opacity: index !== 0 ? 1 : 0.4
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: "StopBits:"
                    color: "#dddddd"
                    font.pixelSize: 12
                }
                ComboBox {
                    id: stopBitsBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["1", "2"]
                    onCurrentIndexChanged: {
                        if (updating)
                            return;
                        updateConfigField("stopBits", model[currentIndex]);
                        if (backend)
                            backend.setStopBits(model[currentIndex]);
                    }
                }
            }
        }

        // DataBits
        Item {
            width: 70
            height: 40
            opacity: index !== 0 ? 1 : 0.4
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: "DataBits:"
                    color: "#dddddd"
                    font.pixelSize: 12
                }
                ComboBox {
                    id: dataBitsBox
                    enabled: index !== 0
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    model: ["5", "6", "7", "8"]
                    onCurrentIndexChanged: {
                        if (updating)
                            return;
                        updateConfigField("dataBits", model[currentIndex]);
                        if (backend)
                            backend.setDataBits(model[currentIndex]);
                    }
                }
            }
        }

        // Hex Output
        Item {
            width: 60
            height: 40
            opacity: 1
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: "Hex Output:"
                    color: "#dddddd"
                    font.pixelSize: 12
                }
                CheckBox {
                    id: hexOutputBox
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    checked: false
                    onCheckedChanged: {
                        if (updating)
                            return;
                        updateConfigField("hexOutput", checked);
                        if (backend)
                            backend.setHexOutput(checked);
                    }
                }
            }
        }

        // Save to File
        Item {
            width: 60
            height: 40
            opacity: 1
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: "Save to File:"
                    color: "#dddddd"
                    font.pixelSize: 12
                }
                CheckBox {
                    id: saveToFileBox
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    checked: false
                    onCheckedChanged: {
                        if (updating)
                            return;
                        updateConfigField("saveToFile", checked);
                        if (backend)
                            backend.setSaveToFile(checked);
                    }
                }
            }
        }

        // MiniPC 专用按钮
        Item {
            width: 90
            height: 40
            opacity: index === 0 ? 1 : 0.4
            Behavior on opacity {
                NumberAnimation {
                    duration: 150
                }
            }

            Column {
                anchors.fill: parent
                spacing: 2
                Label {
                    text: " "
                }
                Button {
                    text: "Restart"
                    icon.name: "refresh"
                    width: parent.width
                    height: 40
                    font.pixelSize: 14
                    enabled: index === 0
                    onClicked: {
                        console.log("重启MiniPC");
                        device_manager.RestartMiniPC();
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (index >= 0 && index < configModel.count) {
            var defaultCfg = backend ? backend.defaultConfig() : null;
            if (defaultCfg) {
                for (var key in defaultCfg)
                    configModel.setProperty(index, key, defaultCfg[key]);
            }
            updateWidgets();
        }
    }
}
