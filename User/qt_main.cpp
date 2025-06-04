#include "DeviceManager.hpp"
#include "QTTimebase.hpp"
#include "TerminalBackend.hpp"
#include "app_main.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebChannel>
#include <QtWebEngineQuick>
#include <qdebug.h>

int main(int argc, char *argv[]) {
  LibXR::QTTimebase timebase;

  ErrorCode (*write_fun)(LibXR::WritePort &port) = [](LibXR::WritePort &port) {
    static uint8_t write_buff[4096];
    LibXR::WriteInfoBlock info;
    while (port.Size() > 0) {
      port.queue_info_->Pop(info);
      port.queue_data_->PopBatch(write_buff, info.data.size_);
      write_buff[info.data.size_] = '\0';
      port.Finish(false, ErrorCode::FAILED, info, 0);
    }

    return ErrorCode::FAILED;
  };

  LibXR::STDIO::write_ = new LibXR::WritePort(32, 4096);

  (*LibXR::STDIO::write_) = write_fun;

  // qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9223");
  QtWebEngineQuick::initialize();
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  AppMain app_main(&engine);

  app.exec();

  app_main.stop();
  app_main.wait();

  return 0;
}