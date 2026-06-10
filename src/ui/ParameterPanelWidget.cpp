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
constexpr int kOperationModeTest = 0;
constexpr int kOperationModeAging = 1;

constexpr int kTriggerSingleShot = 0;
constexpr int kTriggerSingleIncrement = 1;
constexpr int kTriggerSingleCycle = 2;
constexpr int kTriggerIncrementCycle = 3;
constexpr int kTriggerTripleSeparate = 4;
constexpr int kTriggerTripleSimultaneous = 5;

int defaultTriggerModeForOperationMode(const int operationMode)
{
    return operationMode == kOperationModeAging ? kTriggerTripleSimultaneous : kTriggerSingleCycle;
}

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

QWidget *makeModeSelectorRow(QPushButton *testButton, QPushButton *agingButton, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(testButton, 1);
    layout->addWidget(agingButton, 1);
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

    m_testModeButton = new QPushButton(QStringLiteral("测试"), generalGroup);
    m_testModeButton->setCheckable(true);
    m_testModeButton->setAutoExclusive(true);
    m_testModeButton->setProperty("modeToggle", true);

    m_agingModeButton = new QPushButton(QStringLiteral("老化"), generalGroup);
    m_agingModeButton->setCheckable(true);
    m_agingModeButton->setAutoExclusive(true);
    m_agingModeButton->setProperty("modeToggle", true);

    connect(m_testModeButton, &QPushButton::clicked, this, [this]() {
        applyOperationModeSelection(kOperationModeTest);
    });
    connect(m_agingModeButton, &QPushButton::clicked, this, [this]() {
        applyOperationModeSelection(kOperationModeAging);
    });
    generalLayout->addWidget(
        makeFieldRow(
            QStringLiteral("工作模式"),
            makeModeSelectorRow(m_testModeButton, m_agingModeButton, generalGroup),
            generalGroup));

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
    m_blowCountRow = makeFieldRow(
        QStringLiteral("吹气次数"),
        new TouchSpinBoxWidget(m_blowCount, 148, QStringLiteral("输入吹气次数"), generalGroup),
        generalGroup);
    generalLayout->addWidget(m_blowCountRow);

    m_blowInterval = new QSpinBox(generalGroup);
    m_blowInterval->setRange(1, 65535);
    m_blowInterval->setSingleStep(10);
    m_blowInterval->setValue(10);
    m_blowInterval->setSuffix(QStringLiteral(" ms"));
    m_blowIntervalRow = makeFieldRow(
        QStringLiteral("吹气间隔"),
        new TouchSpinBoxWidget(m_blowInterval, 148, QStringLiteral("输入吹气间隔"), generalGroup),
        generalGroup);
    generalLayout->addWidget(m_blowIntervalRow);

    m_testFrequency = new QSpinBox(generalGroup);
    m_testFrequency->setRange(1, 1000);
    m_testFrequency->setValue(100);
    m_testFrequency->setSuffix(QStringLiteral(" 次/秒"));
    connect(m_testFrequency, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        emitParameterCommand(CommandMap::kBlowInterval, quint16(intervalMsFromAgingFrequency(value)), false);
        emitControlParametersChanged();
    });
    m_testFrequencyRow = makeFieldRow(
        QStringLiteral("测试频率"),
        new TouchSpinBoxWidget(m_testFrequency, 148, QStringLiteral("输入测试频率(次/秒)"), generalGroup),
        generalGroup);
    generalLayout->addWidget(m_testFrequencyRow);

    m_agingFrequency = new QSpinBox(generalGroup);
    m_agingFrequency->setRange(1, 1000);
    m_agingFrequency->setValue(100);
    m_agingFrequency->setSuffix(QStringLiteral(" 次/秒"));
    connect(m_agingFrequency, qOverload<int>(&QSpinBox::valueChanged), this, [this](const int value) {
        const int intervalMs = intervalMsFromAgingFrequency(value);
        if (m_blowInterval != nullptr) {
            const QSignalBlocker blocker(m_blowInterval);
            m_blowInterval->setValue(intervalMs);
        }
        emitParameterCommand(CommandMap::kBlowInterval, quint16(intervalMs), false);
        emitControlParametersChanged();
    });
    m_agingFrequencyRow = makeFieldRow(
        QStringLiteral("老化频率"),
        new TouchSpinBoxWidget(m_agingFrequency, 148, QStringLiteral("输入老化频率(次/秒)"), generalGroup),
        generalGroup);
    generalLayout->addWidget(m_agingFrequencyRow);

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

    updateOperationModeUi(false);
    layout->addStretch(1);
}

ControlParameters ParameterPanelWidget::currentControlParameters() const
{
    ControlParameters parameters;
    parameters.operationMode = m_operationModeValue;
    parameters.valveSwitch = m_valveSwitch != nullptr && m_valveSwitch->isChecked() ? 1 : 0;
    parameters.triggerMode = m_triggerMode != nullptr ? m_triggerMode->currentData().toInt() : 0;
    parameters.blowCount = m_blowCount != nullptr ? m_blowCount->value() : 1;
    parameters.blowIntervalMs = intervalMsFromAgingFrequency(m_agingFrequency != nullptr ? m_agingFrequency->value() : 100);
    parameters.testFrequencyHz = m_testFrequency != nullptr ? m_testFrequency->value() : 100;
    parameters.agingFrequencyHz = m_agingFrequency != nullptr ? m_agingFrequency->value() : 100;
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
    const QSignalBlocker testModeBlocker(m_testModeButton);
    const QSignalBlocker agingModeBlocker(m_agingModeButton);
    const QSignalBlocker valveSwitchBlocker(m_valveSwitch);
    const QSignalBlocker triggerModeBlocker(m_triggerMode);
    const QSignalBlocker blowCountBlocker(m_blowCount);
    const QSignalBlocker blowIntervalBlocker(m_blowInterval);
    const QSignalBlocker testFrequencyBlocker(m_testFrequency);
    const QSignalBlocker agingFrequencyBlocker(m_agingFrequency);
    const QSignalBlocker blowTimeBlocker(m_blowTime);
    const QSignalBlocker chargeTimeBlocker(m_chargeTime);
    const QSignalBlocker stopChargeTimeBlocker(m_stopChargeTime);
    const QSignalBlocker rechargeTimeBlocker(m_rechargeTime);
    const QSignalBlocker channelCountBlocker(m_channelCount);
    const QSignalBlocker independentChannelEnableBlocker(m_independentChannelEnable);

    m_operationModeValue = parameters.operationMode == kOperationModeAging ? kOperationModeAging : kOperationModeTest;
    updateOperationModeButtons();

    if (m_valveSwitch != nullptr) {
        m_valveSwitch->setChecked(parameters.valveSwitch != 0);
        updateValveSwitchButton();
    }
    if (m_blowCount != nullptr) {
        m_blowCount->setValue(parameters.blowCount);
    }
    if (m_blowInterval != nullptr) {
        m_blowInterval->setValue(parameters.blowIntervalMs);
    }
    if (m_testFrequency != nullptr) {
        m_testFrequency->setValue(parameters.testFrequencyHz);
    }
    if (m_agingFrequency != nullptr) {
        m_agingFrequency->setValue(parameters.agingFrequencyHz);
    }
    updateOperationModeUi(false);
    if (m_triggerMode != nullptr) {
        int triggerMode = parameters.triggerMode;
        if (m_triggerMode->findData(triggerMode) < 0) {
            triggerMode = defaultTriggerModeForOperationMode(m_operationModeValue);
        }
        setComboData(m_triggerMode, triggerMode);
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
        makePacket(CommandMap::kBlowCount, quint16(effectiveBlowCountForCurrentMode())),
        makePacket(CommandMap::kBlowInterval, quint16(effectiveBlowIntervalMsForCurrentMode())),
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

void ParameterPanelWidget::applyOperationModeSelection(const int operationMode)
{
    if (m_operationModeValue == operationMode) {
        return;
    }

    m_operationModeValue = operationMode == kOperationModeAging ? kOperationModeAging : kOperationModeTest;
    updateOperationModeUi(false);
    emit generalBatchRequested();
    emitControlParametersChanged();
}

void ParameterPanelWidget::emitControlParametersChanged()
{
    emit controlParametersChanged(currentControlParameters());
}

void ParameterPanelWidget::updateOperationModeUi(const bool syncCommands)
{
    updateOperationModeButtons();
    rebuildTriggerModeOptions(syncCommands);

    const bool agingMode = m_operationModeValue == kOperationModeAging;

    if (m_blowCountRow != nullptr) {
        m_blowCountRow->setVisible(agingMode);
    }
    if (m_blowIntervalRow != nullptr) {
        m_blowIntervalRow->setVisible(false);
    }
    if (m_testFrequencyRow != nullptr) {
        m_testFrequencyRow->setVisible(!agingMode);
    }
    if (m_agingFrequencyRow != nullptr) {
        m_agingFrequencyRow->setVisible(agingMode);
    }
}

void ParameterPanelWidget::updateOperationModeButtons()
{
    if (m_testModeButton != nullptr) {
        m_testModeButton->setChecked(m_operationModeValue == kOperationModeTest);
    }
    if (m_agingModeButton != nullptr) {
        m_agingModeButton->setChecked(m_operationModeValue == kOperationModeAging);
    }
}

void ParameterPanelWidget::rebuildTriggerModeOptions(const bool syncCommand)
{
    if (m_triggerMode == nullptr) {
        return;
    }

    const int previousValue = m_triggerMode->currentData().toInt();
    const QSignalBlocker blocker(m_triggerMode);
    m_triggerMode->clear();

    if (m_operationModeValue == kOperationModeAging) {
        m_triggerMode->addItem(QStringLiteral("三抽一分开"), kTriggerTripleSeparate);
        m_triggerMode->addItem(QStringLiteral("三抽一同时"), kTriggerTripleSimultaneous);
    } else {
        m_triggerMode->addItem(QStringLiteral("单次触发"), kTriggerSingleShot);
        m_triggerMode->addItem(QStringLiteral("单次递增"), kTriggerSingleIncrement);
        m_triggerMode->addItem(QStringLiteral("单次循环"), kTriggerSingleCycle);
        m_triggerMode->addItem(QStringLiteral("循环递增"), kTriggerIncrementCycle);
    }

    int targetValue = previousValue;
    if (syncCommand) {
        targetValue = defaultTriggerModeForOperationMode(m_operationModeValue);
    } else if (m_triggerMode->findData(targetValue) < 0) {
        targetValue = defaultTriggerModeForOperationMode(m_operationModeValue);
    }

    setComboData(m_triggerMode, targetValue);
    const int currentValue = m_triggerMode->currentData().toInt();
    if (syncCommand) {
        emitParameterCommand(CommandMap::kTriggerMode, quint16(currentValue), false);
    }
}

void ParameterPanelWidget::updateValveSwitchButton()
{
    if (m_valveSwitch == nullptr) {
        return;
    }

    m_valveSwitch->setText(m_valveSwitch->isChecked() ? QStringLiteral("开启") : QStringLiteral("关闭"));
}

int ParameterPanelWidget::effectiveBlowCountForCurrentMode() const
{
    return m_operationModeValue == kOperationModeAging
        ? (m_blowCount != nullptr ? m_blowCount->value() : 1)
        : 1;
}

int ParameterPanelWidget::effectiveBlowIntervalMsForCurrentMode() const
{
    if (m_operationModeValue == kOperationModeAging) {
        return intervalMsFromAgingFrequency(m_agingFrequency != nullptr ? m_agingFrequency->value() : 100);
    }

    return intervalMsFromAgingFrequency(m_testFrequency != nullptr ? m_testFrequency->value() : 100);
}

int ParameterPanelWidget::agingFrequencyFromIntervalMs(const int intervalMs)
{
    return qBound(1, qRound(1000.0 / qMax(1, intervalMs)), 1000);
}

int ParameterPanelWidget::intervalMsFromAgingFrequency(const int frequency)
{
    return qBound(1, qRound(1000.0 / qMax(1, frequency)), 65535);
}

quint16 ParameterPanelWidget::timingMsToRaw(const double milliseconds)
{
    return quint16(qBound(0, qRound(milliseconds / 0.05), 65535));
}
