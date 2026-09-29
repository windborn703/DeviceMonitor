// ============ 通信层实现（心跳 + 看门狗 + 自动重连）============
#include "DeviceClient.h"
#include "protocol.h"   // 打包/常量
#include <QDateTime>    // currentMSecsSinceEpoch：活性时间戳

DeviceClient::DeviceClient(QObject *parent)
    : QObject(parent)
{
    // ---------- socket 信号 → 本类信号 ----------
    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        m_reconnectTimer.stop();       // ★ 关键：连上了必须停掉重连定时器，
                                       //   否则它会继续到点重拨，把好连接掐断
        m_reconnectAttempt = 0;        // 连上了，重连计数清零
        m_lastActivity = QDateTime::currentMSecsSinceEpoch();
        m_heartbeatTimer.start();      // 开始发心跳
        emit connected();
    });

    connect(&m_socket, &QTcpSocket::disconnected, this, [this]() {
        m_heartbeatTimer.stop();           // 断开了，停心跳
        emit disconnected();
        // 意外断开（非用户主动）→ 安排自动重连
        if (!m_manualDisconnect)
            scheduleReconnect();
    });

    connect(&m_socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                emit connectionError(m_socket.errorString());
            });

    // ---------- 数据到达 → 刷新活性时间 + 喂解析器 ----------
    connect(&m_socket, &QTcpSocket::readyRead,
            this, &DeviceClient::onReadyRead);

    // ---------- 解析结果转发 ----------
    connect(&m_parser, &protocol::FrameParser::dataReportReceived,
            this, &DeviceClient::dataReceived);
    connect(&m_parser, &protocol::FrameParser::ctrlAckReceived,
            this, &DeviceClient::ctrlAckReceived);
    connect(&m_parser, &protocol::FrameParser::frameReceived,
            this, &DeviceClient::onFrameReceived);
    connect(&m_parser, &protocol::FrameParser::parseError,
            this, &DeviceClient::protocolError);

    // ---------- 心跳定时器：每 1.5 秒问一次"你还活着吗" ----------
    m_heartbeatTimer.setInterval(1500);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, [this]() {
        if (!isConnected())
            return;
        auto frame = protocol::packEmptyFrame(protocol::FrameType::HeartbeatReq);
        m_socket.write(reinterpret_cast<const char *>(frame.data()),
                       qint64(frame.size()));
    });

    // ---------- 看门狗：每 2 秒检查一次活性 ----------
    m_watchdogTimer.setInterval(2000);
    connect(&m_watchdogTimer, &QTimer::timeout,
            this, &DeviceClient::onWatchdogTimeout);
    m_watchdogTimer.start();  // 看门狗常开（重连期间也在跑）

    // ---------- 重连定时器：3 秒后重试 ----------
    m_reconnectTimer.setInterval(3000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, [this]() {
        ++m_reconnectAttempt;              // ★ 每次尝试计数 +1（之前漏了，永远"第1次"）
        emit reconnecting(m_reconnectAttempt);
        connectToDevice(m_host, m_port);   // 用记住的地址重拨
    });
}

void DeviceClient::onReadyRead()
{
    m_lastActivity = QDateTime::currentMSecsSinceEpoch();  // 任何数据都是活性证明
    m_parser.appendData(m_socket.readAll());
}

void DeviceClient::onFrameReceived(quint8 type, const QByteArray &)
{
    // 收到心跳响应：刷新活性时间（其他帧在 onReadyRead 里已刷新）
    if (type == quint8(protocol::FrameType::HeartbeatRsp))
        m_lastActivity = QDateTime::currentMSecsSinceEpoch();
}

void DeviceClient::onWatchdogTimeout()
{
    // 已连接但 6 秒没有任何数据（4 个心跳周期 + 3 个采样周期的余量）
    // → 判定"假死连接"：主动掐断，触发重连流程
    if (isConnected() &&
        QDateTime::currentMSecsSinceEpoch() - m_lastActivity > 6000) {
        emit connectionError("连接假死（6 秒无数据），主动断开准备重连");
        m_socket.abort();  // abort：立刻断开，不等待，不通知对端
    }
}

void DeviceClient::scheduleReconnect()
{
    if (!m_reconnectTimer.isActive())
        m_reconnectTimer.start();  // 3 秒后第一次重连
}

void DeviceClient::connectToDevice(const QString &host, quint16 port)
{
    m_host = host;   // 记住参数（重连时要用）
    m_port = port;
    m_manualDisconnect = false;
    m_socket.connectToHost(host, port);
}

void DeviceClient::disconnectFromDevice()
{
    m_manualDisconnect = true;   // 用户主动断开 → 不自动重连
    m_reconnectTimer.stop();     // 取消挂起的重连
    m_socket.disconnectFromHost();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();
}

bool DeviceClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void DeviceClient::sendCtrlCmd(quint8 cmd, quint32 param)
{
    if (!isConnected())
        return;
    auto frame = protocol::packCtrlCmd(cmd, param);
    m_socket.write(reinterpret_cast<const char *>(frame.data()),
                   qint64(frame.size()));
}
