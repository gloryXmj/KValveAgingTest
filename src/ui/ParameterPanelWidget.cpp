#include "src/ui/ParameterPanelWidget.h"

#include "src/core/CommandMap.h"
#include "src/ui/TouchSpinBoxWidget.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
QWidget *makeFieldRow(const QString &labelText, QWidget *editor, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *label = new QLabel(labelText, row);
    label->setProperty("fieldLabel", true);
    layout->addWidget(label);
    layout->addWidget(editor);
    return row;
}

CommandPacket makePacket(const quint8 command, const quint16 data16, const CommandPurpose purpose = CommandPurpose::ParameterWrite)
{
    return CommandPacket{command, data16, purpose};
}

void setComboData(QComboBox *comboBox, const int value)
{
    if (comboBox == nullptr) {
        return;
    }

    const int index = comboBox->findData(value);
    comboBox->setCurrentIndex(index >= 0 ? index : 0);
}

void prepareTouchComboBox(QComboBox *comboBox)
{
    if (comboBox == nullptr) {
        return;
    }

    auto *view = new QListView(comboBox);
    view->setSpacing(4);
    view->setUniformItemSizes(true);
    comboBox->setView(view);
    comboBox->setMinimumHeight(44);
    comboBox->setMaxVisibleItems(10);
}
}

ParameterPanelWidget::ParameterPanelWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("panelCard"));

    auto *layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("控制面板"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    auto *caption = new QLabel(
        QStringLiteral("吹气时间、充电时间、停止充电时间、续充电时间修改时需要输入固定密码确认。"),
        this);
    caption->setObjectName(QStringLiteral("sectionCaption"));
    caption->setWordWrap(true);
    layout->addWidget(title);
    layout->addWidget(caption);

    auto *quickGroup = new QGroupBox(QStringLiteral("快捷操作"), this);
    auto *quickLayout = new QVBoxLayout(quickGroup);
    auto *versionButton = new QPushButton(QStringLiteral("读取固件版本"), quickGroup);
    versionButton->setProperty("secondary", true);
    connect(versionButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(CommandMap::kVersionQuery, 0x0000, false, CommandPurpose::VersionQuery);
    });
    quickLayout->addWidget(versionButton);
    layout->addWidget(quickGroup);

    auto *generalGroup = new QGroupBox(QStringLiteral("基础参数"), this);
    auto *generalLayout = new QVBoxLayout(generalGroup);

    m_valveSwitch = new QPushButton(generalGroup);
    m_valveSwitch->setCheckable(true);
    m_valveSwitch->setProperty("valveToggle", true);
    m_valveSwitch->setMinimumHeight(44);
    updateValveSwitchButton();
    connect(m_valveSwitch, &QPushButton::clicked, this, [this](const bool checked) {
        Q_UNUSED(checked);
        updateValveSwitchButton();
        emitParameterCommand(CommandMap::kValveSwitch, m_valveSwitch->isChecked() ? 1u : 0u, false);
        emitControlParametersChanged();
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("阀开关"), m_valveSwitch, generalGroup));

    m_triggerMode = new QComboBox(generalGroup);
    m_triggerMode->addItem(QStringLiteral("单次触发"), 0);
    m_triggerMode->addItem(QStringLiteral("单次递增"), 1);
    m_triggerMode->addItem(QStringLiteral("单次循环"), 2);
    m_triggerMode->addItem(QStringLiteral("递增循环"), 3);
    m_triggerMode->addItem(QStringLiteral("三抽一分开"), 4);
    m_triggerMode->addItem(QStringLiteral("三抽一同时"), 5);
    prepareTouchComboBox(m_triggerMode);
    connect(m_triggerMode, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](const int index) {
        Q_UNUSED(index);
        emitParameterCommand(CommandMap::kTriggerMode, quint16(m_triggerMode->currentData().toUInt()), false);
        emitControlParametersChanged();
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("触发模式"), m_triggerMode, generalGroup));

    m_blowCount = new QSpinBox(generalGroup);
    m_blowCount->setRange(0, 65535);
    m_blowCount->setValue(1);
    connect(m_blowCount, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        emitParameterCommand(CommandMap::kBlowCount, quint16(value), false);
        emitControlParametersChanged();
    });
    generalLayout->addWidget(
        makeFieldRow(
            QStringLiteral("吹气次数"),
            new TouchSpinBoxWidget(m_blowCount, 148, QStringLiteral("输入吹气次数"), generalGroup),
            generalGroup));

    m_blowInterval = new QSpinBox(generalGroup);
    m_blowInterval->setRange(0, 65535);
    m_blowInterval->setSingleStep(10);
    m_blowInterval->setValue(10);
    m_blowInterval->setSuffix(QStringLiteral(" ms"));
    connect(m_blowInterval, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        emitParameterCommand(CommandMap::kBlowInterval, quint16(value), false);
        emitControlParametersChanged();
    });
    generalLayout->addWidget(
        makeFieldRow(
            QStringLiteral("吹气间隔"),
            new TouchSpinBoxWidget(m_blowInterval, 148, QStringLiteral("输入吹气间隔"), generalGroup),
            generalGroup));

    m_channelCount = new QSpinBox(generalGroup);
    m_channelCount->setRange(1, CommandMap::kChannelCountSupported);
    m_channelCount->setValue(CommandMap::kChannelCountSupported);
    connect(m_channelCount, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        emitParameterCommand(CommandMap::kChannelCount, quint16(value), false);
        emitControlParametersChanged();
    });
    generalLayout->addWidget(
        makeFieldRow(
            QStringLiteral("通道数量"),
            new TouchSpinBoxWidget(m_channelCount, 148, QStringLiteral("输入通道数量"), generalGroup),
            generalGroup));

    m_independentChannelEnable = new QComboBox(generalGroup);
    m_independentChannelEnable->addItem(QStringLiteral("关闭"), 0);
    m_independentChannelEnable->addItem(QStringLiteral("开启"), 1);
    prepareTouchComboBox(m_independentChannelEnable);
    connect(
        m_independentChannelEnable,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this](const int index) {
            Q_UNUSED(index);
            emitParameterCommand(
                CommandMap::kIndependentChannelEnable,
                quint16(m_independentChannelEnable->currentData().toUInt()),
                false);
            emitControlParametersChanged();
        });
    generalLayout->addWidget(
        makeFieldRow(QStringLiteral("通道独立使能"), m_independentChannelEnable, generalGroup));
    layout->addWidget(generalGroup);

    auto *timingGroup = new QGroupBox(QStringLiteral("受保护时间参数"), this);
    auto *timingLayout = new QVBoxLayout(timingGroup);

    m_blowTime = new QDoubleSpinBox(timingGroup);
    m_blowTime->setDecimals(2);
    m_blowTime->setSingleStep(0.01);
    m_blowTime->setRange(0.0, 3276.75);
    m_blowTime->setSuffix(QStringLiteral(" ms"));
    m_blowTime->setValue(2.0);
    timingLayout->addWidget(
        makeFieldRow(
            QStringLiteral("吹气时间"),
            new TouchSpinBoxWidget(m_blowTime, 148, QStringLiteral("输入吹气时间(ms)"), timingGroup),
            timingGroup));

    m_chargeTime = new QDoubleSpinBox(timingGroup);
    m_chargeTime->setDecimals(2);
    m_chargeTime->setSingleStep(0.05);
    m_chargeTime->setRange(0.0, 3276.75);
    m_chargeTime->setSuffix(QStringLiteral(" ms"));
    m_chargeTime->setValue(1.0);
    timingLayout->addWidget(
        makeFieldRow(
            QStringLiteral("充电时间"),
            new TouchSpinBoxWidget(m_chargeTime, 148, QStringLiteral("输入充电时间(ms)"), timingGroup),
            timingGroup));

    m_stopChargeTime = new QDoubleSpinBox(timingGroup);
    m_stopChargeTime->setDecimals(2);
    m_stopChargeTime->setSingleStep(0.05);
    m_stopChargeTime->setRange(0.0, 3276.75);
    m_stopChargeTime->setSuffix(QStringLiteral(" ms"));
    m_stopChargeTime->setValue(1.5);
    timingLayout->addWidget(
        makeFieldRow(
            QStringLiteral("停止充电时间"),
            new TouchSpinBoxWidget(
                m_stopChargeTime,
                148,
                QStringLiteral("输入停止充电时间(ms)"),
                timingGroup),
            timingGroup));

    m_rechargeTime = new QDoubleSpinBox(timingGroup);
    m_rechargeTime->setDecimals(2);
    m_rechargeTime->setSingleStep(0.05);
    m_rechargeTime->setRange(0.0, 3276.75);
    m_rechargeTime->setSuffix(QStringLiteral(" ms"));
    m_rechargeTime->setValue(0.4);
    timingLayout->addWidget(
        makeFieldRow(
            QStringLiteral("续充电时间"),
            new TouchSpinBoxWidget(m_rechargeTime, 148, QStringLiteral("输入续充电时间(ms)"), timingGroup),
            timingGroup));

    auto *timingApplyButton = new QPushButton(QStringLiteral("下发时间参数"), timingGroup);
    connect(timingApplyButton, &QPushButton::clicked, this, &ParameterPanelWidget::timingBatchRequested);
    timingLayout->addWidget(timingApplyButton);

    auto *timingHint = new QLabel(
        QStringLiteral("设备协议时间单位为 0.05 ms，界面输入毫秒值后会自动换算，例如 2.00 ms 实际下发为 40。"),
        timingGroup);
    timingHint->setWordWrap(true);
    timingHint->setObjectName(QStringLiteral("sectionCaption"));
    timingLayout->addWidget(timingHint);
    layout->addWidget(timingGroup);

    connect(m_blowTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_chargeTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_stopChargeTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_rechargeTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);

    layout->addStretch(1);
}

ControlParameters ParameterPanelWidget::currentControlParameters() const
{
    ControlParameters parameters;
    parameters.valveSwitch = m_valveSwitch != nullptr && m_valveSwitch->isChecked() ? 1 : 0;
    parameters.triggerMode = m_triggerMode != nullptr ? m_triggerMode->currentData().toInt() : 0;
    parameters.blowCount = m_blowCount != nullptr ? m_blowCount->value() : 1;
    parameters.blowIntervalMs = m_blowInterval != nullptr ? m_blowInterval->value() : 0;
    parameters.blowTimeMs = m_blowTime != nullptr ? m_blowTime->value() : 2.0;
    parameters.chargeTimeMs = m_chargeTime != nullptr ? m_chargeTime->value() : 1.0;
    parameters.stopChargeTimeMs = m_stopChargeTime != nullptr ? m_stopChargeTime->value() : 1.5;
    parameters.rechargeTimeMs = m_rechargeTime != nullptr ? m_rechargeTime->value() : 0.4;
    parameters.channelCount = m_channelCount != nullptr ? m_channelCount->value() : CommandMap::kChannelCountSupported;
    parameters.independentChannelEnable = m_independentChannelEnable != nullptr
        ? m_independentChannelEnable->currentData().toInt()
        : 0;
    return parameters;
}

void ParameterPanelWidget::applyControlParameters(const ControlParameters &parameters)
{
    const QSignalBlocker valveSwitchBlocker(m_valveSwitch);
    const QSignalBlocker triggerModeBlocker(m_triggerMode);
    const QSignalBlocker blowCountBlocker(m_blowCount);
    const QSignalBlocker blowIntervalBlocker(m_blowInterval);
    const QSignalBlocker blowTimeBlocker(m_blowTime);
    const QSignalBlocker chargeTimeBlocker(m_chargeTime);
    const QSignalBlocker stopChargeTimeBlocker(m_stopChargeTime);
    const QSignalBlocker rechargeTimeBlocker(m_rechargeTime);
    const QSignalBlocker channelCountBlocker(m_channelCount);
    const QSignalBlocker independentChannelEnableBlocker(m_independentChannelEnable);

    if (m_valveSwitch != nullptr) {
        m_valveSwitch->setChecked(parameters.valveSwitch != 0);
        updateValveSwitchButton();
    }
    setComboData(m_triggerMode, parameters.triggerMode);
    if (m_blowCount != nullptr) {
        m_blowCount->setValue(parameters.blowCount);
    }
    if (m_blowInterval != nullptr) {
        m_blowInterval->setValue(parameters.blowIntervalMs);
    }
    if (m_blowTime != nullptr) {
        m_blowTime->setValue(parameters.blowTimeMs);
    }
    if (m_chargeTime != nullptr) {
        m_chargeTime->setValue(parameters.chargeTimeMs);
    }
    if (m_stopChargeTime != nullptr) {
        m_stopChargeTime->setValue(parameters.stopChargeTimeMs);
    }
    if (m_rechargeTime != nullptr) {
        m_rechargeTime->setValue(parameters.rechargeTimeMs);
    }
    if (m_channelCount != nullptr) {
        m_channelCount->setValue(parameters.channelCount);
    }
    setComboData(m_independentChannelEnable, parameters.independentChannelEnable);
}

QVector<CommandPacket> ParameterPanelWidget::generalParameterCommands() const
{
    const ControlParameters parameters = currentControlParameters();
    return QVector<CommandPacket>{
        makePacket(CommandMap::kChannelCount, quint16(parameters.channelCount)),
        makePacket(CommandMap::kIndependentChannelEnable, quint16(parameters.independentChannelEnable)),
        makePacket(CommandMap::kTriggerMode, quint16(parameters.triggerMode)),
        makePacket(CommandMap::kBlowCount, quint16(parameters.blowCount)),
        makePacket(CommandMap::kBlowInterval, quint16(parameters.blowIntervalMs)),
        makePacket(CommandMap::kValveSwitch, quint16(parameters.valveSwitch)),
    };
}

QVector<CommandPacket> ParameterPanelWidget::timingParameterCommands() const
{
    const ControlParameters parameters = currentControlParameters();
    return QVector<CommandPacket>{
        makePacket(CommandMap::kBlowTime, timingMsToRaw(parameters.blowTimeMs)),
        makePacket(CommandMap::kChargeTime, timingMsToRaw(parameters.chargeTimeMs)),
        makePacket(CommandMap::kStopChargeTime, timingMsToRaw(parameters.stopChargeTimeMs)),
        makePacket(CommandMap::kRechargeTime, timingMsToRaw(parameters.rechargeTimeMs)),
    };
}

void ParameterPanelWidget::emitParameterCommand(const quint8 command, const quint16 data16, const bool passwordRequired, const CommandPurpose purpose)
{
    emit commandRequested(CommandPacket{command, data16, purpose}, passwordRequired);
}

void ParameterPanelWidget::emitControlParametersChanged()
{
    emit controlParametersChanged(currentControlParameters());
}

void ParameterPanelWidget::updateValveSwitchButton()
{
    if (m_valveSwitch == nullptr) {
        return;
    }

    m_valveSwitch->setText(m_valveSwitch->isChecked() ? QStringLiteral("开启") : QStringLiteral("关闭"));
}

quint16 ParameterPanelWidget::timingMsToRaw(const double milliseconds)
{
    return quint16(qBound(0, qRound(milliseconds / 0.05), 65535));
}
