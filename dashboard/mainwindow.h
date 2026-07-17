#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    /// Fills the agent list. TODO: replace hardcoded data with server AGENTS payload.
    void populateAgents();

    /// Displays command output. TODO: feed with DashboardResponse data.
    void showOutput(const QString &text);
};
#endif // MAINWINDOW_H
