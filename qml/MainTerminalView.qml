import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebEngine 1.15
import QtWebChannel 1.1

Item {
    id: root
    width: 800
    height: 600

    property var backend0
    property var backend1
    property var backend2
    property var channel

    property int currentIndex: 0

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
        return index === 0 ? backend0 : index === 1 ? backend1 : backend2
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
                console.error("One or more backend/channel objects are null")
                return
            }
            console.log("All backends and channel are valid")

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

    Connections {
        target: clipboardBridge
        onClipboardTextReady: function(text) {
            console.log("QML Receive Clipboard:", text);
            let escaped = text.replace(/\\/g, "\\\\")
                            .replace(/`/g, "\\`")
                            .replace(/\$/g, "\\$");

            const currentWebView = terminalStack.itemAt(root.currentIndex);
            if (currentWebView) {
                currentWebView.runJavaScript(`window.pasteFromClipboard(${JSON.stringify(text)});`)
            } else {
                console.error("Cannot find current WebView");
            }
        }
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

        StackLayout {
            id: terminalStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentIndex

            WebEngineView {
                id: webview1
                url: "qrc:/web/index.html?channel=backend0"
                webChannel: channel
                settings.localContentCanAccessFileUrls: true
                settings.localContentCanAccessRemoteUrls: true
                Component.onCompleted: console.log("Terminal 0 Ready")
                onContextMenuRequested: function(request) {
                    console.error("Context menu triggered with:", request.selectedText);
                    request.accepted = true;
                    const selected = request.selectedText;
                    if (selected && selected.length > 0) {
                        webview1.runJavaScript(`doCopy(${JSON.stringify(request.selectedText)});`);
                    } else {
                        forceActiveFocus(); 
                        clipboardBridge.requestClipboardText(); 
                    }
                }
            }

            WebEngineView {
                id: webview2
                url: "qrc:/web/index.html?channel=backend1"
                webChannel: channel
                settings.localContentCanAccessFileUrls: true
                settings.localContentCanAccessRemoteUrls: true
                Component.onCompleted: console.log("Terminal 1 Ready")
                onContextMenuRequested: function(request) {
                    console.error("Context menu triggered with:", request.selectedText);
                    request.accepted = true;
                    const selected = request.selectedText;
                    if (selected && selected.length > 0) {
                        webview2.runJavaScript(`doCopy(${JSON.stringify(request.selectedText)});`);
                    } else {
                        forceActiveFocus(); 
                        clipboardBridge.requestClipboardText(); 
                    }
                }
            }

            WebEngineView {
                id: webview3
                url: "qrc:/web/index.html?channel=backend2"
                webChannel: channel
                settings.localContentCanAccessFileUrls: true
                settings.localContentCanAccessRemoteUrls: true
                Component.onCompleted: console.log("Terminal 2 Ready")
                onContextMenuRequested: function(request) {
                    console.error("Context menu triggered with:", request.selectedText);
                    request.accepted = true;
                    const selected = request.selectedText;
                    if (selected && selected.length > 0) {
                        webview3.runJavaScript(`doCopy(${JSON.stringify(request.selectedText)});`);
                    } else {
                        forceActiveFocus(); 
                        clipboardBridge.requestClipboardText(); 
                    }
                }
            }
        }

        StatusIndicators {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
        }
    }
}
