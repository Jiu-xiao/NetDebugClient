#include "TerminalBackend.hpp"
#include "libxr_def.hpp"
#include "libxr_rw.hpp"
#include "libxr_type.hpp"
#include "lockfree_queue.hpp"
#include "logger.hpp"
#include "ramfs.hpp"

#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTextStream>
#include <QVariant>
#include <QVariantMap>
#include <string>

using namespace LibXR;

/*
 * 写回调函数：
 * - 将 WritePort 中的数据转为 UTF-8 字符串并发出 receiveText 信号；
 * - 用于终端界面输出；
 */
static ErrorCode Write(WritePort &port) {
  uint8_t buffer[1024];
  WriteInfoBlock info;
  TerminalBackend *self = CONTAINER_OF(&port, TerminalBackend, write_);

  while (port.Size() > 0) {
    port.queue_info_->Pop(info);
    port.queue_data_->PopBatch(buffer, info.data.size_);
    emit self->receiveText(
        QString::fromUtf8(reinterpret_cast<char *>(buffer), info.data.size_));
    port.Finish(false, ErrorCode::OK, info, 0);
  }

  return ErrorCode::OK;
}

/*
 * 读回调函数（当前未使用）；
 */
static ErrorCode Read(ReadPort &port) {
  TerminalBackend *self = CONTAINER_OF(&port, TerminalBackend, read_);
  return ErrorCode::FAILED;
}

/*
 * 构造函数：
 * - 绑定读写函数；
 * - 从配置文件加载串口参数；
 * - 注册 Topic 回调处理；
 * - 将自身注入 QML 上下文中；
 */
TerminalBackend::TerminalBackend(const char *name, uint8_t index,
                                 QQmlApplicationEngine *parent)
    : QObject(parent), name_(name), index_(index), read_(40960), write_(40960),
      topic_(name, 40960) {
  read_ = Read;
  write_ = Write;

  loadConfigFromFile();

  void (*from_tcp_cb_fun)(bool, TerminalBackend *, RawData &) =
      [](bool, TerminalBackend *self, RawData &data) {
        XR_LOG_DEBUG("%s received data: %d", self->name_, data.size_);
        emit self->receiveText(QString::fromUtf8(
            reinterpret_cast<char *>(data.addr_), data.size_));
      };

  auto callback = Topic::Callback::Create(from_tcp_cb_fun, this);
  topic_.RegisterCallback(callback);

  parent->rootContext()->setContextProperty(
      (std::string("backend") + std::to_string(index_) + "Obj").c_str(), this);
}

/*
 * 向串口发送文本指令；
 * - 将指令打包为 Topic 格式数据并写入 ReadPort；
 */
void TerminalBackend::sendText(const QString &command) {
  QByteArray bytes = command.toUtf8();
  XR_LOG_DEBUG("Send command: %s", command.toUtf8().data());

  LibXR::Topic::PackData(topic_.GetKey(), pack_buffer_[0],
                         {bytes.data(), static_cast<size_t>(bytes.size())});

  if (index_ == 0) {
    LibXR::Topic::PackData(topic_.GetKey(), pack_buffer_[1],
                           {pack_buffer_[0], static_cast<size_t>(bytes.size()) +
                                                 LibXR::Topic::PACK_BASE_SIZE});
    read_.queue_data_->PushBatch(pack_buffer_[1],
                                 static_cast<size_t>(bytes.size()) +
                                     LibXR::Topic::PACK_BASE_SIZE * 2);
  } else {
    read_.queue_data_->PushBatch(pack_buffer_[0],
                                 static_cast<size_t>(bytes.size()) +
                                     LibXR::Topic::PACK_BASE_SIZE);
  }
}

/*
 * QML 调用：设置波特率并保存配置；
 */
Q_INVOKABLE void TerminalBackend::setBaudrate(const QString &baud) {
  XR_LOG_INFO("Baudrate set to %s", baud.toStdString().c_str());
  config_.baudrate = baud.toUInt();
  saveConfigToFile();
}

/*
 * QML 调用：设置校验位并保存配置；
 */
Q_INVOKABLE void TerminalBackend::setParity(const QString &parity) {
  XR_LOG_INFO("Parity set to %s", parity.toStdString().c_str());
  if (parity == "None")
    config_.parity = UART::Parity::NO_PARITY;
  else if (parity == "Even")
    config_.parity = UART::Parity::EVEN;
  else if (parity == "Odd")
    config_.parity = UART::Parity::ODD;
  saveConfigToFile();
}

/*
 * QML 调用：设置停止位并保存配置；
 */
Q_INVOKABLE void TerminalBackend::setStopBits(const QString &stopBits) {
  XR_LOG_INFO("Stop bits set to %s", stopBits.toStdString().c_str());
  config_.stop_bits = stopBits.toUInt();
  saveConfigToFile();
}

/*
 * QML 调用：设置数据位并保存配置；
 */
Q_INVOKABLE void TerminalBackend::setDataBits(const QString &dataBits) {
  XR_LOG_INFO("Data bits set to %s", dataBits.toStdString().c_str());
  config_.data_bits = dataBits.toUInt();
  saveConfigToFile();
}

/*
 * QML 获取默认配置（当前配置）；
 */
Q_INVOKABLE QVariantMap TerminalBackend::defaultConfig() const {
  QVariantMap configMap;
  configMap["baudrate"] = QString::number(config_.baudrate);
  configMap["parity"] = (config_.parity == UART::Parity::NO_PARITY) ? "None"
                        : (config_.parity == UART::Parity::EVEN)    ? "Even"
                                                                    : "Odd";
  configMap["stopBits"] = QString::number(config_.stop_bits);
  configMap["dataBits"] = QString::number(config_.data_bits);

  XR_LOG_INFO("Load current config: Baudrate = %d, Parity = %d, Stop Bits = "
              "%d, Data Bits = %d",
              config_.baudrate, static_cast<int>(config_.parity),
              config_.stop_bits, config_.data_bits);

  return configMap;
}

/*
 * 从配置文件加载串口参数（如 uart_config_0.cfg）；
 */
void TerminalBackend::loadConfigFromFile() {
  QString filename = QString("uart_config_%1.cfg").arg(index_);
  QFile file(filename);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    XR_LOG_WARN("Failed to open %s for reading, using default config.",
                filename.toStdString().c_str());
    return;
  }

  QTextStream in(&file);
  QString baudrateStr = in.readLine();
  QString parityStr = in.readLine();
  QString stopBitsStr = in.readLine();
  QString dataBitsStr = in.readLine();

  if (!baudrateStr.isEmpty())
    config_.baudrate = baudrateStr.toUInt();

  if (!parityStr.isEmpty()) {
    if (parityStr == "None")
      config_.parity = UART::Parity::NO_PARITY;
    else if (parityStr == "Even")
      config_.parity = UART::Parity::EVEN;
    else if (parityStr == "Odd")
      config_.parity = UART::Parity::ODD;
  }

  if (!stopBitsStr.isEmpty())
    config_.stop_bits = stopBitsStr.toUInt();

  if (!dataBitsStr.isEmpty())
    config_.data_bits = dataBitsStr.toUInt();

  XR_LOG_INFO("Loaded configuration: Baudrate = %d, Parity = %d, Stop Bits = "
              "%d, Data Bits = %d",
              config_.baudrate, static_cast<int>(config_.parity),
              config_.stop_bits, config_.data_bits);

  file.close();
}

/*
 * 将当前配置保存到文件，并调用 syncConfig 推送命令；
 */
void TerminalBackend::saveConfigToFile() {
  syncConfig();

  QString filename = QString("uart_config_%1.cfg").arg(index_);
  QFile file(filename);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    XR_LOG_WARN("Failed to open %s for writing.",
                filename.toStdString().c_str());
    return;
  }

  QTextStream out(&file);
  out << config_.baudrate << "\n";
  out << (config_.parity == UART::Parity::NO_PARITY
              ? "None"
              : (config_.parity == UART::Parity::EVEN ? "Even" : "Odd"))
      << "\n";
  out << config_.stop_bits << "\n";
  out << config_.data_bits << "\n";

  XR_LOG_INFO("Saved configuration: Baudrate = %d, Parity = %d, Stop Bits = "
              "%d, Data Bits = %d",
              config_.baudrate, static_cast<int>(config_.parity),
              config_.stop_bits, config_.data_bits);

  file.close();
}

/*
 * 将当前 config_ 封装为命令，推送至 ReadPort 供主控模块处理；
 */
void TerminalBackend::syncConfig() {
  static uint32_t topic_key = LibXR::Topic::Find("command")->key;
  Command cmd;
  cmd.type = Command::Type::CONFIG_UART;
  cmd.data.uart_config.uart_index = index_;
  cmd.data.uart_config.config = config_;

  LibXR::Topic::PackedData<Command> packed_cmd;
  LibXR::Topic::PackData(topic_key, packed_cmd, cmd);

  if (read_.EmptySize() < sizeof(packed_cmd)) {
    read_.Reset();
  }

  read_.queue_data_->PushBatch(&packed_cmd, sizeof(packed_cmd));
}
