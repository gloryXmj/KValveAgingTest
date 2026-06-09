#include "src/ui/LogPanelWidget.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace
{
constexpr int kMaxLogRows = 1000;

QColor colorForDirection(const QString &direction)
{
    if (direction == QStringLiteral("TX")) {
        return QColor(QStringLiteral("#156FEC"));
    }
    if (direction == QStringLiteral("ACK") || direction == QStringLiteral("RX")) {
        return QColor(QStringLiteral("#067647"));
    }
    if (direction == QStringLiteral("WARN")) {
        return QColor(QStringLiteral("#B54708"));
    }
    return QColor(QStringLiteral("#B42318"));
}

QString displayDirection(const QString &direction)
{
    if (direction == QStringLiteral("TX")) {
        return QStringLiteral("发送");
    }
    if (direction == QStringLiteral("RX")) {
        return QStringLiteral("接收");
    }
    if (direction == QStringLiteral("ACK")) {
        return QStringLiteral("确认");
    }
    if (direction == QStringLiteral("WARN")) {
        return QStringLiteral("警告");
    }
    if (direction == QStringLiteral("ERR")) {
        return QStringLiteral("错误");
    }
    if (direction == QStringLiteral("AUTH")) {
        return QStringLiteral("鉴权");
    }
    if (direction == QStringLiteral("TIMEOUT")) {
        return QStringLiteral("超时");
    }
    return direction;
}
}

LogPanelWidget::LogPanelWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("logCard"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    auto *sidePanel = new QWidget(this);
    sidePanel->setFixedWidth(156);
    auto *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("协议日志"), sidePanel);
    title->setObjectName(QStringLiteral("sectionTitle"));
    sideLayout->addWidget(title);

    auto *caption = new QLabel(QStringLiteral("左侧保留操作区，右侧优先显示日志内容。"), sidePanel);
    caption->setObjectName(QStringLiteral("sectionCaption"));
    caption->setWordWrap(true);
    sideLayout->addWidget(caption);

    auto *clearButton = new QPushButton(QStringLiteral("清空"), sidePanel);
    clearButton->setProperty("secondary", true);
    sideLayout->addWidget(clearButton);
    sideLayout->addStretch(1);
    layout->addWidget(sidePanel);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels(QStringList{
        QStringLiteral("时间"),
        QStringLiteral("方向"),
        QStringLiteral("报文"),
        QStringLiteral("说明"),
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(false);
    layout->addWidget(m_table, 1);

    connect(clearButton, &QPushButton::clicked, this, [this]() {
        m_table->setRowCount(0);
    });
}

void LogPanelWidget::addLog(const QString &direction, const QString &hex, const QString &description)
{
    if (m_table->rowCount() >= kMaxLogRows) {
        m_table->removeRow(0);
    }

    const int row = m_table->rowCount();
    m_table->insertRow(row);

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    m_table->setItem(row, 0, new QTableWidgetItem(timestamp));

    auto *directionItem = new QTableWidgetItem(displayDirection(direction));
    directionItem->setForeground(colorForDirection(direction));
    m_table->setItem(row, 1, directionItem);
    m_table->setItem(row, 2, new QTableWidgetItem(hex));
    m_table->setItem(row, 3, new QTableWidgetItem(description));
    m_table->scrollToBottom();
}

int LogPanelWidget::rowCount() const
{
    return m_table->rowCount();
}
