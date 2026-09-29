// ============ 数据记录器：在独立工作线程写 CSV 文件 ============
// 定位：Worker Object（moveToThread 模式）——本对象的所有槽函数
//       会在工作线程执行，写磁盘不阻塞 UI。
// 线程间通信方式：主线程发信号投递数据（队列连接，值拷贝，无需加锁）。
#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

class DataLogger : public QObject
{
    Q_OBJECT

public:
    explicit DataLogger(QObject *parent = nullptr);

public slots:
    // 以下槽函数都通过队列连接在工作线程执行：

    // 开始记录：打开/创建 CSV 文件，写入表头
    void start(const QString &filePath);
    // 追加一条采样记录（主线程每秒/每几百毫秒投递一次）
    void appendRecord(quint32 sequence, float temperature, float pressure,
                      const QDateTime &time, bool alarm);
    // 停止记录：关闭文件
    void stop();

signals:
    // 结果信号（从工作线程发回主线程，同样是队列连接）
    void started(const QString &filePath);  // 文件已创建
    void errorOccurred(const QString &what); // 打开/写入失败
    void stopped(int totalRecords);          // 已停止，共写了几条

private:
    QFile m_file;          // 文件对象（只属于工作线程，无人争抢）
    QTextStream m_stream;  // 流式写入
    int m_count = 0;       // 已写入条数
};
