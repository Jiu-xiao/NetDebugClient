import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

RowLayout {
    property bool backendConnected: true
    property bool miniPCOnline: true
    property string deviceName: ""  // 当前设备名称

    Material.theme: Material.Dark
    Material.accent: Material.Teal

    spacing: 24
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignLeft
    Layout.margins: 6

    // 状态指示灯区域
    RowLayout {
        spacing: 6
        Rectangle {
            width: 12; height: 12; radius: 6
            color: backendConnected ? "#4caf50" : "#888888"
            border.color: "#333333"
        }
        Label {
            text: "Connected"
            color: "#cccccc"
            font.pixelSize: 13
        }
    }

    RowLayout {
        spacing: 6
        Rectangle {
            width: 12; height: 12; radius: 6
            color: miniPCOnline ? "#4caf50" : "#888888"
            border.color: "#333333"
        }
        Label {
            text: "MiniPC Online"
            color: "#cccccc"
            font.pixelSize: 13
        }
    }

    // 改名按钮 - 触发弹窗
    Button {
        text: "修改名称"
        onClicked: renameDialog.open()
        Layout.alignment: Qt.AlignLeft
        enabled: backendConnected
    }

    // 设备重命名对话框
    Dialog {
        id: renameDialog
        title: "修改设备名称"
        anchors.centerIn: parent.parent
        width: 300
        modal: true

        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            device_manager.RenameDevice(nameInput.text)
            deviceName = nameInput.text
            console.log("新设备名称:", nameInput.text)
        }

        // 自定义内容区域
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

                // 回车键提交
                Keys.onReturnPressed: renameDialog.accept()
                // ESC键关闭
                Keys.onEscapePressed: renameDialog.reject()
            }

            // 字符计数显示
            Label {
                Layout.alignment: Qt.AlignRight
                text: nameInput.length + "/" + nameInput.maximumLength
                color: nameInput.length >= nameInput.maximumLength ? "#ff6d00" : "#888"
                font.pixelSize: 12
            }
        }

        // 禁用确认按钮当名称为空
        onOpened: {
            nameInput.forceActiveFocus()
            nameInput.selectAll()
        }
        onClosed: nameInput.text = deviceName
        Component.onCompleted: {
            standardButton(Dialog.Ok).enabled = Qt.binding(() => nameInput.text.trim() !== "")
        }
    }
}