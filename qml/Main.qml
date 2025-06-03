import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtWebChannel 1.1

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "XRobot Net Debug Tools"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    // 延迟创建 WebChannel（不带 registeredObjects）
    Component {
        id: webChannelComponent
        WebChannel { }
    }

    // 延迟创建 MainTerminalView
    Component {
        id: terminalComponent
        MainTerminalView {
            backend0: backend0Obj
            backend1: backend1Obj
            backend2: backend2Obj
            channel: qmlWebChannel
            anchors.fill: parent
        }
    }

    property var qmlWebChannel: null
    property var terminalView: null

    function isValidBackend(obj) {
        return obj !== null && typeof obj === "object";
    }

    Component.onCompleted: {
        console.log("✅ ApplicationWindow loaded")
        console.log("🔧 backend0Obj:", backend0Obj)
        console.log("🔧 backend1Obj:", backend1Obj)
        console.log("🔧 backend2Obj:", backend2Obj)

        if (isValidBackend(backend0Obj) && isValidBackend(backend1Obj) && isValidBackend(backend2Obj)) {
            console.log("✅ All backends are valid")

            // 1. 创建 WebChannel
            qmlWebChannel = webChannelComponent.createObject(root)
            if (!qmlWebChannel) {
                console.error("❌ Failed to create WebChannel")
                return
            }
            console.log("✅ WebChannel created")

            // 2. 显式赋值 registeredObjects，避免 null 导致 crash
            qmlWebChannel.registeredObjects["backend0"] = backend0Obj;
            qmlWebChannel.registeredObjects["backend1"] = backend1Obj;
            qmlWebChannel.registeredObjects["backend2"] = backend2Obj;
            console.log("✅ WebChannel registeredObjects assigned")

            // 3. 创建 MainTerminalView
            terminalView = terminalComponent.createObject(root, {
                backend0: backend0Obj,
                backend1: backend1Obj,
                backend2: backend2Obj,
                channel: qmlWebChannel
            })
            if (!terminalView) {
                console.error("❌ Failed to create MainTerminalView")
            } else {
                console.log("✅ MainTerminalView created")
            }
        } else {
            console.error("❌ One or more backends are null or invalid")
        }
    }
}
