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
  /* 初始化 LibXR 时间基准（用于毫秒级计时） */
  LibXR::QTTimebase timebase;

  /*
   * 重定向 STDIO 输出至控制台：
   *  - 从写入端读取并打印 UTF-8 文本；
   *  - 所有通过 LibXR::STDIO::write_ 写入的数据将输出到 qDebug；
   *  - 此逻辑不处理成功状态，始终返回 FAILED 表示模拟失败处理。
   */
  ErrorCode (*write_fun)(LibXR::WritePort &port) = [](LibXR::WritePort &port) {
    static uint8_t write_buff[4096];
    LibXR::WriteInfoBlock info;

    while (port.Size() > 0) {
      port.queue_info_->Pop(info);
      port.queue_data_->PopBatch(write_buff, info.data.size_);
      write_buff[info.data.size_] = '\0';
      qDebug() << reinterpret_cast<char *>(write_buff);
      port.Finish(false, ErrorCode::FAILED, info, 0);
    }

    return ErrorCode::FAILED;
  };

  /* 初始化 LibXR 标准输出写入端口，并绑定重定向函数 */
  LibXR::STDIO::write_ = new LibXR::WritePort(32, 4096);
  (*LibXR::STDIO::write_) = write_fun;

  /* 初始化 Qt WebEngine 环境（必须在 QGuiApplication 前） */
  QtWebEngineQuick::initialize();

  /* 启动 Qt GUI 应用 */
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  /* 添加 QML 导入路径（打包后路径） */
  engine.addImportPath(QCoreApplication::applicationDirPath() + "/../qml");

  /* 创建主控制器并启动工作线程 */
  AppMain app_main(&engine);

  /* 启动 Qt 主事件循环 */
  app.exec();

  return 0;
}
