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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_data;
    QString m_title;

};

#endif // LINECHARTWIDGET_H
