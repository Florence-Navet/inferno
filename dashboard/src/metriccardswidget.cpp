#include "metriccardswidget.h"
#include "uiutils.h"

#include <QLabel>
#include <QString>
#include <QFrame>
#include <QDebug>
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

void MetricCardsWidget::updateFromSample(const MetricsSample &sample)
{
    updateMetric("cpu", QString::number(sample.cpu.total_percent, 'f', 1) + "%");

    const double gb = sample.mem.phys_used / 1024.0 / 1024.0 / 1024.0;
    updateMetric("memory", QString::number(gb, 'f', 1) + " GB");

    double diskRead = 0.0;
    for (const DiskSample &disk : sample.disks) diskRead += disk.read_bytes_per_sec;

    diskRead = diskRead / 1024.0 / 1024.0;
    updateMetric("disk", QString::number(diskRead, 'f', 1) + " MB/s");

    double netRx = 0.0;
    for (const NetSample &iface : sample.interfaces) netRx += iface.rx_bytes_per_sec;

    if (netRx > 1024.0 * 1024.0 * 1024.0) {
        qDebug() << "network rx out of range, sample skipped:" << netRx;
    } else {
        netRx = netRx / 1024.0;
        updateMetric("network", QString::number(netRx, 'f', 1) + " KB/s");
    }
}
