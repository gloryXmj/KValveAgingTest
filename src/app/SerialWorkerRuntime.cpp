#include "src/app/SerialWorkerRuntime.h"

#include "src/controller/ValveTestController.h"
#include "src/serial/CSerialPortAdapter.h"
#include "src/serial/SerialPortService.h"

SerialWorkerRuntime::SerialWorkerRuntime(QObject *parent)
    : QObject(parent)
    , m_transport(new CSerialPortAdapter(this))
    , m_serialService(new SerialPortService(m_transport, this))
    , m_controller(new ValveTestController(m_serialService, this))
{
    connect(m_controller, &ValveTestController::commandRejected, this, &SerialWorkerRuntime::commandRejected);
    connect(m_controller, &ValveTestController::logGenerated, this, &SerialWorkerRuntime::logGenerated);
    connect(m_controller, &ValveTestController::valveActionConfirmed, this, &SerialWorkerRuntime::valveActionConfirmed);
    connect(m_controller, &ValveTestController::channelAlarmUpdated, this, &SerialWorkerRuntime::channelAlarmUpdated);
    connect(m_controller, &ValveTestController::versionReceived, this, &SerialWorkerRuntime::versionReceived);
    connect(m_controller, &ValveTestController::busyChanged, this, &SerialWorkerRuntime::busyChanged);
    connect(m_controller, &ValveTestController::connectionChanged, this, &SerialWorkerRuntime::connectionChanged);
}

void SerialWorkerRuntime::requestAvailablePorts()
{
    emit portsReady(m_controller->availablePorts());
}

void SerialWorkerRuntime::requestOpenPort(const SerialPortSettings &settings)
{
    m_controller->openPort(settings);
}

void SerialWorkerRuntime::requestClosePort()
{
    m_controller->closePort();
}

void SerialWorkerRuntime::requestEnqueueCommand(const CommandPacket &packet, const QString &password)
{
    m_controller->enqueueCommand(packet, password);
}
