#include "TerminalBackend.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QVariant>
#include <QVariantMap>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  TerminalBackend backend0;
  TerminalBackend backend1;
  TerminalBackend backend2;

  QQmlContext *context = engine.rootContext();
  context->setContextProperty("backend0", &backend0);
  context->setContextProperty("backend1", &backend1);
  context->setContextProperty("backend2", &backend2);

  engine.loadFromModule("MyApp", "Main");

  if (engine.rootObjects().isEmpty())
    return -1;
  return app.exec();
}
