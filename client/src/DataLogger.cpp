// ============ 数据记录器实现 ============
// 注意：这个文件里的槽函数全部运行在工作线程！
// 面试点：为什么 m_file/m_stream 不需要加锁？
//   → 因为整个对象被 moveToThread 后，只有工作线程能碰它；
//     主线程只发信号（数据通过参数拷贝过来），从不直接访问它的成员。
//     这就是"线程封闭"（thread confinement）——比加锁更安全的设计。
#include "DataLogger.h"

DataLogger::DataLogger(QObject *parent)
    : QObject(parent)
{
}

void DataLogger::start(const QString &filePath)
{
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QString("无法创建文件 %1：%2")
                               .arg(filePath, m_file.errorString()));
        return;
    }

    m_stream.setDevice(&m_file);
    m_count = 0;

    // CSV 表头（逗号分隔，Excel 可直接打开）
    m_stream << "sequence,temperature,pressure,time,alarm\n";
    m_stream.flush();  // 表头立刻落盘

    emit started(filePath);
}

void DataLogger::appendRecord(quint32 sequence, float temperature,
                              float pressure, const QDateTime &time, bool alarm)
{
    if (!m_file.isOpen())
        return;  // 没在记录状态，丢弃

    // 一行 CSV：字段用逗号分隔。温度/压力保留 2 位小数。
    m_stream << sequence << ','
             << QString::number(double(temperature), 'f', 2) << ','
             << QString::number(double(pressure), 'f', 2) << ','
             << time.toString("yyyy-MM-dd HH:mm:ss") << ','
             << (alarm ? 1 : 0) << '\n';
    ++m_count;

    // 每 10 条 flush 一次：平衡"数据安全"与"磁盘效率"
    // （flush = 把缓冲区真正写进磁盘；太频繁伤性能，太稀疏断电丢数据）
    if (m_count % 10 == 0)
        m_stream.flush();
}

void DataLogger::stop()
{
    if (m_file.isOpen()) {
        m_stream.flush();  // 最后一批数据落盘
        m_file.close();
    }
    emit stopped(m_count);
}
