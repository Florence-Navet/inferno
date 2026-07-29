#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "processtablewidget.h"
#include "agentitemwidget.h"
#include "linechartwidget.h"
#include "metriccardswidget.h"
#include "uiutils.h"
#include "theme.h"
#include <QListWidgetItem>
#include "serverclient.h"

#include <QVector>
#include <QGridLayout>
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

    buildStatusBar();



    qDebug() << "target:" << m_target;

    connect(ui->runButton, &QPushButton::clicked, this, [this]() {
            qDebug() << "target:" << m_target;
        //showOutput(ui->commandEdit->text());
         m_client->sendCommand(m_target, CommandType::SHELL, ui->commandEdit->text());
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

    m_client = new ServerClient(this);
    connect(m_client, &ServerClient::agentReceived, this,
            [this](const QString &id, const QString &name, const QString &details) {
                addAgentItem(id, name, details, true);
            });

    m_client->connectToServer("localhost", 8888);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateAgents()
{

    ui->agentList->setCurrentRow(-1);
}

void MainWindow::buildContentArea()
{

   ui->contentLayout->insertWidget(0, new ProcessTableWidget(this));

    QGridLayout *chartsGrid = new QGridLayout;

    // TODO: replace hardcoded series with real metrics history from the server.
    // One MetricsSample per second (METRICS_INTERVAL_MS); build a rolling history per chart:
    //   CPU:     sample.cpu.per_core        (vector<float>, one value per core)
    //   Memory:  sample.mem.phys_used/total (uint64_t bytes, convert to GB)
    //   Network: sample.interfaces[].rx/tx_bytes_per_sec (float)
    //   Disk:    sample.disks[].read/write_bytes_per_sec (float)
    // Then setSeries each with its rolling history.
    chartsGrid->addWidget(createChart("CPU — per core (last 20 s)",
                                      { { 30, 45, 40, 60, 55, 50, 65, 70, 60 },
                                       { 20, 25, 30, 28, 35, 40, 38, 42, 45 },
                                       { 55, 60, 58, 65, 70, 68, 72, 75, 70 },
                                       { 10, 12, 15, 14, 18, 16, 20, 22, 19 } },
                                      { "core0", "core1", "core2", "core3" }), 0, 0);

    chartsGrid->addWidget(createChart("Memory over time",
                                      { { 40, 42, 45, 44, 48, 50, 52, 55, 58 } },
                                      { "used" },
                                      {},            // dashed: none
                                      { 0 },         // filled: series 0
                                      "16 GB total"), 0, 1);

    chartsGrid->addWidget(createChart("Network I/O — eth0",
                                      { { 30, 50, 40, 60, 55, 70, 50, 65, 45 },
                                       { 10, 15, 12, 18, 14, 20, 16, 22, 18 } },
                                      { "rx", "tx" },
                                      { 1 }), 1, 0);

    chartsGrid->addWidget(createChart("Disk I/O — sda",
                                      { { 20, 35, 30, 45, 40, 55, 35, 50, 40 },
                                       { 5, 8, 6, 10, 7, 12, 8, 14, 9 } },
                                      { "read", "write" }), 1, 1);

    ui->contentLayout->insertLayout(0, chartsGrid);

    ui->contentLayout->insertWidget(0, new MetricCardsWidget(this));
}


LineChartWidget *MainWindow::createChart(const QString &title,
                                         const QVector<QVector<double>> &series,
                                         const QStringList &labels,
                                         const QVector<int> &dashed,
                                         const QVector<int> &filled,
                                         const QString &topRight)
{
    LineChartWidget *chart = new LineChartWidget;
    chart->setMinimumHeight(200);
    chart->setTitle(title);
    chart->setSeries(series);
    chart->setLabels(labels);
    chart->setDashed(dashed);
    chart->setFilled(filled);
    chart->setTopRight(topRight);
    return chart;
}


void MainWindow::buildStatusBar()
{
    ui->statusbar->setStyleSheet(
        QString(
        "QStatusBar { background-color: %1; }"
        "QStatusBar::item { border: none; }"
        "QLabel#statusItem { color: %2; font-size: 12px; padding: 2px 12px; }")
        .arg(Theme::StatusBarBackground.name(), Theme::StatusBarText.name()));

    ui->statusbar->setContentsMargins(280, 0, 16, 0);
    // TODO: replace hardcoded values with live server status
    // ui->statusbar->addWidget(makeLabel(QString("● %1 agents online").arg(ui->agentList->count()), "statusItem"));
    ui->statusbar->addWidget(makeLabel("● 4 agents online", "statusItem"));
    // TODO: show elapsed since last MetricsSample (arrives every METRICS_INTERVAL_MS)
    ui->statusbar->addWidget(makeLabel("last sample: 0.3 s ago", "statusItem"));
    // TODO: db status — no payload exposes it yet, to confirm with the server side
    ui->statusbar->addWidget(makeLabel("db: PostgreSQL connected", "statusItem"));
    // App version — static, not server-driven.
    ui->statusbar->addPermanentWidget(makeLabel("v1.0.0", "statusItem"));
}


void MainWindow::addAgentItem(const QString &id, const QString &name,
                              const QString &details, bool online)
{
    AgentItemWidget *widget = new AgentItemWidget(this);
    widget->setAgent(name, details, online);

    QListWidgetItem *item = new QListWidgetItem(ui->agentList);
    item->setData(Qt::UserRole, id);
    item->setSizeHint(widget->sizeHint());
    ui->agentList->setItemWidget(item, widget);
}


void MainWindow::showOutput(const QString &text)
{
    // TODO: replace with DashboardResponse data
    ui->outputView->setPlainText(text);
}
