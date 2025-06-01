#include "TerminalBackend.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;
  TerminalBackend backend;
  engine.rootContext()->setContextProperty("backend", &backend);
  engine.loadFromModule("MyApp", "Main");
  if (engine.rootObjects().isEmpty())
    return -1;
  return app.exec();
}
