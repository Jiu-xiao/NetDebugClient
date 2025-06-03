#pragma once

#include "libxr.hpp"
#include "libxr_rw.hpp"
#include "ramfs.hpp"
#include "terminal.hpp"
#include <QGuiApplication>
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
  void sendCommand(int terminalIndex, const QString &command);
  Q_INVOKABLE void setBaudrate(const QString &baud);
  Q_INVOKABLE void setParity(const QString &parity);
  Q_INVOKABLE void setStopBits(const QString &stopBits);
  Q_INVOKABLE void setDataBits(const QString &dataBits);
  Q_INVOKABLE void restartPort();

  Q_INVOKABLE QVariantMap defaultConfig() const;

signals:
  void outputReceived(int terminalIndex, const QString &output);

public:
  QStringList history_; // 可选：命令历史
  LibXR::ReadPort read_;
  LibXR::WritePort write_;
  LibXR::RamFS ramfs_;
  LibXR::Terminal<> term_;
  int terminalIndex_ = 0;
  std::string ansiBuffer_; // 当前未处理完成的 ANSI 控制串
  bool inEscape_ = false;  // 是否正在处理 \033[

  QString appendAnsiText(const std::string &chunk) {
    std::ostringstream html;
    html << "<span style='font-family:monospace;'>";

    size_t i = 0;
    while (i < chunk.size()) {
      char c = chunk[i];
      if (!inEscape_) {
        if (c == '\033' && (i + 1 < chunk.size()) && chunk[i + 1] == '[') {
          inEscape_ = true;
          ansiBuffer_.clear();
          i += 2;
          continue;
        } else {
          // 普通字符，直接输出
          html << escapeHtml(c);
          ++i;
        }
      } else {
        if (isalpha(c)) {
          ansiBuffer_ += c;
          applyAnsiCode(html, ansiBuffer_); // 转为 HTML span
          inEscape_ = false;
          ++i;
        } else {
          ansiBuffer_ += c;
          ++i;
        }
      }
    }

    html << "</span>";
    return QString::fromStdString(html.str());
  }

  std::string escapeHtml(char c) {
    switch (c) {
    case '<':
      return "&lt;";
    case '>':
      return "&gt;";
    case '&':
      return "&amp;";
    case '\n':
      return "<br>";
    case ' ':
      return "&nbsp;";
    default:
      return std::string(1, c);
    }
  }

  void applyAnsiCode(std::ostringstream &html, const std::string &code) {
    static const std::map<std::string, std::string> colorMap = {
        {"0m", "</span><span style='font-family:monospace;'>"},
        {"1m", "</span><span style='font-weight:bold;'>"},
        {"4m", "</span><span style='text-decoration:underline;'>"},
        {"30m", "</span><span style='color:black;'>"},
        {"31m", "</span><span style='color:red;'>"},
        {"32m", "</span><span style='color:green;'>"},
        {"33m", "</span><span style='color:yellow;'>"},
        {"34m", "</span><span style='color:blue;'>"},
        {"35m", "</span><span style='color:magenta;'>"},
        {"36m", "</span><span style='color:cyan;'>"},
        {"37m", "</span><span style='color:white;'>"},
        {"40m", "</span><span style='background-color:black;'>"},
        {"41m", "</span><span style='background-color:red;'>"},
        {"42m", "</span><span style='background-color:green;'>"},
        {"43m", "</span><span style='background-color:yellow;'>"},
        {"44m", "</span><span style='background-color:blue;'>"},
        {"45m", "</span><span style='background-color:magenta;'>"},
        {"46m", "</span><span style='background-color:cyan;'>"},
        {"47m", "</span><span style='background-color:white;'>"},
        {"2J", "<clear_screen>"},
        {"2K", "<clear_line>"},
        {"K", "<clear_line_right>"}};

    auto it = colorMap.find(code);
    if (it != colorMap.end()) {
      if (it->second == "<clear_screen>") {
        html.str("");
        html.clear();
        html << "<span style='font-family:monospace;'>";
      } else if (it->second == "<clear_line>" ||
                 it->second == "<clear_line_right>") {
        std::string current = html.str();
        auto lastLine = current.rfind("<br>");
        if (lastLine != std::string::npos) {
          current.erase(lastLine + 4);
        } else {
          current.clear();
          current = "<span style='font-family:monospace;'>";
        }
        html.str("");
        html.clear();
        html << current;
      } else {
        html << it->second;
      }
    }
  }
};
