// ============ FrameParser 单元测试：粘包/半包场景全覆盖 ============
// 面试时被问"你测试过粘包吗"——这个文件就是答案：
// 六个场景模拟了真实网络中字节流的各种到达方式。
#include <QCoreApplication>
#include "FrameParser.h"
#include "protocol.h"
#include "console_utf8.h"

static int g_received = 0;   // 成功解析的数据上报帧数
static int g_errors   = 0;   // 解析异常次数
static int g_failed   = 0;   // 断言失败次数

// 断言：条件为假时报错
static void check(bool cond, const char *name)
{
    if (cond) {
        printUtf8("  ✅ %s\n", name);
    } else {
        printUtf8("  ❌ %s —— 断言失败！\n", name);
        ++g_failed;
    }
}

// 把 std::vector 转成 QByteArray（测试造数据用）
static QByteArray toBytes(const std::vector<uint8_t> &v)
{
    return QByteArray(reinterpret_cast<const char *>(v.data()), int(v.size()));
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    protocol::FrameParser parser;

    QObject::connect(&parser, &protocol::FrameParser::dataReportReceived,
        [](quint16 id, quint32 seq, float t, float p) {
            ++g_received;
            printUtf8("    解析出帧：设备 %u #%u | 温度 %.2f ℃ | 压力 %.2f kPa\n",
                      id, seq, double(t), double(p));
        });
    QObject::connect(&parser, &protocol::FrameParser::parseError,
        [](const QString &r) {
            ++g_errors;
            printUtf8("    （异常）%s\n", qPrintable(r));
        });

    printUtf8("========== FrameParser 粘包拆包测试 ==========\n\n");

    // --- 场景 1：正常情况，一次完整一帧 ---
    printUtf8("[场景1] 单帧完整到达\n");
    {
        g_received = 0;
        parser.appendData(toBytes(protocol::packDataReport(1, 1, 25.0f, 101.0f)));
        check(g_received == 1, "解析出 1 帧");
        check(parser.bufferedBytes() == 0, "缓冲区无残留");
    }

    // --- 场景 2：粘包，两帧一次到达 ---
    printUtf8("\n[场景2] 两帧粘在一起一次到达\n");
    {
        g_received = 0;
        QByteArray glued;
        glued += toBytes(protocol::packDataReport(1, 2, 25.1f, 101.1f));
        glued += toBytes(protocol::packDataReport(1, 3, 25.2f, 101.2f));
        parser.appendData(glued);
        check(g_received == 2, "解析出 2 帧");
        check(parser.bufferedBytes() == 0, "缓冲区无残留");
    }

    // --- 场景 3：半包，一帧分两次到达（拦腰截断）---
    printUtf8("\n[场景3] 一帧被截成两半分两次到达\n");
    {
        g_received = 0;
        auto frame = toBytes(protocol::packDataReport(1, 4, 25.3f, 101.3f));
        parser.appendData(frame.left(10));   // 前 10 字节先到
        check(g_received == 0, "半帧时不应解析出帧");
        check(parser.bufferedBytes() == 10, "10 字节残留在缓冲区");
        parser.appendData(frame.mid(10));    // 后 11 字节补上
        check(g_received == 1, "补全后解析出 1 帧");
        check(parser.bufferedBytes() == 0, "缓冲区清空");
    }

    // --- 场景 4：帧头前有垃圾字节 ---
    printUtf8("\n[场景4] 帧前混入垃圾字节（干扰数据）\n");
    {
        g_received = 0;
        QByteArray dirty;
        dirty += QByteArray::fromHex("13 37 DE AD");   // 4 字节干扰
        dirty += toBytes(protocol::packDataReport(1, 5, 25.4f, 101.4f));
        parser.appendData(dirty);
        check(g_received == 1, "跳过垃圾找到 1 帧");
    }

    // --- 场景 5：校验错误的坏帧（负载被篡改一个字节）---
    printUtf8("\n[场景5] 校验失败的坏帧应被丢弃\n");
    {
        g_received = 0;
        g_errors = 0;
        auto bad = toBytes(protocol::packDataReport(1, 6, 25.5f, 101.5f));
        bad[12] = bad[12] ^ 0xFF;   // 篡改负载中间一个字节 → 校验必失败
        parser.appendData(bad);
        parser.appendData(toBytes(protocol::packDataReport(1, 7, 25.6f, 101.6f))); // 后面跟个好帧
        check(g_errors >= 1, "触发了 parseError");
        check(g_received == 1, "坏帧丢弃、好帧正常解析");
    }

    // --- 场景 6：负载里恰好出现 AA 55（不能误判成帧头）---
    printUtf8("\n[场景6] 负载数据里恰好包含 AA 55\n");
    {
        g_received = 0;
        // 采样序号 0xAA550000 大端写进负载 = 负载里出现字节 AA 55 00 00
        parser.appendData(toBytes(protocol::packDataReport(1, 0xAA550000u, 26.0f, 102.0f)));
        check(g_received == 1, "按长度取整帧，负载里的 AA 55 不误判");
        check(parser.bufferedBytes() == 0, "缓冲区无残留");
    }

    printUtf8("\n========== 结果：%s ==========\n",
              g_failed == 0 ? "全部通过 🎉" : "存在失败！");
    return g_failed == 0 ? 0 : 1;
}
