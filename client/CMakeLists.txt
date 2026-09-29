# ============ 上位机客户端的施工图纸 ============

# 界面程序：需要 Widgets（界面控件）+ Network（TCP 通信）
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

qt_add_executable(DeviceMonitorClient
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/DeviceClient.h
    src/DeviceClient.cpp
    src/RealtimeCurveWidget.h
    src/RealtimeCurveWidget.cpp
    src/HistoryTableModel.h
    src/HistoryTableModel.cpp
)

# 链接 Qt 模块 + 公共代码库（协议、帧解析器、控制台辅助）
target_link_libraries(DeviceMonitorClient PRIVATE
    Qt6::Widgets
    Qt6::Network
    devicecommon
)
