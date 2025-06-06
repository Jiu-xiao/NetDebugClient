#pragma once

#include "logger.hpp"
#include <QDebug>
#include <QFile>
#include <QObject>
#include <QString>
#include <QTextStream>

class DeviceManager : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool backendConnected READ isBackendConnected NOTIFY
                 backendConnectedChanged)
  Q_PROPERTY(bool miniPCOnline READ isMiniPCOnline NOTIFY miniPCOnlineChanged)

public:
  explicit DeviceManager(QObject *parent = nullptr) : QObject(parent) {
    /* 构造函数中尝试读取上次保存的设备名称 */
    readDeviceNameFromFile();
  }

  /*
   * 设置设备名称过滤器：
   * - 用于 UDP 广播匹配；
   * - 设置后会影响广播内容；
   */
  Q_INVOKABLE void SetDeviceNameFilter(const QString &filter) {
    if (filter.isEmpty()) {
      XR_LOG_INFO("Device name filter is empty");
    } else {
      filter_name_ = filter;
      XR_LOG_INFO("Device name filter is: %s", filter.toStdString().c_str());
    }
    filter_is_set_ = true;
  }

  /*
   * 重命名设备：
   * - 设置新名称并持久化到文件；
   * - 标记 require_rename_ 以触发远端改名命令；
   */
  Q_INVOKABLE void RenameDevice(const QString &input) {
    if (input.isEmpty()) {
      XR_LOG_INFO("Device name is empty");
    } else {
      XR_LOG_INFO("Device name is: %s", input.toStdString().c_str());
      last_device_name_ = input;
      require_rename_ = true;
      saveDeviceNameToFile();
    }
  }

  /*
   * 获取当前设备名称（上次重命名或启动时读取的值）
   */
  Q_INVOKABLE QString GetLastDeviceName() { return last_device_name_; }

  /*
   * 触发 MiniPC 重启命令：
   * - 标记 require_restart_；
   * - 后续由定时器检测并发送实际命令。
   */
  Q_INVOKABLE void RestartMiniPC() {
    XR_LOG_INFO("Restart MiniPC");
    require_restart_ = true;
  }

  /*
   * 设置串口后端是否连接状态（用于 UI 显示）
   */
  Q_INVOKABLE void SetBackendConnected(bool connected) {
    if (backend_connected_ != connected) {
      backend_connected_ = connected;
      emit backendConnectedChanged();
    }
  }

  /*
   * 设置 MiniPC 是否在线（用于 UI 显示）
   */
  Q_INVOKABLE void SetMiniPCOnline(bool online) {
    if (mini_pc_online_ != online) {
      mini_pc_online_ = online;
      emit miniPCOnlineChanged();
    }
  }

  /* 获取当前连接状态 */
  bool isBackendConnected() const { return backend_connected_; }

  bool isMiniPCOnline() const { return mini_pc_online_; }

signals:
  void backendConnectedChanged();
  void miniPCOnlineChanged();

public:
  /* 公共状态字段（由外部直接读写） */
  bool filter_is_set_ = false;
  bool require_rename_ = false;
  bool require_restart_ = false;
  bool backend_connected_ = false;
  bool mini_pc_online_ = false;

  QString last_device_name_ = "";
  QString filter_name_ = "";

private:
  /*
   * 从配置文件加载设备名（程序启动时调用）：
   * - 文件不存在时不报错；
   * - 文件路径固定为 "Device.cfg"。
   */
  void readDeviceNameFromFile() {
    QFile file("Device.cfg");
    if (!file.exists()) {
      XR_LOG_INFO(
          "Device.cfg file does not exist. No previous device name to load.");
      return;
    }

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);
      last_device_name_ = in.readLine();
      XR_LOG_INFO("Loaded device name from file: %s",
                  last_device_name_.toStdString().c_str());
      file.close();
    } else {
      XR_LOG_INFO("Failed to open Device.cfg for reading.");
    }
  }

  /*
   * 将当前设备名称写入配置文件：
   * - 覆盖写入 "Device.cfg"；
   * - UTF-8 编码；
   */
  void saveDeviceNameToFile() {
    QFile file("Device.cfg");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&file);
      out << last_device_name_ << "\n";
      XR_LOG_INFO("Saved device name to file: %s",
                  last_device_name_.toStdString().c_str());
      file.close();
    } else {
      XR_LOG_INFO("Failed to open Device.cfg for writing.");
    }
  }
};
