#pragma once

#include "ClipboardBridge.hpp"
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
    /* 初始化串口后端 */
    minipc_ = new LibXR::TerminalBackend("uart_cdc", 0, qmlEngine);
    usart1_ = new LibXR::TerminalBackend("uart1", 1, qmlEngine);
    usart2_ = new LibXR::TerminalBackend("uart2", 2, qmlEngine);

    /* 初始化剪切板 */
    clipboardBridge_ = new ClipboardBridge();
    qmlEngine->rootContext()->setContextProperty("clipboardBridge",
                                                 clipboardBridge_);

    /* 注册 QML 类型和主界面 */
    qmlRegisterType<DeviceManager>("com.example", 1, 0, "DeviceManager");
    qmlEngine->loadFromModule("MyApp", "Main");

    QObject *rootObject = qmlEngine->rootObjects().first();
    deviceManager_ = rootObject->findChild<DeviceManager *>("device_manager");

    ASSERT(deviceManager_ != nullptr);

    /* 注册命令回调 */
    auto cb = LibXR::Topic::Callback::Create(
        [](bool, Worker *self, LibXR::RawData &data) {
          if (data.size_ <= sizeof(Command)) {
            Command cmd = *reinterpret_cast<Command *>(data.addr_);
            if (cmd.type == Command::Type::PING) {
              XR_LOG_DEBUG("Received PING command");
              self->last_ping_time_ = QDateTime::currentMSecsSinceEpoch();
            } else if (cmd.type == Command::Type::REMOTE_PING) {
              XR_LOG_DEBUG("Received REMOTE_PING command");
              self->last_remote_ping_time_ =
                  QDateTime::currentMSecsSinceEpoch();
            }
          }
        },
        this);
    command_topic_.RegisterCallback(cb);

    /* 初始化 Topic Server */
    topicServer_ = new LibXR::Topic::Server(40960);
    topicServer_->Register(minipc_->topic_);
    topicServer_->Register(usart1_->topic_);
    topicServer_->Register(usart2_->topic_);
    topicServer_->Register(command_topic_);
  }

public slots:
  void start() {
    tcpServer_ = new QTcpServer(this);
    udpSocket_ = new QUdpSocket(this);

    /* 启动 TCP Server */
    connect(tcpServer_, &QTcpServer::newConnection, this,
            &Worker::onNewConnection);

    if (!tcpServer_->listen(QHostAddress::Any, kTcpPort)) {
      XR_LOG_ERROR("Failed to start TCP server");
      return;
    }
    XR_LOG_DEBUG("TCP Server started on port %d", kTcpPort);

    /* UDP 广播定时器 */
    broadcastTimer_ = new QTimer(this);
    connect(broadcastTimer_, &QTimer::timeout, this, [this]() {
      if (!tcpClientConnected_) {
        if (deviceManager_->filter_is_set_) {
          broadcastUdpMessage();
        } else {
          XR_LOG_INFO("Device name filter is not set, skipping broadcast");
        }
      }
    });
    broadcastTimer_->start(1000);

    /* 串口数据转发定时器 */
    forwardTimer_ = new QTimer(this);
    connect(forwardTimer_, &QTimer::timeout, this, &Worker::forwardTcpData);
    forwardTimer_->start(1);

    /* PING 检查定时器 */
    pingCheckTimer_ = new QTimer(this);
    connect(pingCheckTimer_, &QTimer::timeout, this, [this]() {
      const qint64 now = QDateTime::currentMSecsSinceEpoch();
      const bool isOnline = (now - last_ping_time_) <= 300;

      if (isOnline == false && tcpClientConnected_) {
        XR_LOG_INFO("TCP client disconnected");
        tcpClientConnected_ = false;
        tcpClientSocket_->disconnectFromHost();
        broadcastUdpMessage();
      }

      if (deviceManager_->isBackendConnected() != isOnline) {
        deviceManager_->SetBackendConnected(isOnline);
        XR_LOG_INFO("Backend status changed: %s",
                    isOnline ? "online" : "offline");
      }

      const bool isRemoteOnline = (now - last_remote_ping_time_) <= 300;
      if (deviceManager_->isMiniPCOnline() != isRemoteOnline) {
        deviceManager_->SetMiniPCOnline(isRemoteOnline);
        XR_LOG_INFO("MiniPC status changed: %s",
                    isRemoteOnline ? "online" : "offline");
      }

      if (deviceManager_->require_restart_) {
        LibXR::Topic::PackedData<Command::Type> command;
        LibXR::Topic::PackData(command_topic_.GetKey(), command,
                               Command::Type::REBOOT);
        uint8_t buf[sizeof(command) + LibXR::Topic::PACK_BASE_SIZE];
        LibXR::Topic::PackData(minipc_->topic_.GetKey(), buf, command);
        tcpClientSocket_->write(reinterpret_cast<char *>(buf),
                                static_cast<qint64>(sizeof(buf)));
        deviceManager_->require_restart_ = false;
      }

      if (deviceManager_->require_rename_) {
        deviceManager_->require_rename_ = false;
        LibXR::Topic::PackedData<Command> command_buf;
        Command cmd;
        cmd.type = Command::Type::RENAME;
        strncpy(cmd.data.device_name,
                deviceManager_->last_device_name_.toUtf8().data(),
                sizeof(cmd.data.device_name));
        LibXR::Topic::PackData(command_topic_.GetKey(), command_buf, cmd);
        tcpClientSocket_->write(reinterpret_cast<char *>(&command_buf),
                                static_cast<qint64>(sizeof(command_buf)));
        XR_LOG_ERROR("Rename");
      }
    });
    pingCheckTimer_->start(100);
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

    XR_LOG_DEBUG("New TCP client connected from %s",
                 tcpClientSocket_->peerAddress().toString().toUtf8().data());
    minipc_->syncConfig();
    usart1_->syncConfig();
    usart2_->syncConfig();
  }

  void onTcpDataReceived() {
    QByteArray data = tcpClientSocket_->readAll();
    topicServer_->ParseData({data.data(), static_cast<size_t>(data.size())});
    XR_LOG_DEBUG("Received TCP data size: %d", data.size());
  }

  void forwardTcpData() {
    if (!tcpClientConnected_)
      return;

    LibXR::ReadPort *ports[3] = {&minipc_->read_, &usart1_->read_,
                                 &usart2_->read_};
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
    QString filter = "";
    if (deviceManager_->filter_name_.size() > 0) {
      filter = kUdpBroadcastMessageFiltered + deviceManager_->filter_name_;
    } else {
      filter = kUdpBroadcastMessageDefault;
    }

    QByteArray message = filter.toUtf8();
    int sent =
        udpSocket_->writeDatagram(message, QHostAddress::Broadcast, kUdpPort);
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
  ClipboardBridge *clipboardBridge_;
  LibXR::Topic::Server *topicServer_;
  LibXR::Topic command_topic_;

  QTcpServer *tcpServer_ = nullptr;
  QTcpSocket *tcpClientSocket_ = nullptr;
  QUdpSocket *udpSocket_ = nullptr;

  QTimer *broadcastTimer_ = nullptr;
  QTimer *forwardTimer_ = nullptr;
  QTimer *pingCheckTimer_ = nullptr;

  bool tcpClientConnected_ = false;

  qint64 last_ping_time_ = 0;
  qint64 last_remote_ping_time_ = 0;

  static constexpr quint16 kTcpPort = 5000;
  static constexpr quint16 kUdpPort = 5001;
  static constexpr char kUdpBroadcastMessageDefault[] =
      "XRobot Debug Tools Default Message";
  static constexpr char kUdpBroadcastMessageFiltered[] =
      "XRobot Debug Tools Message Filtered:";
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
    workerThread_->terminate();
    workerThread_->wait();
    delete worker_;
  }

private:
  QThread *workerThread_;
  Worker *worker_;
};
