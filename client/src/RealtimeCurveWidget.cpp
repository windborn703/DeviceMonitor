// ============ 实时曲线控件实现 ============
#include "RealtimeCurveWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <algorithm>

RealtimeCurveWidget::RealtimeCurveWidget(QWidget *parent)
    : QWidget(parent)
{
    // 控件最小尺寸：太小时曲线没法看
    setMinimumSize(300, 180);
    // 深色工业风背景
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(18, 22, 28));  // 深灰蓝底
    setPalette(pal);
}

void RealtimeCurveWidget::addSample(float temperature, float pressure)
{
    // 1. 写入环形缓冲：写入位置 = (head + count) % N
    const int writePos = (m_head + m_count) % MAX_POINTS;
    m_tempBuffer[writePos] = temperature;
    m_pressBuffer[writePos] = pressure;

    // 2. 更新 head / count
    if (m_count == MAX_POINTS)
        m_head = (m_head + 1) % MAX_POINTS;  // 已满：头指针前进，覆盖最旧
    else
        ++m_count;                           // 未满：数量 +1

    // 3. 更新 Y 轴范围（自适应：取当前所有点的 min/max，上下留 10% 边距）
    m_minTemp = m_maxTemp = tempAt(0);
    m_minPress = m_maxPress = pressAt(0);
    for (int i = 1; i < m_count; ++i) {
        m_minTemp  = std::min(m_minTemp,  tempAt(i));
        m_maxTemp  = std::max(m_maxTemp,  tempAt(i));
        m_minPress = std::min(m_minPress, pressAt(i));
        m_maxPress = std::max(m_maxPress, pressAt(i));
    }
    const float padT = std::max(0.1f, (m_maxTemp - m_minTemp) * 0.1f);
    const float padP = std::max(0.1f, (m_maxPress - m_minPress) * 0.1f);
    m_minTemp -= padT;  m_maxTemp += padT;
    m_minPress -= padP; m_maxPress += padP;

    // 4. 请求重绘（合并机制：多次调用只重绘一次，界面不卡）
    update();
}

void RealtimeCurveWidget::clear()
{
    m_head = 0;
    m_count = 0;
    update();
}

void RealtimeCurveWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);  // 抗锯齿：曲线更平滑

    const int W = width();
    const int H = height();

    // ===== 1. 网格（水平 4 条 + 垂直 6 条虚线）=====
    painter.setPen(QPen(QColor(60, 70, 85), 1, Qt::DashLine));
    for (int i = 1; i < 5; ++i) {
        const int y = H * i / 5;
        painter.drawLine(0, y, W, y);
    }
    for (int i = 1; i < 6; ++i) {
        const int x = W * i / 6;
        painter.drawLine(x, 0, x, H);
    }

    if (m_count < 2)
        return;  // 少于 2 个点画不出线

    // ===== 2. 计算坐标映射 =====
    // 逻辑值（温度/压力）→ 屏幕像素坐标。i 从 0（最旧）到 m_count-1（最新）
    // X 轴策略（像心电监护仪）：
    //   - 最新点永远贴右边缘 → 曲线从右侧"生长"出来
    //   - 点数越多，旧点越靠左；满 MAX_POINTS 后整条曲线向左滚动
    const double step = double(W - 1) / (MAX_POINTS - 1);  // 相邻两点像素间距
    auto xOf = [&](int i) {
        return int((W - 1) - (m_count - 1 - i) * step + 0.5);
    };
    auto yTempOf = [&](float t) {
        return H - 1 - (t - m_minTemp) / (m_maxTemp - m_minTemp) * (H - 1);
    };
    auto yPressOf = [&](float p) {
        return H - 1 - (p - m_minPress) / (m_maxPress - m_minPress) * (H - 1);
    };

    // 环形缓冲的逻辑访问用成员函数 tempAt()/pressAt()（见头文件）

    // ===== 3. 温度曲线（红，左轴）=====
    QPainterPath tempPath;
    tempPath.moveTo(xOf(0), yTempOf(tempAt(0)));
    for (int i = 1; i < m_count; ++i)
        tempPath.lineTo(xOf(i), yTempOf(tempAt(i)));
    painter.setPen(QPen(QColor(255, 90, 90), 2));  // 红色 2 像素线
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(tempPath);

    // ===== 4. 压力曲线（蓝，右轴）=====
    QPainterPath pressPath;
    pressPath.moveTo(xOf(0), yPressOf(pressAt(0)));
    for (int i = 1; i < m_count; ++i)
        pressPath.lineTo(xOf(i), yPressOf(pressAt(i)));
    painter.setPen(QPen(QColor(80, 160, 255), 2));  // 蓝色 2 像素线
    painter.drawPath(pressPath);

    // ===== 5. 图例 + 最新值（右上角）=====
    const float lastTemp  = tempAt(m_count - 1);
    const float lastPress = pressAt(m_count - 1);

    painter.setPen(QColor(255, 90, 90));
    painter.drawText(10, 20, QString("温度 %1 ℃（左轴）").arg(lastTemp, 0, 'f', 2));

    painter.setPen(QColor(80, 160, 255));
    painter.drawText(10, 38, QString("压力 %1 kPa（右轴）").arg(lastPress, 0, 'f', 2));
}
