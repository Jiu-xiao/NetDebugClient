import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

// 可选中标签按钮组件
Button {
    id: tabButton

    checkable: true         // 设置为可选中按钮
    implicitHeight: 72      // 默认高度
    implicitWidth: 160      // 默认宽度
    padding: 8              // 内边距

    // 自定义背景：带边框高亮
    background: Rectangle {
        color: "transparent"
        border.color: checked ? "#2196f3" : "#555555"  // 选中时蓝色边框
        border.width: 1
        radius: 6
    }

    // 自定义文本内容样式
    contentItem: Label {
        text: tabButton.text
        anchors.centerIn: parent
        font.bold: tabButton.checked                    // 选中时加粗
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: "#ffffff"
    }
}
