# Serial Threading And Console Logging Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move all serial send/receive work off the GUI thread and mirror every protocol log line to both the Qt log panel and the console.

**Architecture:** Introduce a worker-thread runtime object that owns `CSerialPortAdapter`, `SerialPortService`, and `ValveTestController`. `MainWindow` stops calling the controller synchronously and instead sends queued requests to the runtime while receiving state updates and protocol logs asynchronously.

**Tech Stack:** C++17, Qt Widgets, Qt signal/slot threading, CMake, CSerialPort

---

## File Structure

- Create: `src/app/SerialWorkerRuntime.h`
  - Owns and wires the transport, serial service, and controller in one worker-thread-facing object.
- Create: `src/app/SerialWorkerRuntime.cpp`
  - Implements async request slots and relays worker results back to the UI.
- Create: `src/app/ProtocolConsoleLogger.h`
  - Declares the console logger for protocol events.
- Create: `src/app/ProtocolConsoleLogger.cpp`
  - Formats and prints `timestamp + direction + hex + description`.
- Modify: `src/app/main.cpp`
  - Creates the worker thread, moves the runtime into it, connects startup/shutdown, and wires the console logger.
- Modify: `src/ui/MainWindow.h`
  - Replaces direct controller ownership with request signals and cached async state.
- Modify: `src/ui/MainWindow.cpp`
  - Sends async requests for port refresh, connect/disconnect, and command enqueue; consumes async port results.
- Modify: `src/controller/ValveTestController.h`
  - Adds async-friendly status/result signals where needed for UI caching.
- Modify: `src/controller/ValveTestController.cpp`
  - Ensures every outgoing and incoming protocol event emits a full log line.
- Modify: `CMakeLists.txt`
  - Adds the new runtime/logger source files to the application target graph.

## Constraints

- No new automated test targets: the user explicitly wants main functionality only.
- Verification is the Windows Debug build of `valve_test_tool`.
- Existing protocol behavior, password protection, timing conversion, valve highlighting, and Chinese UI text must not regress.

### Task 1: Add the Worker Runtime

**Files:**
- Create: `src/app/SerialWorkerRuntime.h`
- Create: `src/app/SerialWorkerRuntime.cpp`
- Modify: `src/controller/ValveTestController.h`
- Modify: `src/controller/ValveTestController.cpp`

- [ ] Define `SerialWorkerRuntime` as a `QObject` that constructs `CSerialPortAdapter`, `SerialPortService`, and `ValveTestController` internally.
- [ ] Add queued-callable public slots:
  - `requestAvailablePorts()`
  - `requestOpenPort(const SerialPortSettings &settings)`
  - `requestClosePort()`
  - `requestEnqueueCommand(const CommandPacket &packet, const QString &password)`
- [ ] Add relay signals:
  - `portsReady(const QVector<SerialPortDescriptor> &ports)`
  - `connectionChanged(bool connected)`
  - `commandRejected(const QString &reason)`
  - `logGenerated(const QString &direction, const QString &hex, const QString &description)`
  - `valveActionConfirmed(int channel, const QList<int> &valves)`
  - `channelAlarmUpdated(const QList<int> &channels)`
  - `versionReceived(const QString &versionText)`
- [ ] Keep `ValveTestController` as the single protocol-log source. Do not duplicate protocol interpretation in the runtime.

### Task 2: Convert the Main Window to Async Requests

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`

- [ ] Remove direct dependence on synchronous controller methods in UI event handlers.
- [ ] Replace controller method calls with emitted request signals:
  - `requestAvailablePorts()`
  - `requestOpenPort(SerialPortSettings)`
  - `requestClosePort()`
  - `requestCommand(CommandPacket, QString)`
- [ ] Add a cached `bool` connection flag in `MainWindow` so the connect button no longer depends on direct `isConnected()` calls across threads.
- [ ] Populate the serial port combo box from `portsReady(...)` rather than a direct return value.
- [ ] Keep the existing valve highlight, abnormal channel update, version text update, and log panel update behaviors unchanged from the user's perspective.

### Task 3: Add Console Protocol Logging

**Files:**
- Create: `src/app/ProtocolConsoleLogger.h`
- Create: `src/app/ProtocolConsoleLogger.cpp`
- Modify: `src/app/main.cpp`

- [ ] Implement a `ProtocolConsoleLogger` `QObject` with a slot such as `printLogLine(const QString &direction, const QString &hex, const QString &description)`.
- [ ] Format console output exactly as:
  - `HH:mm:ss.zzz | DIRECTION | HEX | 中文说明`
- [ ] Connect the runtime's `logGenerated(...)` signal both to:
  - the GUI log path
  - the console logger slot
- [ ] Keep console logging passive: it must not affect command flow or UI responsiveness.

### Task 4: Wire Worker Thread Startup And Shutdown

**Files:**
- Modify: `src/app/main.cpp`
- Modify: `CMakeLists.txt`

- [ ] Create a dedicated `QThread` in `main.cpp`.
- [ ] Construct `SerialWorkerRuntime`, move it to the worker thread, then start the thread before user interaction begins.
- [ ] Construct `MainWindow` on the GUI thread and connect its request signals to the runtime with queued connections.
- [ ] Connect worker signals back to `MainWindow` and `ProtocolConsoleLogger`.
- [ ] On shutdown:
  - close the window
  - request serial close if needed
  - quit the thread
  - wait for the thread
  - destroy thread-owned objects safely

### Task 5: Build Verification

**Files:**
- Verify: `build-vs/Debug/valve_test_tool.exe`

- [ ] Run:

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' --build build-vs --target valve_test_tool --config Debug
```

- [ ] Expected result:
  - build exits with code `0`
  - `build-vs\Debug\valve_test_tool.exe` is updated
- [ ] If the build fails, fix compile or moc issues before claiming completion.

## Self-Review

- Spec coverage:
  - worker-threaded serial send/receive: covered by Tasks 1 and 4
  - async GUI interaction: covered by Task 2
  - console log mirroring: covered by Task 3
  - verification: covered by Task 5
- Placeholder scan:
  - no `TODO`, `TBD`, or “similar to above” shortcuts remain
- Type consistency:
  - request signals in `MainWindow` match runtime slots
  - runtime relays existing controller signal payloads without changing their shapes
