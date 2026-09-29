// ============ 设备模拟器：假装是一台工业传感器设备 ============
// 职责：
//   1. 作为 TCP 服务端监听 8899 端口，接受上位机连接
//   2. 每秒采样一次（温度/压力随机游走），按协议打包后广播给所有客户端
// 头文件只管"声明有什么"，实现细节在 DeviceSimulator.cpp
#pragma once

#include <QObject>      // Qt 所有对象之母：提供信号槽、父子对象树能力
#include <QTcpServer>   // TCP 服务端
#include <QTimer>       // 定时器
#include <QList>        // Qt 风格的动态数组
#include "FrameParser.h"  // 帧解析器：解析上位机发来的指令帧

class QTcpSocket;       // 前置声明：这里只用指针，不用看到完整定义

class DeviceSimulator : public QObject
{
    Q_OBJECT   // Qt 的"身份证"宏：告诉 moc 工具这个类有信号槽要处理
               // （面试高频：Q_OBJECT 是什么？moc 拿它做什么？）

public:
    explicit DeviceSimulator(QObject *parent = nullptr);

    // 启动：开始监听端口 + 启动采样定时器。失败返回 false。
    bool start(quint16 port);

private slots:   // 槽函数：被信号触发时执行的"回调"
    void onNewConnection();      // 槽：有新客户端连进来了
    void onClientDisconnected(); // 槽：有客户端断开了
    void onTick();               // 槽：定时器到点 → 采样 + 广播
    void onClientDataReady();    // 槽：有客户端发来数据
    void onCtrlCmdReceived(quint8 cmd, quint32 param);  // 槽：解析出控制指令
    void onFrameReceived(quint8 type, const QByteArray &payload);  // 槽：通用帧（心跳等）

private:
    // 把数据发给所有已连接的客户端
    void broadcast(const QByteArray &data);

    // --- 网络部分 ---
    QTcpServer m_server;            // 服务端对象
    QList<QTcpSocket *> m_clients;  // 当前所有连接（每个客户端一个 socket）
    QTimer m_timer;                 // 采样定时器
    protocol::FrameParser m_parser; // 帧解析器：把客户端发来的字节流还原成指令

    // --- 设备状态（模拟的传感器数据）---
    quint16 m_deviceId    = 1;        // 本设备 ID
    quint32 m_sequence    = 0;        // 采样序号，每采一次 +1
    float   m_temperature = 25.0f;    // 当前温度（从室温开始）
    float   m_pressure    = 101.3f;   // 当前压力（从标准大气压开始）
};
