#include "TerminalBackend.hpp"
#include "libxr_def.hpp"
#include "libxr_rw.hpp"
#include "libxr_type.hpp"
#include "logger.hpp"
#include "ramfs.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QVariant>
#include <QVariantMap>
#include <qdebug.h>
#include <qvariant.h>

using namespace LibXR;

static ErrorCode Write(WritePort &port) {
  uint8_t buffer[1024];
  WriteInfoBlock info;
  TerminalBackend *self = CONTAINER_OF(&port, TerminalBackend, write_);
  while (port.Size() > 0) {
    port.queue_info_->Pop(info);
    port.queue_data_->PopBatch(buffer, info.data.size_);
    std::string chunk(reinterpret_cast<char *>(buffer), info.data.size_);
    QString html = self->appendAnsiText(chunk);
    emit self->outputReceived(self->terminalIndex_, html);
    port.Finish(false, ErrorCode::OK, info, 0);
  }
  return ErrorCode::OK;
}

static ErrorCode Read(ReadPort &port) {
  TerminalBackend *self = CONTAINER_OF(&port, TerminalBackend, read_);
  return ErrorCode::FAILED;
}

TerminalBackend::TerminalBackend(QObject *parent)
    : QObject(parent), ramfs_("ramfs"),
      term_(ramfs_, nullptr, &read_, &write_) {
  read_ = Read;
  write_ = Write;
  STDIO::read_ = &read_;
  STDIO::write_ = &write_;
}

void TerminalBackend::sendCommand(int terminalIndex, const QString &command) {
  terminalIndex_ = terminalIndex;
  XR_LOG_INFO("Send command: %s", command.toUtf8().data());
  read_.queue_data_->PushBatch(
      reinterpret_cast<const uint8_t *>(command.toUtf8().data()),
      command.size());
  read_.ProcessPendingReads(false);
  term_.TaskFun(&term_);
}

Q_INVOKABLE void TerminalBackend::setBaudrate(const QString &baud) {
  qInfo() << "Baudrate set to " << baud;
}

Q_INVOKABLE void TerminalBackend::setParity(const QString &parity) {
  qInfo() << "Parity set to " << parity;
}

Q_INVOKABLE void TerminalBackend::setStopBits(const QString &stopBits) {
  qInfo() << "Stop bits set to " << stopBits;
}

Q_INVOKABLE void TerminalBackend::setDataBits(const QString &dataBits) {
  qInfo() << "Data bits set to " << dataBits;
}

Q_INVOKABLE void TerminalBackend::restartPort() { qInfo() << "Port restarted"; }

Q_INVOKABLE QVariantMap TerminalBackend::defaultConfig() const {
  qInfo() << "Default config";
  return {{"baudrate", "460800"},
          {"parity", "None"},
          {"stopBits", "1"},
          {"dataBits", "8"}};
}
