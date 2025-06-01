#pragma once
#include <QObject>
#include <QProcess>

class TerminalBackend : public QObject {
  Q_OBJECT
public:
  explicit TerminalBackend(QObject *parent = nullptr);

  Q_INVOKABLE void sendCommand(const QString &cmd);

signals:
  void outputReceived(QString output);

private:
  QProcess *process;
};
