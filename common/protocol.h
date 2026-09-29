// ============ 通信协议定义（模拟器和客户端共用）============
// 设计要点：
//   1. 二进制帧：帧头 + 版本 + 类型 + 长度 + 负载 + 校验
//   2. 纯 C++ 实现，不依赖 Qt —— 协议层与界面层解耦，
//      将来如果要移植到嵌入式设备或非 Qt 程序，这个文件原样可用
//   3. 统一"网络字节序"（大端）：所有多字节整数按"高字节在前"写入字节流。
//      这样不管发送方/接收方 CPU 是小端（x86）还是大端（部分 ARM），
//      双方解读出的数值都一致。面试考点：什么是大小端？
#pragma once

#include <cstdint>   // uint8_t / uint16_t / uint32_t 这些定长整数类型
#include <cstring>   // memcpy
#include <vector>

namespace protocol {

// ---------- 帧格式常量 ----------
constexpr uint8_t  FRAME_HEAD_1    = 0xAA;  // 帧头第一字节
constexpr uint8_t  FRAME_HEAD_2    = 0x55;  // 帧头第二字节
constexpr uint8_t  PROTOCOL_VERSION = 0x01; // 协议版本
constexpr size_t   HEADER_SIZE     = 7;     // 帧头2 + 版本1 + 类型1 + 长度2 + 校验1

// ---------- 帧类型 ----------
enum class FrameType : uint8_t {
    HeartbeatReq = 0x01,  // 心跳请求："你还活着吗？"
    HeartbeatRsp = 0x02,  // 心跳响应："我活着"
    DataReport   = 0x03,  // 数据上报：设备周期上报采样数据
    CtrlCmd      = 0x04,  // 控制指令：上位机下发（如修改采样周期）
    CtrlAck      = 0x05,  // 控制响应：设备对指令的应答
};

// ---------- 负载字段布局（文档性定义，供拆包参考）----------
// 数据上报帧负载布局（字节序统一大端）：
//   [设备ID: 2B] [采样序号: 4B] [温度: 4B] [压力: 4B] = 14 字节
// 控制指令帧负载布局：
//   [指令码: 1B] [参数: 4B] = 5 字节
//
// 注意：下面两个结构体只用于表达"字段含义"，**不要直接 memcpy 发送**！
// 因为结构体在内存里是小端布局，直接拷字节就会带上错误的字节序。
// 打包请使用专门的 packDataReport() / packCtrlCmd() 函数。
#pragma pack(push, 1)
struct DataReportPayload {
    uint16_t deviceId;    // 设备 ID
    uint32_t sequence;    // 采样序号（每采一次 +1，接收方靠它判断丢没丢数据）
    float    temperature; // 温度 ℃
    float    pressure;    // 压力 kPa
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CtrlCmdPayload {
    uint8_t  cmd;         // 指令码：1 = 设置采样周期（毫秒）
    uint32_t param;       // 参数值：新的采样周期，如 2000 表示 2 秒
};
#pragma pack(pop)

// ---------- 大端序列化辅助函数 ----------
// 面试考点：大小端。
//   大端 = 高位字节在前（和人读数字习惯一致），网络传输统一用它；
//   小端 = 低位字节在前（x86 CPU 内存里的存法）。
// 这两个函数把"数值"直接拆成"高字节在前"的字节序列，写进缓冲区。
inline void writeU16BigEndian(std::vector<uint8_t> &buf, uint16_t v)
{
    buf.push_back(static_cast<uint8_t>(v >> 8));        // 先写高 8 位
    buf.push_back(static_cast<uint8_t>(v & 0xFF));      // 再写低 8 位
}

inline void writeU32BigEndian(std::vector<uint8_t> &buf, uint32_t v)
{
    buf.push_back(static_cast<uint8_t>(v >> 24));       // 最高字节最先
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));      // 最低字节最后
}

// 把 float 的 4 字节位模式（IEEE754）按大端写入。
// 方法：先 memcpy 成 uint32_t（位模式不变），再按大端写字节流。
// 接收方做逆操作即可还原 float，且与 CPU 大小端无关。
inline void writeFloatBigEndian(std::vector<uint8_t> &buf, float f)
{
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    writeU32BigEndian(buf, bits);
}

// ---------- 校验和 ----------
// sum8：把一段字节全部相加，取结果的最低 8 位。
// 接收方收到帧后对同样的范围重算一遍，结果不一致 → 传输出错 → 丢弃该帧。
inline uint8_t checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i)
        sum += data[i];
    return static_cast<uint8_t>(sum & 0xFF);
}

// ---------- 打包核心：负载 → 完整帧 ----------
inline std::vector<uint8_t> packFrame(FrameType type, const uint8_t *payload, size_t payloadLen)
{
    std::vector<uint8_t> frame;
    frame.reserve(HEADER_SIZE + payloadLen);

    // 1. 帧头（2 字节，固定值，接收方靠它找帧边界）
    frame.push_back(FRAME_HEAD_1);   // 0xAA
    frame.push_back(FRAME_HEAD_2);   // 0x55

    // 2. 版本 + 类型
    frame.push_back(PROTOCOL_VERSION);
    frame.push_back(static_cast<uint8_t>(type));

    // 3. 负载长度（2 字节，大端）
    writeU16BigEndian(frame, static_cast<uint16_t>(payloadLen));

    // 4. 负载
    frame.insert(frame.end(), payload, payload + payloadLen);

    // 5. 校验（对负载部分计算 sum8）
    frame.push_back(checksum(payload, payloadLen));

    return frame;
}

// ---------- 拆包：大端字节流 → 数值（打包的逆过程）----------
// 这些是"翻译器"：把网络来的大端字节按协议还原成本机数值。
// 拆包和打包必须严格对称，否则两端读到的数就不一样。

// 从大端字节流读 uint16_t（打包时 writeU16BigEndian 的逆操作）
inline uint16_t readU16BigEndian(const uint8_t *p)
{
    return uint16_t((uint16_t(p[0]) << 8) | p[1]);
}

// 从大端字节流读 uint32_t
inline uint32_t readU32BigEndian(const uint8_t *p)
{
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
         | (uint32_t(p[2]) << 8)  |  uint32_t(p[3]);
}

// 从大端字节流读 float（先读回位模式，再按位变回 float）
inline float readFloatBigEndian(const uint8_t *p)
{
    uint32_t bits = readU32BigEndian(p);
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

// 解析数据上报帧的负载：p 指向负载开头，len 是负载长度。
// 成功返回 true 并把字段填进引用参数；长度不够返回 false。
inline bool parseDataReportPayload(const uint8_t *p, size_t len,
                                   uint16_t &deviceId, uint32_t &sequence,
                                   float &temperature, float &pressure)
{
    if (len < 14)   // 负载固定 14 字节，短了就不是合法的数据上报帧
        return false;
    deviceId    = readU16BigEndian(p);      // 偏移 0，长 2
    sequence    = readU32BigEndian(p + 2);  // 偏移 2，长 4
    temperature = readFloatBigEndian(p + 6); // 偏移 6，长 4
    pressure    = readFloatBigEndian(p + 10); // 偏移 10，长 4
    return true;
}

// ---------- 各类型帧的打包函数 ----------

// 数据上报帧：设备周期上报一次采样
inline std::vector<uint8_t> packDataReport(uint16_t deviceId, uint32_t sequence,
                                           float temperature, float pressure)
{
    std::vector<uint8_t> payload;
    payload.reserve(14);
    writeU16BigEndian(payload, deviceId);
    writeU32BigEndian(payload, sequence);
    writeFloatBigEndian(payload, temperature);
    writeFloatBigEndian(payload, pressure);
    return packFrame(FrameType::DataReport, payload.data(), payload.size());
}

// 控制指令帧：上位机下发指令（目前仅"设置采样周期"）
inline std::vector<uint8_t> packCtrlCmd(uint8_t cmd, uint32_t param)
{
    std::vector<uint8_t> payload;
    payload.reserve(5);
    payload.push_back(cmd);
    writeU32BigEndian(payload, param);
    return packFrame(FrameType::CtrlCmd, payload.data(), payload.size());
}

// 心跳请求 / 心跳响应 / 控制响应：无负载，只发帧头
inline std::vector<uint8_t> packEmptyFrame(FrameType type)
{
    return packFrame(type, nullptr, 0);
}

} // namespace protocol
