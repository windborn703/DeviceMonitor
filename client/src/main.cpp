// ============ 上位机客户端 - 第一个程序 ============
// 今天的目标：验证整个工具链能跑通，弹出一个窗口

#include <QApplication>  // Qt 程序的"总管家"
#include <QLabel>        // 文字标签控件

int main(int argc, char *argv[])
{
    // 第 1 步：创建 QApplication 对象
    // 它是整个 Qt 程序的"总管家"，负责管理窗口、事件、资源
    // 任何 Qt GUI 程序的第一行几乎都是它，必须在所有窗口之前创建
    QApplication app(argc, argv);

    // 第 2 步：创建一个窗口上的控件
    // QLabel 是显示文字的标签，这里是"Hello World"
    QLabel label("设备监控系统启动成功！");
    label.resize(400, 200);  // 设置窗口大小：宽 400 像素，高 200 像素
    label.show();            // 显示出来

    // 第 3 步：进入事件循环
    // 程序执行到这里会"停住"，开始不断等待并处理用户操作
    // （点鼠标、敲键盘、点关闭按钮...）——这就是 Qt 的事件循环机制
    // 直到用户关闭所有窗口，exec() 才返回，程序结束
    return app.exec();
}
