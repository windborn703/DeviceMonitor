// ============ 上位机客户端入口 ============
// 有界面 → 用 QApplication（对比：模拟器无界面用 QCoreApplication）
#include <QApplication>
#include "console_utf8.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    consoleInitUtf8();

    QApplication app(argc, argv);

    MainWindow window;
    window.show();   // 显示主窗口

    return app.exec();  // 事件循环：UI 的"心跳"
}
