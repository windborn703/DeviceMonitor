# ============ 测试程序的施工图纸 ============

# --- test_protocol：纯 C++ 协议打包测试，不依赖 Qt ---
# 验证协议层与 Qt 解耦：协议代码不依赖任何 Qt 类，将来可以直接拿到别的环境用
add_executable(test_protocol test_protocol.cpp)
target_include_directories(test_protocol PRIVATE ${CMAKE_SOURCE_DIR}/common)

# --- test_connect：验证小客户端，需要 Qt 的 Network 模块 ---
find_package(Qt6 REQUIRED COMPONENTS Network)
qt_add_executable(test_connect test_connect.cpp)
target_link_libraries(test_connect PRIVATE Qt6::Network)
