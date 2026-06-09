# KValveAgingTestTool

基于 Qt Widgets 和 CMake 的阀板老化测试上位机，协议依据 [doc/阀板测试.xlsx](doc/阀板测试.xlsx) 实现。

软件面向触摸屏操作场景，支持 8 个通道、每通道最多 128 个阀位显示与测试，串口通信后端使用成熟第三方串口库 `CSerialPort`，不依赖 `QSerialPort`。

## 主要功能

- 支持 8 个通道阀板测试。
- 每个通道最多支持 128 个阀位，界面可配置显示阀个数。
- 支持单阀点测，收到回包后自动点亮对应通道和阀位指示灯。
- 支持读取固件版本、读取通道异常状态。
- 支持参数保存、自动加载、自动连接上次成功连接的串口。
- 非加密参数修改后立即下发，无需再点单独“下发”按钮。
- 受保护时间参数支持一次密码确认、批量下发。
- 下发日志和回包日志同时输出到界面和控制台。
- 串口通信、收发处理、超时处理均运行在线程中，不阻塞 GUI。

## 当前交互逻辑

- 数值输入框采用触摸输入方式。
  点击参数输入框时，会弹出数字键盘进行输入。
- `阀开关` 使用按钮切换。
  当前按钮文字和颜色会直接反映开/关状态。
- 非加密参数修改后立即下发。
  包括：
  `阀开关`、`触发模式`、`吹气次数`、`吹气间隔`、`通道数量`、`通道独立使能`。
- 加密参数不自动下发。
  包括：
  `吹气时间`、`充电时间`、`停止充电时间`、`续充电时间`。
- 4 个加密时间参数通过一个 `下发时间参数` 按钮统一下发，只输入一次密码。

## 加密参数说明

- 固定密码：`123`
- 需要密码保护的命令：
  - `吹气时间`
  - `充电时间`
  - `停止充电时间`
  - `续充电时间`

时间参数在界面上以 `ms` 显示，但下发时会自动转换为协议原始值：

```text
raw = ms / 0.05
```

示例：

```text
吹气时间输入 2.00 ms
实际下发原始值 40
下发帧数据示例：AA 05 00 28
```

## 串口协议说明

### 基本帧格式

所有帧固定 4 字节：

```text
Header(1 byte) + Command(1 byte) + Data(2 byte)
```

### 发送规则

- 普通控制写命令头：`0xAA`
- 查询类写命令头：`0xA0`

当前实现中：

- `0xF0` 通道异常状态读取，使用 `0xA0`
- `0xFF` 固件版本读取，使用 `0xA0`
- 其余参数写入、阀动作写入，使用 `0xAA`

### 回包规则

- 设备回包可能是 `0xAA`，也可能是 `0xA0`
- 软件不会只依赖回包头判断合法性，而是结合命令地址进行匹配
- 阀测试过程中，如果下位机连续主动上报阀动作，软件会按回包命令地址解析对应通道与阀位

### 阀位地址规则

- 阀命令起始地址：`0x50`
- 阀命令结束地址：`0x8F`
- 每个通道 8 段
- 每段 16 个阀
- 每通道最多 128 个阀
- 共支持 8 个通道

计算规则：

- 通道 1：`0x50` ~ `0x57`
- 通道 2：`0x58` ~ `0x5F`
- 依次类推

示例：

```text
操作通道 1 的 1 号阀
发送：AA 50 00 01
回复：AA 50 00 01 或 A0 50 00 01
```

## 命令映射

| 命令 | 含义 |
| --- | --- |
| `0x00` | 阀开关 |
| `0x01` | 触发模式 |
| `0x02` | 吹气次数 |
| `0x03` | 吹气间隔 |
| `0x05` | 吹气时间 |
| `0x06` | 充电时间 |
| `0x07` | 停止充电时间 |
| `0x08` | 续充电时间 |
| `0x09` | 通道数量 |
| `0x0A` | 通道独立使能 |
| `0xF0` | 通道异常状态查询 |
| `0xFF` | 固件版本查询 |
| `0x50` ~ `0x8F` | 阀动作命令区 |

## 状态与指示灯说明

- 默认未连接串口时，所有通道显示为异常状态。
- 串口连接成功且收到有效数据后，通道异常状态会根据设备回包更新。
- 收到阀动作回包时，对应指示灯显示绿色。
- 指示灯采用短脉冲显示，当前亮灯时长约 `10 ms`，用于避免高速回包时旧阀位长时间残留高亮。

## 日志说明

日志会同时显示在界面表格和控制台输出中，格式为：

```text
时间戳 | 方向 | 十六进制 | 中文说明
```

示例：

```text
18:25:31.125 | TX | AA 50 00 01 | 请求测试通道 1 的阀 1
18:25:31.129 | RX | AA 50 00 01 | 通道 1 的阀 1 已确认执行
```

方向字段常见取值：

- `TX`：发送
- `RX`：接收
- `ERR`：错误
- `WARN`：警告
- `TIMEOUT`：等待回包超时
- `AUTH`：密码校验相关日志

## 参数保存与自动加载

软件使用 `QSettings` 保存以下内容：

- 串口参数
- 上次成功连接的串口
- 所有控制参数
- 显示阀个数
- 窗口几何信息

启动行为如下：

- 自动加载上次保存的控制参数
- 自动尝试连接上次成功连接的串口
- 串口连接成功后，自动下发非加密参数
- 加密时间参数只加载显示，不会在启动时自动下发

## 技术实现

### 架构划分

- `valve_core`
  协议编解码、命令定义、安全策略、阀地址解析
- `valve_serial`
  串口收发封装，基于 `CSerialPort`
- `valve_controller`
  命令排队、收发匹配、超时控制、状态解析
- `valve_ui`
  Qt Widgets 界面、参数面板、通道卡片、日志面板
- `valve_test_tool`
  最终可执行程序

### 线程模型

- GUI 主线程负责界面显示和用户交互
- `SerialWorkerRuntime` 运行在独立线程中
- 串口打开、写入、读取、回包处理均在线程内完成

## 依赖项

- CMake 3.16 及以上
- C++17 编译器
- Qt 5 或 Qt 6
  当前在 Windows 下按 Qt 5.14.2 配置验证
- 第三方串口库：`third_party/CSerialPort-master`

## Windows 编译

如果你的 Qt 安装路径与当前仓库一致：

```powershell
cmake -S . -B build-vs -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="D:\work\tools\QT\setup\5.14.2\msvc2017_64\lib\cmake"
cmake --build build-vs --target valve_test_tool --config Debug
```

生成结果：

```text
build-vs/Debug/valve_test_tool.exe
```

## Linux 编译

Linux 下请根据实际环境自行设置编译器、工具链和 Qt 路径。

一个典型示例：

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/lib/cmake
cmake --build build-linux --target valve_test_tool -j
```

如果使用交叉编译工具链，可在配置时额外传入：

```bash
-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake
```

仓库内默认预留了工具链文件变量：

```text
toolchain/toolchain_aarch64_up_3588.cmake
```

## 运行方式

Windows 下编译完成后直接运行：

```powershell
.\build-vs\Debug\valve_test_tool.exe
```

程序启动后会默认最大化显示。

## 常见问题

### 1. 旧的 `build-vs` 工程突然无法正确编译

如果出现以下情况：

- `valve_ui` 只编译 `mocs_compilation`
- 链接阶段提示 `ApplicationTheme::apply`、`MainWindow`、`SerialWorkerRuntime` 等符号无法解析

通常是 `build-vs` 下的 `CMakeCache.txt` 或 `CMakeFiles` 损坏。可按下面步骤重建：

```powershell
Remove-Item .\build-vs\CMakeCache.txt -Force
Remove-Item .\build-vs\CMakeFiles -Recurse -Force
cmake -S . -B build-vs -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="D:\work\tools\QT\setup\5.14.2\msvc2017_64\lib\cmake"
cmake --build build-vs --target valve_test_tool --config Debug
```

### 2. 软件启动时界面无法打开，并提示串口打开失败

当前版本只会自动连接“上次成功连接”的串口。

如果之前选择了一个无权限或被占用的串口：

- 请关闭占用该串口的其他程序
- 重新刷新并选择正确串口
- 连接成功后，软件会自动保存新的有效串口

### 3. 时间参数显示是毫秒，但日志里看到的是整数原始值

这是正常行为。

- 界面显示单位：`ms`
- 协议发送单位：`0.05 ms`

软件内部会自动完成换算，并在日志中文说明里同时显示毫秒值和实际下发原始值。

## 目录说明

```text
doc/                 协议文档
src/app/             程序入口、主题、工作线程运行时
src/core/            协议、命令映射、安全策略、地址解析
src/serial/          串口适配与收发服务
src/controller/      命令队列、超时、回包匹配
src/ui/              Qt Widgets 界面
third_party/         第三方依赖
toolchain/           工具链文件
```

## 说明

- 本项目协议实现以 `doc/阀板测试.xlsx` 为准。
- 如果后续协议发生变化，请优先更新 `src/core/CommandMap.h`、`src/core/ProtocolCodec.cpp`、`src/core/ValveAddressResolver.cpp` 及本 README。
