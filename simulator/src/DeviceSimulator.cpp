// ============ 设备模拟器实现 ============
#include "DeviceSimulator.h"

#include <QTcpSocket>       // 接受连接后每个客户端对应一个 QTcpSocket
#include <QHostAddress>     // IP 地址（QHostAddress::Any = 监听所有网卡）
#include <QRandomGenerator> // 随机数（Qt 的随机数类，比 rand() 好用）
#include <QDebug>           // qInfo()/qCritical() 日志输出

#include "protocol.h"       // 共享协议：打包函数在这

DeviceSimulator::DeviceSimulator(QObject *parent)
    : QObject(parent)
{
    // --- 信号槽连接（事件驱动：告诉 Qt "发生 X 时叫我"）---
    // 有新客户端连接进来 → onNewConnection()
    connect(&m_server, &QTcpServer::newConnection,
            this, &DeviceSimulator::onNewConnection);

    // 定时器到点 → onTick() 采样一次
    connect(&m_timer, &QTimer::timeout,
            this, &DeviceSimulator::onTick);
}

bool DeviceSimulator::start(quint16 port)
{
    // 开始监听。QHostAddress::Any = 不管本机有几个网卡/什么 IP，都接受连接。
    if (!m_server.listen(QHostAddress::Any, port)) {
        qCritical() << "监听失败：" << m_server.errorString();
        return false;
    }

    qInfo() << "设备模拟器启动，监听端口" << port
            << "（设备 ID =" << m_deviceId << "）";

    m_timer.start(1000);  // 1000 毫秒 = 每秒采样一次
    return true;
}

void DeviceSimulator::onNewConnection()
{
    // nextPendingConnection()：取出一个等待中的新连接。
    // 可能同时来好几个，所以用 while 全部接完。
    while (QTcpSocket *socket = m_server.nextPendingConnection()) {
        m_clients.append(socket);  // 登记到客户端列表

        // 这个客户端断开时 → onClientDisconnected()
        connect(socket, &QTcpSocket::disconnected,
                this, &DeviceSimulator::onClientDisconnected);

        qInfo() << "客户端接入：" << socket->peerAddress().toString()
                << ":" << socket->peerPort()
                << "（当前在线" << m_clients.size() << "个）";
    }
}

void DeviceSimulator::onClientDisconnected()
{
    // sender()：返回"是谁发的这个信号"——因为多个客户端共用一个槽，
    // 我们得知道断开的到底是哪一个 socket。
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;

    m_clients.removeAll(socket);
    qInfo() << "客户端断开（剩余" << m_clients.size() << "个连接）";

    // deleteLater()：不能直接 delete——当前可能还在这个 socket 的信号
    // 处理流程中，直接删会出事。交给事件循环"下次有空时"再销毁。
    // （面试点：deleteLater 和 delete 的区别）
    socket->deleteLater();
}

void DeviceSimulator::onTick()
{
    // --- 1. 随机游走：模拟真实传感器数据的连续小幅波动 ---
    // generateDouble() 返回 [0,1) 的随机数。
    // 温度每次变化 ±0.3℃：v * 0.6 - 0.3 ∈ [-0.3, +0.3)
    // 压力每次变化 ±0.2 kPa
    m_temperature += float(QRandomGenerator::global()->generateDouble() * 0.6 - 0.3);
    m_pressure    += float(QRandomGenerator::global()->generateDouble() * 0.4 - 0.2);
    ++m_sequence;

    // --- 2. 按协议打包成帧（复用 common/protocol.h，大端字节序）---
    std::vector<uint8_t> frame = protocol::packDataReport(
        m_deviceId, m_sequence, m_temperature, m_pressure);

    // --- 3. std::vector → QByteArray（Qt 网络 API 收发用的字节容器）---
    QByteArray bytes(reinterpret_cast<const char *>(frame.data()),
                     int(frame.size()));

    // --- 4. 广播给所有客户端 ---
    broadcast(bytes);

    qInfo().noquote() << QString("采样 #%1 | 温度 %2 ℃ | 压力 %3 kPa | 发送帧 %4 字节")
                             .arg(m_sequence)
                             .arg(m_temperature, 0, 'f', 2)
                             .arg(m_pressure, 0, 'f', 2)
                             .arg(frame.size());
}

void DeviceSimulator::broadcast(const QByteArray &data)
{
    for (QTcpSocket *socket : m_clients) {
        // 只发给还在线（连接正常）的客户端
        if (socket->state() == QAbstractSocket::ConnectedState)
            socket->write(data);  // write 只是把数据交给 Qt，Qt 会异步发出去
    }
}
