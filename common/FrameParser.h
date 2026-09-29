// ============ 帧解析器：解决 TCP 粘包/半包问题 ============
// 职责：把"连续的字节流"还原成"一条条完整帧"。
//   输入：appendData() 喂入任意长度的新到数据（可能半帧、可能多帧粘一起）
//   输出：通过信号吐出验证通过的帧（dataReportReceived / frameReceived）
// 设计：与网络、UI 完全解耦 —— 谁来喂数据都行（TCP、串口、文件），
//   这也是它能被模拟器和客户端共享的原因。
#pragma once

#include <QObject>
#include <QByteArray>

namespace protocol {

class FrameParser : public QObject
{
    Q_OBJECT

public:
    explicit FrameParser(QObject *parent = nullptr);

    // 喂入新收到的字节。内部会循环剥帧：能剥多少剥多少，
    // 剩下的半帧留在缓冲区，等下次数据到了继续拼。
    void appendData(const QByteArray &data);

    // 缓冲区里还剩下多少未处理的字节（0 = 当前没有残留的半帧）
    qint64 bufferedBytes() const { return m_buffer.size(); }

signals:
    // 成功解析出一帧"数据上报"（帧头+长度+校验全部验证通过）
    void dataReportReceived(quint16 deviceId, quint32 sequence,
                            float temperature, float pressure);
    // 成功解析出一帧"控制指令"（设备侧订阅：收到上位机的指挥）
    void ctrlCmdReceived(quint8 cmd, quint32 param);
    // 成功解析出一帧"控制响应"（上位机侧订阅：设备对指令的回执）
    void ctrlAckReceived(quint8 cmd, quint8 result);
    // 解析出任意一帧（类型 + 负载）。心跳等通用帧走这里。
    void frameReceived(quint8 frameType, const QByteArray &payload);
    // 遇到异常帧（校验失败/长度异常）：给上层记日志、告警用
    void parseError(const QString &reason);

private:
    // 尝试从缓冲区剥出一帧。剥出返回 true（可能还有下一帧），
    // 不够一帧返回 false（等下次数据）。
    bool tryParseOneFrame();

    QByteArray m_buffer;  // 累积缓冲区：所有未处理完的字节都在这
};

} // namespace protocol
