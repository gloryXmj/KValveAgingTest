cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchain/toolchain_aarch64_up_3588.cmake") 
set(KVALVE_DEFAULT_TOOLCHAIN_FILE
    "${CMAKE_CURRENT_LIST_DIR}/toolchain/toolchain_aarch64_up_3588.cmake")

if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${KVALVE_DEFAULT_TOOLCHAIN_FILE}"
        CACHE FILEPATH "Toolchain file for ARM64 builds")
endif()

message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
project(KValveAgingTestTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)
include(cmake/AddCSerialPort.cmake)

add_cserialport_dependency()

add_library(valve_core
    src/core/ProtocolCodec.cpp
    src/core/SecurityPolicy.cpp
    src/core/ValveAddressResolver.cpp
)
target_include_directories(valve_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_core PUBLIC Qt${QT_VERSION_MAJOR}::Core)

add_library(valve_serial
    src/serial/ISerialTransport.h
    src/serial/AckFrameStreamParser.cpp
    src/serial/CSerialPortAdapter.h
    src/serial/CSerialPortAdapter.cpp
    src/serial/SerialPortService.h
    src/serial/SerialPortService.cpp
)
target_include_directories(valve_serial PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_serial PUBLIC Qt${QT_VERSION_MAJOR}::Core valve_core libcserialport)

add_library(valve_controller
    src/controller/ValveTestController.h
    src/controller/ValveTestController.cpp
)
target_include_directories(valve_controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_controller PUBLIC Qt${QT_VERSION_MAJOR}::Core valve_core valve_serial)

add_library(valve_ui
    src/app/ApplicationTheme.h
    src/app/ApplicationTheme.cpp
    src/app/ProtocolConsoleLogger.h
    src/app/ProtocolConsoleLogger.cpp
    src/app/SerialWorkerRuntime.h
    src/app/SerialWorkerRuntime.cpp
    src/config/AppSettings.h
    src/config/AppSettings.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PasswordDialog.h
    src/ui/PasswordDialog.cpp
    src/ui/NumericInputDialog.h
    src/ui/NumericInputDialog.cpp
    src/ui/TouchSpinBoxWidget.h
    src/ui/TouchSpinBoxWidget.cpp
    src/ui/ChannelCardWidget.h
    src/ui/ChannelCardWidget.cpp
    src/ui/ValveGridWidget.h
    src/ui/ValveGridWidget.cpp
    src/ui/ValveIndicatorWidget.h
    src/ui/ValveIndicatorWidget.cpp
    src/ui/ParameterPanelWidget.h
    src/ui/ParameterPanelWidget.cpp
    src/ui/LogPanelWidget.h
    src/ui/LogPanelWidget.cpp
)
target_include_directories(valve_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(valve_ui PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets valve_controller valve_serial valve_core)

add_executable(valve_test_tool
    src/app/main.cpp
)
target_link_libraries(valve_test_tool PRIVATE valve_ui)
