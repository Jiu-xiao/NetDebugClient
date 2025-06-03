#pragma once

#include "libxr.hpp"
#include "libxr_rw.hpp"
#include "ramfs.hpp"
#include "terminal.hpp"
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

class TerminalBackend : public QObject {
  Q_OBJECT
public:
  explicit TerminalBackend(QObject *parent = nullptr);

public slots:
  void sendText(const QString &text);
  Q_INVOKABLE void setBaudrate(const QString &baud);
  Q_INVOKABLE void setParity(const QString &parity);
  Q_INVOKABLE void setStopBits(const QString &stopBits);
  Q_INVOKABLE void setDataBits(const QString &dataBits);
  Q_INVOKABLE void restartPort();

  Q_INVOKABLE QVariantMap defaultConfig() const;

signals:
  void receiveText(const QString &text);

public:
  QStringList history_; // 可选：命令历史
  LibXR::ReadPort read_;
  LibXR::WritePort write_;
  LibXR::RamFS ramfs_;
  LibXR::Terminal<> term_;
  std::string ansiBuffer_; // 当前未处理完成的 ANSI 控制串
  bool inEscape_ = false;  // 是否正在处理 \033[
};
