// ============ 通信层：上位机与设备之间的"接线员" ============
// 职责：
//   1. 建立/断开与设备模拟器的 TCP 连接（含自动重连）
//   2. 心跳保活 + 看门狗：检测"假死连接"（对端断电/断网但不发断开通知）
//   3. 把收到的字节流喂给 FrameParser，解析结果以信号形式向上抛出
// 设计：不包含任何界面代码，不依赖 MainWindow —— 纯信号对外。
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "FrameParser.h"

class DeviceClient : public QObject
{
    Q_OBJECT

public:
    explicit DeviceClient(QObject *parent = nullptr);

    // 发起连接（异步：结果通过 connected/connectionError 信号通知）
    void connectToDevice(const QString &host, quint16 port);
    // 手动断开（不会触发自动重连）
    void disconnectFromDevice();
    bool isConnected() const;
    // 向设备发送控制指令
    void sendCtrlCmd(quint8 cmd, quint32 param);

signals:
    void connected();                          // 连接建立成功
    void disconnected();                       // 连接断开
    void connectionError(const QString &what); // 连接失败
    void reconnecting(int attempt);            // 正在尝试第 N 次自动重连
    void dataReceived(quint16 deviceId, quint32 sequence,
                      float temperature, float pressure);
    void ctrlAckReceived(quint8 cmd, quint8 result);
    void protocolError(const QString &reason);

private:
    void onReadyRead();
    void onFrameReceived(quint8 type, const QByteArray &payload);
    void onWatchdogTimeout();      // 看门狗：检查是否"假死"
    void scheduleReconnect();      // 安排自动重连

    QTcpSocket m_socket;
    protocol::FrameParser m_parser;

    QTimer m_heartbeatTimer;  // 每 1.5 秒发一次心跳请求
    QTimer m_watchdogTimer;   // 每 2 秒检查一次活性
    QTimer m_reconnectTimer;  // 重连定时器（3 秒一次）

    qint64 m_lastActivity = 0;   // 上次收到任何数据的时刻（毫秒时间戳）
    QString m_host;              // 记住连接参数，重连要用
    quint16 m_port = 0;
    bool m_manualDisconnect = false;  // 区分"用户主动断开"和"意外断开"
    int m_reconnectAttempt = 0;       // 重连尝试次数
};
