// ============ 协议打包单元测试（纯 C++ 控制台程序，不依赖 Qt）============
// 作用：逐字节断言打包结果与协议设计一致（大端字节序 + 校验和），
//       任何一个字节错了都会立刻报错 —— 这就是单元测试的意义：
//       协议是双方通信的"合同"，错一个字节，两端就互相读不懂了。
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "protocol.h"

using namespace protocol;

// 断言辅助：实际字节串必须与期望字节串完全一致
static bool expectBytes(const char *name,
                        const std::vector<uint8_t> &actual,
                        std::initializer_list<uint8_t> expected)
{
    if (actual.size() != expected.size()) {
        printf("❌ [%s] 长度不符：实际 %zu 字节，期望 %zu 字节\n",
               name, actual.size(), expected.size());
        return false;
    }
    size_t i = 0;
    for (uint8_t e : expected) {
        if (actual[i] != e) {
            printf("❌ [%s] 第 %zu 字节不符：实际 0x%02X，期望 0x%02X\n",
                   name, i, actual[i], e);
            return false;
        }
        ++i;
    }
    printf("✅ [%s] %zu 字节全部一致\n", name, actual.size());
    return true;
}

// 打印十六进制（对比学习用）
static void printHex(const char *title, const std::vector<uint8_t> &bytes)
{
    printf("   %s：", title);
    for (uint8_t b : bytes)
        printf("%02X ", b);
    printf("\n");
}

int main()
{
    printf("========== 协议打包单元测试 ==========\n\n");
    bool allPassed = true;

    // --- 测试 1：数据上报帧 ---
    {
        auto frame = packDataReport(/*deviceId=*/1, /*sequence=*/2,
                                    /*temperature=*/23.5f, /*pressure=*/101.3f);
        printHex("数据上报帧", frame);
        // 期望（对照协议图）：
        // AA 55 | 01 | 03 | 00 0E | 00 01 | 00 00 00 02 | 41 BC 00 00 | 42 CA 99 9A | 3F
        // 帧头    版本  类型  长度14  设备ID  序号2大端    温度23.5大端    压力101.3大端   校验
        allPassed &= expectBytes("数据上报帧（大端）", frame,
            {0xAA, 0x55, 0x01, 0x03, 0x00, 0x0E,
             0x00, 0x01,                       // 设备 ID = 1
             0x00, 0x00, 0x00, 0x02,           // 采样序号 = 2
             0x41, 0xBC, 0x00, 0x00,           // 23.5f 的 IEEE754 位模式
             0x42, 0xCA, 0x99, 0x9A,           // 101.3f 的 IEEE754 位模式
             0x3F});                           // 校验和
    }

    // --- 测试 2：控制指令帧 ---
    {
        auto frame = packCtrlCmd(/*cmd=*/1, /*param=*/2000);
        printHex("控制指令帧", frame);
        // AA 55 | 01 | 04 | 00 05 | 01 | 00 00 07 D0 | D8
        //                         指令码1  参数2000大端  校验
        allPassed &= expectBytes("控制指令帧（大端）", frame,
            {0xAA, 0x55, 0x01, 0x04, 0x00, 0x05,
             0x01,
             0x00, 0x00, 0x07, 0xD0,           // 2000 = 0x7D0
             0xD8});
    }

    // --- 测试 3：心跳帧（无负载）---
    {
        auto frame = packEmptyFrame(FrameType::HeartbeatReq);
        printHex("心跳请求帧", frame);
        // 无负载：长度 00 00，校验 00
        allPassed &= expectBytes("心跳请求帧", frame,
            {0xAA, 0x55, 0x01, 0x01, 0x00, 0x00, 0x00});
    }

    // --- 测试 4：校验和自动重算 ---
    {
        printf("   校验和抽查：");
        auto frame = packCtrlCmd(1, 2000);
        const uint8_t *payload = frame.data() + 6;      // 跳过 帧头2+版本1+类型1+长度2
        size_t payloadLen = frame.size() - HEADER_SIZE; // 总长 - 帧头7 = 负载长
        uint8_t recomputed = checksum(payload, payloadLen);
        if (recomputed == frame.back()) {
            printf("✅ 帧内校验 0x%02X 与重算一致\n", recomputed);
        } else {
            printf("❌ 帧内 0x%02X != 重算 0x%02X\n", frame.back(), recomputed);
            allPassed = false;
        }
    }

    printf("\n========== 结果：%s ==========\n",
           allPassed ? "全部通过 🎉" : "存在失败，请修复！");
    return allPassed ? 0 : 1;  // 返回码：0=成功，1=失败（CI 系统会看这个）
}
