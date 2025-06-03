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
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include "uart.hpp"

namespace LibXR
{

  class TerminalBackend : public QObject
  {
    Q_OBJECT
  public:
    explicit TerminalBackend(uint8_t index, QObject *parent = nullptr);

  public slots:
    void sendText(const QString &text);

    // UART configuration setters
    Q_INVOKABLE void setBaudrate(const QString &baud);
    Q_INVOKABLE void setParity(const QString &parity);
    Q_INVOKABLE void setStopBits(const QString &stopBits);
    Q_INVOKABLE void setDataBits(const QString &dataBits);

    // Return current configuration
    Q_INVOKABLE QVariantMap defaultConfig() const;

  signals:
    void receiveText(const QString &text);

  public:
    // Load UART configuration from file
    void loadConfigFromFile();

    // Save UART configuration to file
    void saveConfigToFile();

    uint8_t index_;          // Index for the terminal backend
    LibXR::ReadPort read_;   // Read port for terminal
    LibXR::WritePort write_; // Write port for terminal
    LibXR::RamFS ramfs_;     // RAM file system for terminal
    LibXR::Terminal<> term_; // Terminal instance

    // UART configuration with default values
    LibXR::UART::Configuration config_ = {460800, UART::Parity::NO_PARITY, 8, 1}; // Default config values
  };

} // namespace LibXR
