#include "mainwindow.h"

#include <QCloseEvent>
#include <QGridLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QVector>
#include <QStyle>

#include "./ui_mainwindow.h"
#include "agentitemwidget.h"
#include "env_helper.hpp"
#include "linechartwidget.h"
#include "metriccardswidget.h"
#include "processtablewidget.h"
#include "serverclient.h"
#include "theme.h"
#include "uiutils.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    m_client = new ServerClient(this);

    for (QPushButton* button : findChildren<QPushButton*>())
        button->setCursor(Qt::PointingHandCursor);

    populateAgents();

    buildContentArea();

    buildStatusBar();

    updateStatusBadge();


    qDebug() << "target:" << m_target;

    connect(ui->runButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "target:" << m_target;
        // showOutput(ui->commandEdit->text());
        m_client->sendCommand(m_target, CommandType::SHELL,
                              ui->commandEdit->text());
    });

    const QVector<QPair<QPushButton*, QString>> presets = {
                                                            {ui->presetDfButton, "df -h"},
                                                            {ui->presetNetstatButton, "netstat -tulpn"},
                                                            {ui->presetTopButton, "top -bn1"}};
    for (const QPair<QPushButton*, QString>& preset : presets) {
        connect(preset.first, &QPushButton::clicked, this,
                [this, preset]() { ui->commandEdit->setText(preset.second); });
    }
    connect(ui->agentList, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
                const QString clicked = item->data(Qt::UserRole).toString();
                ui->agentNameLabel->setText(
                    item->data(Qt::UserRole + 1).toString());
                ui->osBadge->setText(item->data(Qt::UserRole + 2).toString());
                ui->ipBadge->setText(item->data(Qt::UserRole + 3).toString());
                const bool wasStreaming = (clicked == m_streamingTarget);

                if (!m_streamingTarget.isEmpty()) {
                    m_client->sendCommand(m_streamingTarget,
                                          CommandType::STOP_METRICS, QString());
                    m_streamingTarget.clear();
                }

                if (clicked != m_target) {
                    m_cpuHistory.clear();
                    m_memoryHistory.clear();
                    m_networkHistory.clear();
                    m_diskHistory.clear();
                }

                m_target = clicked;

                if (wasStreaming) {
                    updateStatusBadge();
                    return;
                }

                m_client->sendCommand(m_target, CommandType::START_METRICS,
                                      QString());
                m_streamingTarget = m_target;
                updateStatusBadge();
            });

    connect(m_client, &ServerClient::agentReceived, this,
            [this](const QString& id, const QString& name, const QString& os,
                   const QString& ip, bool online) { addAgentItem(id, name, os, ip, online); });

    connect(
        m_client, &ServerClient::responseReceived, this,
        [this](const QString& target, const QString& text) { showOutput(text); });

    connect(
        m_client, &ServerClient::processListReceived, this,
        [this](const QString& target, const std::vector<ProcessInfo>& processes) {
            m_processTable->setProcesses(processes);
        });

    connect(m_client, &ServerClient::metricsReceived, this,
            &MainWindow::onMetricsReceived);

    connect(ui->processListButton, &QPushButton::clicked, this, [this]() {
        m_client->sendCommand(m_target, CommandType::RUNNING_PROCESSES, QString());
    });

    m_client->connectToServer("localhost", EnvHelper::resolvePort());
    //   m_client->connectToServer("localhost", 8888);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::populateAgents() { ui->agentList->setCurrentRow(-1); }

void MainWindow::buildContentArea() {
    // ui->contentLayout->insertWidget(0, new ProcessTableWidget(this));

    QGridLayout* chartsGrid = new QGridLayout;

    m_cpuChart = createChart("CPU — per core (last 20 s)",
                             {{30, 45, 40, 60, 55, 50, 65, 70, 60},
                              {20, 25, 30, 28, 35, 40, 38, 42, 45},
                              {55, 60, 58, 65, 70, 68, 72, 75, 70},
                              {10, 12, 15, 14, 18, 16, 20, 22, 19}},
                             {"core0", "core1", "core2", "core3"});
    chartsGrid->addWidget(m_cpuChart, 0, 0);


    m_memoryChart =
        createChart("Memory over time", {{40, 42, 45, 44, 48, 50, 52, 55, 58}},
                    {"used"}, {},  // dashed: none
                    {0},           // filled: series 0
                    "16 GB total");
    chartsGrid->addWidget(m_memoryChart, 0, 1);

    m_networkChart = createChart("Network I/O — eth0",
                                 {{30, 50, 40, 60, 55, 70, 50, 65, 45},
                                  {10, 15, 12, 18, 14, 20, 16, 22, 18}},
                                 {"rx", "tx"}, {1});
    chartsGrid->addWidget(m_networkChart, 1, 0);

    m_diskChart = createChart("Disk I/O — sda",
                              {{20, 35, 30, 45, 40, 55, 35, 50, 40},
                               {5, 8, 6, 10, 7, 12, 8, 14, 9}},
                              {"read", "write"});
    chartsGrid->addWidget(m_diskChart, 1, 1);

    ui->contentLayout->insertLayout(0, chartsGrid);

    m_processTable = new ProcessTableWidget(this);
    ui->contentLayout->insertWidget(0, m_processTable);

    m_metricCards = new MetricCardsWidget(this);
    ui->contentLayout->insertWidget(0, m_metricCards);
}

LineChartWidget* MainWindow::createChart(const QString& title,
                                         const QVector<QVector<double>>& series,
                                         const QStringList& labels,
                                         const QVector<int>& dashed,
                                         const QVector<int>& filled,
                                         const QString& topRight) {
    LineChartWidget* chart = new LineChartWidget;
    chart->setMinimumHeight(200);
    chart->setTitle(title);
    chart->setSeries(series);
    chart->setLabels(labels);
    chart->setDashed(dashed);
    chart->setFilled(filled);
    chart->setTopRight(topRight);
    return chart;
}

void MainWindow::buildStatusBar() {
    ui->statusbar->setStyleSheet(
        QString("QStatusBar { background-color: %1; }"
                "QStatusBar::item { border: none; }"
                "QLabel#statusItem { color: %2; font-size: 12px; padding: 2px "
                "12px; }")
            .arg(Theme::StatusBarBackground.name(), Theme::StatusBarText.name()));

    ui->statusbar->setContentsMargins(280, 0, 16, 0);
    // TODO: replace hardcoded values with live server status
    // ui->statusbar->addWidget(makeLabel(QString("● %1 agents
    // online").arg(ui->agentList->count()), "statusItem"));
    ui->statusbar->addWidget(makeLabel("● 4 agents online", "statusItem"));
    // TODO: show elapsed since last MetricsSample (arrives every
    // METRICS_INTERVAL_MS)
    ui->statusbar->addWidget(makeLabel("last sample: 0.3 s ago", "statusItem"));
    // TODO: db status — no payload exposes it yet, to confirm with the server
    // side
    ui->statusbar->addWidget(makeLabel("db: PostgreSQL connected", "statusItem"));
    // App version — static, not server-driven.
    ui->statusbar->addPermanentWidget(makeLabel("v1.0.0", "statusItem"));
}

void MainWindow::addAgentItem(const QString& id, const QString& name,
                              const QString& os, const QString&ip, bool online) {
    const QString details = os + " · " + ip;

    AgentItemWidget* widget = new AgentItemWidget(this);
    widget->setAgent(name, details, online);

    QListWidgetItem* item = new QListWidgetItem(ui->agentList);
    item->setData(Qt::UserRole, id);
    item->setData(Qt::UserRole + 1, name);
    item->setData(Qt::UserRole + 2, os);
    item->setData(Qt::UserRole + 3, ip);
    item->setSizeHint(widget->sizeHint());
    ui->agentList->setItemWidget(item, widget);
}

void MainWindow::showOutput(const QString& text) {
    // TODO: replace with DashboardResponse data
    ui->outputView->setPlainText(text);
}

void MainWindow::onMetricsReceived(const QString& target,
                                   const MetricsSample& sample) {
    if (target != m_target) return;

    m_metricCards->updateFromSample(sample);


    // Memory
    const double memGb = sample.mem.phys_used / 1024.0 / 1024.0 / 1024.0;
    m_memoryHistory.append({memGb});

    m_memoryChart->setSeries(m_memoryHistory.series());

    //CPU
    if (sample.cpu.per_core.size() >= 4) {
        QVector<double> cores;
        for (int i = 0; i < 4; ++i) cores.append(sample.cpu.per_core[i]);

        m_cpuHistory.append(cores);
        m_cpuChart->setSeries(m_cpuHistory.series());
    }

    //DISK
    double diskRead = 0.0;
    double diskWrite = 0.0;
    for (const DiskSample& disk : sample.disks) {
        diskRead += disk.read_bytes_per_sec;
        diskWrite += disk.write_bytes_per_sec;
    }

    m_diskHistory.append({diskRead / 1024.0 / 1024.0, diskWrite / 1024.0 / 1024.0});
    m_diskChart->setSeries(m_diskHistory.series());

    //NETWORK
    double netRx = 0.0;
    double netTx = 0.0;
    for (const NetSample& iface : sample.interfaces) {
        netRx += iface.rx_bytes_per_sec;
        netTx += iface.tx_bytes_per_sec;
    }

    const double maxRate = 1024.0 * 1024.0 * 1024.0;
    if (netRx <= maxRate && netTx <= maxRate) {
        m_networkHistory.append({netRx / 1024.0, netTx / 1024.0});
        m_networkChart->setSeries(m_networkHistory.series());
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_client && !m_streamingTarget.isEmpty())
        m_client->sendCommand(m_streamingTarget, CommandType::STOP_METRICS,
                              QString());

    QMainWindow::closeEvent(event);
}

void MainWindow::updateStatusBadge() {
    const bool streaming = !m_target.isEmpty() && m_target == m_streamingTarget;

    ui->statusBadge->setText(streaming ? "● streaming" : "● stopped");
    ui->statusBadge->setProperty("streaming", streaming);
    ui->statusBadge->style()->unpolish(ui->statusBadge);
    ui->statusBadge->style()->polish(ui->statusBadge);
}
