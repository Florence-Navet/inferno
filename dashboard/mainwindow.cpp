#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->setupUi(this);
    populateAgents();
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
