// ============ 界面层实现 ============
#include "MainWindow.h"
#include "DeviceClient.h"
#include "RealtimeCurveWidget.h"
#include "HistoryTableModel.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QStatusBar>
#include <QDateTime>
#include <QTableView>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include "protocol.h"   // 指令码常量 CTRL_CMD_SET_PERIOD

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("DeviceMonitor 设备监控上位机");

    buildUi();  // 先画界面

    // 创建通信层（界面是它的 parent：窗口关闭时自动销毁它）
    m_client = new DeviceClient(this);

    // --- 界面操作 → 通信层 ---
    connect(m_connectBtn, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    connect(m_disconnectBtn, &QPushButton::clicked,
            this, &MainWindow::onDisconnectClicked);
    connect(m_applyBtn, &QPushButton::clicked,
            this, &MainWindow::onApplyClicked);

    // --- 通信层的信号 → 界面响应（"收听广播"）---
    connect(m_client, &DeviceClient::connected,
            this, &MainWindow::onConnected);
    connect(m_client, &DeviceClient::disconnected,
            this, &MainWindow::onDisconnected);
    connect(m_client, &DeviceClient::connectionError,
            this, &MainWindow::onConnectionError);
    connect(m_client, &DeviceClient::dataReceived,
            this, &MainWindow::onDataReceived);
    connect(m_client, &DeviceClient::ctrlAckReceived,
            this, &MainWindow::onCtrlAckReceived);
    connect(m_client, &DeviceClient::protocolError,
            this, &MainWindow::onProtocolError);

    setConnectedState(false);  // 初始：未连接
    log("上位机已启动。请确认设备模拟器在运行，然后点击【连接】。");
}

void MainWindow::buildUi()
{
    // ===== 顶部：连接设置 =====
    auto *connectGroup = new QGroupBox("连接设置", this);
    m_hostEdit = new QLineEdit("127.0.0.1", connectGroup);
    m_portEdit = new QLineEdit("8899", connectGroup);
    m_portEdit->setMaximumWidth(80);
    m_connectBtn = new QPushButton("连接", connectGroup);
    m_disconnectBtn = new QPushButton("断开", connectGroup);

    auto *connectLayout = new QHBoxLayout(connectGroup);
    connectLayout->addWidget(new QLabel("设备 IP：", connectGroup));
    connectLayout->addWidget(m_hostEdit);
    connectLayout->addWidget(new QLabel("端口：", connectGroup));
    connectLayout->addWidget(m_portEdit);
    connectLayout->addSpacing(20);

    // 采样周期设置（下发指令用）
    m_periodSpin = new QSpinBox(connectGroup);
    // 注意：客户端范围（100~100000）比设备的合法范围（200~60000）宽 ——
    // 这是故意的"双重校验"设计：
    //   客户端 UI 只是输入便利（软约束），设备端校验才是安全底线（硬约束）。
    //   设备不信任任何客户端——哪怕客户端发来 100000，设备也要能拒绝。
    //   （如果两边范围一样，设备的拒绝逻辑永远没机会触发，等于白写）
    m_periodSpin->setRange(100, 100000);
    m_periodSpin->setValue(2000);         // 默认 2000ms，与模拟器初始一致
    m_periodSpin->setSingleStep(100);     // 步进 100ms
    m_periodSpin->setSuffix(" ms");       // 显示单位
    m_applyBtn = new QPushButton("应用", connectGroup);
    connectLayout->addWidget(new QLabel("采样周期：", connectGroup));
    connectLayout->addWidget(m_periodSpin);
    connectLayout->addWidget(m_applyBtn);

    connectLayout->addStretch();       // 弹性空白：把按钮推到右边
    connectLayout->addWidget(m_connectBtn);
    connectLayout->addWidget(m_disconnectBtn);

    // ===== 中部：实时曲线 =====
    auto *curveGroup = new QGroupBox("实时曲线", this);
    m_curveWidget = new RealtimeCurveWidget(curveGroup);
    auto *curveLayout = new QVBoxLayout(curveGroup);
    curveLayout->addWidget(m_curveWidget);

    // ===== 中部：实时数据 =====
    auto *dataGroup = new QGroupBox("实时数据", this);
    m_devIdValue = new QLabel("--", dataGroup);
    m_seqValue   = new QLabel("--", dataGroup);
    m_tempValue  = new QLabel("-- ℃", dataGroup);
    m_pressValue = new QLabel("-- kPa", dataGroup);
    m_statusLabel = new QLabel("● 未连接", dataGroup);

    // 报警阈值 + 报警状态
    m_tempHighSpin = new QDoubleSpinBox(dataGroup);
    m_tempHighSpin->setRange(-50.0, 150.0);
    m_tempHighSpin->setValue(35.0);
    m_tempHighSpin->setSuffix(" ℃");
    m_tempLowSpin = new QDoubleSpinBox(dataGroup);
    m_tempLowSpin->setRange(-50.0, 150.0);
    m_tempLowSpin->setValue(15.0);
    m_tempLowSpin->setSuffix(" ℃");
    m_alarmStatus = new QLabel("✓ 温度正常", dataGroup);
    m_alarmStatus->setStyleSheet("color: green;");

    // 温压显示用大字（24 号）更醒目
    QFont bigFont = m_tempValue->font();
    bigFont.setPointSize(24);
    m_tempValue->setFont(bigFont);
    m_pressValue->setFont(bigFont);

    // QFormLayout：标签-值 成对排列的表单布局
    auto *dataLayout = new QFormLayout(dataGroup);
    dataLayout->addRow("设备 ID：", m_devIdValue);
    dataLayout->addRow("采样序号：", m_seqValue);
    dataLayout->addRow("温度：", m_tempValue);
    dataLayout->addRow("压力：", m_pressValue);
    dataLayout->addRow("状态：", m_statusLabel);
    dataLayout->addRow("温度上限：", m_tempHighSpin);
    dataLayout->addRow("温度下限：", m_tempLowSpin);
    dataLayout->addRow("报警：", m_alarmStatus);

    // ===== 中部：历史数据表格（Model/View）=====
    auto *historyGroup = new QGroupBox("历史数据（最近 100 条，超限标红）", this);
    m_historyModel = new HistoryTableModel(historyGroup);   // Model：持有数据
    m_tableView = new QTableView(historyGroup);             // View：只负责显示
    m_tableView->setModel(m_historyModel);                  // 关键一行：视图挂上模型
    m_tableView->setEditTriggers(QTableView::NoEditTriggers);  // 只读
    m_tableView->setSelectionBehavior(QTableView::SelectRows); // 整行选中
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    auto *historyLayout = new QVBoxLayout(historyGroup);
    historyLayout->addWidget(m_tableView);

    // ===== 底部：日志 =====
    auto *logGroup = new QGroupBox("通信日志", this);
    m_logEdit = new QTextEdit(logGroup);
    m_logEdit->setReadOnly(true);              // 日志只读，用户不能编辑
    auto *logLayout = new QVBoxLayout(logGroup);
    logLayout->addWidget(m_logEdit);

    // ===== 组装：垂直主布局 =====
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->addWidget(connectGroup);
    mainLayout->addWidget(curveGroup, /*stretch=*/2);  // 曲线区占大头
    mainLayout->addWidget(dataGroup);
    mainLayout->addWidget(historyGroup, /*stretch=*/2);  // 历史表格
    mainLayout->addWidget(logGroup, /*stretch=*/1);      // 日志区占剩余空间
    setCentralWidget(central);

    resize(760, 900);
    statusBar()->showMessage("就绪");
}

void MainWindow::log(const QString &msg, const QString &color)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    // 用 HTML 上色（QTextEdit 支持简单 HTML）
    m_logEdit->append(QString("<span style='color:%1'>[%2] %3</span>")
                          .arg(color, time, msg.toHtmlEscaped()));
}

void MainWindow::setConnectedState(bool connected)
{
    m_hostEdit->setEnabled(!connected);      // 已连接时禁止改地址
    m_portEdit->setEnabled(!connected);
    m_connectBtn->setEnabled(!connected);
    m_disconnectBtn->setEnabled(connected);
    m_periodSpin->setEnabled(connected);     // 采样周期设置只在连接时可用
    m_applyBtn->setEnabled(connected);
}

// ============ 用户操作 ============

void MainWindow::onConnectClicked()
{
    bool ok = false;
    const quint16 port = m_portEdit->text().toUShort(&ok);
    if (!ok || port == 0) {
        log("端口无效，请输入 1-65535 之间的数字。", "red");
        return;
    }
    log(QString("正在连接 %1:%2 ...")
            .arg(m_hostEdit->text()).arg(port), "gray");
    m_client->connectToDevice(m_hostEdit->text(), port);
}

void MainWindow::onDisconnectClicked()
{
    m_client->disconnectFromDevice();
}

void MainWindow::onApplyClicked()
{
    const int periodMs = m_periodSpin->value();
    log(QString("下发指令：采样周期改为 %1 ms，等待设备确认...").arg(periodMs), "gray");
    m_client->sendCtrlCmd(protocol::CTRL_CMD_SET_PERIOD, quint32(periodMs));
}

// ============ 通信层的信号响应 ============

void MainWindow::onConnected()
{
    setConnectedState(true);
    m_statusLabel->setText("● 已连接");
    m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
    statusBar()->showMessage("已连接");
    log("连接成功，等待设备数据...", "green");
}

void MainWindow::onDisconnected()
{
    setConnectedState(false);
    m_statusLabel->setText("● 未连接");
    m_statusLabel->setStyleSheet("color: red;");
    m_devIdValue->setText("--");
    m_seqValue->setText("--");
    m_tempValue->setText("-- ℃");
    m_pressValue->setText("-- kPa");
    m_curveWidget->clear();       // 曲线清空，下次连接重新画
    m_historyModel->clear();      // 历史表格清空
    m_alarmCount = 0;             // 报警计数复位
    m_alarmStatus->setText("✓ 温度正常");
    m_alarmStatus->setStyleSheet("color: green;");
    statusBar()->showMessage("已断开");
    log("连接已断开。", "orange");
}

void MainWindow::onConnectionError(const QString &what)
{
    log(QString("连接失败：%1").arg(what), "red");
}

void MainWindow::onDataReceived(quint16 deviceId, quint32 sequence,
                                float temperature, float pressure)
{
    m_curveWidget->addSample(temperature, pressure);  // 曲线加一个点（内部会请求重绘）
    m_devIdValue->setText(QString::number(deviceId));
    m_seqValue->setText(QString::number(sequence));
    m_tempValue->setText(QString("%1 ℃").arg(temperature, 0, 'f', 2));
    m_pressValue->setText(QString("%1 kPa").arg(pressure, 0, 'f', 2));

    // ---- 超限报警判断 ----
    const bool alarm = temperature > m_tempHighSpin->value()
                    || temperature < m_tempLowSpin->value();
    if (alarm) {
        ++m_alarmCount;
        m_alarmStatus->setText(QString("⚠ 超温报警（累计 %1 次）").arg(m_alarmCount));
        m_alarmStatus->setStyleSheet("color: red; font-weight: bold;");
        log(QString("🚨 温度 %.2f ℃ 超出阈值范围 [%.1f, %.1f]！")
                .arg(double(temperature))
                .arg(m_tempLowSpin->value())
                .arg(m_tempHighSpin->value()), "red");
    } else {
        m_alarmStatus->setText("✓ 温度正常");
        m_alarmStatus->setStyleSheet("color: green;");
    }

    // ---- 写入历史表格（超限行会标红）----
    m_historyModel->addRecord(sequence, temperature, pressure,
                              QDateTime::currentDateTime(), alarm);

    log(QString("采样 #%1：温度 %2 ℃，压力 %3 kPa")
            .arg(sequence)
            .arg(temperature, 0, 'f', 2)
            .arg(pressure, 0, 'f', 2));
}

void MainWindow::onProtocolError(const QString &reason)
{
    log(QString("协议告警：%1").arg(reason), "orange");
}

void MainWindow::onCtrlAckReceived(quint8 cmd, quint8 result)
{
    if (cmd != protocol::CTRL_CMD_SET_PERIOD)
        return;
    if (result == protocol::CTRL_RESULT_OK)
        log("✅ 设备确认：采样周期已更新，观察数据刷新节奏的变化", "green");
    else
        log("❌ 设备拒绝了指令（参数可能超范围）", "red");
}
