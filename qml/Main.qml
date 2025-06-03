import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtWebChannel 1.1
import com.example 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "XRobot Net Debug Tools"
    Material.theme: Material.Dark
    Material.accent: Material.Blue

    DeviceManager {
        id: device_manager
    }

    // 对话框组件，用于接受用户输入
    Dialog {
        id: inputDialog
        title: "查找设备名称(留空查找所有设备)"
        visible: true // 默认显示对话框
        modal: true
        anchors.centerIn: parent
        height: parent.height
        width: parent.width

        // 对话框内容
        Column {
            anchors.centerIn: parent

            TextField {
                id: inputField
                placeholderText: device_manager.GetLastDeviceName()
                width: 200
            }

            Row {
                spacing: 10

                Button {
                    text: "确定"
                    onClicked: {
                        // 调用 C++ 函数处理输入
                        device_manager.SetDeviceNameFilter(inputField.text)
                        inputDialog.close()  // 关闭对话框
                    }
                }
            }
        }
    }

    // 延迟创建 WebChannel（不带 registeredObjects）
    Component {
        id: webChannelComponent
        WebChannel {
            id: channel
        }
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
        console.debug("🔧 backend0Obj:", backend0Obj)
        console.debug("🔧 backend1Obj:", backend1Obj)
        console.debug("🔧 backend2Obj:", backend2Obj)

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
            qmlWebChannel.registerObject("backend0" ,backend0Obj);
            qmlWebChannel.registerObject("backend1" ,backend1Obj);
            qmlWebChannel.registerObject("backend2" ,backend2Obj);
            console.log("✅ WebChannel registered objects")

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
