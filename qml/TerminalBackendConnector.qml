import QtQuick 2.15

// 用于连接某个终端后端的输出信号
Item {
    // 绑定属性
    property var backend         // 对应终端后端对象
    property int index           // 当前连接的终端编号
    property int currentIndex    // 当前正在显示的终端编号
    property var terminalOutputs // 所有终端的输出文本缓存列表

    visible: false  // 本组件不参与显示，仅用于信号连接

    // 信号连接
    Connections {
        target: backend

        // 当后端发出新输出信号时执行
        function onOutputReceived(idx, output) {
            if (idx !== index)
                return;

            terminalOutputs[index] += output;

            // 若该终端当前正在显示，强制触发 terminalOutputs 的绑定更新
            if (currentIndex === index)
                terminalOutputs = terminalOutputs;
        }
    }
}
