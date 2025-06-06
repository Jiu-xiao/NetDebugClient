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

    /* 后端设备管理对象（绑定至 C++） */
    DeviceManager {
        id: device_manager
        objectName: "device_manager"
    }

    /* 对话框：开机时提示用户输入设备名 */
    Dialog {
        id: inputDialog
        title: "查找设备名称(留空查找所有设备)"
        visible: true
        modal: true
        anchors.centerIn: parent
        height: parent.height
        width: parent.width

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
                        device_manager.SetDeviceNameFilter(inputField.text)
                        inputDialog.close()
                    }
                }
            }
        }
    }

    /* 延迟构造 WebChannel（用于 JS/C++ 通信） */
    Component {
        id: webChannelComponent
        WebChannel {
            id: channel
        }
    }

    /* 延迟构造主终端界面视图 */
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

    /* 属性变量：WebChannel 实例与终端视图引用 */
    property var qmlWebChannel: null
    property var terminalView: null

    /* 工具函数：验证 backend 是否可用 */
    function isValidBackend(obj) {
        return obj !== null && typeof obj === "object";
    }

    /* 初始化回调：检查并创建 WebChannel 和终端视图 */
    Component.onCompleted: {
        console.log("ApplicationWindow loaded")
        console.debug("backend0Obj:", backend0Obj)
        console.debug("backend1Obj:", backend1Obj)
        console.debug("backend2Obj:", backend2Obj)

        if (isValidBackend(backend0Obj) &&
            isValidBackend(backend1Obj) &&
            isValidBackend(backend2Obj)) {

            console.log("All backends are valid")

            qmlWebChannel = webChannelComponent.createObject(root)
            if (!qmlWebChannel) {
                console.error("Failed to create WebChannel")
                return
            }
            console.log("WebChannel created")

            qmlWebChannel.registerObject("backend0", backend0Obj)
            qmlWebChannel.registerObject("backend1", backend1Obj)
            qmlWebChannel.registerObject("backend2", backend2Obj)
            console.log("WebChannel registered objects")

            terminalView = terminalComponent.createObject(root, {
                backend0: backend0Obj,
                backend1: backend1Obj,
                backend2: backend2Obj,
                channel: qmlWebChannel
            })

            if (!terminalView) {
                console.error("Failed to create MainTerminalView")
            } else {
                console.log("MainTerminalView created")
            }

        } else {
            console.error("One or more backends are null or invalid")
        }
    }
}
