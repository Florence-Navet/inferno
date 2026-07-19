#include "linechartwidget.h"

#include <QPainter>

LineChartWidget::LineChartWidget(QWidget *parent)
    : QWidget(parent)
{
}

void LineChartWidget::setData(const QVector<double> &data)
{
    m_data = data;
    update();
}

void LineChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);

    QRectF card = rect().adjusted(0, 0, -1, -1);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRoundedRect(card, 12, 12);

    // if no data no graph
    if (m_data.isEmpty())
        return;

    double w = width();
    double h = height();

    // TODO: make the Y scale configurable per chart (0-100 for CPU %,
    // but GB for memory, MB/s for I/O). Hardcoded to percentages for now.
    double minVal = 0.0;
    double maxVal = 100.0;

    QPolygonF points;
    for (int i = 0; i < m_data.size(); ++i) {
        double x = i * (w / (m_data.size() - 1));
        double y = h - (m_data[i] - minVal) / (maxVal - minVal) * h; // in pixels (*h)
        points << QPointF(x, y); // store the point
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#89B6A5"), 2));
    painter.drawPolyline(points);
}
