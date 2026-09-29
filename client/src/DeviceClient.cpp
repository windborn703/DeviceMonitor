// ============ 通信层实现 ============
#include "DeviceClient.h"

DeviceClient::DeviceClient(QObject *parent)
    : QObject(parent)
{
    // ---------- socket 的信号 → 本类的信号（向上转发）----------
    connect(&m_socket, &QTcpSocket::connected,
            this, &DeviceClient::connected);

    connect(&m_socket, &QTcpSocket::disconnected,
            this, &DeviceClient::disconnected);

    // errorOccurred：连接被拒、超时等都会触发
    connect(&m_socket, &QTcpSocket::errorOccurred,
            this, [this](QAbstractSocket::SocketError) {
                emit connectionError(m_socket.errorString());
            });

    // ---------- 收到字节 → 喂给 FrameParser ----------
    connect(&m_socket, &QTcpSocket::readyRead,
            this, [this]() {
                m_parser.appendData(m_socket.readAll());
            });

    // ---------- FrameParser 的解析结果 → 继续向上转发 ----------
    connect(&m_parser, &protocol::FrameParser::dataReportReceived,
            this, &DeviceClient::dataReceived);

    connect(&m_parser, &protocol::FrameParser::parseError,
            this, &DeviceClient::protocolError);
}

void DeviceClient::connectToDevice(const QString &host, quint16 port)
{
    // connectToHost 是异步的：调用后立刻返回，
    // 真正的结果（成功/失败）稍后通过 connected/errorOccurred 信号来
    m_socket.connectToHost(host, port);
}

void DeviceClient::disconnectFromDevice()
{
    m_socket.disconnectFromHost();
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();  // 立即断开（不等待队列里的数据发完）
}

bool DeviceClient::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}
