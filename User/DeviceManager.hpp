// DeviceManager.h
#ifndef DeviceManager_H
#define DeviceManager_H

#include <QObject>
#include <QString>
#include "logger.hpp"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void SetDeviceNameFilter(const QString &filter)
    {
        if (filter.isEmpty())
        {
            XR_LOG_DEBUG("Device name filter is empty");
        }
        else
        {
            XR_LOG_DEBUG("Device name filter is: %s", filter.toStdString().c_str());
        }
    }

    Q_INVOKABLE void RenameDevice(const QString &input)
    {
        if (input.isEmpty())
        {
            XR_LOG_DEBUG("Device name is empty");
        }
        else
        {
            XR_LOG_DEBUG("Device name is: %s", input.toStdString().c_str());
        }
    }

    Q_INVOKABLE QString GetLastDeviceName()
    {
        return "测试设备1";
    }
};

#endif // DeviceManager_H
