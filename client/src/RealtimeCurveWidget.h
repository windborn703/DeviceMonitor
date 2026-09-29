// ============ 实时曲线控件：自绘工业风监控曲线 ============
// 自定义控件 = 继承 QWidget + 重写 paintEvent + 用 QPainter 画。
// 特性：
//   - 双曲线：温度（红，左轴）+ 压力（蓝，右轴）
//   - 环形缓冲：只保留最近 MAX_POINTS 个点，内存固定
//   - 深色工业风：像真实监控屏
#pragma once

#include <QWidget>
#include <QVector>

class RealtimeCurveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RealtimeCurveWidget(QWidget *parent = nullptr);

    // 喂入一个新采样点（曲线自动右移）
    void addSample(float temperature, float pressure);
    // 清空曲线
    void clear();

protected:
    // 重绘入口：系统每次需要重绘窗口时调用（显示、遮挡恢复、update() 请求）
    void paintEvent(QPaintEvent *event) override;

private:
    // ---------- 环形缓冲（教科书式实现）----------
    // head = "最旧数据的下标"（不是下一个写入位置！）
    //   写入位置 = (head + count) % MAX_POINTS
    //   逻辑上第 i 个点（0=最旧）= 数组[(head + i) % MAX_POINTS]
    //   这个公式在"未满"和"已满"两种状态下都成立 —— 这是环形缓冲
    //   最容易写错的地方（经典面试题：手写环形缓冲）
    static constexpr int MAX_POINTS = 300;   // 保留最近 300 个点
    float m_tempBuffer[MAX_POINTS] = {};     // 温度环形缓冲
    float m_pressBuffer[MAX_POINTS] = {};    // 压力环形缓冲
    int m_head = 0;                          // 最旧数据的下标
    int m_count = 0;                         // 当前实际点数（≤ MAX_POINTS）

    // 按逻辑序号取值（0 = 最旧）
    float tempAt(int i) const  { return m_tempBuffer[(m_head + i) % MAX_POINTS]; }
    float pressAt(int i) const { return m_pressBuffer[(m_head + i) % MAX_POINTS]; }

    // ---------- 显示范围（Y 轴自适应）----------
    float m_minTemp = 0.0f, m_maxTemp = 0.0f;
    float m_minPress = 0.0f, m_maxPress = 0.0f;
};
