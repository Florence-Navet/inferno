#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "agentitemwidget.h"

#include <QListWidgetItem>
#include <QVector>

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    for (QPushButton *button : findChildren<QPushButton *>())
        button->setCursor(Qt::PointingHandCursor);

    populateAgents();

    buildContentArea();



    qDebug() << "target:" << m_target;

    connect(ui->runButton, &QPushButton::clicked, this, [this]() {
            qDebug() << "target:" << m_target;
        showOutput(ui->commandEdit->text());
        });

    const QVector<QPair<QPushButton *, QString>> presets = {
        { ui->presetDfButton, "df -h" },
        { ui->presetNetstatButton, "netstat -tulpn" },
        { ui->presetTopButton, "top -bn1" }
    };
    for (const QPair<QPushButton *, QString> &preset : presets) {
        connect(preset.first, &QPushButton::clicked, this, [this, preset]() {
            ui->commandEdit->setText(preset.second);
        });
    }

    connect(ui->agentList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *current, QListWidgetItem *) {
                m_target = current ? current->data(Qt::UserRole).toString() : QString();
            });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateAgents()
{
    // TODO: replace with RegisterPayload list from DataType::AGENTS
    addAgentItem("agent1-desktop", "Windows · x64 · 192.168.1.10", true);
    addAgentItem("agent2-srv", "Linux · x64 · 192.168.1.42", true);
    addAgentItem("agent3-lab", "Windows · x86 · 192.168.1.55", true);
    addAgentItem("agent4-pi", "Linux · ARM · 192.168.1.80", true);
    addAgentItem("agent5-mac", "macOS · x64 · offline", false);

    ui->agentList->setCurrentRow(-1);
}

void MainWindow::buildContentArea()
{
    // TODO: build metric cards, process table
    QHBoxLayout *metricsRow = new QHBoxLayout;
    metricsRow->addWidget(createMetricCard("cpu", "CPU overall", "34%", "4 cores"));
    metricsRow->addWidget(createMetricCard("memory", "Memory used", "5.2 GB", "of 16 GB · swap 0.1"));
    metricsRow->addWidget(createMetricCard("disk", "Disk read", "12 MB/s", "write 4 MB/s"));
    metricsRow->addWidget(createMetricCard("network", "Network rx", "820 KB/s", "tx 210 KB/s"));
    ui->contentLayout->insertLayout(0, metricsRow);
}

QLabel *MainWindow::makeLabel(const QString &text, const QString &objectName)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(objectName);
    return label;
}

QWidget *MainWindow::createMetricCard(const QString &key, const QString &title, const QString &value, const QString &subtitle)
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

    return card;
}

void MainWindow::updateMetric(const QString &key, const QString &value)
{
    if (m_metricValues.contains(key))
        m_metricValues[key]->setText(value);
}

void MainWindow::addAgentItem(const QString &name, const QString &details, bool online)
{
    AgentItemWidget *widget = new AgentItemWidget(this);
    widget->setAgent(name, details, online);

    QListWidgetItem *item = new QListWidgetItem(ui->agentList);
    item->setData(Qt::UserRole, name);
    item->setSizeHint(widget->sizeHint());
    ui->agentList->setItemWidget(item, widget);
}


void MainWindow::showOutput(const QString &text)
{
    // TODO: replace with DashboardResponse data
    ui->outputView->setPlainText(text);
}
