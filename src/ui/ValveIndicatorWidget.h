#pragma once

#include <QWidget>

class QTimer;

class ValveIndicatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ValveIndicatorWidget(int channel, int valveNumber, QWidget *parent = nullptr);
    static QSize preferredSize();

    int valveNumber() const;
    bool isPulsing() const;
    void pulse(int durationMs = 10);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sizeHint() const override;

signals:
    void clicked(int channel, int valveNumber);

private:
    void setPulsing(bool pulsing);

    int m_channel = 0;
    int m_valveNumber = 0;
    bool m_pulsing = false;
    bool m_hovered = false;
    QTimer *m_pulseTimer = nullptr;
};
