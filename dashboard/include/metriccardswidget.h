#ifndef METRICCARDSWIDGET_H
#define METRICCARDSWIDGET_H

#include <QWidget>
#include <QString>
#include <QHash>


class QLabel;

class MetricCardsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MetricCardsWidget(QWidget *parent = nullptr);

    /// Updates a metric card value by key. TODO: call from server DataPayload.
    void updateMetric(const QString &key, const QString &value);


private:


    /// Creates one metric card (title + big value + subtitle).
    QWidget *createMetricCard(const QString &key, const QString &title, const QString &value, const QString &subtitle);

    QHash<QString, QLabel *> m_metricValues;


};

#endif // METRICCARDSWIDGET_H
