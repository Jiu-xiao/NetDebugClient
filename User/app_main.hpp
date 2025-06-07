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
      : QObject(parent), qmlEngine_(qmlEngine),
        command_topic_("command", sizeof(Command)), tcpClientConnected_(false) {
    initBackends();
    initClipboard();
    initQmlUI();
    initCommandHandler();
    initTopicServer();
  }

public slots:
  void start() {
    initTcpServer();
    initTimers();
  }

private:
  void initBackends() {
    /* 创建三个串口后端实例，分别对应 MiniPC、USART1、USART2 */
    minipc_ = new TerminalBackend("uart_cdc", 0, qmlEngine_);
    usart1_ = new TerminalBackend("uart1", 1, qmlEngine_);
    usart2_ = new TerminalBackend("uart2", 2, qmlEngine_);
  }

  void initClipboard() {
    /* 创建剪贴板桥接，并注入到 QML 上下文中 */
    clipboardBridge_ = new ClipboardBridge();
    qmlEngine_->rootContext()->setContextProperty("clipboardBridge",
                                                  clipboardBridge_);
  }

  void initQmlUI() {
    /* 注册 DeviceManager 类型并加载主界面 */
    qmlRegisterType<DeviceManager>("com.example", 1, 0, "DeviceManager");
    qmlEngine_->loadFromModule("MyApp", "Main");

    /* 获取 QML 中的 DeviceManager 对象 */
    QObject *rootObject = qmlEngine_->rootObjects().first();
    deviceManager_ = rootObject->findChild<DeviceManager *>("device_manager");
    ASSERT(deviceManager_ != nullptr);
  }

  void initCommandHandler() {
    /* 注册处理 PING 和 REMOTE_PING 的命令回调 */
    auto cb = LibXR::Topic::Callback::Create(
        [](bool, Worker *self, LibXR::RawData &data) {
          if (data.size_ <= sizeof(Command)) {
            Command cmd = *reinterpret_cast<Command *>(data.addr_);
            switch (cmd.type) {
            case Command::Type::PING:
              XR_LOG_DEBUG("Received PING command");
              self->last_ping_time_ = QDateTime::currentMSecsSinceEpoch();
              break;
            case Command::Type::REMOTE_PING:
              XR_LOG_DEBUG("Received REMOTE_PING command");
              self->last_remote_ping_time_ =
                  QDateTime::currentMSecsSinceEpoch();
              break;
            default:
              break;
            }
          }
        },
        this);
    command_topic_.RegisterCallback(cb);
  }

  void initTopicServer() {
    /* 创建 Topic Server 并注册四个 Topic */
    topicServer_ = new LibXR::Topic::Server(40960);
    topicServer_->Register(minipc_->topic_);
    topicServer_->Register(usart1_->topic_);
    topicServer_->Register(usart2_->topic_);
    topicServer_->Register(command_topic_);
  }

  void initTcpServer() {
    /* 启动 TCP 服务器，监听端口 */
    tcpServer_ = new QTcpServer(this);
    udpSocket_ = new QUdpSocket(this);

    connect(tcpServer_, &QTcpServer::newConnection, this,
            &Worker::onNewConnection);
    if (!tcpServer_->listen(QHostAddress::Any, kTcpPort)) {
      XR_LOG_ERROR("Failed to start TCP server");
      return;
    }
    XR_LOG_DEBUG("TCP Server started on port %d", kTcpPort);
  }

  void initTimers() {
    /*
     * 定时广播设备标识（通过 UDP）：
     *  - 仅在没有 TCP 客户端连接时执行；
     *  - 若设置了设备名过滤器，则广播带过滤名的消息；
     *  - 否则广播默认消息；
     *  - 周期为 1000 毫秒。
     */
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

    /*
     * 串口数据转发到 TCP 客户端：
     *  - 每 1 毫秒检查串口缓冲区是否有数据；
     *  - 若有，则读取并通过 TCP 发送；
     *  - 用于将串口实时数据透传到远程客户端。
     */
    forwardTimer_ = new QTimer(this);
    connect(forwardTimer_, &QTimer::timeout, this, &Worker::forwardTcpData);
    forwardTimer_->start(1);

    /*
     * 定期检测本地和远程 PING 状态：
     *  - 周期为 100 毫秒；
     *  - 若本地 PING 超时则断开客户端连接并广播；
     *  - 若状态变化，更新 UI 状态（Backend/MiniPC Online）；
     *  - 若有重启/重命名请求，则向 minipc 发送命令。
     */
    pingCheckTimer_ = new QTimer(this);
    connect(pingCheckTimer_, &QTimer::timeout, this, [this]() {
      const qint64 now = QDateTime::currentMSecsSinceEpoch();
      const bool isOnline = (now - last_ping_time_) <= 300;

      /* 本地 PING 超时处理 */
      if (!isOnline && tcpClientConnected_) {
        XR_LOG_INFO("TCP client disconnected");
        tcpClientConnected_ = false;
        tcpClientSocket_->disconnectFromHost();
        broadcastUdpMessage();
      }

      /* 更新本地后端在线状态 */
      if (deviceManager_->isBackendConnected() != isOnline) {
        deviceManager_->SetBackendConnected(isOnline);
        XR_LOG_INFO("Backend status changed: %s",
                    isOnline ? "online" : "offline");
      }

      /* 更新 MiniPC 在线状态 */
      const bool isRemoteOnline = (now - last_remote_ping_time_) <= 300;
      if (deviceManager_->isMiniPCOnline() != isRemoteOnline) {
        deviceManager_->SetMiniPCOnline(isRemoteOnline);
        XR_LOG_INFO("MiniPC status changed: %s",
                    isRemoteOnline ? "online" : "offline");
      }

      /* 如果 UI 请求设备重启，发送 REBOOT 命令 */
      if (deviceManager_->require_restart_) {
        LibXR::Topic::PackedData<Command::Type> command;
        LibXR::Topic::PackData(command_topic_.GetKey(), command,
                               Command::Type::REBOOT);
        uint8_t buf[sizeof(command) + LibXR::Topic::PACK_BASE_SIZE];
        LibXR::Topic::PackData(minipc_->topic_.GetKey(), buf, command);
        tcpClientSocket_->write(reinterpret_cast<char *>(buf), sizeof(buf));
        deviceManager_->require_restart_ = false;
      }

      /* 如果 UI 请求设备改名，发送 RENAME 命令 */
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
                                sizeof(command_buf));
        XR_LOG_INFO("Rename");
      }
    });
    pingCheckTimer_->start(100);
  }

private slots:
  void onNewConnection() {
    /*
     * 处理新的 TCP 客户端连接：
     *  - 如果已有客户端连接，则拒绝新连接；
     *  - 否则接收连接并绑定读取信号；
     *  - 连接成功后，同步三个串口配置。
     */
    if (tcpClientConnected_) {
      QTcpSocket *newClient = tcpServer_->nextPendingConnection();
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

    /* 同步串口配置到客户端 */
    minipc_->syncConfig();
    usart1_->syncConfig();
    usart2_->syncConfig();
  }

  void onTcpDataReceived() {
    /*
     * 读取 TCP 客户端发送的数据：
     *  - 读取全部数据并解析为 Topic 消息；
     *  - Topic 协议用于多通道数据接收与命令分发。
     */
    QByteArray data = tcpClientSocket_->readAll();
    topicServer_->ParseData({data.data(), static_cast<size_t>(data.size())});
    XR_LOG_DEBUG("Received TCP data size: %d", data.size());
  }

  void forwardTcpData() {
    /*
     * 从所有串口中读取待转发数据：
     *  - 每次扫描所有串口（MiniPC / USART1 / USART2）；
     *  - 若有新数据，则从队列中读取并转发到 TCP 客户端；
     *  - 此函数由定时器定期触发，确保数据低延迟转发。
     */
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
    /*
     * 通过 UDP 广播设备识别信息：
     *  - 如果已设置设备名过滤器，则使用带设备名的消息；
     *  - 否则使用默认广播消息；
     *  - 用于局域网内设备自动发现；
     *  - 实际广播地址为 255.255.255.255:kUdpPort。
     */
    QString filter =
        deviceManager_->filter_name_.isEmpty()
            ? kUdpBroadcastMessageDefault
            : kUdpBroadcastMessageFiltered + deviceManager_->filter_name_;

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
  QQmlApplicationEngine *qmlEngine_;

  /* 系统组件 */
  DeviceManager *deviceManager_;
  TerminalBackend *minipc_;
  TerminalBackend *usart1_;
  TerminalBackend *usart2_;
  ClipboardBridge *clipboardBridge_;
  LibXR::Topic::Server *topicServer_;
  LibXR::Topic command_topic_;

  /* 网络通信 */
  QTcpServer *tcpServer_ = nullptr;
  QTcpSocket *tcpClientSocket_ = nullptr;
  QUdpSocket *udpSocket_ = nullptr;

  /* 定时器 */
  QTimer *broadcastTimer_ = nullptr;
  QTimer *forwardTimer_ = nullptr;
  QTimer *pingCheckTimer_ = nullptr;

  /* 状态变量 */
  bool tcpClientConnected_ = false;
  qint64 last_ping_time_ = 0;
  qint64 last_remote_ping_time_ = 0;

  /* 常量定义 */
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
