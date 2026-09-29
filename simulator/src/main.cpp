// ============ 设备模拟器入口 ============
// 注意：这里用的是 QCoreApplication 而不是 QApplication ——
// 模拟器 v1 没有界面，是纯"后台程序"。
// 教学点：Qt 不只做界面。无 GUI 的程序（服务、后台进程）用 QCoreApplication，
// 事件循环、信号槽、网络、定时器全部照常可用。
#include <QCoreApplication>
#include "console_utf8.h"     // consoleInitUtf8：中文输出不乱码
#include "DeviceSimulator.h"

int main(int argc, char *argv[])
{
    consoleInitUtf8();  // 控制台代码页设为 UTF-8（双保险之一）

    QCoreApplication app(argc, argv);

    DeviceSimulator simulator;
    if (!simulator.start(8899))   // 8899：我们协议约定好的端口号
        return 1;

    return app.exec();  // 进入事件循环：程序"停"在这里，靠信号驱动干活
}
