# Valve Board Test Tool Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Qt Widgets desktop application that sends the valve-board protocol, waits for `A0 + cmd + data16` acknowledgements, highlights the addressed valve for `100 ms`, and password-protects the four sensitive timing parameters.

**Architecture:** The application is split into a thin Qt Widgets UI, a `ValveTestController` coordination layer, a protocol/domain layer for frame encoding and command-to-valve resolution, and a serial layer that wraps a vendored `CSerialPort` dependency. Core logic is covered first with Qt Test unit tests, then wired into widgets and the real serial adapter.

**Tech Stack:** C++17, Qt 6 Widgets/Core/Test, CMake, vendored `CSerialPort`, QSettings

---

**Repository note:** `E:\GitLab\KValveAgingTestTool` is not currently an initialized Git repository. Run the commit steps only after `.git` exists; otherwise skip the commit commands and continue with the next task.

## Planned File Structure

- `CMakeLists.txt`
- `cmake/AddCSerialPort.cmake`
- `src/app/main.cpp`
- `src/app/ApplicationTheme.h`
- `src/app/ApplicationTheme.cpp`
- `src/config/AppSettings.h`
- `src/config/AppSettings.cpp`
- `src/controller/ValveTestController.h`
- `src/controller/ValveTestController.cpp`
- `src/core/CommandPacket.h`
- `src/core/CommandMap.h`
- `src/core/ProtocolCodec.h`
- `src/core/ProtocolCodec.cpp`
- `src/core/SecurityPolicy.h`
- `src/core/SecurityPolicy.cpp`
- `src/core/ValveAddressResolver.h`
- `src/core/ValveAddressResolver.cpp`
- `src/serial/ISerialTransport.h`
- `src/serial/AckFrameStreamParser.h`
- `src/serial/AckFrameStreamParser.cpp`
- `src/serial/SerialPortTypes.h`
- `src/serial/SerialPortService.h`
- `src/serial/SerialPortService.cpp`
- `src/serial/CSerialPortAdapter.h`
- `src/serial/CSerialPortAdapter.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/PasswordDialog.h`
- `src/ui/PasswordDialog.cpp`
- `src/ui/ChannelCardWidget.h`
- `src/ui/ChannelCardWidget.cpp`
- `src/ui/ValveGridWidget.h`
- `src/ui/ValveGridWidget.cpp`
- `src/ui/ValveIndicatorWidget.h`
- `src/ui/ValveIndicatorWidget.cpp`
- `src/ui/ParameterPanelWidget.h`
- `src/ui/ParameterPanelWidget.cpp`
- `src/ui/LogPanelWidget.h`
- `src/ui/LogPanelWidget.cpp`
- `resources/app.qrc`
- `tests/core/protocol_codec_test.cpp`
- `tests/core/valve_address_resolver_test.cpp`
- `tests/core/security_policy_test.cpp`
- `tests/serial/ack_frame_stream_parser_test.cpp`
- `tests/controller/valve_test_controller_test.cpp`
- `tests/ui/valve_grid_widget_test.cpp`
- `tests/ui/main_window_smoke_test.cpp`
- `tests/ui/password_dialog_test.cpp`

### File Responsibilities

- `ProtocolCodec` owns byte-level encode/decode only.
- `CommandMap` owns constants and command categorization.
- `ValveAddressResolver` converts valve commands into `channel + valve indices`.
- `SecurityPolicy` decides whether a command is password-protected and validates the fixed password.
- `AckFrameStreamParser` converts an arbitrary byte stream into `4`-byte ack frames.
- `ISerialTransport` gives the controller a testable transport boundary.
- `SerialPortService` wraps the concrete adapter and emits transport events in Qt-friendly form.
- `ValveTestController` owns one-command-in-flight behavior, ack matching, timeout handling, and decoded UI events.
- `MainWindow` composes widgets and forwards signals without protocol math.
- `ChannelCardWidget`, `ValveGridWidget`, and `ValveIndicatorWidget` render the `8 x 128` valve dashboard.

### Task 1: Bootstrap CMake, App Skeleton, and Protocol Encode Test

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/app/main.cpp`
- Create: `src/app/ApplicationTheme.h`
- Create: `src/app/ApplicationTheme.cpp`
- Create: `src/ui/MainWindow.h`
- Create: `src/ui/MainWindow.cpp`
- Create: `src/core/CommandPacket.h`
- Create: `src/core/ProtocolCodec.h`
- Create: `src/core/ProtocolCodec.cpp`
- Create: `tests/core/protocol_codec_test.cpp`

- [ ] **Step 1: Write the failing protocol encode test first**

```cpp
// tests/core/protocol_codec_test.cpp
#include <QtTest>
#include "src/core/ProtocolCodec.h"

class ProtocolCodecTest : public QObject {
    Q_OBJECT
private slots:
    void encodeValveCommandBuildsAaCmdAndBigEndianData() {
        const QByteArray frame = ProtocolCodec::encodeWriteFrame(0x50, 0x0001);
        QCOMPARE(frame, QByteArray::fromHex("AA500001"));
    }
};

QTEST_APPLESS_MAIN(ProtocolCodecTest)
#include "protocol_codec_test.moc"
```

- [ ] **Step 2: Add the minimum CMake targets needed to build the test and the app shell**

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.21)
project(KValveAgingTestTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)
enable_testing()

add_library(valve_core
    src/core/ProtocolCodec.cpp
)
target_include_directories(valve_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_core PUBLIC Qt6::Core)

qt_add_executable(valve_test_tool
    src/app/main.cpp
    src/app/ApplicationTheme.cpp
    src/ui/MainWindow.cpp
)
target_include_directories(valve_test_tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_test_tool PRIVATE Qt6::Core Qt6::Widgets valve_core)

qt_add_executable(protocol_codec_test
    tests/core/protocol_codec_test.cpp
)
target_include_directories(protocol_codec_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(protocol_codec_test PRIVATE Qt6::Core Qt6::Test valve_core)
add_test(NAME protocol_codec_test COMMAND protocol_codec_test)
```

- [ ] **Step 3: Run the test to verify the red state**

Run: `cmake -S . -B build && cmake --build build --target protocol_codec_test && ctest --test-dir build -R protocol_codec_test --output-on-failure`  
Expected: build fails because `ProtocolCodec` and app shell sources are not implemented yet, or the test binary fails because `encodeWriteFrame` is missing.

- [ ] **Step 4: Implement the smallest app shell and protocol encode logic that makes the test pass**

```cpp
// src/core/CommandPacket.h
#pragma once

#include <QtCore>

enum class CommandPurpose {
    ValveAction,
    ParameterWrite,
    VersionQuery,
    ChannelAlarm
};

struct CommandPacket {
    quint8 command = 0;
    quint16 data16 = 0;
    CommandPurpose purpose = CommandPurpose::ParameterWrite;
};
```

```cpp
// src/core/ProtocolCodec.h
#pragma once

#include <QtCore>

class ProtocolCodec {
public:
    static QByteArray encodeWriteFrame(quint8 command, quint16 data16);
};
```

```cpp
// src/core/ProtocolCodec.cpp
#include "src/core/ProtocolCodec.h"

QByteArray ProtocolCodec::encodeWriteFrame(quint8 command, quint16 data16) {
    QByteArray frame;
    frame.reserve(4);
    frame.append(char(0xAA));
    frame.append(char(command));
    frame.append(char((data16 >> 8) & 0xFF));
    frame.append(char(data16 & 0xFF));
    return frame;
}
```

```cpp
// src/ui/MainWindow.h
#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
};
```

```cpp
// src/ui/MainWindow.cpp
#include "src/ui/MainWindow.h"

#include <QLabel>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Valve Board Test Tool");
    setCentralWidget(new QLabel("UI bootstrap in progress", this));
    resize(1440, 900);
}
```

```cpp
// src/app/ApplicationTheme.h
#pragma once

#include <QApplication>

namespace ApplicationTheme {
void apply(QApplication &app);
}
```

```cpp
// src/app/ApplicationTheme.cpp
#include "src/app/ApplicationTheme.h"

namespace ApplicationTheme {
void apply(QApplication &app) {
    app.setStyleSheet("QMainWindow { background: #F3F7FB; }");
}
}
```

```cpp
// src/app/main.cpp
#include "src/app/ApplicationTheme.h"
#include "src/ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ApplicationTheme::apply(app);
    MainWindow window;
    window.show();
    return app.exec();
}
```

- [ ] **Step 5: Run the test again to verify the green state**

Run: `cmake -S . -B build && cmake --build build --target protocol_codec_test && ctest --test-dir build -R protocol_codec_test --output-on-failure`  
Expected: `protocol_codec_test` passes.

- [ ] **Step 6: Commit the bootstrap checkpoint if Git is available**

```bash
git add CMakeLists.txt src/app src/core src/ui tests/core/protocol_codec_test.cpp
git commit -m "feat: bootstrap Qt app and protocol encoder"
```

### Task 2: Add Ack Decode and Valve Address Resolution

**Files:**
- Create: `src/core/CommandMap.h`
- Create: `src/core/ValveAddressResolver.h`
- Create: `src/core/ValveAddressResolver.cpp`
- Modify: `src/core/ProtocolCodec.h`
- Modify: `src/core/ProtocolCodec.cpp`
- Create: `tests/core/valve_address_resolver_test.cpp`

- [ ] **Step 1: Write failing tests for ack decoding and valve resolution**

```cpp
// tests/core/valve_address_resolver_test.cpp
#include <QtTest>
#include "src/core/ProtocolCodec.h"
#include "src/core/ValveAddressResolver.h"

class ValveAddressResolverTest : public QObject {
    Q_OBJECT
private slots:
    void decodeAckParsesHeaderCommandAndData() {
        const auto ack = ProtocolCodec::decodeAckFrame(QByteArray::fromHex("A0500001"));
        QVERIFY(ack.has_value());
        QCOMPARE(ack->command, quint8(0x50));
        QCOMPARE(ack->data16, quint16(0x0001));
    }

    void resolveValveAckMapsToChannelOneValveOne() {
        const auto result = ValveAddressResolver::resolveValves(0x50, 0x0001);
        QCOMPARE(result.channel, 1);
        QCOMPARE(result.valves, QList<int>({1}));
    }

    void resolveMultiBitAckMapsAllSetBits() {
        const auto result = ValveAddressResolver::resolveValves(0x51, 0x8001);
        QCOMPARE(result.channel, 1);
        QCOMPARE(result.valves, QList<int>({17, 32}));
    }

    void resolveCrossChannelAckMapsCorrectSegment() {
        const auto result = ValveAddressResolver::resolveValves(0x5B, 0x0010);
        QCOMPARE(result.channel, 2);
        QCOMPARE(result.valves, QList<int>({53}));
    }
};

QTEST_APPLESS_MAIN(ValveAddressResolverTest)
#include "valve_address_resolver_test.moc"
```

- [ ] **Step 2: Register the new test target in CMake**

```cmake
add_library(valve_core
    src/core/ProtocolCodec.cpp
    src/core/ValveAddressResolver.cpp
)

qt_add_executable(valve_address_resolver_test
    tests/core/valve_address_resolver_test.cpp
)
target_include_directories(valve_address_resolver_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_address_resolver_test PRIVATE Qt6::Core Qt6::Test valve_core)
add_test(NAME valve_address_resolver_test COMMAND valve_address_resolver_test)
```

- [ ] **Step 3: Run the new tests to verify they fail for the expected reason**

Run: `cmake --build build --target valve_address_resolver_test && ctest --test-dir build -R valve_address_resolver_test --output-on-failure`  
Expected: compile failure or test failure because `decodeAckFrame` and `resolveValves` do not exist yet.

- [ ] **Step 4: Implement command mapping, ack decode, and valve resolution**

```cpp
// src/core/CommandMap.h
#pragma once

#include <QtCore>

namespace CommandMap {
inline constexpr quint8 kAckHeader = 0xA0;
inline constexpr quint8 kWriteHeader = 0xAA;
inline constexpr quint8 kFirstValveCommand = 0x50;
inline constexpr quint8 kLastValveCommand = 0x8F;
inline constexpr quint8 kChannelAlarmCommand = 0xF0;
inline constexpr quint8 kVersionQueryCommand = 0xFF;

inline bool isValveCommand(quint8 command) {
    return command >= kFirstValveCommand && command <= kLastValveCommand;
}
}
```

```cpp
// src/core/ProtocolCodec.h
#pragma once

#include <QtCore>
#include <optional>

struct AckFrame {
    quint8 command = 0;
    quint16 data16 = 0;
};

class ProtocolCodec {
public:
    static QByteArray encodeWriteFrame(quint8 command, quint16 data16);
    static std::optional<AckFrame> decodeAckFrame(const QByteArray &frame);
};
```

```cpp
// src/core/ProtocolCodec.cpp
#include "src/core/ProtocolCodec.h"
#include "src/core/CommandMap.h"

std::optional<AckFrame> ProtocolCodec::decodeAckFrame(const QByteArray &frame) {
    if (frame.size() != 4 || quint8(frame[0]) != CommandMap::kAckHeader) {
        return std::nullopt;
    }

    AckFrame ack;
    ack.command = quint8(frame[1]);
    ack.data16 = (quint16(quint8(frame[2])) << 8) | quint16(quint8(frame[3]));
    return ack;
}
```

```cpp
// src/core/ValveAddressResolver.h
#pragma once

#include <QtCore>

struct ValveResolution {
    int channel = 0;
    QList<int> valves;
};

class ValveAddressResolver {
public:
    static ValveResolution resolveValves(quint8 command, quint16 data16);
};
```

```cpp
// src/core/ValveAddressResolver.cpp
#include "src/core/ValveAddressResolver.h"
#include "src/core/CommandMap.h"

ValveResolution ValveAddressResolver::resolveValves(quint8 command, quint16 data16) {
    ValveResolution result;
    if (!CommandMap::isValveCommand(command)) {
        return result;
    }

    const int zeroBased = int(command - CommandMap::kFirstValveCommand);
    result.channel = (zeroBased / 8) + 1;
    const int segment = zeroBased % 8;
    const int startValve = segment * 16 + 1;

    for (int bit = 0; bit < 16; ++bit) {
        if (data16 & (quint16(1) << bit)) {
            result.valves.append(startValve + bit);
        }
    }

    return result;
}
```

- [ ] **Step 5: Run the protocol and valve tests and confirm they pass**

Run: `cmake --build build --target protocol_codec_test valve_address_resolver_test && ctest --test-dir build -R "protocol_codec_test|valve_address_resolver_test" --output-on-failure`  
Expected: both tests pass.

- [ ] **Step 6: Commit the protocol decode checkpoint if Git is available**

```bash
git add CMakeLists.txt src/core tests/core/valve_address_resolver_test.cpp
git commit -m "feat: decode ack frames and resolve valve addresses"
```

### Task 3: Add Command Metadata and Password Policy

**Files:**
- Create: `src/core/SecurityPolicy.h`
- Create: `src/core/SecurityPolicy.cpp`
- Modify: `src/core/CommandMap.h`
- Create: `tests/core/security_policy_test.cpp`

- [ ] **Step 1: Write failing tests for protected-command detection and password validation**

```cpp
// tests/core/security_policy_test.cpp
#include <QtTest>
#include "src/core/CommandMap.h"
#include "src/core/SecurityPolicy.h"

class SecurityPolicyTest : public QObject {
    Q_OBJECT
private slots:
    void protectedTimingCommandsRequirePassword() {
        QVERIFY(SecurityPolicy::requiresPassword(0x05));
        QVERIFY(SecurityPolicy::requiresPassword(0x06));
        QVERIFY(SecurityPolicy::requiresPassword(0x07));
        QVERIFY(SecurityPolicy::requiresPassword(0x08));
        QVERIFY(!SecurityPolicy::requiresPassword(0x03));
    }

    void fixedPasswordMustMatchExactly() {
        QVERIFY(SecurityPolicy::validatePassword("Keye@1234"));
        QVERIFY(!SecurityPolicy::validatePassword("keye@1234"));
        QVERIFY(!SecurityPolicy::validatePassword("Keye@12345"));
    }
};

QTEST_APPLESS_MAIN(SecurityPolicyTest)
#include "security_policy_test.moc"
```

- [ ] **Step 2: Register the security policy test in CMake**

```cmake
add_library(valve_core
    src/core/ProtocolCodec.cpp
    src/core/SecurityPolicy.cpp
    src/core/ValveAddressResolver.cpp
)

qt_add_executable(security_policy_test
    tests/core/security_policy_test.cpp
)
target_include_directories(security_policy_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(security_policy_test PRIVATE Qt6::Core Qt6::Test valve_core)
add_test(NAME security_policy_test COMMAND security_policy_test)
```

- [ ] **Step 3: Run the test to verify the red state**

Run: `cmake --build build --target security_policy_test && ctest --test-dir build -R security_policy_test --output-on-failure`  
Expected: failure because `SecurityPolicy` is missing.

- [ ] **Step 4: Implement command metadata helpers and fixed password validation**

```cpp
// src/core/CommandMap.h
#pragma once

#include <QtCore>
#include "src/core/CommandPacket.h"

namespace CommandMap {
inline constexpr quint8 kAckHeader = 0xA0;
inline constexpr quint8 kWriteHeader = 0xAA;
inline constexpr quint8 kFirstValveCommand = 0x50;
inline constexpr quint8 kLastValveCommand = 0x8F;
inline constexpr quint8 kChannelAlarmCommand = 0xF0;
inline constexpr quint8 kVersionQueryCommand = 0xFF;

inline bool isValveCommand(quint8 command) { return command >= kFirstValveCommand && command <= kLastValveCommand; }
inline bool isTimingCommand(quint8 command) { return command >= 0x05 && command <= 0x08; }

inline CommandPurpose classify(quint8 command) {
    if (isValveCommand(command)) return CommandPurpose::ValveAction;
    if (command == kVersionQueryCommand) return CommandPurpose::VersionQuery;
    if (command == kChannelAlarmCommand) return CommandPurpose::ChannelAlarm;
    return CommandPurpose::ParameterWrite;
}
}
```

```cpp
// src/core/SecurityPolicy.h
#pragma once

#include <QString>

class SecurityPolicy {
public:
    static bool requiresPassword(quint8 command);
    static bool validatePassword(const QString &password);
};
```

```cpp
// src/core/SecurityPolicy.cpp
#include "src/core/SecurityPolicy.h"
#include "src/core/CommandMap.h"

bool SecurityPolicy::requiresPassword(quint8 command) {
    return CommandMap::isTimingCommand(command);
}

bool SecurityPolicy::validatePassword(const QString &password) {
    return password == QStringLiteral("Keye@1234");
}
```

- [ ] **Step 5: Run all core tests and confirm they are green**

Run: `cmake --build build --target protocol_codec_test valve_address_resolver_test security_policy_test && ctest --test-dir build -R "protocol_codec_test|valve_address_resolver_test|security_policy_test" --output-on-failure`  
Expected: all three tests pass.

- [ ] **Step 6: Commit the security checkpoint if Git is available**

```bash
git add CMakeLists.txt src/core tests/core/security_policy_test.cpp
git commit -m "feat: add command metadata and password policy"
```

### Task 4: Add Stream Parsing and the CSerialPort Transport Boundary

**Files:**
- Create: `cmake/AddCSerialPort.cmake`
- Create: `src/serial/ISerialTransport.h`
- Create: `src/serial/AckFrameStreamParser.h`
- Create: `src/serial/AckFrameStreamParser.cpp`
- Create: `src/serial/SerialPortTypes.h`
- Create: `src/serial/SerialPortService.h`
- Create: `src/serial/SerialPortService.cpp`
- Create: `src/serial/CSerialPortAdapter.h`
- Create: `src/serial/CSerialPortAdapter.cpp`
- Create: `tests/serial/ack_frame_stream_parser_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Vendor `CSerialPort` and inspect the shipped Qt example before writing adapter code**

```text
third_party/CSerialPort/
third_party/CSerialPort/examples/CommQT/
third_party/CSerialPort/CMakeLists.txt
```

Run this check after vendoring: `cmake -S . -B build`  
Expected: configuration still fails because the transport wrapper and CMake integration are not implemented yet, but the dependency tree is now present locally.

- [ ] **Step 2: Write the failing stream parser test**

```cpp
// tests/serial/ack_frame_stream_parser_test.cpp
#include <QtTest>
#include "src/serial/AckFrameStreamParser.h"

class AckFrameStreamParserTest : public QObject {
    Q_OBJECT
private slots:
    void parserIgnoresNoiseAndExtractsCompleteFrame() {
        AckFrameStreamParser parser;
        parser.pushBytes(QByteArray::fromHex("FFFFA050"));
        parser.pushBytes(QByteArray::fromHex("0001"));
        const auto frames = parser.takeFrames();
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.front(), QByteArray::fromHex("A0500001"));
    }

    void parserKeepsIncompleteTailForNextRead() {
        AckFrameStreamParser parser;
        parser.pushBytes(QByteArray::fromHex("A05000"));
        QVERIFY(parser.takeFrames().isEmpty());
        parser.pushBytes(QByteArray::fromHex("01"));
        QCOMPARE(parser.takeFrames(), QList<QByteArray>({QByteArray::fromHex("A0500001")}));
    }
};

QTEST_APPLESS_MAIN(AckFrameStreamParserTest)
#include "ack_frame_stream_parser_test.moc"
```

- [ ] **Step 3: Add the parser test target and verify the red state**

```cmake
include(cmake/AddCSerialPort.cmake)

qt_add_executable(ack_frame_stream_parser_test
    tests/serial/ack_frame_stream_parser_test.cpp
)
target_include_directories(ack_frame_stream_parser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ack_frame_stream_parser_test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME ack_frame_stream_parser_test COMMAND ack_frame_stream_parser_test)
```

Run: `cmake --build build --target ack_frame_stream_parser_test && ctest --test-dir build -R ack_frame_stream_parser_test --output-on-failure`  
Expected: failure because `AckFrameStreamParser` does not exist yet.

- [ ] **Step 4: Implement the parser, transport interface, and service shell**

```cpp
// src/serial/ISerialTransport.h
#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>

class ISerialTransport : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~ISerialTransport() override = default;

    virtual QStringList availablePorts() const = 0;
    virtual bool open(const QString &portName, qint32 baudRate) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool writeFrame(const QByteArray &frame) = 0;

signals:
    void frameReceived(const QByteArray &frame);
    void transportError(const QString &message);
    void portStateChanged(bool open);
};
```

```cpp
// src/serial/AckFrameStreamParser.h
#pragma once

#include <QtCore>

class AckFrameStreamParser {
public:
    void pushBytes(const QByteArray &bytes);
    QList<QByteArray> takeFrames();

private:
    QByteArray m_buffer;
    QList<QByteArray> m_frames;
};
```

```cpp
// src/serial/AckFrameStreamParser.cpp
#include "src/serial/AckFrameStreamParser.h"

void AckFrameStreamParser::pushBytes(const QByteArray &bytes) {
    m_buffer.append(bytes);

    while (!m_buffer.isEmpty()) {
        const int headerIndex = m_buffer.indexOf(char(0xA0));
        if (headerIndex < 0) {
            m_buffer.clear();
            return;
        }
        if (headerIndex > 0) {
            m_buffer.remove(0, headerIndex);
        }
        if (m_buffer.size() < 4) {
            return;
        }

        m_frames.append(m_buffer.left(4));
        m_buffer.remove(0, 4);
    }
}

QList<QByteArray> AckFrameStreamParser::takeFrames() {
    const auto frames = m_frames;
    m_frames.clear();
    return frames;
}
```

```cpp
// src/serial/SerialPortTypes.h
#pragma once

#include <QtCore>

struct SerialConnectionRequest {
    QString portName;
    qint32 baudRate = 115200;
};
```

```cpp
// src/serial/SerialPortService.h
#pragma once

#include "src/serial/ISerialTransport.h"
#include "src/serial/AckFrameStreamParser.h"

class SerialPortService : public QObject {
    Q_OBJECT
public:
    explicit SerialPortService(ISerialTransport *transport, QObject *parent = nullptr);
    QStringList availablePorts() const;
    bool open(const QString &portName, qint32 baudRate);
    void close();
    bool isOpen() const;
    bool writeFrame(const QByteArray &frame);

signals:
    void ackFrameReceived(const QByteArray &frame);
    void transportError(const QString &message);
    void portStateChanged(bool open);

private:
    ISerialTransport *m_transport;
    AckFrameStreamParser m_parser;
};
```

```cpp
// cmake/AddCSerialPort.cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/CSerialPort/CMakeLists.txt")
    add_subdirectory(third_party/CSerialPort EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "Vendor CSerialPort into third_party/CSerialPort before building")
endif()
```

- [ ] **Step 5: Mirror the vendored `examples/CommQT` open, close, and write flow into `CSerialPortAdapter.cpp`, then run the parser test**

Run: `cmake --build build --target ack_frame_stream_parser_test && ctest --test-dir build -R ack_frame_stream_parser_test --output-on-failure`  
Expected: parser test passes after the parser and service shell are implemented.

- [ ] **Step 6: Commit the transport boundary checkpoint if Git is available**

```bash
git add CMakeLists.txt cmake src/serial tests/serial third_party/CSerialPort
git commit -m "feat: add stream parser and serial transport boundary"
```

### Task 5: Implement the Controller Queue, Ack Matching, and Timeout Rules

**Files:**
- Create: `src/controller/ValveTestController.h`
- Create: `src/controller/ValveTestController.cpp`
- Create: `tests/controller/valve_test_controller_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing controller tests for protected sends, valve ack success, mismatch, and timeout**

```cpp
// tests/controller/valve_test_controller_test.cpp
#include <QtTest>
#include "src/controller/ValveTestController.h"

class FakeTransport : public ISerialTransport {
    Q_OBJECT
public:
    QStringList availablePorts() const override { return {"COM1"}; }
    bool open(const QString &, qint32) override { m_open = true; emit portStateChanged(true); return true; }
    void close() override { m_open = false; emit portStateChanged(false); }
    bool isOpen() const override { return m_open; }
    bool writeFrame(const QByteArray &frame) override { writes.append(frame); return true; }

    bool m_open = true;
    QList<QByteArray> writes;
};

class ValveTestControllerTest : public QObject {
    Q_OBJECT
private slots:
    void protectedCommandIsRejectedWhenPasswordFails();
    void valveAckEmitsResolvedChannelAndValve();
    void mismatchedAckDoesNotConfirmCommand();
    void timeoutMarksCommandFailed();
};
```

Use the fake transport through the real service boundary in each test:

```cpp
FakeTransport transport;
SerialPortService serial(&transport);
ValveTestController controller(&serial);
```

- [ ] **Step 2: Add the controller test target to CMake**

```cmake
add_library(valve_controller
    src/controller/ValveTestController.cpp
)
target_include_directories(valve_controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_controller PUBLIC Qt6::Core valve_core)

qt_add_executable(valve_test_controller_test
    tests/controller/valve_test_controller_test.cpp
)
target_include_directories(valve_test_controller_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_test_controller_test PRIVATE Qt6::Core Qt6::Test valve_controller)
add_test(NAME valve_test_controller_test COMMAND valve_test_controller_test)
```

- [ ] **Step 3: Run the controller tests and verify the red state**

Run: `cmake --build build --target valve_test_controller_test && ctest --test-dir build -R valve_test_controller_test --output-on-failure`  
Expected: failure because `ValveTestController` is not implemented.

- [ ] **Step 4: Implement one-command-in-flight coordination and decoded UI events**

```cpp
// src/controller/ValveTestController.h
#pragma once

#include <QObject>
#include <QQueue>
#include <QTimer>

#include "src/core/CommandPacket.h"
#include "src/serial/ISerialTransport.h"

class ValveTestController : public QObject {
    Q_OBJECT
public:
    explicit ValveTestController(ISerialTransport *serial, QObject *parent = nullptr);

    void setAckTimeoutMs(int timeoutMs);
    bool sendCommand(const CommandPacket &packet, const QString &password = QString());

signals:
    void valveActionConfirmed(int channel, const QList<int> &valves);
    void parameterConfirmed(quint8 command, quint16 data16);
    void channelAlarmUpdated(quint16 alarmBits);
    void logLineReady(const QString &line);
    void commandFailed(const QString &message);

private:
    void handleAckFrame(const QByteArray &frame);
    void handleTimeout();
    bool tryStartNextCommand();

    struct PendingCommand {
        CommandPacket packet;
        QByteArray frame;
    };

    ISerialTransport *m_serial;
    QQueue<PendingCommand> m_queue;
    std::optional<PendingCommand> m_inFlight;
    QTimer m_ackTimer;
    int m_ackTimeoutMs = 400;
};
```

```cpp
// src/controller/ValveTestController.cpp
#include "src/controller/ValveTestController.h"

#include "src/core/CommandMap.h"
#include "src/core/ProtocolCodec.h"
#include "src/core/SecurityPolicy.h"
#include "src/core/ValveAddressResolver.h"

bool ValveTestController::sendCommand(const CommandPacket &packet, const QString &password) {
    if (SecurityPolicy::requiresPassword(packet.command) && !SecurityPolicy::validatePassword(password)) {
        emit commandFailed(QStringLiteral("Protected parameter update rejected: invalid password"));
        return false;
    }

    PendingCommand pending{packet, ProtocolCodec::encodeWriteFrame(packet.command, packet.data16)};
    m_queue.enqueue(pending);
    return tryStartNextCommand();
}

bool ValveTestController::tryStartNextCommand() {
    if (m_inFlight.has_value() || m_queue.isEmpty()) {
        return true;
    }

    m_inFlight = m_queue.dequeue();
    if (!m_serial->writeFrame(m_inFlight->frame)) {
        emit commandFailed(QStringLiteral("Failed to write serial frame"));
        m_inFlight.reset();
        return false;
    }

    m_ackTimer.start(m_ackTimeoutMs);
    emit logLineReady(QStringLiteral("TX %1").arg(QString::fromLatin1(m_inFlight->frame.toHex(' ').toUpper())));
    return true;
}
```

- [ ] **Step 5: Complete the ack branch handling and run the controller tests**

Run: `cmake --build build --target valve_test_controller_test && ctest --test-dir build -R valve_test_controller_test --output-on-failure`  
Expected: controller tests pass after adding:
- ack decode through `ProtocolCodec::decodeAckFrame`
- in-flight command match on `command` and `data16`
- valve resolution and `valveActionConfirmed`
- timeout failure path
- `0xF0` channel alarm emission

- [ ] **Step 6: Commit the controller checkpoint if Git is available**

```bash
git add CMakeLists.txt src/controller tests/controller
git commit -m "feat: add controller queue and ack handling"
```

### Task 6: Build the Main Window Shell, Theme, Parameter Panel, Log Panel, and Settings

**Files:**
- Create: `src/config/AppSettings.h`
- Create: `src/config/AppSettings.cpp`
- Create: `src/ui/ParameterPanelWidget.h`
- Create: `src/ui/ParameterPanelWidget.cpp`
- Create: `src/ui/LogPanelWidget.h`
- Create: `src/ui/LogPanelWidget.cpp`
- Modify: `src/app/ApplicationTheme.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Create: `tests/ui/main_window_smoke_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write a failing smoke test for the themed main window shell**

```cpp
// tests/ui/main_window_smoke_test.cpp
#include <QtTest>
#include "src/ui/MainWindow.h"

class MainWindowSmokeTest : public QObject {
    Q_OBJECT
private slots:
    void windowBuildsCommercialShellLayout() {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.windowTitle(), QStringLiteral("Valve Board Test Tool"));
        QVERIFY(window.findChild<QWidget *>("parameterPanel"));
        QVERIFY(window.findChild<QWidget *>("logPanel"));
    }
};

QTEST_MAIN(MainWindowSmokeTest)
#include "main_window_smoke_test.moc"
```

- [ ] **Step 2: Add the smoke test target and verify the red state**

```cmake
add_library(valve_ui
    src/app/ApplicationTheme.cpp
    src/config/AppSettings.cpp
    src/ui/MainWindow.cpp
    src/ui/ParameterPanelWidget.cpp
    src/ui/LogPanelWidget.cpp
)
target_include_directories(valve_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_ui PUBLIC Qt6::Core Qt6::Widgets valve_controller)

set_property(TARGET valve_test_tool PROPERTY SOURCES src/app/main.cpp)
target_link_libraries(valve_test_tool PRIVATE valve_ui)

qt_add_executable(main_window_smoke_test
    tests/ui/main_window_smoke_test.cpp
)
target_include_directories(main_window_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(main_window_smoke_test PRIVATE Qt6::Core Qt6::Widgets Qt6::Test valve_ui)
add_test(NAME main_window_smoke_test COMMAND main_window_smoke_test)
```

Run: `cmake --build build --target main_window_smoke_test && ctest --test-dir build -R main_window_smoke_test --output-on-failure`  
Expected: failure because the shell widgets do not exist.

- [ ] **Step 3: Implement the shell widgets and local settings wrapper**

```cpp
// src/config/AppSettings.h
#pragma once

#include <QByteArray>
#include <QSettings>
#include <QString>

class AppSettings {
public:
    AppSettings();
    QString lastPortName() const;
    void setLastPortName(const QString &portName);
    qint32 lastBaudRate() const;
    void setLastBaudRate(qint32 baudRate);
    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray &geometry);

private:
    mutable QSettings m_settings;
};
```

```cpp
// src/ui/ParameterPanelWidget.cpp
#include "src/ui/ParameterPanelWidget.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSpinBox>

ParameterPanelWidget::ParameterPanelWidget(QWidget *parent) : QWidget(parent) {
    setObjectName("parameterPanel");
    auto *layout = new QVBoxLayout(this);
    auto *connectionBox = new QGroupBox("Connection", this);
    auto *parameterBox = new QGroupBox("Test Parameters", this);
    auto *actionBox = new QGroupBox("Quick Actions", this);
    layout->addWidget(connectionBox);
    layout->addWidget(parameterBox);
    layout->addWidget(actionBox);
    layout->addStretch();
}
```

```cpp
// src/ui/LogPanelWidget.cpp
#include "src/ui/LogPanelWidget.h"

#include <QPlainTextEdit>
#include <QVBoxLayout>

LogPanelWidget::LogPanelWidget(QWidget *parent) : QWidget(parent) {
    setObjectName("logPanel");
    auto *layout = new QVBoxLayout(this);
    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    layout->addWidget(m_logView);
}
```

```cpp
// src/ui/MainWindow.cpp
#include "src/ui/MainWindow.h"
#include "src/ui/LogPanelWidget.h"
#include "src/ui/ParameterPanelWidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Valve Board Test Tool");

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    auto *content = new QSplitter(Qt::Horizontal, central);

    auto *parameterPanel = new ParameterPanelWidget(content);
    auto *workspace = new QWidget(content);
    auto *workspaceLayout = new QVBoxLayout(workspace);
    m_logPanel = new LogPanelWidget(workspace);

    workspaceLayout->addStretch();
    workspaceLayout->addWidget(m_logPanel);
    content->addWidget(parameterPanel);
    content->addWidget(workspace);
    root->addWidget(content);
    setCentralWidget(central);
    resize(1600, 960);
}
```

- [ ] **Step 4: Apply the commercial baseline theme and rerun the smoke test**

```cpp
// src/app/ApplicationTheme.cpp
#include "src/app/ApplicationTheme.h"

namespace ApplicationTheme {
void apply(QApplication &app) {
    app.setStyleSheet(R"(
        QWidget { background: #EEF3F8; color: #1A2533; font-family: "Segoe UI"; font-size: 13px; }
        QGroupBox {
            border: 1px solid #D4DEE8;
            border-radius: 14px;
            margin-top: 12px;
            padding: 16px;
            background: #FFFFFF;
            font-weight: 600;
        }
        QPushButton {
            background: #0F5E8C;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 8px 14px;
        }
        QPlainTextEdit {
            background: #0F1722;
            color: #B9D3EA;
            border-radius: 12px;
            border: 1px solid #233246;
        }
    )");
}
}
```

Run: `cmake --build build --target main_window_smoke_test && ctest --test-dir build -R main_window_smoke_test --output-on-failure`  
Expected: `main_window_smoke_test` passes.

- [ ] **Step 5: Commit the shell UI checkpoint if Git is available**

```bash
git add CMakeLists.txt src/app src/config src/ui tests/ui/main_window_smoke_test.cpp
git commit -m "feat: add main window shell and theme"
```

### Task 7: Build the Channel Dashboard and Valve Flash Widgets

**Files:**
- Create: `src/ui/ValveIndicatorWidget.h`
- Create: `src/ui/ValveIndicatorWidget.cpp`
- Create: `src/ui/ValveGridWidget.h`
- Create: `src/ui/ValveGridWidget.cpp`
- Create: `src/ui/ChannelCardWidget.h`
- Create: `src/ui/ChannelCardWidget.cpp`
- Create: `tests/ui/valve_grid_widget_test.cpp`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write a failing widget test for flashing specific valves**

```cpp
// tests/ui/valve_grid_widget_test.cpp
#include <QtTest>
#include "src/ui/ValveGridWidget.h"

class ValveGridWidgetTest : public QObject {
    Q_OBJECT
private slots:
    void flashValvesActivatesOnlyRequestedIndicators() {
        ValveGridWidget grid;
        grid.flashValves({1, 128});
        QVERIFY(grid.isValveActive(1));
        QVERIFY(grid.isValveActive(128));
        QVERIFY(!grid.isValveActive(64));
    }

    void flashValvesAutoResetsAfterOneHundredMilliseconds() {
        ValveGridWidget grid;
        grid.flashValves({7});
        QVERIFY(grid.isValveActive(7));
        QTest::qWait(140);
        QVERIFY(!grid.isValveActive(7));
    }
};

QTEST_MAIN(ValveGridWidgetTest)
#include "valve_grid_widget_test.moc"
```

- [ ] **Step 2: Add the widget test target and confirm the red state**

```cmake
qt_add_executable(valve_grid_widget_test
    tests/ui/valve_grid_widget_test.cpp
)
target_include_directories(valve_grid_widget_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_grid_widget_test PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)
add_test(NAME valve_grid_widget_test COMMAND valve_grid_widget_test)
```

Run: `cmake --build build --target valve_grid_widget_test && ctest --test-dir build -R valve_grid_widget_test --output-on-failure`  
Expected: failure because the valve dashboard widgets do not exist.

- [ ] **Step 3: Implement the indicator, grid, and channel card widgets**

```cpp
// src/ui/ValveIndicatorWidget.h
#pragma once

#include <QWidget>
#include <QTimer>

class ValveIndicatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit ValveIndicatorWidget(int valveNumber, QWidget *parent = nullptr);
    void flashActive(int durationMs = 100);
    bool isActive() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_valveNumber;
    bool m_active = false;
};
```

```cpp
// src/ui/ValveGridWidget.h
#pragma once

#include <QWidget>

class ValveIndicatorWidget;

class ValveGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit ValveGridWidget(QWidget *parent = nullptr);
    void flashValves(const QList<int> &valves);
    bool isValveActive(int valveNumber) const;

private:
    QHash<int, ValveIndicatorWidget *> m_indicators;
};
```

```cpp
// src/ui/ValveGridWidget.cpp
#include "src/ui/ValveGridWidget.h"
#include "src/ui/ValveIndicatorWidget.h"

#include <QGridLayout>

ValveGridWidget::ValveGridWidget(QWidget *parent) : QWidget(parent) {
    auto *layout = new QGridLayout(this);
    layout->setSpacing(4);

    for (int valve = 1; valve <= 128; ++valve) {
        auto *indicator = new ValveIndicatorWidget(valve, this);
        m_indicators.insert(valve, indicator);
        const int row = (valve - 1) / 16;
        const int column = (valve - 1) % 16;
        layout->addWidget(indicator, row, column);
    }
}

void ValveGridWidget::flashValves(const QList<int> &valves) {
    for (const int valve : valves) {
        if (auto *indicator = m_indicators.value(valve, nullptr)) {
            indicator->flashActive(100);
        }
    }
}

bool ValveGridWidget::isValveActive(int valveNumber) const {
    if (auto *indicator = m_indicators.value(valveNumber, nullptr)) {
        return indicator->isActive();
    }
    return false;
}
```

```cpp
// src/ui/ChannelCardWidget.cpp
#include "src/ui/ChannelCardWidget.h"

#include <QLabel>
#include <QVBoxLayout>

ChannelCardWidget::ChannelCardWidget(int channelNumber, QWidget *parent) : QFrame(parent) {
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(QStringLiteral("CH %1").arg(channelNumber, 2, 10, QChar('0')), this);
    m_grid = new ValveGridWidget(this);
    layout->addWidget(title);
    layout->addWidget(m_grid);
}
```

- [ ] **Step 4: Replace the empty workspace in `MainWindow` with a `2 x 4` channel-card dashboard**

```cpp
// inside MainWindow.cpp
auto *dashboard = new QWidget(content);
auto *dashboardLayout = new QGridLayout(dashboard);
for (int channel = 1; channel <= 8; ++channel) {
    auto *card = new ChannelCardWidget(channel, dashboard);
    dashboardLayout->addWidget(card, (channel - 1) / 4, (channel - 1) % 4);
    m_channelCards.insert(channel, card);
}
workspaceLayout->insertWidget(0, dashboard, 1);
```

- [ ] **Step 5: Run the widget and smoke tests and verify the dashboard works**

Run: `cmake --build build --target valve_grid_widget_test main_window_smoke_test && ctest --test-dir build -R "valve_grid_widget_test|main_window_smoke_test" --output-on-failure`  
Expected: both tests pass.

- [ ] **Step 6: Commit the valve dashboard checkpoint if Git is available**

```bash
git add CMakeLists.txt src/ui tests/ui/valve_grid_widget_test.cpp
git commit -m "feat: add channel dashboard and valve flash widgets"
```

### Task 8: Wire the UI to the Controller, Add Password Dialog, and Finish End-to-End Verification

**Files:**
- Create: `src/ui/PasswordDialog.h`
- Create: `src/ui/PasswordDialog.cpp`
- Modify: `src/ui/ParameterPanelWidget.h`
- Modify: `src/ui/ParameterPanelWidget.cpp`
- Modify: `src/ui/LogPanelWidget.h`
- Modify: `src/ui/LogPanelWidget.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/controller/ValveTestController.cpp`
- Create: `resources/app.qrc`
- Create: `tests/ui/password_dialog_test.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing password-dialog test**

```cpp
// tests/ui/password_dialog_test.cpp
#include <QtTest>
#include <QLineEdit>
#include "src/ui/PasswordDialog.h"

class PasswordDialogTest : public QObject {
    Q_OBJECT
private slots:
    void passwordInputIsMasked() {
        PasswordDialog dialog;
        auto *edit = dialog.findChild<QLineEdit *>();
        QVERIFY(edit != nullptr);
        QCOMPARE(edit->echoMode(), QLineEdit::Password);
    }
};

QTEST_MAIN(PasswordDialogTest)
#include "password_dialog_test.moc"
```

- [ ] **Step 2: Register the dialog test and verify the red state**

```cmake
qt_add_executable(password_dialog_test
    tests/ui/password_dialog_test.cpp
)
target_include_directories(password_dialog_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(password_dialog_test PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)
add_test(NAME password_dialog_test COMMAND password_dialog_test)
```

Run: `cmake --build build --target password_dialog_test && ctest --test-dir build -R password_dialog_test --output-on-failure`  
Expected: failure because `PasswordDialog` does not exist yet.

- [ ] **Step 3: Implement the password dialog and parameter-panel command signals**

```cpp
// src/ui/PasswordDialog.h
#pragma once

#include <QDialog>

class QLineEdit;

class PasswordDialog : public QDialog {
    Q_OBJECT
public:
    explicit PasswordDialog(QWidget *parent = nullptr);
    QString password() const;

private:
    QLineEdit *m_passwordEdit = nullptr;
};
```

```cpp
// src/ui/ParameterPanelWidget.h
#pragma once

#include <QWidget>

class ParameterPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ParameterPanelWidget(QWidget *parent = nullptr);

signals:
    void parameterApplyRequested(quint8 command, quint16 data16);
    void protectedParameterApplyRequested(quint8 command, quint16 data16);
};
```

```cpp
// src/ui/PasswordDialog.cpp
#include "src/ui/PasswordDialog.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QVBoxLayout>

PasswordDialog::PasswordDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Password Confirmation");
    auto *layout = new QVBoxLayout(this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_passwordEdit);
    layout->addWidget(buttons);
}
```

- [ ] **Step 4: Wire `MainWindow` to the controller, logs, settings, and channel dashboard**

```cpp
// inside MainWindow constructor
connect(m_parameterPanel, &ParameterPanelWidget::parameterApplyRequested,
        this, [this](quint8 command, quint16 data16) {
            CommandPacket packet{command, data16, CommandMap::classify(command)};
            m_controller->sendCommand(packet);
        });

connect(m_parameterPanel, &ParameterPanelWidget::protectedParameterApplyRequested,
        this, [this](quint8 command, quint16 data16) {
            PasswordDialog dialog(this);
            if (dialog.exec() != QDialog::Accepted) {
                return;
            }
            CommandPacket packet{command, data16, CommandMap::classify(command)};
            m_controller->sendCommand(packet, dialog.password());
        });

connect(m_controller, &ValveTestController::valveActionConfirmed,
        this, [this](int channel, const QList<int> &valves) {
            if (auto *card = m_channelCards.value(channel, nullptr)) {
                card->flashValves(valves);
            }
        });

connect(m_controller, &ValveTestController::logLineReady,
        m_logPanel, &LogPanelWidget::appendLine);
```

- [ ] **Step 5: Add the version field, alarm display, and resource registration, then rerun the UI tests**

```xml
<!-- resources/app.qrc -->
<RCC>
    <qresource prefix="/">
        <file>resources/icons/app.svg</file>
    </qresource>
</RCC>
```

Run: `cmake --build build --target password_dialog_test main_window_smoke_test valve_test_tool && ctest --test-dir build -R "main_window_smoke_test|password_dialog_test|valve_grid_widget_test|valve_test_controller_test" --output-on-failure`  
Expected: all UI and controller tests pass, and the app target builds successfully.

Then add `password_dialog_test` to the same verification run:

Run: `ctest --test-dir build -R "main_window_smoke_test|valve_grid_widget_test|valve_test_controller_test|password_dialog_test" --output-on-failure`  
Expected: all four tests pass.

- [ ] **Step 6: Perform the manual hardware verification pass**

Run:

```bash
cmake -S . -B build
cmake --build build --target valve_test_tool
ctest --test-dir build --output-on-failure
```

Manual checklist:

- Launch the app on Windows and Linux.
- Confirm the port list populates through `CSerialPort`.
- Send `AA 50 00 01` and verify `A0 50 00 01` flashes `CH 01` valve `1` for `100 ms`.
- Send a multi-bit valve payload such as `AA 51 80 01` and verify valves `17` and `32` flash.
- Change each protected timing field and verify password prompt appears every time.
- Enter the wrong password and verify no serial write occurs.
- Trigger `0xF0` abnormal bits and verify the correct channel cards enter warning state.
- Query version and verify the version field and log update.

- [ ] **Step 7: Commit the end-to-end integration checkpoint if Git is available**

```bash
git add CMakeLists.txt resources src/ui src/controller tests/ui
git commit -m "feat: wire UI to controller and finish valve test tool"
```

## Plan Self-Review

### Spec Coverage

- Cross-platform Qt Widgets app: covered in Tasks 1, 4, 6, and 8.
- CMake project management: covered in Tasks 1 through 8.
- No `QSerialPort`: covered in Task 4 through `CSerialPort`.
- Protocol `AA + cmd + data16`: covered in Tasks 1 and 2.
- Ack `A0 + cmd + data16`: covered in Tasks 2, 4, and 5.
- Eight channels and `128` valves each: covered in Tasks 2 and 7.
- Password protection for `0x05` to `0x08`: covered in Tasks 3, 5, and 8.
- Command completion valve flash: covered in Tasks 5, 7, and 8.
- Channel abnormal handling: covered in Tasks 5 and 8.
- Raw and decoded logging: covered in Tasks 5, 6, and 8.
- Windows and Linux verification: covered in Task 8.

### Placeholder Scan

- No `TODO`, `TBD`, or unresolved placeholders remain in this plan.
- The only implementation detail intentionally deferred to vendor inspection is the exact `CSerialPort` adapter call sequence, and the plan points directly to the vendored `examples/CommQT` source before that code is written.

### Type Consistency

- `CommandPacket`, `AckFrame`, `ValveResolution`, and `PendingCommand` naming is consistent across core and controller tasks.
- The transport boundary consistently uses `ISerialTransport` and `SerialPortService`.
- UI flash behavior consistently targets `ChannelCardWidget -> ValveGridWidget -> ValveIndicatorWidget`.
