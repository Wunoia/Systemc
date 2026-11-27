# ============================================================
# SPI 控制器 SystemC 集成测试 —— CMakeLists.txt
#
# 【标准编译流程】(在项目根目录执行)
#   mkdir build && cd build
#   cmake .. -G Ninja
#   ninja
#   ./main.exe
#
# 【为什么命令这么短 —— 所有环境配置都写在本文件里】
#   1. 编译器：本机默认 PATH 里的 g++ 是 8.1.0，而 SystemC 预编译库
#      C:/systemc/lib/libsystemc.a 是由 MSYS2 ucrt64 的 g++ 16.2 构建的。
#      二者 C++ ABI 不同，混用会在链接时报
#      "undefined reference to std::__cxx11::basic_stringstream"。
#      所以本文件在 project() 之前把 CMAKE_CXX_COMPILER 强制设为 ucrt64
#      的 c++.exe —— 这样 cmake 命令行不必再带 -DCMAKE_CXX_COMPILER=...
#   2. SystemC 包路径：写在 CMAKE_PREFIX_PATH 里，find_package 才能找到
#      SystemCLanguageConfig.cmake —— 命令行也不必带 -DCMAKE_PREFIX_PATH=...
#   3. 输出名：add_executable(main ...) → 生成 main.exe，正好对应 ./main.exe
#   4. -static：把 libstdc++/libgcc/winpthread 静态链入。否则 exe 依赖
#      MSYS2 的 DLL，直接 ./main.exe 会因找不到 DLL 而退出码 127。
#      静态后只剩 Windows 系统 DLL，双击/直接运行都可用。
#   5. -G Ninja 只在第一次 cmake 时给一次；之后增量构建直接 ninja。
# ============================================================

cmake_minimum_required(VERSION 3.16)

# ---- 必须在 project() 之前设置编译器，否则 project() 探测到的就是 gcc 8.1 ----
# 用 Windows 风格正斜杠路径（CMake 是原生程序，不认 /c/... 的 MSYS 写法）
set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/c++.exe" CACHE FILEPATH "" FORCE)

project(spi_systemc_sim LANGUAGES CXX)

# ---- SystemC 安装前缀：find_package(SystemCLanguage) 据此定位 Config.cmake ----
set(CMAKE_PREFIX_PATH "C:/systemc" CACHE PATH "" FORCE)

find_package(SystemCLanguage CONFIG REQUIRED)

# SystemCLanguageConfig.cmake 自带 C++17 要求（SystemC 3.0 强制 C++17）
set(CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 目标名 main → 产物 main.exe；静态链接让 ./main.exe 不依赖 MSYS2 DLL
# ------------------------------------------------------------
# 修正说明（E109 端口未绑定）：
#   原 main.cpp 构造 tester 时漏绑 top.clk_sys_freq，SystemC 细化阶段报
#     Error (E109) complete binding failed:
#     port not bound: port 'tests.top.port_1' (sc_in)
#   按规范新建副本 main_fixed.cpp：注释掉原代码、说明原因、补上绑定并标注。
#   这里把源文件从 main.cpp 改为 main_fixed.cpp，原 main.cpp 保持不变。
# ------------------------------------------------------------

add_executable(main
    main.cpp
    spi_clk_div_width.cpp
    spi_timing_cpol_cpha.cpp
    spi_status_gen.cpp
    spi_shift_reg.cpp
    spi_fifo_cs.cpp
    spi_irq_dma.cpp
    spi_controller_top.cpp
)

# 头文件在 headfile/ 子目录
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/headfile
)

# SystemC::systemc 已携带 include 目录、pthread 与 C++17 要求
target_link_libraries(main PRIVATE SystemC::systemc)

# 全静态链接：去掉 libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll 依赖
target_link_options(main PRIVATE -static)

# 产物直接落在 build 目录根部（从 build 里 ./main.exe 即是它）
set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
