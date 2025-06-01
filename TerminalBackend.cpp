#include "TerminalBackend.h"

TerminalBackend::TerminalBackend(QObject *parent) : QObject(parent) {
  process = new QProcess(this);
  process->start("/bin/bash");

  connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
    emit outputReceived(QString::fromUtf8(process->readAllStandardOutput()));
  });

  connect(process, &QProcess::readyReadStandardError, this, [=]() {
    emit outputReceived(QString::fromUtf8(process->readAllStandardError()));
  });
}

void TerminalBackend::sendCommand(const QString &cmd) {
  process->write((cmd + "\n").toUtf8());
}
