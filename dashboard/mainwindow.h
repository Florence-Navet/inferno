#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

#include <QMainWindow>
#include <QString>

#include <QHash>

class QLabel;

QT_BEGIN_NAMESPACE


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

    /// Creates a QLabel with the given text and object name.QWidget *createMetricCard(const QString &key, const QString &title, const QString &value, const QString &subtitle);
    QLabel *makeLabel(const QString &text, const QString &objectName);

    QString m_target;

    QHash<QString, QLabel *> m_metricValues;

    /// Updates a metric card value by key. TODO: call from server DataPayload.
    void updateMetric(const QString &key, const QString &value);
};
#endif // MAINWINDOW_H
