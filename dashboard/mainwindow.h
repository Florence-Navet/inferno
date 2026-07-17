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

    /// Adds one rich agent row to the list.
    void addAgentItem(const QString &name, const QString &details, bool online);

    /// Displays command output. TODO: feed with DashboardResponse data.
    void showOutput(const QString &text);

    QString m_target;
};
#endif // MAINWINDOW_H
