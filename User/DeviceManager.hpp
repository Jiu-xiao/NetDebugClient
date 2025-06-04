#ifndef DeviceManager_H
#define DeviceManager_H

#include "logger.hpp"
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTextStream>

class DeviceManager : public QObject {
  Q_OBJECT
public:
  bool filter_is_set_ = false;

  bool require_rename_ = false;

  bool require_restart_ = false;

  QString last_device_name_ = "";

  explicit DeviceManager(QObject *parent = nullptr) : QObject(parent) {
    // 开机时尝试读取设备名称
    readDeviceNameFromFile();
  }

  Q_INVOKABLE void SetDeviceNameFilter(const QString &filter) {
    if (filter.isEmpty()) {
      XR_LOG_INFO("Device name filter is empty");
    } else {
      XR_LOG_INFO("Device name filter is: %s", filter.toStdString().c_str());
    }
  }

  Q_INVOKABLE void RenameDevice(const QString &input) {
    if (input.isEmpty()) {
      XR_LOG_INFO("Device name is empty");
    } else {
      XR_LOG_INFO("Device name is: %s", input.toStdString().c_str());
      last_device_name_ = input;
      // 保存新的设备名称到文件
      saveDeviceNameToFile();
    }
  }

  Q_INVOKABLE QString GetLastDeviceName() { return last_device_name_; }

  Q_INVOKABLE void RestartMiniPC() {
    XR_LOG_INFO("Restart MiniPC");
    require_restart_ = true;
  }

private:
  void readDeviceNameFromFile() {
    QFile file("Device.info");
    if (!file.exists()) {
      XR_LOG_INFO(
          "Device.info file does not exist. No previous device name to load.");
      return;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);
      last_device_name_ = in.readLine();
      XR_LOG_INFO("Loaded device name from file: %s",
                  last_device_name_.toStdString().c_str());
      file.close();
    } else {
      XR_LOG_INFO("Failed to open Device.info for reading.");
    }
  }

  void saveDeviceNameToFile() {
    QFile file("Device.info");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&file);
      out << last_device_name_ << "\n";
      XR_LOG_INFO("Saved device name to file: %s",
                  last_device_name_.toStdString().c_str());
      file.close();
    } else {
      XR_LOG_INFO("Failed to open Device.info for writing.");
    }
  }
};

#endif // DeviceManager_H
