// ============ 帧解析器实现 ============
#include "FrameParser.h"
#include "protocol.h"

namespace protocol {

FrameParser::FrameParser(QObject *parent)
    : QObject(parent)
{
}

void FrameParser::appendData(const QByteArray &data)
{
    m_buffer.append(data);         // 1. 累积：不管几帧，先全倒进缓冲区
    while (tryParseOneFrame()) {   // 2. 循环剥帧：能剥多少剥多少
    }
}

bool FrameParser::tryParseOneFrame()
{
    // ============ 第 1 关：找帧头 AA 55 ============
    const int bufSize = m_buffer.size();
    int start = -1;
    for (int i = 0; i + 1 < bufSize; ++i) {
        // 注意 uint8_t 转换：char 是有符号的，0xAA 在 char 里是负数，
        // 不转换直接比较永远不相等
        if (uint8_t(m_buffer[i]) == FRAME_HEAD_1 &&
            uint8_t(m_buffer[i + 1]) == FRAME_HEAD_2) {
            start = i;
            break;
        }
    }
    if (start < 0) {
        // 整个缓冲区没有帧头：全部丢弃？
        // 不行——最后 1 个字节可能是"0xAA"（帧头的第一半），
        // 下批数据可能带来"0x55"拼成完整帧头。所以保留最后一个 AA。
        if (bufSize > 0 && uint8_t(m_buffer.back()) == FRAME_HEAD_1)
            m_buffer = QByteArray(1, char(FRAME_HEAD_1));
        else
            m_buffer.clear();
        return false;
    }
    if (start > 0)
        m_buffer.remove(0, start);  // 帧头前的垃圾字节（错位数据）丢掉

    // ============ 第 2 关：头部凑齐了吗？ ============
    // 至少需要 帧头2+版本1+类型1+长度2+校验1 = 7 字节
    if (m_buffer.size() < int(HEADER_SIZE))
        return false;  // 连头部都不齐，等下次数据

    // ============ 第 3 关：读长度，等整帧 ============
    const uint8_t *p = reinterpret_cast<const uint8_t *>(m_buffer.constData());
    const uint16_t payloadLen = readU16BigEndian(p + 4);  // 长度字段在偏移 4

    // 防御：长度异常大（比如垃圾数据碰巧以 AA 55 开头，读出的"长度"离谱），
    // 说明这不是真帧头 → 丢掉帧头第一个字节，从下一个位置重新找。
    // 4096 是上限：我们的协议负载最多几十字节，4KB 足够宽裕。
    if (payloadLen > 4096) {
        m_buffer.remove(0, 1);
        emit parseError(QString("长度字段异常(%1)，丢弃 1 字节重找帧头").arg(payloadLen));
        return true;  // 继续循环尝试
    }

    const int totalLen = int(HEADER_SIZE) + payloadLen;  // 整帧长度
    if (m_buffer.size() < totalLen)
        return false;  // 半帧：负载还没收全，等下次数据补上

    // ============ 第 4 关：校验和 ============
    const uint8_t sum = checksum(p + 6, payloadLen);  // 对负载重算
    if (sum != uint8_t(m_buffer[totalLen - 1])) {     // 和帧尾携带的对比
        m_buffer.remove(0, 2);  // 校验失败：这不是合法帧，丢帧头重找
        emit parseError("校验和不符，丢弃 2 字节重找帧头");
        return true;
    }

    // ============ 第 5 关：过关！剥出这一帧 ============
    // 类型在偏移 3（帧头占 0、1，版本占 2）
    const uint8_t frameType = uint8_t(m_buffer[3]);
    const QByteArray payload = m_buffer.mid(6, payloadLen);  // 负载从偏移 6 开始

    // 按类型分发（数据上报帧直接翻译成数值，方便上层直接用）
    if (frameType == uint8_t(FrameType::DataReport)) {
        uint16_t deviceId = 0;
        uint32_t sequence = 0;
        float temperature = 0.0f, pressure = 0.0f;
        if (parseDataReportPayload(reinterpret_cast<const uint8_t *>(payload.constData()),
                                   size_t(payload.size()),
                                   deviceId, sequence, temperature, pressure)) {
            emit dataReportReceived(deviceId, sequence, temperature, pressure);
        }
    }
    emit frameReceived(frameType, payload);

    m_buffer.remove(0, totalLen);  // 剥掉已处理的整帧
    return true;                   // 缓冲区里可能还粘着下一帧，继续循环
}

} // namespace protocol
