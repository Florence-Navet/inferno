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

    const double pad = 30.0;
    const double leftPad = 44.0;
    double plotLeft = leftPad;
    double plotTop = pad;
    double plotWidth = w - leftPad - pad;
    double plotHeight = h - 2 * pad;

    // TODO: make the Y scale configurable per chart (0-100 for CPU %,
    // but GB for memory, MB/s for I/O). Hardcoded to percentages for now.
    double minVal = 0.0;
    double maxVal = 100.0;

    //hache et graduation

    for (int i = 0; i <= 2; ++i) {
        double y = plotTop + (plotHeight / 2) * i;

        painter.setPen(QPen(QColor(229, 229, 229), 1));
        painter.drawLine(QPointF(plotLeft, y), QPointF(plotLeft + plotWidth, y));

        int percent = 100 - i * 50;
        painter.setPen(QColor(120, 120, 120));
        painter.drawText(QRectF(0, y - 8, plotLeft - 4, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(percent) + "%");
    }

    QPolygonF points;
    for (int i = 0; i < m_data.size(); ++i) {
        double x = plotLeft + i * (plotWidth / (m_data.size() - 1));
        double y = plotTop + plotHeight - (m_data[i] - minVal) / (maxVal - minVal) * plotHeight; // in pixels (*h)
        points << QPointF(x, y); // store the point
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor("#89B6A5"), 2));
    painter.drawPolyline(points);
}
