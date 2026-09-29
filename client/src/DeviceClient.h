// ============ 通信层：上位机与设备之间的"接线员" ============
// 职责：
//   1. 建立/断开与设备模拟器的 TCP 连接
//   2. 把收到的字节流喂给 FrameParser，把解析结果以信号形式向上抛出
// 设计：不包含任何界面代码，不依赖 MainWindow ——
//   界面通过信号槽"收听"它。这就是分层和解耦。
#pragma once

#include <QObject>
#include <QTcpSocket>
#include "FrameParser.h"

class DeviceClient : public QObject
{
    Q_OBJECT

public:
    explicit DeviceClient(QObject *parent = nullptr);

    // 发起连接（异步：函数立刻返回，成功/失败通过信号通知）
    void connectToDevice(const QString &host, quint16 port);
    // 断开连接
    void disconnectFromDevice();
    // 当前是否已连接
    bool isConnected() const;
    // 向设备发送控制指令（如设置采样周期），设备会回 CtrlAck
    void sendCtrlCmd(quint8 cmd, quint32 param);

signals:
    void connected();                        // 连接建立成功
    void disconnected();                     // 连接断开（主动或被动）
    void connectionError(const QString &what);  // 连接失败（如"目标机器拒绝"）
    // 收到一帧数据上报（已通过协议层全部验证）
    void dataReceived(quint16 deviceId, quint32 sequence,
                      float temperature, float pressure);
    // 收到设备的控制响应（result：0=成功，1=失败）
    void ctrlAckReceived(quint8 cmd, quint8 result);
    void protocolError(const QString &reason);  // 协议层异常（坏帧等）

private:
    QTcpSocket m_socket;              // Qt 的 TCP 客户端套接字
    protocol::FrameParser m_parser;   // 上一课的帧解析器（粘包拆包）
};
