// ============ 界面层：上位机主窗口 ============
// 职责：画界面、响应用户操作、显示数据。
// 注意它不直接碰 QTcpSocket —— 通信全部通过 DeviceClient 的信号。
#pragma once

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QTextEdit;
class QTableView;
class DeviceClient;
class RealtimeCurveWidget;
class HistoryTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // --- 用户操作 ---
    void onConnectClicked();     // 点了"连接"
    void onDisconnectClicked();  // 点了"断开"
    void onApplyClicked();       // 点了"应用"（下发采样周期指令）

    // --- DeviceClient 的信号（通信层的"广播"，界面负责响应）---
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString &what);
    void onDataReceived(quint16 deviceId, quint32 sequence,
                        float temperature, float pressure);
    void onCtrlAckReceived(quint8 cmd, quint8 result);
    void onProtocolError(const QString &reason);

private:
    void buildUi();                        // 创建控件和布局
    void log(const QString &msg, const QString &color = "black");  // 写日志
    void setConnectedState(bool connected);  // 切换按钮/输入框的可用状态

    // --- 控件（指针成员，析构由 Qt 父子对象树自动处理）---
    QLineEdit   *m_hostEdit    = nullptr;  // IP 输入框
    QLineEdit   *m_portEdit    = nullptr;  // 端口输入框
    QPushButton *m_connectBtn  = nullptr;  // 连接按钮
    QPushButton *m_disconnectBtn = nullptr; // 断开按钮
    QSpinBox    *m_periodSpin  = nullptr;  // 采样周期设置（毫秒）
    QPushButton *m_applyBtn    = nullptr;  // 应用按钮（下发指令）
    QLabel      *m_devIdValue  = nullptr;  // 设备 ID 显示
    QLabel      *m_seqValue    = nullptr;  // 采样序号显示
    QLabel      *m_tempValue   = nullptr;  // 温度显示（大字）
    QLabel      *m_pressValue  = nullptr;  // 压力显示（大字）
    QLabel      *m_statusLabel = nullptr;  // 连接状态灯
    QTextEdit   *m_logEdit     = nullptr;  // 日志区
    RealtimeCurveWidget *m_curveWidget = nullptr;  // 实时曲线

    // --- 历史数据表格（Model/View）---
    QTableView        *m_tableView    = nullptr;  // View：只负责显示
    HistoryTableModel *m_historyModel = nullptr;  // Model：数据都在这里

    // --- 报警 ---
    QDoubleSpinBox *m_tempHighSpin = nullptr;  // 温度上限阈值
    QDoubleSpinBox *m_tempLowSpin  = nullptr;  // 温度下限阈值
    QLabel         *m_alarmStatus  = nullptr;  // 报警状态标签
    int m_alarmCount = 0;                      // 累计报警次数

    // --- 通信层 ---
    DeviceClient *m_client = nullptr;
};
