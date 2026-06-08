# Serial Threading And Console Logging Design

## Goal

将串口打开、发送、接收、ACK 解析全部放入独立工作线程，避免阻塞 GUI；同时把所有协议日志同时输出到界面日志面板和控制台，输出格式统一为：

- 时间戳
- 方向
- 十六进制报文
- 中文说明

## Scope

本次改动只覆盖以下内容：

- 串口相关对象迁移到工作线程
- UI 到串口层的调用改为异步请求
- 协议日志增加控制台输出
- 所有下发数据继续保留在界面日志中，并同步打印到控制台

本次不改动：

- 协议格式
- 命令映射
- 参数换算逻辑
- UI 样式和布局
- 密码保护逻辑

## Current Problem

当前 `main.cpp` 中直接在 GUI 线程创建并持有以下对象：

- `CSerialPortAdapter`
- `SerialPortService`
- `ValveTestController`

`MainWindow` 通过同步函数直接调用：

- `availablePorts()`
- `openPort()`
- `closePort()`
- `enqueueCommand()`

这意味着串口打开、写入、读取回调、ACK 解析都沿着 GUI 线程对象关系工作。虽然部分底层库可能内部异步，但从 Qt 对象组织上看，收发边界不清晰，也不满足“下发和读取全都使用线程操作不要影响 GUI”的要求。

## Target Architecture

### Thread Boundary

新增一个串口工作线程，串口运行时相关对象全部归属该线程：

- `CSerialPortAdapter`
- `SerialPortService`
- `ValveTestController`

GUI 线程中只保留：

- `QApplication`
- `MainWindow`
- `AppSettings`

这样可以确保：

- 串口打开和关闭不占用 GUI 线程
- 串口发送不占用 GUI 线程
- 串口接收和 ACK 解析不占用 GUI 线程
- GUI 只负责显示和用户输入

### Communication Model

GUI 与工作线程之间仅通过 Qt 信号槽交互，不再通过跨线程同步成员函数直接访问控制器。

#### GUI -> Worker 请求

新增一个面向线程边界的请求对象，例如 `SerialWorkerFacade` 或 `SerialWorkerRuntime`，负责暴露槽函数：

- `requestAvailablePorts()`
- `requestOpenPort(SerialPortSettings)`
- `requestClosePort()`
- `requestEnqueueCommand(CommandPacket, QString password)`

GUI 通过 `Qt::QueuedConnection` 发起请求。

#### Worker -> GUI 回传

工作线程继续向 GUI 发出状态信号，沿用现有语义：

- `portsEnumerated(QVector<SerialPortDescriptor>)`
- `connectionChanged(bool)`
- `versionReceived(QString)`
- `valveActionConfirmed(int channel, QList<int>)`
- `channelAlarmUpdated(QList<int>)`
- `commandRejected(QString)`
- `logGenerated(QString direction, QString hex, QString description)`

GUI 只接收这些信号并更新界面。

## Logging Design

### Shared Log Source

仍以 `ValveTestController::logGenerated` 作为统一日志出口，不新增第二套日志生成逻辑。

覆盖范围保持完整：

- `TX`
- `RX`
- `ACK`
- `WARN`
- `ERR`
- `AUTH`
- `TIMEOUT`

### UI Log

现有 `LogPanelWidget` 继续消费 `logGenerated`，显示在协议日志面板中。

### Console Log

新增一个轻量日志接收器，例如：

- `ProtocolConsoleLogger`

职责：

- 监听同一份 `logGenerated`
- 按固定格式输出到标准输出

输出格式：

```text
HH:mm:ss.zzz | TX | AA 05 00 28 | 吹气时间设置为 2.00 ms，实际下发原始值 40
```

说明：

- 时间戳在日志输出器生成
- 方向使用控制器发出的方向值
- 十六进制报文直接使用控制器提供的 `hex`
- 中文说明直接使用控制器提供的 `description`

如果某些日志没有原始报文，十六进制字段允许为空，但格式列数保持一致。

## Object Lifetime

### Startup

在 `main.cpp` 中：

1. 创建 `QApplication`
2. 创建 `AppSettings`
3. 创建 `QThread`
4. 创建串口运行时对象
5. 将串口运行时对象及其内部控制器/服务/适配器迁移到工作线程
6. 创建控制台日志器并连接到运行时的日志信号
7. 创建 `MainWindow`
8. 连接 GUI <-> Worker 信号槽
9. 启动线程
10. 显示窗口

### Shutdown

应用退出时：

1. GUI 请求关闭串口
2. 调用 `thread->quit()`
3. `thread->wait()`
4. 再销毁线程相关对象

必须避免在错误线程中销毁 `QObject`。

## MainWindow Changes

`MainWindow` 需要从“直接持有控制器指针”调整为“持有线程运行时接口信号连接”。

### Existing Synchronous Calls To Replace

- 刷新串口：从同步获取端口列表改为异步请求端口列表
- 连接串口：从同步 `openPort()` 改为异步请求
- 断开串口：从同步 `closePort()` 改为异步请求
- 下发命令：从同步 `enqueueCommand()` 改为异步请求

### UI Behavior

界面行为不变：

- 点击按钮立即响应
- 日志照常显示
- ACK 到来后照常点亮阀位
- 状态栏照常更新

变化仅在实现层：

- GUI 不等待串口函数返回
- 成功或失败完全由异步信号驱动

## Port Enumeration

串口枚举也放在工作线程处理，原因：

- 保持与串口操作同一线程边界
- 避免某些平台枚举耗时卡住 GUI

新增信号：

- `portsReady(QVector<SerialPortDescriptor>)`

`MainWindow::refreshPorts()` 改为：

1. 发出请求
2. 等待 `portsReady`
3. 刷新下拉框

## Error Handling

### Open Failure

工作线程发出：

- `commandRejected("打开串口失败...")`
- `logGenerated("ERR", "", "...")`

GUI 不做同步错误判断。

### Write Failure

发送失败仍由控制器记录日志并发出拒绝信号，同时打印到控制台。

### Thread Safety

不得跨线程直接读取 worker 内部状态，例如：

- 不在 GUI 线程直接调用 `isConnected()`
- 不在 GUI 线程直接读取端口对象

如果 GUI 需要知道状态，必须靠信号同步缓存。

## Files To Modify

预计会改动：

- `src/app/main.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/controller/ValveTestController.h`
- `src/controller/ValveTestController.cpp`
- `src/serial/SerialPortService.h`
- `src/serial/SerialPortService.cpp`

预计新增：

- `src/app/SerialWorkerRuntime.h`
- `src/app/SerialWorkerRuntime.cpp`
- `src/app/ProtocolConsoleLogger.h`
- `src/app/ProtocolConsoleLogger.cpp`

如果实现中发现职责更适合拆分，也可以把 worker 封装进一步拆成：

- runtime facade
- worker object

但不应扩展到无关模块。

## Acceptance Criteria

完成后应满足：

1. 所有串口发送和接收路径都运行在工作线程，不阻塞 GUI
2. 界面日志面板仍然完整显示协议日志
3. 控制台同步输出相同日志，格式为“时间戳 + 方向 + 十六进制 + 中文说明”
4. 所有下发数据都能看到完整十六进制帧
5. 现有 ACK 匹配、密码保护、阀位点亮、异常通道显示行为不回退
6. Windows 和 Linux 的 Qt/CMake 构建方式保持兼容

## Self-Review

- 没有引入第二套协议日志逻辑，日志源仍然唯一
- 线程边界明确，GUI 不直接调用 worker 内部同步函数
- 范围控制在串口线程化和日志输出，没有夹带 UI 或协议变更
