#include "src/app/ApplicationTheme.h"
#include "src/app/ProtocolConsoleLogger.h"
#include "src/app/SerialWorkerRuntime.h"
#include "src/config/AppSettings.h"
#include "src/core/CommandPacket.h"
#include "src/serial/SerialPortTypes.h"
#include "src/ui/MainWindow.h"

#include <QApplication>
#include <QList>
#include <QMetaObject>
#include <QMetaType>
#include <QThread>

int main(int argc, char *argv[])
{
    qRegisterMetaType<CommandPacket>("CommandPacket");
    qRegisterMetaType<SerialPortSettings>("SerialPortSettings");
    qRegisterMetaType<QVector<SerialPortDescriptor>>("QVector<SerialPortDescriptor>");
    qRegisterMetaType<QList<int>>("QList<int>");

    QApplication app(argc, argv);
    ApplicationTheme::apply(app);

    AppSettings settings;
    QThread serialThread;
    auto *runtime = new SerialWorkerRuntime();
    runtime->moveToThread(&serialThread);

    ProtocolConsoleLogger consoleLogger;
    QObject::connect(&serialThread, &QThread::finished, runtime, &QObject::deleteLater);
    QObject::connect(
        runtime,
        &SerialWorkerRuntime::logGenerated,
        &consoleLogger,
        &ProtocolConsoleLogger::printLogLine,
        Qt::QueuedConnection
    );

    serialThread.start();

    MainWindow window(runtime, &settings);
    window.show();

    const int exitCode = app.exec();

    if (serialThread.isRunning()) {
        QMetaObject::invokeMethod(runtime, "requestClosePort", Qt::BlockingQueuedConnection);
        serialThread.quit();
        serialThread.wait();
    }

    return exitCode;
}
