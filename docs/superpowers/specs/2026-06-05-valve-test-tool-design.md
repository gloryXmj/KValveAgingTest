# Valve Board Test Tool Design

## Overview

This document defines the first release of a cross-platform desktop test tool for the valve board protocol described in the protocol workbook under `doc/`. The application is implemented in C++ with a Qt Widgets UI, uses CMake for project management, and supports both Windows and Linux.

The tool sends fixed-length serial commands in the format `0xAA + command(1 byte) + data(2 bytes)` and interprets fixed-length acknowledgement frames in the format `0xA0 + command(1 byte) + data(2 bytes)`. The software must present an operator-friendly commercial-style interface, protect sensitive timing parameters with password confirmation, and highlight the exact valve that was reported as executed by the device.

## Scope

The first release includes:

- Cross-platform Qt desktop application for Windows and Linux
- CMake-based build system
- Serial communication through a mature third-party GitHub library rather than `QSerialPort`
- Full command mapping for the protocol ranges used by channels `1` through `8`
- Visual display of `8` channels with `128` valves per channel
- Password confirmation for modifications to blow time, charge time, stop charge time, and recharge time
- Command send/ack workflow with timeout, mismatch, and error reporting
- Raw hexadecimal log view with human-readable interpretation

The first release explicitly excludes:

- Automated test scripting or scenario playback
- Remote network control
- Database persistence
- Fine-grained user roles beyond the fixed password gate

## Protocol Assumptions

The following assumptions were confirmed during design review:

- The hardware supports exactly `8` channels.
- Valve control commands occupy `0x50` through `0x8F`.
- A command frame is always `4` bytes:
  - `0xAA`
  - `command`
  - `data high`
  - `data low`
- A successful acknowledgement frame is always `4` bytes:
  - `0xA0`
  - `same command`
  - `same data high`
  - `same data low`
- Example:
  - Sent: `AA 50 00 01`
  - Ack: `A0 50 00 01`
  - Meaning: channel `1`, valve `1` executed successfully
- The fixed password for protected timing parameters is `123`.

If the firmware later introduces checksum bytes, extended responses, or asynchronous event frames, the protocol layer must be extended without changing the UI or controller contracts.

## Functional Requirements

### Connection and Device Control

- Operators can select a serial port and baud rate, then connect or disconnect.
- The application shows current connection state clearly.
- The operator can send protocol commands for:
  - valve switch
  - trigger mode
  - blow count
  - blow interval
  - blow time
  - charge time
  - stop charge time
  - recharge time
  - channel count
  - version query

### Valve Visualization

- The main view shows `8` channel cards.
- Each channel card contains `128` valve indicators.
- When a valve ack is received, the matching indicator lights for `100 ms`.
- If a response represents multiple active bits in the same `data16`, all affected valves must light.
- A channel-level abnormal state is shown when an abnormal bit is reported through command `0xF0`.

### Password Protection

- The following parameters require password confirmation before send:
  - `0x05` blow time
  - `0x06` charge time
  - `0x07` stop charge time
  - `0x08` recharge time
- These fields use explicit apply actions rather than auto-submit on edit.
- If the password is wrong:
  - no command is sent
  - the pending edited value is not promoted to confirmed device state
  - the log records the rejected attempt

### Logging and Feedback

- Every send operation is logged in raw hexadecimal form.
- Every received frame is logged in raw hexadecimal form.
- A decoded operator-facing description accompanies each log line, such as `CH1 Valve 1 action confirmed`.
- Errors, timeouts, mismatches, and abnormal channel reports are logged with timestamps.

## Non-Functional Requirements

- The project must build on Windows and Linux with the same top-level CMake flow.
- The UI must look intentional and operator-friendly rather than like a default engineering form.
- Core protocol logic must be testable without the UI or real hardware.
- Serial implementation details must stay behind an abstraction so the communication library can be replaced if needed.

## High-Level Architecture

The application is split into four layers:

1. UI layer
2. Application/controller layer
3. Protocol/domain layer
4. Serial communication layer

This separation prevents protocol parsing, serial I/O, and UI rendering from becoming tightly coupled in `MainWindow`.

### UI Layer

Primary responsibilities:

- compose the main window
- collect user actions
- render live channel/valve state
- display connection, parameter, and log status
- prompt for password when required

Main UI elements:

- top connection/status bar
- left parameter and quick-action panel
- right-side `2 x 4` channel card grid
- bottom log panel

### Controller Layer

`ValveTestController` is the application coordinator.

Responsibilities:

- accept UI requests
- validate whether a command is allowed
- request password validation for protected parameters
- encode commands into protocol frames
- send through the serial service
- track the single in-flight command awaiting acknowledgement
- process received ack frames
- emit decoded results back to the UI

### Protocol and Domain Layer

This layer contains serial-frame-independent logic:

- command definitions and categories
- frame encode/decode
- command-to-channel/valve mapping
- abnormal bitmap interpretation
- security policy for protected commands

This layer must have no widget or third-party serial dependencies.

### Serial Communication Layer

The serial layer wraps a mature GitHub-hosted cross-platform library instead of `QSerialPort`.

The selected initial dependency is `CSerialPort`, vendored into `third_party/CSerialPort/` and hidden behind `SerialPortService`.

Responsibilities:

- enumerate ports
- open and close a port
- configure baud rate and port settings
- send raw bytes
- receive raw bytes asynchronously
- buffer incoming bytes and forward complete frames upward
- surface transport-level errors

## Command Model

All outgoing user actions are converted into a small domain object:

```text
CommandPacket {
  quint8 command;
  quint16 data16;
  CommandPurpose purpose;
}
```

The UI never constructs raw byte arrays directly.

### General Parameters

- `0x00` valve switch
- `0x01` trigger mode
- `0x02` blow count
- `0x03` blow interval
- `0x05` blow time
- `0x06` charge time
- `0x07` stop charge time
- `0x08` recharge time
- `0x09` channel count
- `0xFF` version query

### Valve Segment Addressing

Valve state commands are mapped as follows:

- `0x50` to `0x57`: channel `1`
- `0x58` to `0x5F`: channel `2`
- `0x60` to `0x67`: channel `3`
- `0x68` to `0x6F`: channel `4`
- `0x70` to `0x77`: channel `5`
- `0x78` to `0x7F`: channel `6`
- `0x80` to `0x87`: channel `7`
- `0x88` to `0x8F`: channel `8`

Within each channel:

- each command covers `16` valves
- `segment = (command - channelBase) % 8`
- segment `0` maps to valves `1` through `16`
- segment `1` maps to valves `17` through `32`
- segment `7` maps to valves `113` through `128`

Bit interpretation:

- `bit0` means the first valve in the segment
- `bit15` means the last valve in the segment

Example:

- `AA 50 00 01` targets channel `1`, segment `0`, valve `1`
- `AA 50 00 04` targets channel `1`, segment `0`, valve `3`
- `AA 51 80 01` targets channel `1`, segment `1`, valves `17` and `32`

## Acknowledgement Handling

Incoming data is treated as a byte stream. The serial layer uses a sliding buffer parser:

- scan for header `0xA0`
- wait until at least `4` bytes are available
- extract one candidate frame
- pass the candidate to the protocol layer

The controller enforces a single-command-in-flight rule for the first release. This is intentional because the current ack format has no sequence identifier. Limiting traffic to one pending command at a time prevents ambiguous matching when the operator clicks quickly.

An ack is considered successful only if all of the following match the in-flight command:

- header is `0xA0`
- command byte matches
- `data16` matches

If the ack matches a valve segment command, the controller resolves the affected valves and tells the UI which channel and valve indices to flash for `100 ms`.

If the ack matches a parameter or version command, the controller updates the corresponding displayed state and log entry but does not flash a valve indicator.

## Channel and Valve Resolution

`ValveAddressResolver` converts command/data combinations into valve identities.

For valve commands:

- `channel = ((command - 0x50) / 8) + 1`
- `segment = (command - 0x50) % 8`
- `startValve = segment * 16 + 1`
- for each set bit in `data16`:
  - `valve = startValve + bitIndex`

Example:

- ack `A0 50 00 01`
  - channel `1`
  - valve `1`
- ack `A0 50 00 03`
  - channel `1`
  - valves `1` and `2`
- ack `A0 5B 00 10`
  - channel `2`
  - segment `3`
  - valve `53`

## Abnormal Channel Handling

Command `0xF0` is interpreted as a channel abnormal bitmap:

- low `8` bits correspond to channels `1` through `8`
- `0` means normal
- `1` means abnormal

UI behavior:

- affected channel card changes to warning styling
- the top status bar shows a board warning summary
- the log records the abnormal bitmap and resolved channel list

The application does not auto-disconnect on a channel abnormal event. Operators keep manual control unless transport-level failure occurs.

## Password Workflow

Protected timing fields are edited as pending values and confirmed only after successful password entry and device acknowledgement.

Flow:

1. operator edits one of the protected timing values
2. operator clicks apply
3. `PasswordDialog` appears
4. `SecurityPolicy` validates the typed password against `123`
5. on success:
   - the command is sent
   - the field enters pending state
6. on ack success:
   - the displayed confirmed value updates
7. on password failure:
   - no command is sent
   - the confirmed value remains unchanged
   - a warning log is added

This design avoids the common failure mode where UI state changes before the device has actually accepted the parameter.

## Error Handling

### Connection Errors

- If opening the serial port fails, the UI shows `connection failed`.
- Control actions that require a live connection are disabled.
- The error message from the serial layer is logged.

### Send Errors

- If the port is not connected, send actions are blocked.
- If the library reports a write failure, the in-flight command is rejected and the log records the failure.

### Timeout

- Each command starts an acknowledgement timer.
- The default timeout is configurable and initially set in the `300 ms` to `500 ms` range.
- On timeout:
  - the command is marked failed
  - the UI is informed
  - the log records the raw command that timed out

### Unexpected or Invalid Frames

- Frames with a wrong header, incomplete length, or invalid structure are logged as protocol anomalies.
- Frames that do not match the current in-flight command are logged as unexpected acknowledgements and must not be treated as success.

### Valve Indicator Recovery

- Each valve flash uses a one-shot `100 ms` reset path.
- Concurrent flashes on different valves must not interfere with each other.

## UI Design

The UI style should look like a commercial device console:

- dark-on-light or deep-neutral card styling rather than the default widget appearance
- clear visual hierarchy
- distinct status colors
- compact but readable density
- consistent spacing, borders, and typography

### Main Layout

#### Top Status Bar

Shows:

- application title
- current connection state
- selected serial port
- baud rate
- last ack time
- device version
- global abnormal summary

#### Left Control Column

Contains three cards:

- connection settings
- test parameters
- quick actions

The protected timing fields have explicit apply buttons and a visual cue indicating they are password-protected.

#### Right Channel Dashboard

- `8` channel cards arranged as a `2 x 4` grid
- each card shows:
  - channel title such as `CH 01`
  - channel state badge
  - valve count text
  - `16 x 8` valve indicator matrix

#### Bottom Log Panel

Shows timestamped send, receive, and error events with both raw hexadecimal and decoded descriptions.

### Indicator States

- idle: muted gray-blue
- active: bright cyan-green
- abnormal: red emphasis on the channel card
- disconnected: desaturated or disabled appearance

## Project Structure

```text
CMakeLists.txt
cmake/
third_party/CSerialPort/
src/app/
src/config/
src/controller/
src/core/
src/serial/
src/ui/
resources/
tests/
```

Proposed file groups:

- `src/app/`
  - `main.cpp`
  - `ApplicationTheme.*`
- `src/config/`
  - `AppSettings.*`
- `src/controller/`
  - `ValveTestController.*`
- `src/core/`
  - `CommandPacket.*`
  - `ProtocolCodec.*`
  - `CommandMap.*`
  - `ValveAddressResolver.*`
  - `SecurityPolicy.*`
- `src/serial/`
  - `SerialPortService.*`
  - `SerialPortTypes.*`
- `src/ui/`
  - `MainWindow.*`
  - `PasswordDialog.*`
  - `ChannelCardWidget.*`
  - `ValveGridWidget.*`
  - `ValveIndicatorWidget.*`
  - `ParameterPanelWidget.*`
  - `LogPanelWidget.*`

## Configuration Persistence

Persisted locally through `QSettings`:

- last selected serial port
- last selected baud rate
- window geometry
- splitter or panel layout state
- ack timeout setting
- auto-refresh port list preference

Not persisted:

- protected password
- transient in-flight commands
- raw device traffic history

## Build and Dependency Strategy

- Top-level project uses CMake.
- Qt Widgets is the UI framework.
- `CSerialPort` is vendored under `third_party/` and linked through a narrow wrapper.
- The application must build on:
  - Windows
  - Linux

The codebase should avoid platform-specific branches outside the serial-wrapper boundary unless packaging or OS-specific port naming requires it.

## Test Strategy

Testing focuses on deterministic logic first.

### Unit Tests

Target the core layer:

- frame encoding
- ack parsing
- channel and valve resolution
- multi-bit valve decoding
- abnormal bitmap decoding
- password gating behavior

### Controller Tests

Use a fake serial service to verify:

- one-command-in-flight behavior
- success ack matching
- mismatch rejection
- timeout handling
- protected parameter send suppression on bad password
- valve flash event emission

### Manual Integration Tests

Required checks on both Windows and Linux:

- application launches
- serial port connects and disconnects
- `AA 50 00 01` followed by `A0 50 00 01` flashes `CH1 Valve 1`
- multi-bit acknowledgements flash all expected valves
- protected timing fields always prompt for password
- abnormal channel bitmaps mark the correct channel cards

## Risks and Mitigations

### Risk: Ambiguous Acks Under Fast User Input

Mitigation:

- enforce one command in flight
- queue later actions

### Risk: Future Firmware Changes Ack Format

Mitigation:

- isolate frame parsing in `ProtocolCodec`
- avoid letting UI or controller depend on raw byte positions

### Risk: Large Widget Count Hurts Performance

Mitigation:

- keep each valve indicator lightweight
- prefer thin widgets or custom painting over heavy nested behavior

### Risk: Third-Party Serial Dependency Integration

Mitigation:

- wrap all direct library usage in `SerialPortService`
- vendor the library source rather than scattering usage across the codebase

## Acceptance Criteria

The first release is accepted when:

- the project builds with CMake on Windows and Linux
- the UI presents `8` channels with `128` valves each
- commands are sent in the specified `AA + cmd + data16` format
- acknowledgements in `A0 + cmd + data16` format are matched correctly
- the correct valve indicator flashes for `100 ms` after a successful valve ack
- the four protected timing parameters require the fixed password `123`
- abnormal channel bitmaps are shown in the UI
- logs display both raw bytes and decoded action meaning
