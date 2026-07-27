#include "metriccardswidget.h"
#include "uiutils.h"

#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>

MetricCardsWidget::MetricCardsWidget(QWidget *parent)
    : QWidget{parent}
{
    QHBoxLayout *row = new QHBoxLayout(this);

    // TODO: replace hardcoded values with live server DataPayload.
    row->addWidget(createMetricCard("cpu", "CPU overall", "34%", "4 cores"));
    row->addWidget(createMetricCard("memory", "Memory used", "5.2 GB", "of 16 GB · swap 0.1"));
    row->addWidget(createMetricCard("disk", "Disk read", "12 MB/s", "write 4 MB/s"));
    row->addWidget(createMetricCard("network", "Network rx", "820 KB/s", "tx 210 KB/s"));
}


void MetricCardsWidget::updateMetric(const QString &key, const QString &value)
{
    if (m_metricValues.contains(key))
        m_metricValues[key]->setText(value);
}


QWidget *MetricCardsWidget::createMetricCard(const QString &key, const QString &title, const QString &value, const QString &subtitle)
{
    QFrame *card = new QFrame;
    card->setObjectName("metricCard");
    QLabel *valueLabel = makeLabel(value, "metricValue");
    m_metricValues.insert(key, valueLabel);
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->addWidget(makeLabel(title, "metricTitle"));
    layout->addWidget(valueLabel);
    layout->addWidget(makeLabel(subtitle, "metricSubtitle"));

    return card;
}
