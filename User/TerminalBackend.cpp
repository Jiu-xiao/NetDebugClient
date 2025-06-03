#include "TerminalBackend.hpp"
#include "libxr_def.hpp"
#include "libxr_rw.hpp"
#include "libxr_type.hpp"
#include "logger.hpp"
#include "ramfs.hpp"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QVariant>
#include <QVariantMap>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qvariant.h>

using namespace LibXR;

static ErrorCode Write(WritePort &port)
{
  uint8_t buffer[1024];
  WriteInfoBlock info;
  TerminalBackend *self = CONTAINER_OF(&port, TerminalBackend, write_);
  while (port.Size() > 0)
  {
    port.queue_info_->Pop(info);
    port.queue_data_->PopBatch(buffer, info.data.size_);
    emit self->receiveText(
        QString::fromUtf8(reinterpret_cast<char *>(buffer), info.data.size_));
    QString hexString = "";

    for (int i = 0; i < info.data.size_; i++)
    {
      hexString += QString("%1 ").arg(buffer[i], 2, 16, QChar('0')).toUpper();
    }

    port.Finish(false, ErrorCode::OK, info, 0);
  }
  return ErrorCode::OK;
}

static ErrorCode Read(ReadPort &port)
{
  TerminalBackend *self = CONTAINER_OF(&port, TerminalBackend, read_);
  return ErrorCode::FAILED;
}

TerminalBackend::TerminalBackend(QObject *parent)
    : QObject(parent), ramfs_("ramfs"),
      term_(ramfs_, nullptr, &read_, &write_)
{
  read_ = Read;
  write_ = Write;
}

void TerminalBackend::sendText(const QString &command)
{
  QByteArray bytes = command.toUtf8(); // 转为字节数组
  QString hexString;

  for (unsigned char b : bytes)
  {
    hexString += QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
  }

  XR_LOG_DEBUG("Send command: %s", command.toUtf8().data());
  read_.queue_data_->PushBatch(
      reinterpret_cast<const uint8_t *>(command.toUtf8().data()),
      command.size());
  while (read_.Size() > 0)
  {
    read_.ProcessPendingReads(false);
    term_.TaskFun(&term_);
    read_.ProcessPendingReads(false);
    term_.TaskFun(&term_);
  }

  return;
}

Q_INVOKABLE void TerminalBackend::setBaudrate(const QString &baud)
{
  XR_LOG_INFO("Baudrate set to %s", baud.toStdString().c_str());
}

Q_INVOKABLE void TerminalBackend::setParity(const QString &parity)
{
  XR_LOG_INFO("Parity set to %s", parity.toStdString().c_str());
}

Q_INVOKABLE void TerminalBackend::setStopBits(const QString &stopBits)
{
  XR_LOG_INFO("Stop bits set to %s", stopBits.toStdString().c_str());
}

Q_INVOKABLE void TerminalBackend::setDataBits(const QString &dataBits)
{
  XR_LOG_INFO("Data bits set to %s", dataBits.toStdString().c_str());
}

Q_INVOKABLE void TerminalBackend::restartPort()
{
  XR_LOG_INFO("Port restarted");
}

Q_INVOKABLE QVariantMap TerminalBackend::defaultConfig() const
{
  XR_LOG_INFO("Load Default config");
  return {{"baudrate", "460800"},
          {"parity", "None"},
          {"stopBits", "1"},
          {"dataBits", "8"}};
}
