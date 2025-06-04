#ifndef APPMAIN_HPP
#define APPMAIN_HPP

#include "DeviceManager.hpp"
#include "TerminalBackend.hpp"
#include "libxr.hpp"
#include "libxr_rw.hpp"
#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QUdpSocket>
#include <qtypes.h>

class AppMain : public QThread {
  Q_OBJECT

public:
  enum class Command : uint8_t {
    PING = 0,
    REBOOT = 1,
  };

  explicit AppMain(QQmlApplicationEngine *parent)
      : QThread(parent), command_topic_("command", sizeof(Command)),
        tcpClientConnected_(false), stopRequested_(false) {
    minipc_ = new LibXR::TerminalBackend("uart_cdc", 0, parent);
    usart1_ = new LibXR::TerminalBackend("uart1", 1, parent);
    usart2_ = new LibXR::TerminalBackend("uart2", 2, parent);

    qmlRegisterType<DeviceManager>("com.example", 1, 0, "DeviceManager");

    parent->loadFromModule("MyApp", "Main");

    QObject *rootObject = parent->rootObjects().first();
    deviceManager_ = rootObject->findChild<DeviceManager *>("deviceManager");

    void (*command_cb_fun)(bool, AppMain *, LibXR::RawData &) =
        [](bool, AppMain *self, LibXR::RawData &data) {
          if (data.size_ == sizeof(Command)) {
            Command command = *reinterpret_cast<Command *>(data.addr_);
            switch (command) {
            case Command::PING:
              XR_LOG_INFO("Received PING command");
              break;
            default:
              break;
            }
          }
        };

    auto command_cb = LibXR::Topic::Callback::Create(command_cb_fun, this);
    command_topic_.RegisterCallback(command_cb);

    // 初始化 TCP 服务器和 UDP 套接字
    tcpServer_ = new QTcpServer(this);
    udpSocket_ = new QUdpSocket(this);
    topicServer_ = new LibXR::Topic::Server(40960);
    topicServer_->Register(minipc_->topic_);
    topicServer_->Register(usart1_->topic_);
    topicServer_->Register(usart2_->topic_);
    topicServer_->Register(command_topic_);
    toTcpThread_ = new ToTcpThread(parent, this);
    start();
  }

  class ToTcpThread : public QThread {
  public:
    explicit ToTcpThread(QQmlApplicationEngine *parent, AppMain *main)
        : QThread(parent), main_(main) {
      start();
    }

    void run() override {
      LibXR::ReadPort *ports[3] = {&main_->minipc_->read_,
                                   &main_->usart1_->read_,
                                   &main_->usart2_->read_};

      static uint8_t buffer[40960];

      while (true) {
        if (!main_->tcpClientConnected_) {
          QThread::msleep(10);
          continue;
        }

        for (int i = 0; i < 3; i++) {
          auto readable = ports[i]->Size();
          if (readable > 0) {
            ports[i]->queue_data_->PopBatch(buffer, readable);
            main_->tcpClientSocket_->write(reinterpret_cast<char *>(buffer),
                                           static_cast<qint64>(readable));
            main_->tcpClientSocket_->flush();

            XR_LOG_DEBUG("TCP Server write %d bytes", readable);
          }
        }

        QThread::msleep(1);
      }
    }

  private:
    AppMain *main_;
  };

  ~AppMain() {
    stop();
    toTcpThread_->terminate();
    if (isRunning()) {
      quit();
      wait();
    }
  }

  void stop() {
    stopRequested_ = true;
    quit();
  }

protected:
  void run() override {
    // 启动 TCP 服务器
    if (!tcpServer_->listen(QHostAddress::Any, kTcpPort)) {
      XR_LOG_ERROR("Failed to start TCP server");
      return;
    }
    XR_LOG_DEBUG("TCP Server started on port %d", kTcpPort);

    // 创建定时器用于UDP广播
    QTimer *broadcastTimer = new QTimer();
    connect(broadcastTimer, &QTimer::timeout, this, [this]() {
      if (!tcpClientConnected_) {
        broadcastUdpMessage();
      }
    });
    broadcastTimer->start(1000); // 每秒广播一次

    // 监听 TCP 客户端连接
    connect(tcpServer_, &QTcpServer::newConnection, this,
            &AppMain::onNewConnection);

    exec(); // 事件循环会处理所有网络事件

    // 清理
    broadcastTimer->stop();
    broadcastTimer->deleteLater();
    tcpServer_->close();
    XR_LOG_DEBUG("TCP Server stopped");
  }

private slots:
  void onNewConnection() {
    if (tcpClientConnected_) {
      // 如果已有一个客户端连接，则直接关闭新的连接
      QTcpSocket *newClient = tcpServer_->nextPendingConnection();
      newClient->disconnectFromHost();
      XR_LOG_DEBUG("Rejecting new client connection, already connected");
      return;
    }

    // 获取新的客户端连接
    tcpClientSocket_ = tcpServer_->nextPendingConnection();
    tcpClientConnected_ = true; // 标记已连接

    // 连接信号与槽
    connect(tcpClientSocket_, &QTcpSocket::readyRead, this,
            &AppMain::onTcpDataReceived);
    connect(tcpClientSocket_, &QTcpSocket::disconnected, this, [this]() {
      XR_LOG_DEBUG("TCP client disconnected");
      tcpClientConnected_ = false;
      tcpClientSocket_->deleteLater();
      broadcastUdpMessage(); // 立即广播
    });

    XR_LOG_DEBUG("New TCP client connected from %s",
                 tcpClientSocket_->peerAddress().toString().toUtf8().data());
  }

  void onTcpDataReceived() {
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (clientSocket) {
      QByteArray data = clientSocket->readAll();
      XR_LOG_DEBUG("Received data from client. Size:%d", data.size());
      topicServer_->ParseData({data.data(), static_cast<size_t>(data.size())});
    }
  }

  void broadcastUdpMessage() {
    // 广播内容
    QByteArray message = kUdpBroadcastMessage;

    // 目标地址和端口
    QHostAddress broadcastAddress = QHostAddress::Broadcast;
    quint16 port = kUdpPort;

    // 广播 UDP 消息
    int bytesSent = udpSocket_->writeDatagram(message, broadcastAddress, port);
    if (bytesSent == -1) {
      XR_LOG_ERROR("Failed to send UDP broadcast message");
    } else {
      XR_LOG_DEBUG("UDP broadcast message sent");
    }
  }

private:
  LibXR::Topic command_topic_;

  DeviceManager *deviceManager_;
  LibXR::TerminalBackend *minipc_;
  LibXR::TerminalBackend *usart1_;
  LibXR::TerminalBackend *usart2_;
  LibXR::Topic::Server *topicServer_;

  QTcpServer *tcpServer_; // TCP 服务器
  QUdpSocket *udpSocket_; // UDP 套接字

  QTcpSocket *tcpClientSocket_ = nullptr;

  ToTcpThread *toTcpThread_ = nullptr;

  bool tcpClientConnected_; // 标记是否有客户端连接
  bool stopRequested_;      // 用于请求停止线程

  // 定义常量
  static constexpr quint16 kTcpPort = 5000; // TCP 端口
  static constexpr quint16 kUdpPort = 5001; // UDP 端口
  static constexpr char kTcpResponseMessage[] =
      "Hello from server"; // TCP 响应消息
  static constexpr char kUdpBroadcastMessage[] =
      "Broadcast message to UDP clients"; // UDP 广播消息
};

#endif // APPMAIN_HPP