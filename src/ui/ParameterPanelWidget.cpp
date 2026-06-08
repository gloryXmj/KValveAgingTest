#include "src/ui/ParameterPanelWidget.h"

#include "src/core/CommandMap.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
QWidget *makeFieldRow(const QString &labelText, QWidget *editor, QPushButton *button, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *label = new QLabel(labelText, row);
    label->setMinimumWidth(110);
    layout->addWidget(label);
    layout->addWidget(editor, 1);
    if (button != nullptr) {
        layout->addWidget(button);
    }
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
}

ParameterPanelWidget::ParameterPanelWidget(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("panelCard"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("控制面板"), this);
    title->setObjectName(QStringLiteral("sectionTitle"));
    auto *caption = new QLabel(QStringLiteral("吹气时间、充电时间、停止充电时间、续充电时间修改时需要输入固定密码确认。"), this);
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

    m_valveSwitch = new QComboBox(generalGroup);
    m_valveSwitch->addItem(QStringLiteral("关闭"), 0);
    m_valveSwitch->addItem(QStringLiteral("开启"), 1);
    auto *valveSwitchButton = new QPushButton(QStringLiteral("下发"), generalGroup);
    connect(valveSwitchButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(CommandMap::kValveSwitch, quint16(m_valveSwitch->currentData().toUInt()), false);
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("阀开关"), m_valveSwitch, valveSwitchButton, generalGroup));

    m_triggerMode = new QComboBox(generalGroup);
    m_triggerMode->addItem(QStringLiteral("单次触发"), 0);
    m_triggerMode->addItem(QStringLiteral("单次递增"), 1);
    m_triggerMode->addItem(QStringLiteral("单次循环"), 2);
    m_triggerMode->addItem(QStringLiteral("递增循环"), 3);
    m_triggerMode->addItem(QStringLiteral("三抽一分开"), 4);
    m_triggerMode->addItem(QStringLiteral("三抽一同时"), 5);
    auto *triggerButton = new QPushButton(QStringLiteral("下发"), generalGroup);
    connect(triggerButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(CommandMap::kTriggerMode, quint16(m_triggerMode->currentData().toUInt()), false);
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("触发模式"), m_triggerMode, triggerButton, generalGroup));

    m_blowCount = new QSpinBox(generalGroup);
    m_blowCount->setRange(0, 65535);
    m_blowCount->setValue(1);
    auto *blowCountButton = new QPushButton(QStringLiteral("下发"), generalGroup);
    connect(blowCountButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(CommandMap::kBlowCount, quint16(m_blowCount->value()), false);
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("吹气次数"), m_blowCount, blowCountButton, generalGroup));

    m_blowInterval = new QSpinBox(generalGroup);
    m_blowInterval->setRange(0, 65535);
    m_blowInterval->setSuffix(QStringLiteral(" ms"));
    auto *blowIntervalButton = new QPushButton(QStringLiteral("下发"), generalGroup);
    connect(blowIntervalButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(CommandMap::kBlowInterval, quint16(m_blowInterval->value()), false);
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("吹气间隔"), m_blowInterval, blowIntervalButton, generalGroup));

    m_channelCount = new QSpinBox(generalGroup);
    m_channelCount->setRange(1, CommandMap::kChannelCountSupported);
    m_channelCount->setValue(CommandMap::kChannelCountSupported);
    auto *channelCountButton = new QPushButton(QStringLiteral("下发"), generalGroup);
    connect(channelCountButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(CommandMap::kChannelCount, quint16(m_channelCount->value()), false);
    });
    generalLayout->addWidget(makeFieldRow(QStringLiteral("通道数量"), m_channelCount, channelCountButton, generalGroup));

    m_independentChannelEnable = new QComboBox(generalGroup);
    m_independentChannelEnable->addItem(QStringLiteral("关闭"), 0);
    m_independentChannelEnable->addItem(QStringLiteral("开启"), 1);
    auto *independentEnableButton = new QPushButton(QStringLiteral("下发"), generalGroup);
    connect(independentEnableButton, &QPushButton::clicked, this, [this]() {
        emitParameterCommand(
            CommandMap::kIndependentChannelEnable,
            quint16(m_independentChannelEnable->currentData().toUInt()),
            false
        );
    });
    generalLayout->addWidget(
        makeFieldRow(QStringLiteral("通道独立使能"), m_independentChannelEnable, independentEnableButton, generalGroup)
    );
    layout->addWidget(generalGroup);

    auto *timingGroup = new QGroupBox(QStringLiteral("受保护时间参数"), this);
    auto *timingLayout = new QVBoxLayout(timingGroup);

    m_blowTime = new QDoubleSpinBox(timingGroup);
    m_blowTime->setDecimals(2);
    m_blowTime->setSingleStep(0.01);
    m_blowTime->setRange(0.0, 3276.75);
    m_blowTime->setSuffix(QStringLiteral(" ms"));
    m_blowTime->setValue(2.0);
    timingLayout->addWidget(makeFieldRow(QStringLiteral("吹气时间"), m_blowTime, nullptr, timingGroup));

    m_chargeTime = new QDoubleSpinBox(timingGroup);
    m_chargeTime->setDecimals(2);
    m_chargeTime->setSingleStep(0.05);
    m_chargeTime->setRange(0.0, 3276.75);
    m_chargeTime->setSuffix(QStringLiteral(" ms"));
    m_chargeTime->setValue(1.0);
    timingLayout->addWidget(makeFieldRow(QStringLiteral("充电时间"), m_chargeTime, nullptr, timingGroup));

    m_stopChargeTime = new QDoubleSpinBox(timingGroup);
    m_stopChargeTime->setDecimals(2);
    m_stopChargeTime->setSingleStep(0.05);
    m_stopChargeTime->setRange(0.0, 3276.75);
    m_stopChargeTime->setSuffix(QStringLiteral(" ms"));
    m_stopChargeTime->setValue(1.5);
    timingLayout->addWidget(makeFieldRow(QStringLiteral("停止充电时间"), m_stopChargeTime, nullptr, timingGroup));

    m_rechargeTime = new QDoubleSpinBox(timingGroup);
    m_rechargeTime->setDecimals(2);
    m_rechargeTime->setSingleStep(0.05);
    m_rechargeTime->setRange(0.0, 3276.75);
    m_rechargeTime->setSuffix(QStringLiteral(" ms"));
    m_rechargeTime->setValue(0.4);
    timingLayout->addWidget(makeFieldRow(QStringLiteral("续充电时间"), m_rechargeTime, nullptr, timingGroup));

    auto *timingApplyButton = new QPushButton(QStringLiteral("下发时间参数"), timingGroup);
    connect(timingApplyButton, &QPushButton::clicked, this, &ParameterPanelWidget::timingBatchRequested);
    timingLayout->addWidget(timingApplyButton);

    auto *timingHint = new QLabel(QStringLiteral("设备协议时间单位为 0.05 ms，界面输入毫秒值后会自动换算，例如 2.00 ms 实际下发为 40。"), timingGroup);
    timingHint->setWordWrap(true);
    timingHint->setObjectName(QStringLiteral("sectionCaption"));
    timingLayout->addWidget(timingHint);
    layout->addWidget(timingGroup);

    connect(m_valveSwitch, qOverload<int>(&QComboBox::currentIndexChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_triggerMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_blowCount, qOverload<int>(&QSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_blowInterval, qOverload<int>(&QSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_channelCount, qOverload<int>(&QSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(
        m_independentChannelEnable,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &ParameterPanelWidget::emitControlParametersChanged
    );
    connect(m_blowTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_chargeTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_stopChargeTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);
    connect(m_rechargeTime, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &ParameterPanelWidget::emitControlParametersChanged);

    layout->addStretch(1);
}

ControlParameters ParameterPanelWidget::currentControlParameters() const
{
    ControlParameters parameters;
    parameters.valveSwitch = m_valveSwitch != nullptr ? m_valveSwitch->currentData().toInt() : 0;
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

    setComboData(m_valveSwitch, parameters.valveSwitch);
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

quint16 ParameterPanelWidget::timingMsToRaw(const double milliseconds)
{
    return quint16(qBound(0, qRound(milliseconds / 0.05), 65535));
}
