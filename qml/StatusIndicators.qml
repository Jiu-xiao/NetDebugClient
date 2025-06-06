import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

RowLayout {
    // 当前设备名称（用于展示与重命名）
    property string deviceName: ""

    Material.theme: Material.Dark
    Material.accent: Material.Teal

    spacing: 24
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignLeft
    Layout.margins: 6

    // 指示灯：后端连接状态
    RowLayout {
        spacing: 6
        Rectangle {
            width: 12; height: 12; radius: 6
            color: device_manager.backendConnected ? "#4caf50" : "#888888"
            border.color: "#333333"
        }
        Label {
            text: "Connected"
            color: "#cccccc"
            font.pixelSize: 13
        }
    }

    // 指示灯：MiniPC 在线状态
    RowLayout {
        spacing: 6
        Rectangle {
            width: 12; height: 12; radius: 6
            color: device_manager.miniPCOnline ? "#4caf50" : "#888888"
            border.color: "#333333"
        }
        Label {
            text: "MiniPC Online"
            color: "#cccccc"
            font.pixelSize: 13
        }
    }

    // 重命名按钮：打开对话框
    Button {
        text: "修改名称"
        enabled: device_manager.backendConnected
        onClicked: renameDialog.open()
        Layout.alignment: Qt.AlignLeft
    }

    // 弹出对话框：重命名设备
    Dialog {
        id: renameDialog
        title: "修改设备名称"
        modal: true
        width: 300
        anchors.centerIn: parent.parent

        standardButtons: Dialog.Ok | Dialog.Cancel

        // 接受时发送重命名请求
        onAccepted: {
            device_manager.RenameDevice(nameInput.text)
            deviceName = nameInput.text
            console.log("新设备名称:", nameInput.text)
        }

        // 内容区域：输入 + 计数
        contentItem: ColumnLayout {
            spacing: 16
            width: parent.width

            TextField {
                id: nameInput
                Layout.fillWidth: true
                placeholderText: device_manager.GetLastDeviceName()
                text: deviceName
                maximumLength: 20
                selectByMouse: true

                Keys.onReturnPressed: renameDialog.accept()
                Keys.onEscapePressed: renameDialog.reject()
            }

            // 实时字符数显示
            Label {
                Layout.alignment: Qt.AlignRight
                text: nameInput.length + "/" + nameInput.maximumLength
                color: nameInput.length >= nameInput.maximumLength ? "#ff6d00" : "#888"
                font.pixelSize: 12
            }
        }

        // 对话框打开时自动聚焦并全选
        onOpened: {
            nameInput.forceActiveFocus()
            nameInput.selectAll()
        }

        // 关闭时恢复内容
        onClosed: nameInput.text = deviceName

        // 禁用确认按钮当输入为空
        Component.onCompleted: {
            standardButton(Dialog.Ok).enabled = Qt.binding(() => nameInput.text.trim() !== "")
        }
    }
}
