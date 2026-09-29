// ============ 设备模拟器入口 ============
// 注意：这里用的是 QCoreApplication 而不是 QApplication ——
// 模拟器 v1 没有界面，是纯"后台程序"。
// 教学点：Qt 不只做界面。无 GUI 的程序（服务、后台进程）用 QCoreApplication，
// 事件循环、信号槽、网络、定时器全部照常可用。
#include <QCoreApplication>
#ifdef Q_OS_WIN
#  include <windows.h>   // Windows API（SetConsoleOutputCP 在这）
#endif
#include "DeviceSimulator.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // 解决 Windows 控制台中文乱码：告诉控制台"本程序的输出是 UTF-8 编码"
    // （CP_UTF8 = 65001。不设置的话，CMD 默认按 GBK 解码我们的 UTF-8 输出 → 乱码）
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);

    DeviceSimulator simulator;
    if (!simulator.start(8899))   // 8899：我们协议约定好的端口号
        return 1;

    return app.exec();  // 进入事件循环：程序"停"在这里，靠信号驱动干活
}
