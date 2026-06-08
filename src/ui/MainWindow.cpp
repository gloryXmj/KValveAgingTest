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
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
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
        connect(this, &MainWindow::requestEnqueueCommand, m_runtime, &SerialWorkerRuntime::requestEnqueueCommand, Qt::QueuedConnection);

        connect(m_runtime, &SerialWorkerRuntime::portsReady, this, &MainWindow::handlePortsReady, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::connectionChanged, this, [this](const bool connected) {
            m_connected = connected;
            if (!connected) {
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
        connect(m_runtime, &SerialWorkerRuntime::versionReceived, this, [this](const QString &versionText) {
            m_firmwareValue->setText(versionText);
        }, Qt::QueuedConnection);
        connect(m_runtime, &SerialWorkerRuntime::valveActionConfirmed, this, [this](const int channel, const QList<int> &valves) {
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

    auto *parameterScroll = new QScrollArea(upperWidget);
    parameterScroll->setWidgetResizable(true);
    parameterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    parameterScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    parameterScroll->setFixedWidth(448);

    m_parameterPanel = new ParameterPanelWidget(parameterScroll);
    m_parameterPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    connect(m_parameterPanel, &ParameterPanelWidget::commandRequested, this, &MainWindow::handleCommandRequest);
    connect(m_parameterPanel, &ParameterPanelWidget::timingBatchRequested, this, &MainWindow::handleTimingBatchRequest);
    connect(m_parameterPanel, &ParameterPanelWidget::controlParametersChanged, this, [this](const ControlParameters &parameters) {
        if (m_settings != nullptr) {
            m_settings->saveControlParameters(parameters);
        }
    });
    parameterScroll->setWidget(m_parameterPanel);
    upperLayout->addWidget(parameterScroll);

    m_cardsScroll = new QScrollArea(upperWidget);
    m_cardsScroll->setWidgetResizable(true);
    m_cardsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_cardsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_cardsPage = new QWidget(m_cardsScroll);
    m_cardsPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto *cardsLayout = new QGridLayout(m_cardsPage);
    cardsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setHorizontalSpacing(20);
    cardsLayout->setVerticalSpacing(20);
    cardsLayout->setColumnStretch(0, 1);
    cardsLayout->setColumnStretch(1, 1);

    for (int channel = 1; channel <= 8; ++channel) {
        auto *card = new ChannelCardWidget(channel, m_cardsPage);
        card->setVisibleValveCount(m_savedVisibleValveCount);
        connect(card, &ChannelCardWidget::valveInvoked, this, [this](const int invokedChannel, const int valveNumber) {
            const auto packet = ValveAddressResolver::buildSingleValveCommand(invokedChannel, valveNumber);
            if (!packet.has_value()) {
                statusBar()->showMessage(QStringLiteral("通道或阀号无效。"), 5000);
                return;
            }

            emit requestEnqueueCommand(*packet, QString());
        });

        const int zeroBased = channel - 1;
        cardsLayout->addWidget(card, zeroBased / 2, zeroBased % 2);
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
}

void MainWindow::applyVisibleValveCount(const int count)
{
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

    PasswordDialog dialog(
        QStringLiteral("该时间参数受密码保护，请输入固定密码后继续。"),
        this
    );
    if (dialog.exec() == QDialog::Accepted) {
        emit requestEnqueueCommand(packet, dialog.password());
    } else {
        statusBar()->showMessage(QStringLiteral("已取消受保护参数下发。"), 3000);
    }
}

void MainWindow::handleTimingBatchRequest()
{
    if (m_parameterPanel == nullptr) {
        return;
    }

    PasswordDialog dialog(
        QStringLiteral("时间参数受密码保护，请输入固定密码后继续。"),
        this
    );
    if (dialog.exec() != QDialog::Accepted) {
        statusBar()->showMessage(QStringLiteral("已取消时间参数下发。"), 3000);
        return;
    }

    if (!SecurityPolicy::isPasswordValid(dialog.password())) {
        statusBar()->showMessage(QStringLiteral("密码校验失败。"), 5000);
        return;
    }

    enqueueCommands(m_parameterPanel->timingParameterCommands(), dialog.password());
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
