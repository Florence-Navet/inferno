#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QVector>
#include <QStringList>

#include <QHash>

class QLabel;
class LineChartWidget;

struct ProcessInfo {
    QString pid;
    QString name;
    QString cpuPercent;  // display text, e.g. "18%"
    QString memPercent; // display text, e.g. "2.1%"
    QString status;
    int cpuValue = 0;      // 0-100, drives the CPU bar
    int memValue = 0;      // 0-100, drives the Mem bar
};




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

    ///Builds the metric cards, table and console inside contentArea
    void buildContentArea();

    /// Creates one metric card (title + big value + subtitle).
    QWidget *createMetricCard(const QString &key, const QString &title, const QString &value, const QString &subtitle);

    /// Creates a ready-to-use line chart (title + series + legend labels).
    LineChartWidget *createChart(const QString &title,
                                 const QVector<QVector<double>> &series,
                                 const QStringList &labels);

    /// Creates a QLabel with the given text and object name.
    QLabel *makeLabel(const QString &text, const QString &objectName);

    QString m_target;

    QHash<QString, QLabel *> m_metricValues;

    /// Updates a metric card value by key. TODO: call from server DataPayload.
    void updateMetric(const QString &key, const QString &value);

    /// Builds one process table row from a ProcessInfo.
    QWidget *createProcessRow(const ProcessInfo &process, bool isHeader = false);

    /// Builds the RUNNING PROCESSES section (title + header + rows).
    QWidget *createProcessTable();

    /// Creates a mini progress bar (0-100) for the process table.
    QWidget *createBar(int value);

    /// Creates a thin horizontal separator line.
    QWidget *createSeparator();

    /// Fills the bottom status bar. TODO: feed with live server status.
    void buildStatusBar();
};
#endif // MAINWINDOW_H
