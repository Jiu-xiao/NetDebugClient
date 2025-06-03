#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebChannel>
#include <QtWebEngineQuick>
#include "TerminalBackend.hpp"
#include "InputProcessor.hpp"

int main(int argc, char *argv[])
{
    // qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9223");
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // 创建后端对象（确保它们在作用域内）
    auto backend0 = new TerminalBackend(&engine);
    auto backend1 = new TerminalBackend(&engine);
    auto backend2 = new TerminalBackend(&engine);
    qmlRegisterType<InputProcessor>("com.example", 1, 0, "InputProcessor");

    // 同时暴露后端对象（可选）
    engine.rootContext()->setContextProperty("backend0Obj", backend0);
    engine.rootContext()->setContextProperty("backend1Obj", backend1);
    engine.rootContext()->setContextProperty("backend2Obj", backend2);

    engine.loadFromModule("MyApp", "Main");

    return app.exec();
}