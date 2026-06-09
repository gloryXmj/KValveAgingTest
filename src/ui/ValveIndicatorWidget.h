#pragma once

#include <QWidget>

class QTimer;

class ValveIndicatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ValveIndicatorWidget(int channel, int valveNumber, QWidget *parent = nullptr);
    static QSize preferredSize(bool largeTouchMode = false);

    int valveNumber() const;
    bool isPulsing() const;
    void pulse(int durationMs = 50);
    void setLargeTouchMode(bool enabled);

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
    bool m_largeTouchMode = false;
    QTimer *m_pulseTimer = nullptr;
};
