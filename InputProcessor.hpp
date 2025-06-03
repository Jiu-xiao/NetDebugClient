// InputProcessor.h
#ifndef INPUTPROCESSOR_H
#define INPUTPROCESSOR_H

#include <QObject>
#include <QString>
#include <qdebug.h>

class InputProcessor : public QObject
{
    Q_OBJECT
public:
    explicit InputProcessor(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void processInput(const QString &input)
    {
        // 在这里处理输入的字符串
        if (input.isEmpty())
        {
            qDebug() << "Input is empty!";
        }
        else
        {
            qDebug() << "Processed input: " << input;
        }
    }
};

#endif // INPUTPROCESSOR_H
