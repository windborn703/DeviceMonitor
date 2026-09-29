// ============ 历史数据表格模型（Model/View 架构的 Model）============
// 职责：存储最近 MAX_RECORDS 条采样记录，回答视图的提问。
// 它不知道也不关心谁来显示自己（可以同时被表格、图表、打印模块使用）。
// 面试必背：Model/View 比 QTableWidget 好在哪？
//   → 数据与显示分离、一份数据多视图、大数据量性能、代码可测试
#pragma once

#include <QAbstractTableModel>
#include <QDateTime>
#include <deque>

class HistoryTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    // 列定义（面试时讲表格结构就说这个）
    enum Column {
        ColSequence = 0,   // 采样序号
        ColTemperature,    // 温度 ℃
        ColPressure,       // 压力 kPa
        ColTime,           // 时间
        ColumnCount        // 列数（哨兵值）
    };

    explicit HistoryTableModel(QObject *parent = nullptr);

    // ---- QAbstractTableModel 必须实现的 4 个纯虚函数 ----
    // View 通过它们"提问"：几行？几列？某格显示什么？表头是什么？
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // ---- 业务接口 ----
    // 添加一条采样记录（内部自动处理"满 100 条挤掉最旧"）
    void addRecord(quint32 sequence, float temperature, float pressure,
                   const QDateTime &time, bool alarm = false);
    void clear();

private:
    struct Record {
        quint32 sequence;
        float temperature;
        float pressure;
        QDateTime time;
        bool alarm;  // 是否超限（表格里标红）
    };

    static constexpr int MAX_RECORDS = 100;  // 只保留最近 100 条
    std::deque<Record> m_records;  // deque：尾部插入、头部弹出都很高效
};
