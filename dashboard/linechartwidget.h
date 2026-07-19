#ifndef LINECHARTWIDGET_H
#define LINECHARTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>

class LineChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineChartWidget(QWidget *parent = nullptr);
    void setData(const QVector<double> &data);
    void setTitle(const QString &title);
    void setSeries(const QVector<QVector<double>> &series);
    void setLabels(const QStringList &labels);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_data; // a list of number -> one one ligne
    QVector<QVector<double>> m_series;   // one inner vector per curve
    QString m_title;
    QStringList m_labels;   // one name per series, for the legend

};

#endif // LINECHARTWIDGET_H
