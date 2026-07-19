#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "agentitemwidget.h"
#include "linechartwidget.h"

#include <QListWidgetItem>
#include <QVector>

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>

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
    ui->contentLayout->insertWidget(0, createProcessTable());

    LineChartWidget *chart = new LineChartWidget;
    chart->setMinimumHeight(200);
    chart->setTitle("CPU — per core (last 20 s)");
    // TODO: replace hardcoded series with real per-core CPU history.
    // Server sends one MetricsSample per second (METRICS_INTERVAL_MS);
    // each sample.cpu.per_core is a vector<float> (one value per core).
    // Append each core's value to a rolling history, then setSeries it.
    // Labels can be built as "core0".."coreN" from per_core.size().
    chart->setSeries({
        { 30, 45, 40, 60, 55, 50, 65, 70, 60 },
        { 20, 25, 30, 28, 35, 40, 38, 42, 45 },
        { 55, 60, 58, 65, 70, 68, 72, 75, 70 },
        { 10, 12, 15, 14, 18, 16, 20, 22, 19 }
    });
    chart->setLabels({ "core0", "core1", "core2", "core3" });
    chart->setSeries({
        { 30, 45, 40, 60, 55, 50, 65, 70, 60 },
        { 20, 25, 30, 28, 35, 40, 38, 42, 45 },
        { 55, 60, 58, 65, 70, 68, 72, 75, 70 },
        { 10, 12, 15, 14, 18, 16, 20, 22, 19 }
    });
    chart->setLabels({ "core0", "core1", "core2", "core3" });
    ui->contentLayout->insertWidget(0, chart);

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
}


void MainWindow::updateMetric(const QString &key, const QString &value)
{
    if (m_metricValues.contains(key))
        m_metricValues[key]->setText(value);
}

// Builds one display row from a ProcessInfo. Works the same for hardcoded
// or server data — no change needed when wiring the network.
QWidget *MainWindow::createProcessRow(const ProcessInfo &process, bool isHeader)
{
    QWidget *row = new QWidget;
    row->setObjectName("processRow");

     const QString cellName = isHeader ? "processHeaderCell" : "processCell";

    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->addWidget(makeLabel(process.pid, cellName));
    layout->addWidget(makeLabel(process.name, cellName));
    layout->addWidget(makeLabel(process.cpuPercent, cellName));

    if (isHeader)
        layout->addWidget(makeLabel("CPU bar", cellName));
    else
        layout->addWidget(createBar(process.cpuValue));

    layout->addWidget(makeLabel(process.memPercent, cellName));

    if (isHeader)
        layout->addWidget(makeLabel("Mem bar", cellName));
    else
        layout->addWidget(createBar(process.memValue));

    layout->addWidget(makeLabel(process.status, cellName));



    return row;
}

void MainWindow::buildStatusBar()
{
    ui->statusbar->setStyleSheet(
        "QStatusBar { background-color: #4C3B4D; }"
        "QStatusBar::item { border: none; }"
        "QLabel#statusItem { color: #b8b2bd; font-size: 12px; padding: 2px 12px; }");

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

QWidget *MainWindow::createProcessTable()
{
    QWidget *section = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(section);

    outer->addWidget(makeLabel("RUNNING PROCESSES (TOP BY CPU)", "sectionTitle"));

    QWidget *grid = new QWidget;
    grid->setObjectName("processGrid");
    QGridLayout *g = new QGridLayout(grid);

    // Header row (row 0) — static column labels, keep as-is.
    const QStringList headers = { "PID", "Name", "CPU %", "CPU bar", "Mem %", "Mem bar", "Status" };
    for (int col = 0; col < headers.size(); ++col)
        g->addWidget(makeLabel(headers[col], "processHeaderCell"), 0, col);

    // TODO: replace this hardcoded vector with the process list from the
    // server ProcessListPayload. Map each incoming entry to a ProcessInfo;
    // the fill loop below (labels + createBar) stays unchanged.
    const QVector<ProcessInfo> processes = {
        { "1284", "nginx", "18%", "2.1%", "running", 18, 2 },
        { "3041", "postgres", "12%", "8.4%", "running", 12, 8 },
        { "887", "python3", "9%", "3.2%", "running", 70, 3 },
        { "512", "systemd", "0.4%", "0.8%", "running", 1, 1 }
    };

    // Fill loop: one grid row per process. Column order matches the header above.
    int row = 1;
    for (const ProcessInfo &p : processes) {
        g->addWidget(makeLabel(p.pid, "processPid"), row, 0);
        g->addWidget(makeLabel(p.name, "processCell"), row, 1);
        g->addWidget(makeLabel(p.cpuPercent, "processCell"), row, 2);
        g->addWidget(createBar(p.cpuValue), row, 3);
        g->addWidget(makeLabel(p.memPercent, "processCell"), row, 4);
        g->addWidget(createBar(p.memValue), row, 5);
        g->addWidget(makeLabel(p.status, "processStatus"), row, 6);
        ++row;

        // Separator line spanning all 7 columns
        g->addWidget(createSeparator(), row, 0, 1, 7);
        ++row;
    }

    outer->addWidget(grid);
    return section;
}

QWidget *MainWindow::createBar(int value)
{
    QProgressBar *bar = new QProgressBar;
    bar->setObjectName("processBar");
    bar->setRange(0, 100);
    bar->setValue(value);
    bar->setTextVisible(false);
    bar->setFixedHeight(6);
    bar->setMaximumWidth(140);
    bar->setProperty("high", value >= 50);   // ← more when >=50 the color change
    return bar;
}

QWidget *MainWindow::createSeparator()
{
    QWidget *line = new QWidget;
    line->setObjectName("processSeparator");
    line->setFixedHeight(1);
    return line;
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
