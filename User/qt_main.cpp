#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebChannel>
#include <QtWebEngineQuick>
#include "TerminalBackend.hpp"
#include "DeviceManager.hpp"
#include "QTTimebase.hpp"
#include <qdebug.h>
#include "app_main.hpp"

int main(int argc, char *argv[])
{
    LibXR::QTTimebase timebase;

    ErrorCode (*write_fun)(LibXR::WritePort &port) = [](LibXR::WritePort &port)
    {
        static uint8_t write_buff[4096];
        LibXR::WriteInfoBlock info;
        while (port.Size() > 0)
        {
            port.queue_info_->Pop(info);
            port.queue_data_->PopBatch(write_buff, info.data.size_);
            write_buff[info.data.size_] = '\0';
            qDebug() << reinterpret_cast<char *>(write_buff);
            port.Finish(false, ErrorCode::FAILED, info, 0);
        }

        return ErrorCode::FAILED;
    };

    LibXR::STDIO::write_ =
        new LibXR::WritePort(32, 4096);

    (*LibXR::STDIO::write_) = write_fun;

    // qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9223");
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // 创建后端对象（确保它们在作用域内）
    auto backend0 = new TerminalBackend(&engine);
    auto backend1 = new TerminalBackend(&engine);
    auto backend2 = new TerminalBackend(&engine);
    qmlRegisterType<DeviceManager>("com.example", 1, 0, "DeviceManager");

    // 同时暴露后端对象（可选）
    engine.rootContext()->setContextProperty("backend0Obj", backend0);
    engine.rootContext()->setContextProperty("backend1Obj", backend1);
    engine.rootContext()->setContextProperty("backend2Obj", backend2);

    engine.loadFromModule("MyApp", "Main");

    AppMain app_main;
    app_main.start();

    return app.exec();
}