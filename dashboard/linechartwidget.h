#ifndef LINECHARTWIDGET_H
#define LINECHARTWIDGET_H

#include <QWidget>
#include <QVector>

class LineChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineChartWidget(QWidget *parent = nullptr);
    void setData(const QVector<double> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_data;
};

#endif // LINECHARTWIDGET_H
