import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebEngine 1.15
import QtWebChannel 1.1

Item {
    id: root
    width: 800
    height: 600

    // 由外部传入
    property var backend0
    property var backend1
    property var backend2
    property var channel

    // 当前选中的串口 index
    property int currentIndex: 0

    // 每个串口的配置缓存
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

    function getBackend(index) {
        if (index === 0) return backend0
        if (index === 1) return backend1
        if (index === 2) return backend2
        return null
    }

    function copyConfig(cfg) {
        return cfg ? {
            baudrate: cfg.baudrate || "115200",
            parity: cfg.parity || "None",
            stopBits: cfg.stopBits || "1",
            dataBits: cfg.dataBits || "8"
        } : {
            baudrate: "115200",
            parity: "None",
            stopBits: "1",
            dataBits: "8"
        }
    }

    Component.onCompleted: {
        Qt.callLater(() => {
            if (!backend0 || !backend1 || !backend2 || !channel) {
                console.error("❌ One or more backend/channel objects are null")
                return
            }

            console.log("✅ All backends and channel are valid")

            for (var i = 0; i < 3; ++i) {
                var b = getBackend(i)
                if (b && b.defaultConfig) {
                    var cfg = b.defaultConfig()
                    if (cfg) configs[i] = copyConfig(cfg)
                }
            }
            configPanel.config = copyConfig(configs[currentIndex])
        })
    }

    onCurrentIndexChanged: {
        configPanel.config = copyConfig(configs[currentIndex])
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

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
                if (newConfig) {
                    configs[currentIndex] = copyConfig(newConfig)
                    let backend = getBackend(currentIndex)
                    if (backend) {
                        if (backend.setBaudrate)
                            backend.setBaudrate(parseInt(newConfig.baudrate))
                        if (backend.setParity)
                            backend.setParity(newConfig.parity)
                        if (backend.setStopBits)
                            backend.setStopBits(newConfig.stopBits)
                        if (backend.setDataBits)
                            backend.setDataBits(parseInt(newConfig.dataBits))
                    }
                }
            }
        }

        Loader {
            id: terminalLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: {
                if (currentIndex === 0) return term0
                if (currentIndex === 1) return term1
                return term2
            }
        }

        StatusIndicators {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            backendConnected: true
            miniPCOnline: true
        }
    }

    // === 各终端视图 ===
    Component {
        id: term0
        WebEngineView {
            id: view0
            anchors.fill: parent
            url: "qrc:/web/index.html?channel=backend0"
            webChannel: channel
            settings.localContentCanAccessFileUrls: true
            settings.localContentCanAccessRemoteUrls: true
            Component.onCompleted: {
                if (webChannel && view0.page) {
                    view0.page.webChannel = webChannel
                    console.log("✅ WebEngineView 0 loaded")
                }
            }
            onLoadingChanged: function(loadRequest) {
                console.error("🔍 Status:", loadRequest.status, "URL:", loadRequest.url);
            }

        }
    }

    Component {
        id: term1
        WebEngineView {
            id: view1
            anchors.fill: parent
            url: "qrc:/web/web/index.html?channel=backend1"
            webChannel: channel
            settings.localContentCanAccessFileUrls: true
            settings.localContentCanAccessRemoteUrls: true
            Component.onCompleted: {
                if (webChannel && view1.page) {
                    view1.page.webChannel = webChannel
                    console.log("✅ WebEngineView 1 loaded")
                }
            }
        }
    }

    Component {
        id: term2
        WebEngineView {
            id: view2
            anchors.fill: parent
            url: "qrc:/web/web/index.html?channel=backend2"
            webChannel: channel
            settings.localContentCanAccessFileUrls: true
            settings.localContentCanAccessRemoteUrls: true
            Component.onCompleted: {
                if (webChannel && view2.page) {
                    view2.page.webChannel = webChannel
                    console.log("✅ WebEngineView 2 loaded")
                }
            }
        }
    }
}
