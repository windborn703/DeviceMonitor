// ============ 验证小客户端：连上模拟器，收 5 帧数据后退出 ============
// 作用：验证模拟器真的在按协议发包。
// 这个程序其实就是"上位机客户端"的雏形——下一课我们把它发展成正式客户端。
#include <QCoreApplication>
#include <QTcpSocket>
#include "console_utf8.h"  // printUtf8：中文输出不乱码
#include "protocol.h"       // 协议拆包函数（把字节流翻译成数值）

int main(int argc, char *argv[])
{
    consoleInitUtf8();  // 初始化控制台代码页（双保险之一）

    QCoreApplication app(argc, argv);
    QTcpSocket socket;

    // 连上时打个招呼
    QObject::connect(&socket, &QTcpSocket::connected, []() {
        printUtf8("已连上设备模拟器，等待数据帧...\n\n");
    });

    // 有数据到达时（readyRead 信号）：读出来 + 翻译成人类可读的值
    QObject::connect(&socket, &QTcpSocket::readyRead, [&]() {
        static int frameCount = 0;

        QByteArray data = socket.readAll();  // 把到的数据全读出来
        printUtf8("收到 %d 字节:", int(data.size()));
        for (char c : data)
            printUtf8("%02X ", static_cast<unsigned char>(c));
        printUtf8("\n");

        // --- 拆包：把二进制帧翻译成人类可读的值 ---
        // 帧布局：帧头2 + 版本1 + 类型1 + 长度2 = 前 6 字节，负载从 data[6] 开始
        uint16_t deviceId = 0;
        uint32_t sequence = 0;
        float temperature = 0.0f, pressure = 0.0f;
        if (protocol::parseDataReportPayload(
                reinterpret_cast<const uint8_t *>(data.constData()) + 6,
                size_t(data.size() - 7),  // 总长 - 帧头6 - 校验1 = 负载长
                deviceId, sequence, temperature, pressure)) {
            printUtf8("   → 设备 %u 第 %u 次采样：温度 %.2f ℃，压力 %.2f kPa\n\n",
                      deviceId, sequence, double(temperature), double(pressure));
        }

        // 模拟器每 2 秒 1 帧、每帧 21 字节，收满 5 次就退出
        if (++frameCount >= 5) {
            printUtf8("\n✅ 收到 5 帧，模拟器工作正常！\n");
            app.quit();
        }
    });

    // 主动拨号：连本机的 8899 端口
    socket.connectToHost("127.0.0.1", 8899);

    return app.exec();
}
