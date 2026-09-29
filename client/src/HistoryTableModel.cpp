// ============ 历史数据表格模型实现 ============
#include "HistoryTableModel.h"

#include <QBrush>
#include <QColor>

HistoryTableModel::HistoryTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int HistoryTableModel::rowCount(const QModelIndex &parent) const
{
    // parent 无效时返回顶层行数；本模型没有树形层级，parent 一律忽略
    return parent.isValid() ? 0 : int(m_records.size());
}

int HistoryTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant HistoryTableModel::data(const QModelIndex &index, int role) const
{
    // 越界保护：View 可能问出界的位置
    if (!index.isValid() || index.row() < 0 || index.row() >= int(m_records.size()))
        return {};

    const Record &rec = m_records[size_t(index.row())];

    // ---- 按"角色"回答不同的提问 ----
    // DisplayRole：显示的文字（View 问"这格显示什么"）
    // BackgroundRole：背景色（我们用来给超限行标红 —— 经典用法）
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColSequence:    return rec.sequence;
        case ColTemperature: return QString::number(double(rec.temperature), 'f', 2);
        case ColPressure:    return QString::number(double(rec.pressure), 'f', 2);
        case ColTime:        return rec.time.toString("HH:mm:ss");
        }
    } else if (role == Qt::BackgroundRole && rec.alarm) {
        return QBrush(QColor(255, 80, 80, 60));  // 半透明红：超限行醒目但不刺眼
    } else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;  // 居中显示更整齐
    }
    return {};
}

QVariant HistoryTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    if (orientation == Qt::Horizontal) {
        switch (section) {
        case ColSequence:    return QStringLiteral("采样序号");
        case ColTemperature: return QStringLiteral("温度 (℃)");
        case ColPressure:    return QStringLiteral("压力 (kPa)");
        case ColTime:        return QStringLiteral("时间");
        }
    }
    return {};
}

void HistoryTableModel::addRecord(quint32 sequence, float temperature,
                                  float pressure, const QDateTime &time, bool alarm)
{
    // ---- 插入新行 ----
    // 必须成对调用 begin/endInsertRows —— 这是通知 View 的关键机制：
    // View 收到通知后才会刷新显示、滚动条等。
    const int newRow = int(m_records.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_records.push_back({sequence, temperature, pressure, time, alarm});
    endInsertRows();

    // ---- 挤掉最旧（满 100 条后）----
    if (int(m_records.size()) > MAX_RECORDS) {
        beginRemoveRows(QModelIndex(), 0, 0);   // 通知：第 0 行要没了
        m_records.pop_front();
        endRemoveRows();
    }
}

void HistoryTableModel::clear()
{
    if (m_records.empty())
        return;
    beginRemoveRows(QModelIndex(), 0, int(m_records.size()) - 1);
    m_records.clear();
    endRemoveRows();
}
