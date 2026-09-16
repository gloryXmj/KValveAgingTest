#include "src/ui/MainWindow.h"

#include "src/app/SerialWorkerRuntime.h"
#include "src/config/AppSettings.h"
#include "src/core/CommandMap.h"
#include "src/core/SecurityPolicy.h"
#include "src/core/ValveAddressResolver.h"
#include "src/serial/SerialPortTypes.h"
#include "src/ui/ChannelCardWidget.h"
#include "src/ui/LogPanelWidget.h"
#include "src/ui/ParameterPanelWidget.h"
#include "src/ui/PasswordDialog.h"
#include "src/ui/TouchSpinBoxWidget.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>

#include <optional>

namespace
{
std::optional<quint8> extractProtocolCommand(const QString &hex)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QStringList parts = hex.split(QChar(' '), Qt::SkipEmptyParts);
#else
    const QStringList parts = hex.split(QChar(' '), QString::SkipEmptyParts);
#endif
    if (parts.size() < 2) {
        return std::nullopt;
    }

    bool ok = false;
    const int command = parts.at(1).toInt(&ok, 16);
    if (!ok) {
        return std::nullopt;
    }

    return quint8(command);
}

bool isValveFeedbackLog(const QString &direction, const QString &hex)
{
    if (direction != QStringLiteral("RX")) {
        return false;
    }

    const auto command = extractProtocolCommand(hex);
    return command.has_value() && CommandMap::isValveCommand(*command);
}

void prepareTouchComboBox(QComboBox *comboBox, const int minWidth, const int maxVisibleItems = 10)
{
    if (comboBox == nullptr) {
        return;
    }

    auto *view = new QListView(comboBox);
    view->setSpacing(4);
    view->setUniformItemSizes(true);
    comboBox->setView(view);
    comboBox->setMinimumHeight(44);
    comboBox->setMinimumWidth(minWidth);
    comboBox->setMaxVisibleItems(maxVisibleItems);
}
}

MainWindow::MainWindow(SerialWorkerRuntime *runtime, AppSettings *settings, QWidget *parent)
    : QMainWindow(parent)
    , m_runtime(runtime)
    , m_settings(settings)
{
    if (m_settings != nullptr) {
        m_savedSerialSettings = m_settings->loadSerialSettings();
        m_lastConnectedSerialSettings = m_settings->loadLastConnectedSerialSettings();
        m_savedControlParameters = m_settings->loadControlParameters();
        m_savedVisibleValveCount = m_settings->loadVisibleValveCount();
    } else {
        m_lastConnectedSerialSettings = SerialPortSettings{};
        m_savedControlParameters = ControlParameters{};
        m_savedVisibleValveCount = CommandMap::kValvesPerChannel;
    }

    m_autoConnectPending = !m_lastConnectedSerialSettings.portName.isEmpty();
    m_autoApplyGeneralParametersPending = false;

    m_savedVisibleValveCount = qBound(
        1,
        m_savedVisibleValveCount > 0 ? m_savedVisibleValveCount : CommandMap::kValvesPerChannel,
        CommandMap::kValvesPerChannel
    );

    buildUi();
    updateChannelAlarmDisplay();

    if (m_runtime != nullptr) {
        connect(this, &MainWindow::requestAvailablePorts, m_runtime, &SerialWorkerRuntime::requestAvailablePorts, Qt::QueuedConnection);
        connect(this, &MainWindow::requestOpenPort, m_runtime, &SerialWorkerRuntime::requestOpenPort, Qt::QueuedConnection);
        connect(this, &MainWindow::requestClosePort, m_runtime, &SerialWorkerRuntime::requestClosePort, Qt::QueuedConnection);
        connect(this, &MainWindow::requestCancelSingleValveCycleCommands, m_runtime, &SerialWorkerRuntime::requestCancelSingleValveCycleCommands, Qt::QueuedConnection);
        connect(this, &MainWindow::requestEnqueueCommand, m_runtime, &SerialWorkerRuntime::requestEnqueueCommand, Qt::QueuedConnection);

        connect(m_runtime, &SerialWorkerRuntime::portsReady, this, &MainWindow::handlePortsReady, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::connectionChanged, this, [this](const bool connected) {
            m_connected = connected;
            if (!connected) {
                stopAllSingleValveCycleTests();
                if (m_manualContinuousChannel > 0) {
                    clearContinuousValve(m_manualContinuousChannel);
                    m_manualContinuousChannel = 0;
                    m_manualContinuousValve = 0;
                }
                m_abnormalChannels.clear();
                m_hasCommunicationFault = true;
            }

            updateConnectionBadge(connected);
            updateChannelAlarmDisplay();
            m_connectButton->setText(connected ? QStringLiteral("断开连接") : QStringLiteral("连接串口"));
            statusBar()->showMessage(connected ? QStringLiteral("串口已连接。") : QStringLiteral("串口已断开。"), 3000);

            if (connected && m_settings != nullptr) {
                m_settings->saveSerialSettings(currentSerialSettings());
                m_settings->saveLastConnectedSerialSettings(currentSerialSettings());
            }
            if (connected && m_autoApplyGeneralParametersPending && m_parameterPanel != nullptr) {
                enqueueCommands(m_parameterPanel->generalParameterCommands());
                m_autoApplyGeneralParametersPending = false;
            }
        }, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::logGenerated, this, [this](const QString &direction, const QString &hex, const QString &description) {
            const bool valveFeedback = isValveFeedbackLog(direction, hex);

            if (direction == QStringLiteral("RX")) {
                if (m_hasCommunicationFault) {
                    m_hasCommunicationFault = false;
                    updateChannelAlarmDisplay();
                }
            } else if (direction == QStringLiteral("ERR")
                || direction == QStringLiteral("TIMEOUT")
                || direction == QStringLiteral("WARN")) {
                m_hasCommunicationFault = true;
                updateChannelAlarmDisplay();
            }

            if (!valveFeedback) {
                m_logPanel->addLog(direction, hex, description);
            }
            if (direction != QStringLiteral("RX")) {
                statusBar()->showMessage(description, 5000);
            }
        }, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::commandRejected, this, [this](const QString &reason) {
            if (!m_connected) {
                m_autoApplyGeneralParametersPending = false;
            }
            statusBar()->showMessage(reason, 5000);
        }, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::commandSent, this, &MainWindow::handleCommandSent, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::versionReceived, this, [this](const QString &versionText) {
            m_firmwareValue->setText(versionText);
        }, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::valveActionConfirmed, this, [this](const int channel, const QList<int> &valves) {
            if (m_singleValveCycleStates.contains(channel)
                || m_manualContinuousChannel == channel) {
                return;
            }
            for (auto *card : m_channelCards) {
                if (card != nullptr && card->channelNumber() == channel) {
                    card->pulseValves(valves);
                    break;
                }
            }
        }, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::channelAlarmUpdated, this, [this](const QList<int> &channels) {
            m_hasCommunicationFault = false;
            m_abnormalChannels.clear();
            for (const int channel : channels) {
                m_abnormalChannels.insert(channel);
            }
            updateChannelAlarmDisplay();
        }, Qt::QueuedConnection);
    }

    refreshPorts();
    applySavedSettings();

    if (m_settings != nullptr) {
        const QByteArray geometry = m_settings->loadWindowGeometry();
        if (!geometry.isEmpty()) {
            restoreGeometry(geometry);
        }
    }

    statusBar()->showMessage(QStringLiteral("就绪"));
}

int MainWindow::channelCardCount() const
{
    return m_channelCards.size();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_settings != nullptr) {
        m_settings->saveWindowGeometry(saveGeometry());
        m_settings->saveSerialSettings(currentSerialSettings());
        if (m_parameterPanel != nullptr) {
            m_settings->saveControlParameters(m_parameterPanel->currentControlParameters());
        }
        m_settings->saveVisibleValveCount(m_visibleValveCountSpin != nullptr ? m_visibleValveCountSpin->value() : m_savedVisibleValveCount);
    }

    stopAllSingleValveCycleTests();
    if (m_manualContinuousChannel > 0) {
        clearContinuousValve(m_manualContinuousChannel);
        m_manualContinuousChannel = 0;
        m_manualContinuousValve = 0;
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("阀板老化测试工具"));
    resize(1680, 1040);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(20, 20, 20, 16);
    rootLayout->setSpacing(16);

    auto *topBar = new QFrame(central);
    topBar->setObjectName(QStringLiteral("topBar"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(20, 16, 20, 16);
    topLayout->setSpacing(12);

    auto *titleLayout = new QVBoxLayout();
    auto *heroTitle = new QLabel(QStringLiteral("阀板老化测试控制台"), topBar);
    heroTitle->setObjectName(QStringLiteral("heroTitle"));
    auto *heroSubtitle = new QLabel(QStringLiteral("支持 8 通道阀板控制、受保护时间参数设置与实时回包指示。"), topBar);
    heroSubtitle->setObjectName(QStringLiteral("heroSubtitle"));
    titleLayout->addWidget(heroTitle);
    titleLayout->addWidget(heroSubtitle);
    topLayout->addLayout(titleLayout, 1);

    const QString topBarLabelStyle = QStringLiteral("color: rgba(255,255,255,0.75);");

    auto *portLabel = new QLabel(QStringLiteral("串口"), topBar);
    portLabel->setObjectName(QStringLiteral("sectionCaption"));
    portLabel->setStyleSheet(topBarLabelStyle);
    topLayout->addWidget(portLabel);
    m_portCombo = new QComboBox(topBar);
    prepareTouchComboBox(m_portCombo, 220, 12);
    topLayout->addWidget(m_portCombo);

    auto *baudLabel = new QLabel(QStringLiteral("波特率"), topBar);
    baudLabel->setObjectName(QStringLiteral("sectionCaption"));
    baudLabel->setStyleSheet(topBarLabelStyle);
    topLayout->addWidget(baudLabel);
    m_baudCombo = new QComboBox(topBar);
    m_baudCombo->setEditable(true);
    m_baudCombo->addItems(QStringList{
        QStringLiteral("9600"),
        QStringLiteral("19200"),
        QStringLiteral("38400"),
        QStringLiteral("57600"),
        QStringLiteral("115200"),
        QStringLiteral("921600"),
    });
    m_baudCombo->setCurrentText(QStringLiteral("115200"));
    prepareTouchComboBox(m_baudCombo, 136, 8);
    topLayout->addWidget(m_baudCombo);

    auto *visibleValveLabel = new QLabel(QStringLiteral("显示阀数"), topBar);
    visibleValveLabel->setObjectName(QStringLiteral("sectionCaption"));
    visibleValveLabel->setStyleSheet(topBarLabelStyle);
    topLayout->addWidget(visibleValveLabel);
    m_visibleValveCountSpin = new QSpinBox(topBar);
    m_visibleValveCountSpin->setRange(1, CommandMap::kValvesPerChannel);
    m_visibleValveCountSpin->setValue(m_savedVisibleValveCount);
    topLayout->addWidget(
        new TouchSpinBoxWidget(m_visibleValveCountSpin, 88, QStringLiteral("输入显示阀数"), topBar));

    auto *refreshButton = new QPushButton(QStringLiteral("刷新串口"), topBar);
    refreshButton->setProperty("secondary", true);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    topLayout->addWidget(refreshButton);

    m_connectButton = new QPushButton(QStringLiteral("连接串口"), topBar);
    connect(m_connectButton, &QPushButton::clicked, this, [this]() {
        if (m_connected) {
            statusBar()->showMessage(QStringLiteral("正在断开串口..."), 3000);
            emit requestClosePort();
            return;
        }

        statusBar()->showMessage(QStringLiteral("正在连接串口..."), 3000);
        emit requestOpenPort(currentSerialSettings());
    });
    topLayout->addWidget(m_connectButton);

    m_connectionBadge = new QLabel(topBar);
    m_connectionBadge->setObjectName(QStringLiteral("connectionBadge"));
    topLayout->addWidget(m_connectionBadge);

    auto *firmwareLabel = new QLabel(QStringLiteral("固件版本"), topBar);
    firmwareLabel->setStyleSheet(QStringLiteral("color: rgba(255,255,255,0.75);"));
    topLayout->addWidget(firmwareLabel);
    m_firmwareValue = new QLabel(QStringLiteral("--"), topBar);
    m_firmwareValue->setStyleSheet(QStringLiteral("color: #FFFFFF; font-weight: 700;"));
    topLayout->addWidget(m_firmwareValue);
    rootLayout->addWidget(topBar);

    auto *contentSplitter = new QSplitter(Qt::Vertical, central);
    auto *upperWidget = new QWidget(contentSplitter);
    auto *upperLayout = new QHBoxLayout(upperWidget);
    upperLayout->setContentsMargins(0, 0, 0, 0);
    upperLayout->setSpacing(16);

    m_parameterScroll = new QScrollArea(upperWidget);
    m_parameterScroll->setWidgetResizable(true);
    m_parameterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_parameterScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_parameterScroll->setFixedWidth(448);

    m_parameterPanel = new ParameterPanelWidget(m_parameterScroll);
    m_parameterPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    connect(m_parameterPanel, &ParameterPanelWidget::commandRequested, this, &MainWindow::handleCommandRequest);
    connect(m_parameterPanel, &ParameterPanelWidget::triggerModeChanged, this, &MainWindow::handleTriggerModeChanged);
    connect(m_parameterPanel, &ParameterPanelWidget::singleValveCycleToggled, this, &MainWindow::handleSingleValveCycleToggled);
    connect(m_parameterPanel, &ParameterPanelWidget::generalBatchRequested, this, [this]() {
        stopAllSingleValveCycleTests();
        if (m_parameterPanel != nullptr) {
            m_parameterPanel->setSingleValveCycleRunning(false);
        }
        if (m_manualContinuousChannel > 0) {
            const auto offPacket = ValveAddressResolver::buildValveOffCommand(
                m_manualContinuousChannel,
                m_manualContinuousValve);
            if (offPacket.has_value()) {
                emit requestEnqueueCommand(*offPacket, QString());
            }
            clearContinuousValve(m_manualContinuousChannel);
            m_manualContinuousChannel = 0;
            m_manualContinuousValve = 0;
        }
        if (m_parameterPanel != nullptr) {
            enqueueCommands(m_parameterPanel->generalParameterCommands());
        }
    });
    connect(m_parameterPanel, &ParameterPanelWidget::timingBatchRequested, this, &MainWindow::handleTimingBatchRequest);
    connect(m_parameterPanel, &ParameterPanelWidget::controlParametersChanged, this, [this](const ControlParameters &parameters) {
        if (m_settings != nullptr) {
            m_settings->saveControlParameters(parameters);
        }
        updateChannelCardLayout(parameters.channelCount);
    });
    m_parameterScroll->setWidget(m_parameterPanel);
    upperLayout->addWidget(m_parameterScroll);

    m_cardsScroll = new QScrollArea(upperWidget);
    m_cardsScroll->setWidgetResizable(true);
    m_cardsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_cardsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_cardsPage = new QWidget(m_cardsScroll);
    m_cardsPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_cardsLayout = new QGridLayout(m_cardsPage);
    m_cardsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setHorizontalSpacing(20);
    m_cardsLayout->setVerticalSpacing(20);
    m_cardsLayout->setColumnStretch(0, 1);
    m_cardsLayout->setColumnStretch(1, 1);

    for (int channel = 1; channel <= 8; ++channel) {
        auto *card = new ChannelCardWidget(channel, m_cardsPage);
        card->setVisibleValveCount(m_savedVisibleValveCount);
        connect(card, &ChannelCardWidget::valveInvoked, this, &MainWindow::handleValveInvoked);

        const int zeroBased = channel - 1;
        m_cardsLayout->addWidget(card, zeroBased / 2, zeroBased % 2);
        m_channelCards.append(card);
    }

    m_cardsScroll->setWidget(m_cardsPage);
    upperLayout->addWidget(m_cardsScroll, 1);
    upperLayout->setStretch(0, 0);
    upperLayout->setStretch(1, 1);
    contentSplitter->addWidget(upperWidget);

    m_logPanel = new LogPanelWidget(contentSplitter);
    contentSplitter->addWidget(m_logPanel);
    contentSplitter->setStretchFactor(0, 8);
    contentSplitter->setStretchFactor(1, 3);
    rootLayout->addWidget(contentSplitter, 1);

    setCentralWidget(central);
    updateConnectionBadge(false);

    connect(m_visibleValveCountSpin, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::applyVisibleValveCount);
}

void MainWindow::refreshPorts()
{
    emit requestAvailablePorts();
}

void MainWindow::applySavedSettings()
{
    if (m_savedSerialSettings.baudRate > 0) {
        m_baudCombo->setCurrentText(QString::number(m_savedSerialSettings.baudRate));
    }

    if (!m_savedSerialSettings.portName.isEmpty()) {
        const int index = m_portCombo->findData(m_savedSerialSettings.portName);
        if (index >= 0) {
            m_portCombo->setCurrentIndex(index);
        }
    }

    if (m_visibleValveCountSpin != nullptr) {
        m_visibleValveCountSpin->setValue(m_savedVisibleValveCount);
    }

    if (m_parameterPanel != nullptr) {
        m_parameterPanel->applyControlParameters(m_savedControlParameters);
    }

    updateChannelCardLayout(m_savedControlParameters.channelCount);
}

void MainWindow::applyVisibleValveCount(const int count)
{
    stopAllSingleValveCycleTests();
    if (m_manualContinuousChannel > 0) {
        const auto offPacket = ValveAddressResolver::buildValveOffCommand(
            m_manualContinuousChannel,
            m_manualContinuousValve);
        if (offPacket.has_value()) {
            emit requestEnqueueCommand(*offPacket, QString());
        }
        clearContinuousValve(m_manualContinuousChannel);
        m_manualContinuousChannel = 0;
        m_manualContinuousValve = 0;
    }
    m_savedVisibleValveCount = qBound(1, count, CommandMap::kValvesPerChannel);

    for (auto *card : m_channelCards) {
        card->setVisibleValveCount(m_savedVisibleValveCount);
        card->adjustSize();
    }

    if (m_cardsPage != nullptr) {
        if (auto *layout = m_cardsPage->layout(); layout != nullptr) {
            layout->activate();
        }
        m_cardsPage->adjustSize();
        m_cardsPage->updateGeometry();
    }

    if (m_cardsScroll != nullptr) {
        m_cardsScroll->widget()->updateGeometry();
        m_cardsScroll->viewport()->update();
    }

    if (m_settings != nullptr) {
        m_settings->saveVisibleValveCount(m_savedVisibleValveCount);
    }
}

void MainWindow::updateChannelCardLayout(const int channelCount)
{
    if (m_cardsLayout == nullptr || m_cardsPage == nullptr) {
        return;
    }

    const int activeChannelCount = qBound(1, channelCount, CommandMap::kChannelCountSupported);
    const bool singleChannelMode = activeChannelCount == 1;

    for (auto *card : m_channelCards) {
        if (card != nullptr) {
            m_cardsLayout->removeWidget(card);
        }
    }

    m_cardsLayout->setHorizontalSpacing(singleChannelMode ? 0 : 20);
    m_cardsLayout->setVerticalSpacing(singleChannelMode ? 0 : 20);
    m_cardsLayout->setColumnStretch(0, 1);
    m_cardsLayout->setColumnStretch(1, singleChannelMode ? 0 : 1);

    for (int index = 0; index < m_channelCards.size(); ++index) {
        auto *card = m_channelCards.at(index);
        if (card == nullptr) {
            continue;
        }

        const bool visible = index < activeChannelCount;
        card->setVisible(visible);
        card->setLargeTouchMode(singleChannelMode && index == 0);
        if (!visible) {
            continue;
        }

        if (singleChannelMode) {
            m_cardsLayout->addWidget(card, 0, 0, 1, 2);
        } else {
            m_cardsLayout->addWidget(card, index / 2, index % 2);
        }
    }

    m_cardsLayout->activate();
    m_cardsPage->adjustSize();
    m_cardsPage->updateGeometry();
    if (m_cardsScroll != nullptr) {
        m_cardsScroll->viewport()->update();
    }
}

void MainWindow::updateConnectionBadge(const bool connected)
{
    m_connectionBadge->setProperty("connected", connected);
    m_connectionBadge->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
    m_connectionBadge->style()->unpolish(m_connectionBadge);
    m_connectionBadge->style()->polish(m_connectionBadge);
    m_connectionBadge->update();
}

void MainWindow::updateChannelAlarmDisplay()
{
    const bool showAllAbnormal = !m_connected || m_hasCommunicationFault;
    for (auto *card : m_channelCards) {
        const bool abnormal = showAllAbnormal || m_abnormalChannels.contains(card->channelNumber());
        card->setAbnormal(abnormal);
    }
}

void MainWindow::handlePortsReady(const QVector<SerialPortDescriptor> &ports)
{
    const QString selectedPort = m_portCombo->currentData().toString().isEmpty()
        ? m_portCombo->currentText()
        : m_portCombo->currentData().toString();

    m_portCombo->clear();
    for (const auto &port : ports) {
        const QString label = port.description.isEmpty()
            ? port.name
            : QStringLiteral("%1  |  %2").arg(port.name, port.description);
        m_portCombo->addItem(label, port.name);
    }

    if (!selectedPort.isEmpty()) {
        const int index = m_portCombo->findData(selectedPort);
        if (index >= 0) {
            m_portCombo->setCurrentIndex(index);
        }
    }

    if (!m_savedSerialSettings.portName.isEmpty()) {
        const int savedIndex = m_portCombo->findData(m_savedSerialSettings.portName);
        if (savedIndex >= 0) {
            m_portCombo->setCurrentIndex(savedIndex);
        }
    }

    if (m_autoConnectPending && !m_connected) {
        m_autoConnectPending = false;

        const int autoConnectIndex = m_portCombo->findData(m_lastConnectedSerialSettings.portName);
        if (autoConnectIndex >= 0) {
            m_portCombo->setCurrentIndex(autoConnectIndex);
            m_autoApplyGeneralParametersPending = true;
            statusBar()->showMessage(QStringLiteral("正在自动连接上次成功连接的串口..."), 3000);
            emit requestOpenPort(m_lastConnectedSerialSettings);
        } else {
            m_autoApplyGeneralParametersPending = false;
            statusBar()->showMessage(QStringLiteral("未找到上次成功连接的串口，已跳过自动连接。"), 5000);
        }
    }
}

void MainWindow::handleCommandRequest(const CommandPacket &packet, const bool passwordRequired)
{
    if (!passwordRequired) {
        emit requestEnqueueCommand(packet, QString());
        return;
    }

    QString password;
    if (!requestProtectedPassword(
            QStringLiteral("该时间参数受密码保护，请输入固定密码后继续。"),
            QStringLiteral("密码校验通过，已开始下发受保护参数。"),
            QStringLiteral("已取消受保护参数下发。"),
            &password)) {
        return;
    }

    emit requestEnqueueCommand(packet, password);
}

void MainWindow::handleTimingBatchRequest()
{
    if (m_parameterPanel == nullptr) {
        return;
    }

    QString password;
    if (!requestProtectedPassword(
            QStringLiteral("时间参数受密码保护，请输入固定密码后继续。"),
            QStringLiteral("密码校验通过，已开始下发时间参数。"),
            QStringLiteral("已取消时间参数下发。"),
            &password)) {
        return;
    }

    enqueueCommands(m_parameterPanel->timingParameterCommands(), password);
}

ChannelCardWidget *MainWindow::channelCard(const int channel) const
{
    for (auto *card : m_channelCards) {
        if (card != nullptr && card->channelNumber() == channel) {
            return card;
        }
    }
    return nullptr;
}

void MainWindow::setContinuousValve(const int channel, const int valveNumber)
{
    if (auto *card = channelCard(channel); card != nullptr) {
        card->setContinuousValve(valveNumber);
    }
}

void MainWindow::clearContinuousValve(const int channel)
{
    if (auto *card = channelCard(channel); card != nullptr) {
        card->clearContinuousValve();
    }
}

void MainWindow::handleValveInvoked(const int channel, const int valveNumber)
{
    stopAllSingleValveCycleTests();

    const bool continuousValveMode = m_parameterPanel != nullptr
        && m_parameterPanel->currentControlParameters().operationMode == 0
        && m_parameterPanel->currentControlParameters().triggerMode
            == ParameterPanelWidget::kSoftwareSingleValveCycleMode;

    const auto packet = ValveAddressResolver::buildSingleValveCommand(channel, valveNumber);
    if (!packet.has_value()) {
        statusBar()->showMessage(QStringLiteral("通道或阀号无效。"), 5000);
        return;
    }

    if (!continuousValveMode) {
        emit requestEnqueueCommand(*packet, QString());
        statusBar()->showMessage(
            QStringLiteral("已发送通道 %1 的阀 %2 测试命令。").arg(channel).arg(valveNumber),
            3000);
        return;
    }

    if (m_manualContinuousChannel == channel && m_manualContinuousValve == valveNumber) {
        const auto offPacket = ValveAddressResolver::buildValveOffCommand(channel, valveNumber);
        if (offPacket.has_value()) {
            emit requestEnqueueCommand(*offPacket, QString());
        }
        clearContinuousValve(channel);
        m_manualContinuousChannel = 0;
        m_manualContinuousValve = 0;
        statusBar()->showMessage(
            QStringLiteral("已停止通道 %1 的阀 %2 持续吹气。").arg(channel).arg(valveNumber),
            3000);
        return;
    }

    if (m_manualContinuousChannel > 0) {
        const auto offPacket = ValveAddressResolver::buildValveOffCommand(
            m_manualContinuousChannel,
            m_manualContinuousValve);
        if (offPacket.has_value()) {
            emit requestEnqueueCommand(*offPacket, QString());
        }
        clearContinuousValve(m_manualContinuousChannel);
    }

    if (m_parameterPanel != nullptr) {
        enqueueCommands(m_parameterPanel->singleValveCycleCommands());
    }
    emit requestEnqueueCommand(*packet, QString());
    setContinuousValve(channel, valveNumber);
    m_manualContinuousChannel = channel;
    m_manualContinuousValve = valveNumber;
    statusBar()->showMessage(
        QStringLiteral("通道 %1 的阀 %2 正在持续吹气，再次点击可停止。").arg(channel).arg(valveNumber),
        3000);
}

void MainWindow::handleTriggerModeChanged(const int triggerMode)
{
    stopAllSingleValveCycleTests();

    if (m_manualContinuousChannel > 0) {
        const auto offPacket = ValveAddressResolver::buildValveOffCommand(
            m_manualContinuousChannel,
            m_manualContinuousValve);
        if (offPacket.has_value()) {
            emit requestEnqueueCommand(*offPacket, QString());
        }
        clearContinuousValve(m_manualContinuousChannel);
        m_manualContinuousChannel = 0;
        m_manualContinuousValve = 0;
    }

    if (triggerMode == ParameterPanelWidget::kSoftwareSingleValveCycleMode
        && m_parameterPanel != nullptr) {
        enqueueCommands(m_parameterPanel->singleValveCycleCommands());
    }
}

void MainWindow::handleSingleValveCycleToggled(const bool running)
{
    if (running) {
        startSingleValveCycleTests();
    } else {
        stopAllSingleValveCycleTests();
    }
}

void MainWindow::startSingleValveCycleTests()
{
    if (m_parameterPanel == nullptr) {
        return;
    }

    const ControlParameters parameters = m_parameterPanel->currentControlParameters();
    if (parameters.operationMode != 0
        || parameters.triggerMode != ParameterPanelWidget::kSoftwareSingleValveCycleMode) {
        m_parameterPanel->setSingleValveCycleRunning(false);
        statusBar()->showMessage(QStringLiteral("单阀循环递增仅支持测试模式。"), 5000);
        return;
    }

    stopAllSingleValveCycleTests();

    if (m_manualContinuousChannel > 0) {
        const auto offPacket = ValveAddressResolver::buildValveOffCommand(
            m_manualContinuousChannel,
            m_manualContinuousValve);
        if (offPacket.has_value()) {
            emit requestEnqueueCommand(*offPacket, QString());
        }
        clearContinuousValve(m_manualContinuousChannel);
        m_manualContinuousChannel = 0;
        m_manualContinuousValve = 0;
    }

    enqueueCommands(m_parameterPanel->singleValveCycleCommands());

    const int channelCount = qBound(1, parameters.channelCount, CommandMap::kChannelCountSupported);
    for (int channel = 1; channel <= channelCount; ++channel) {
        startSingleValveCycleTest(channel);
    }

    if (m_singleValveCycleStates.isEmpty()) {
        m_parameterPanel->setSingleValveCycleRunning(false);
        return;
    }

    m_parameterPanel->setSingleValveCycleRunning(true);
    statusBar()->showMessage(QStringLiteral("已开始单阀循环递增。"), 3000);
}

void MainWindow::startSingleValveCycleTest(const int channel)
{
    if (m_parameterPanel == nullptr) {
        return;
    }

    const auto packet = ValveAddressResolver::buildSingleValveCommand(channel, 1);
    if (!packet.has_value()) {
        return;
    }

    SingleValveCycleState state;
    state.nextValve = 1;
    m_singleValveCycleStates.insert(channel, state);

    auto *timer = m_singleValveCycleTimers.value(channel, nullptr);
    if (timer == nullptr) {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        m_singleValveCycleTimers.insert(channel, timer);
        connect(timer, &QTimer::timeout, this, [this, channel]() {
            handleSingleValveCycleTimeout(channel);
        });
    }

    enqueueSingleValveCycleOpenCommand(channel, *packet);
}

void MainWindow::stopSingleValveCycleTest(const int channel, const bool sendOff)
{
    if (auto *timer = m_singleValveCycleTimers.value(channel, nullptr); timer != nullptr) {
        timer->stop();
    }

    const SingleValveCycleState state = m_singleValveCycleStates.take(channel);
    if (sendOff) {
        QList<int> valvesToClose;
        if (state.currentValve > 0) {
            valvesToClose.append(state.currentValve);
        }
        if (state.pendingOpenRequestId != 0
            && state.nextValve > 0
            && !valvesToClose.contains(state.nextValve)) {
            valvesToClose.append(state.nextValve);
        }

        for (const int valveNumber : valvesToClose) {
            const auto offPacket = ValveAddressResolver::buildValveOffCommand(channel, valveNumber);
            if (offPacket.has_value()) {
                emit requestEnqueueCommand(*offPacket, QString());
            }
        }
    }
    if (state.currentValve > 0 || state.nextValve > 0) {
        clearContinuousValve(channel);
    }
}

void MainWindow::stopAllSingleValveCycleTests()
{
    emit requestCancelSingleValveCycleCommands();
    const QList<int> channels = m_singleValveCycleStates.keys();
    for (const int channel : channels) {
        stopSingleValveCycleTest(channel);
    }
    if (m_parameterPanel != nullptr) {
        m_parameterPanel->setSingleValveCycleRunning(false);
    }
}

void MainWindow::handleSingleValveCycleTimeout(const int channel)
{
    auto stateIt = m_singleValveCycleStates.find(channel);
    if (stateIt == m_singleValveCycleStates.end()
        || stateIt->pendingOpenRequestId != 0
        || stateIt->currentValve <= 0
        || m_parameterPanel == nullptr) {
        return;
    }

    const int oldValve = stateIt->currentValve;
    const int valveCount = qBound(1, m_savedVisibleValveCount, CommandMap::kValvesPerChannel);
    const int nextValve = oldValve >= valveCount ? 1 : oldValve + 1;

    const auto offPacket = ValveAddressResolver::buildValveOffCommand(channel, oldValve);
    const auto onPacket = ValveAddressResolver::buildSingleValveCommand(channel, nextValve);
    if (!offPacket.has_value() || !onPacket.has_value()) {
        stopSingleValveCycleTest(channel, false);
        return;
    }

    stateIt->nextValve = nextValve;
    CommandPacket trackedOffPacket = *offPacket;
    trackedOffPacket.purpose = CommandPurpose::SingleValveCycleAction;
    emit requestEnqueueCommand(trackedOffPacket, QString());
    enqueueSingleValveCycleOpenCommand(channel, *onPacket);
}

void MainWindow::handleCommandSent(const CommandPacket &packet)
{
    if (packet.purpose != CommandPurpose::SingleValveCycleAction) {
        return;
    }

    const auto resolved = ValveAddressResolver::resolveValveAck(packet.command, packet.data16);
    if (!resolved.has_value()) {
        return;
    }

    const int channel = resolved->channel;
    auto stateIt = m_singleValveCycleStates.find(channel);
    if (stateIt == m_singleValveCycleStates.end()
        || stateIt->pendingOpenRequestId != packet.requestId) {
        return;
    }

    stateIt->pendingOpenRequestId = 0;
    stateIt->currentValve = stateIt->nextValve;
    setContinuousValve(channel, stateIt->currentValve);

    if (auto *timer = m_singleValveCycleTimers.value(channel, nullptr); timer != nullptr) {
        timer->start(qMax(1, m_parameterPanel->singleValveTestTimeMs()));
    }
}

void MainWindow::enqueueSingleValveCycleOpenCommand(
    const int channel,
    const CommandPacket &packet)
{
    auto stateIt = m_singleValveCycleStates.find(channel);
    if (stateIt == m_singleValveCycleStates.end()) {
        return;
    }

    CommandPacket trackedPacket = packet;
    trackedPacket.purpose = CommandPurpose::SingleValveCycleAction;
    trackedPacket.requestId = m_nextCycleRequestId++;
    if (m_nextCycleRequestId == 0) {
        m_nextCycleRequestId = 1;
    }

    stateIt->pendingOpenRequestId = trackedPacket.requestId;
    emit requestEnqueueCommand(trackedPacket, QString());
}

bool MainWindow::requestProtectedPassword(
    const QString &prompt,
    const QString &successMessage,
    const QString &cancelMessage,
    QString *password)
{
    PasswordDialog dialog(prompt, this);
    if (dialog.exec() != QDialog::Accepted) {
        statusBar()->showMessage(cancelMessage, 3000);
        return false;
    }

    const QString inputPassword = dialog.password();
    if (!SecurityPolicy::isPasswordValid(inputPassword)) {
        QMessageBox::warning(this, QStringLiteral("密码错误"), QStringLiteral("密码校验失败，参数未下发。"));
        statusBar()->showMessage(QStringLiteral("密码校验失败，参数未下发。"), 5000);
        return false;
    }

    if (password != nullptr) {
        *password = inputPassword;
    }
    QMessageBox::information(this, QStringLiteral("密码通过"), successMessage);
    statusBar()->showMessage(successMessage, 5000);
    return true;
}

void MainWindow::enqueueCommands(const QVector<CommandPacket> &packets, const QString &password)
{
    for (const auto &packet : packets) {
        emit requestEnqueueCommand(packet, password);
    }
}

SerialPortSettings MainWindow::currentSerialSettings() const
{
    SerialPortSettings settings = m_savedSerialSettings;
    settings.portName = m_portCombo->currentData().toString();
    if (settings.portName.isEmpty()) {
        settings.portName = m_portCombo->currentText();
    }
    settings.baudRate = m_baudCombo->currentText().toInt();
    if (settings.baudRate <= 0) {
        settings.baudRate = 115200;
    }
    settings.dataBits = 8;
    settings.stopBits = 1;
    settings.parity = 0;
    settings.flowControl = 0;
    return settings;
}
