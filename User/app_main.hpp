#ifndef APPMAIN_HPP
#define APPMAIN_HPP

#include "DeviceManager.hpp"
#include "TerminalBackend.hpp"
#include "libxr.hpp"
#include "libxr_rw.hpp"
#include <QDebug>
#include <QHostAddress>
#include <QQmlApplicationEngine>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QUdpSocket>

class Worker : public QObject {
  Q_OBJECT
public:
  explicit Worker(QQmlApplicationEngine *qmlEngine, QObject *parent = nullptr)
      : QObject(parent), command_topic_("command", sizeof(Command)),
        tcpClientConnected_(false) {
    // 初始化串口后端
    minipc_ = new LibXR::TerminalBackend("uart_cdc", 0, qmlEngine);
    usart1_ = new LibXR::TerminalBackend("uart1", 1, qmlEngine);
    usart2_ = new LibXR::TerminalBackend("uart2", 2, qmlEngine);

    // 注册 QML 类型和主界面
    qmlRegisterType<DeviceManager>("com.example", 1, 0, "DeviceManager");
    qmlEngine->loadFromModule("MyApp", "Main");

    QObject *rootObject = qmlEngine->rootObjects().first();
    deviceManager_ = rootObject->findChild<DeviceManager *>("deviceManager");

    // 注册命令回调
    auto cb = LibXR::Topic::Callback::Create(
        [](bool, Worker *self, LibXR::RawData &data) {
          if (data.size_ == sizeof(Command)) {
            Command cmd = *reinterpret_cast<Command *>(data.addr_);
            if (cmd == Command::PING)
              XR_LOG_INFO("Received PING command");
          }
        },
        this);
    command_topic_.RegisterCallback(cb);

    // 初始化 Topic Server
    topicServer_ = new LibXR::Topic::Server(40960);
    topicServer_->Register(minipc_->topic_);
    topicServer_->Register(usart1_->topic_);
    topicServer_->Register(usart2_->topic_);
    topicServer_->Register(command_topic_);
  }

  enum class Command : uint8_t { PING = 0, REBOOT = 1 };

public slots:
  void start() {
    tcpServer_ = new QTcpServer(this);
    udpSocket_ = new QUdpSocket(this);

    // 启动 TCP Server
    connect(tcpServer_, &QTcpServer::newConnection, this,
            &Worker::onNewConnection);

    if (!tcpServer_->listen(QHostAddress::Any, kTcpPort)) {
      XR_LOG_ERROR("Failed to start TCP server");
      return;
    }
    XR_LOG_DEBUG("TCP Server started on port %d", kTcpPort);

    // UDP 广播定时器
    broadcastTimer_ = new QTimer(this);
    connect(broadcastTimer_, &QTimer::timeout, this, [this]() {
      if (!tcpClientConnected_)
        broadcastUdpMessage();
    });
    broadcastTimer_->start(1000); // 每秒广播一次

    // 串口数据转发定时器（替代跨线程匿名线程）
    forwardTimer_ = new QTimer(this);
    connect(forwardTimer_, &QTimer::timeout, this, &Worker::forwardTcpData);
    forwardTimer_->start(1); // 高频轮询转发
  }

private slots:
  void onNewConnection() {
    if (tcpClientConnected_) {
      auto *newClient = tcpServer_->nextPendingConnection();
      newClient->disconnectFromHost();
      XR_LOG_DEBUG("Rejecting new client connection");
      return;
    }

    tcpClientSocket_ = tcpServer_->nextPendingConnection();
    tcpClientConnected_ = true;

    connect(tcpClientSocket_, &QTcpSocket::readyRead, this,
            &Worker::onTcpDataReceived);
    connect(tcpClientSocket_, &QTcpSocket::disconnected, this, [this]() {
      XR_LOG_DEBUG("TCP client disconnected");
      tcpClientConnected_ = false;
      tcpClientSocket_->deleteLater();
      broadcastUdpMessage();
    });

    XR_LOG_DEBUG("New TCP client connected from %s",
                 tcpClientSocket_->peerAddress().toString().toUtf8().data());
  }

  void onTcpDataReceived() {
    QByteArray data = tcpClientSocket_->readAll();
    topicServer_->ParseData({data.data(), static_cast<size_t>(data.size())});
    XR_LOG_DEBUG("Received TCP data size: %d", data.size());
  }

  void forwardTcpData() {
    if (!tcpClientConnected_) return;

    LibXR::ReadPort *ports[3] = {&minipc_->read_, &usart1_->read_, &usart2_->read_};
    static uint8_t buffer[40960];

    for (int i = 0; i < 3; ++i) {
      size_t size = ports[i]->Size();
      if (size > 0) {
        ports[i]->queue_data_->PopBatch(buffer, size);
        tcpClientSocket_->write(reinterpret_cast<char *>(buffer),
                                static_cast<qint64>(size));
        tcpClientSocket_->flush();
        XR_LOG_DEBUG("Forwarded %zu bytes to TCP client", size);
      }
    }
  }

  void broadcastUdpMessage() {
    QByteArray message = kUdpBroadcastMessage;
    int sent = udpSocket_->writeDatagram(message, QHostAddress::Broadcast,
                                         kUdpPort);
    if (sent == -1) {
      XR_LOG_ERROR("Failed to send UDP broadcast");
    } else {
      XR_LOG_DEBUG("UDP broadcast sent");
    }
  }

private:
  DeviceManager *deviceManager_;
  LibXR::TerminalBackend *minipc_;
  LibXR::TerminalBackend *usart1_;
  LibXR::TerminalBackend *usart2_;
  LibXR::Topic::Server *topicServer_;
  LibXR::Topic command_topic_;

  QTcpServer *tcpServer_ = nullptr;
  QTcpSocket *tcpClientSocket_ = nullptr;
  QUdpSocket *udpSocket_ = nullptr;

  QTimer *broadcastTimer_ = nullptr;
  QTimer *forwardTimer_ = nullptr;

  bool tcpClientConnected_ = false;

  static constexpr quint16 kTcpPort = 5000;
  static constexpr quint16 kUdpPort = 5001;
  static constexpr char kUdpBroadcastMessage[] =
      "Broadcast message to UDP clients";
};

class AppMain : public QObject {
  Q_OBJECT
public:
  explicit AppMain(QQmlApplicationEngine *qmlEngine, QObject *parent = nullptr)
      : QObject(parent) {
    workerThread_ = new QThread(this);
    worker_ = new Worker(qmlEngine);
    worker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::started, worker_, &Worker::start);
    workerThread_->start();
  }

  ~AppMain() {
    workerThread_->quit();
    workerThread_->wait();
    delete worker_;
  }

private:
  QThread *workerThread_;
  Worker *worker_;
};

#endif // APPMAIN_HPP
