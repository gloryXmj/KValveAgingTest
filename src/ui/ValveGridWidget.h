#pragma once

#include <QHash>
#include <QList>
#include <QWidget>

class QGridLayout;
class QResizeEvent;
class ValveIndicatorWidget;

class ValveGridWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ValveGridWidget(int channel, QWidget *parent = nullptr);

    int valveCount() const;
    int visibleValveCount() const;
    bool isValvePulsing(int valveNumber) const;
    void setVisibleValveCount(int count);
    void pulseValves(const QList<int> &valveNumbers);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void valveInvoked(int channel, int valveNumber);

private:
    void updateIndicatorVisibility();
    void rebuildGrid();
    int calculateColumnCount(int availableWidth) const;

    int m_channel = 0;
    int m_visibleValveCount = 0;
    int m_columnCount = 0;
    QGridLayout *m_layout = nullptr;
    QHash<int, ValveIndicatorWidget *> m_indicators;
};
