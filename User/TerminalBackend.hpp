#pragma once

#include "libxr.hpp"
#include "libxr_rw.hpp"
#include "ramfs.hpp"
#include "terminal.hpp"
#include "uart.hpp"

#include <QFile>
#include <QIODevice>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStringList>
#include <QTextStream>
#include <QVariant>
#include <QVariantMap>

/*
 * 命令结构体：
 * 通过 Topic 通道在前后端之间传输控制命令；
 * 包括 Ping、重启、重命名、串口配置等。
 */
class Command {
public:
  enum class Type : uint8_t {
    PING = 0,        /* 前端 Ping 后端，用于状态检测 */
    REMOTE_PING = 1, /* 后端 Ping 前端，用于状态检测 */
    REBOOT = 2,      /* 重启 MiniPC */
    RENAME = 3,      /* 重命名设备 */
    CONFIG_UART = 4  /* 配置指定串口参数 */
  };

  Type type;

  union {
    char device_name[32]; /* 用于 RENAME 命令 */

    struct {
      uint8_t uart_index;                /* 串口索引 */
      LibXR::UART::Configuration config; /* 串口配置结构体 */
    } uart_config;
  } data;
};

/*
 * TerminalBackend：终端后端管理类
 * - 管理与具体串口的读写通道；
 * - 维护每个终端的 Topic 通道；
 * - 管理串口配置及保存加载；
 * - 支持与 QML UI 交互。
 */
class TerminalBackend : public QObject {
  Q_OBJECT

public:
  /*
   * 构造函数：
   * - name 表示终端名称（如 "uart1"）；
   * - index 表示序号；
   * - parent 通常为 QQmlApplicationEngine 指针，用于注入上下文。
   */
  explicit TerminalBackend(const char *name, uint8_t index,
                           QQmlApplicationEngine *parent = nullptr);

public slots:
  /*
   * 向串口发送文本：
   * - 支持从 QML 调用；
   */
  void sendText(const QString &text);

  /*
   * 设置串口配置项（由 QML 控制）：
   * - 分别为波特率、校验位、停止位、数据位；
   */
  Q_INVOKABLE void setBaudrate(const QString &baud);
  Q_INVOKABLE void setParity(const QString &parity);
  Q_INVOKABLE void setStopBits(const QString &stopBits);
  Q_INVOKABLE void setDataBits(const QString &dataBits);
  Q_INVOKABLE void setHexOutput(bool enabled);
  Q_INVOKABLE void setSaveToFile(bool enabled);
  /*
   * 获取默认串口配置（用于界面初始化）；
   */
  Q_INVOKABLE QVariantMap defaultConfig() const;

signals:
  /*
   * 接收到串口文本数据信号（供 QML 显示）；
   */
  void receiveText(const QString &text);

public:
  /*
   * 从配置文件加载当前终端串口参数；
   */
  void loadConfigFromFile();

  /*
   * 将当前终端串口参数保存到文件；
   */
  void saveConfigToFile();

  /*
   * 将当前 config_ 打包为 CONFIG_UART 命令，推送到 Topic；
   */
  void syncConfig();

public:
  const char *name_;       /* 串口终端名称 */
  uint8_t index_;          /* 串口索引 */
  LibXR::ReadPort read_;   /* 读取端口 */
  LibXR::WritePort write_; /* 写入端口 */
  LibXR::Topic topic_;     /* 本终端使用的 Topic 通道 */

  uint8_t pack_buffer_[2][40960]; /* 打包用的临时缓冲区 */

  /*
   * 串口配置（默认值为 460800 8N1）：
   * 可通过 UI 或命令修改；
   */
  LibXR::UART::Configuration config_ = {460800, LibXR::UART::Parity::NO_PARITY,
                                        8, 1};

  bool hex_output_ = false;
  bool save_to_file_ = false;
  uint64_t count_ = 0;

  const QString output_file_dir_;
  const QString output_file_tag_ =
      QDateTime::currentDateTime().toString("yyyy-MM-dd_HH:mm:ss_");
  QFile output_file_;
  QTextStream *output_file_stream_ = NULL;
};
