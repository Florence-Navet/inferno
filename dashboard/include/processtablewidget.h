#ifndef PROCESSTABLEWIDGET_H
#define PROCESSTABLEWIDGET_H

#include <QWidget>
#include <QString>


struct ProcessInfo {
    QString pid;
    QString name;
    QString cpuPercent;  // display text, e.g. "18%"
    QString memPercent; // display text, e.g. "2.1%"
    QString status;
    int cpuValue = 0;      // 0-100, drives the CPU bar
    int memValue = 0;      // 0-100, drives the Mem bar
};


class ProcessTableWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProcessTableWidget(QWidget *parent = nullptr);

private:

    /// Creates a thin horizontal separator line.
    QWidget *createSeparator();

    /// Creates a mini progress bar (0-100) for the process table.
    QWidget *createBar(int value);

signals:
};

#endif // PROCESSTABLEWIDGET_H
