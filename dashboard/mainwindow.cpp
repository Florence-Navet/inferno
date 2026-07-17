#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->setupUi(this);
    populateAgents();

    qDebug() << "target:" << m_target;

    connect(ui->runButton, &QPushButton::clicked, this, [this]() {
            qDebug() << "target:" << m_target;
        showOutput(ui->commandEdit->text());
        });

    connect(ui->agentList, &QListWidget::currentTextChanged,
            this, [this](const QString &text) {
                m_target = text;
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::populateAgents()
{
    // TODO: replace with RegisterPayload list from DataType::AGENTS
    ui->agentList->addItem("agent1-desktop");
    ui->agentList->addItem("agent2-srv");
    ui->agentList->addItem("agent3-lab");
    ui->agentList->addItem("agent4-pi");
    ui->agentList->addItem("agent5-mac");
}

void MainWindow::showOutput(const QString &text)
{
    // TODO: replace with DashboardResponse data
    ui->outputView->setPlainText(text);
}
