#pragma once

#include <QFrame>

class QTableWidget;

class LogPanelWidget : public QFrame
{
    Q_OBJECT

public:
    explicit LogPanelWidget(QWidget *parent = nullptr);

    void addLog(const QString &direction, const QString &hex, const QString &description);
    int rowCount() const;

private:
    QTableWidget *m_table = nullptr;
};
