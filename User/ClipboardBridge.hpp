#pragma once

#include <QObject>
#include <QClipboard>
#include <QGuiApplication>

class ClipboardBridge : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE void requestClipboardText() {
        QString text = QGuiApplication::clipboard()->text();
        emit clipboardTextReady(text);
    }

signals:
    void clipboardTextReady(const QString &text);
};
