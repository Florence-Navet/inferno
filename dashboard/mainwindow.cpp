#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "agentitemwidget.h"

#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->setupUi(this);

    for (QPushButton *button : findChildren<QPushButton *>())
        button->setCursor(Qt::PointingHandCursor);

    populateAgents();

    qDebug() << "target:" << m_target;

    connect(ui->runButton, &QPushButton::clicked, this, [this]() {
            qDebug() << "target:" << m_target;
        showOutput(ui->commandEdit->text());
        });

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
