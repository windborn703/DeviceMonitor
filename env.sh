# ============ 开发环境配置脚本 ============
# 作用：把 Qt 工具链临时加进 PATH（只在当前命令行窗口有效）
# 用法：每次新开一个命令行窗口编译前，先运行：
#   source env.sh
# 面试可聊：为什么 Qt 不自动加 PATH？→ 避免和系统里其他版本编译器冲突

export PATH="/d/Qt6/Tools/CMake_64/bin:/d/Qt6/Tools/Ninja:/d/Qt6/Tools/mingw1120_64/bin:/d/Qt6/6.5.3/mingw_64/bin:$PATH"

echo "✅ 环境就绪：cmake $(cmake --version | head -1 | awk '{print $3}') | g++ $(g++ -dumpversion) | Qt 6.5.3"
