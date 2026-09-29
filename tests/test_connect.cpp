// ============ 验证小客户端：连上模拟器，收 5 帧数据后退出 ============
// 作用：验证模拟器真的在按协议发包。
// 这个程序其实就是"上位机客户端"的雏形——下一课我们把它发展成正式客户端。
#include <QCoreApplication>
#include <QTcpSocket>
#include <cstdio>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTcpSocket socket;

    // 连上时打个招呼
    QObject::connect(&socket, &QTcpSocket::connected, []() {
        printf("已连上设备模拟器，等待数据帧...\n\n");
    });

    // 有数据到达时（readyRead 信号）：读出来打印十六进制
    QObject::connect(&socket, &QTcpSocket::readyRead, [&]() {
        static int frameCount = 0;

        QByteArray data = socket.readAll();  // 把到的数据全读出来
        printf("收到 %d 字节:", int(data.size()));
        for (char c : data)
            printf("%02X ", static_cast<unsigned char>(c));
        printf("\n");

        // 每秒 1 帧、每帧 21 字节，收满 5 次就退出
        if (++frameCount >= 5) {
            printf("\n✅ 收到 5 帧，模拟器工作正常！\n");
            app.quit();
        }
    });

    // 主动拨号：连本机的 8899 端口
    socket.connectToHost("127.0.0.1", 8899);

    return app.exec();
}
