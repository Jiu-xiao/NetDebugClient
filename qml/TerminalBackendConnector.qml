import QtQuick 2.15

Item {
    property var backend
    property int index
    property int currentIndex
    property var terminalOutputs

    Connections {
        target: backend
        function onOutputReceived(idx, output) {
            if (idx !== index)
                return
            terminalOutputs[index] += output
            if (currentIndex === index)
                terminalOutputs = terminalOutputs
        }
    }

    visible: false
}
