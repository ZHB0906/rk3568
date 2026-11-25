# 交叉编译工具链（aarch64 / Buildroot）
#
# 用法：
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux-gnu.cmake \
#         -DLVGL_DIR=/path/to/lvgl
#
# 两种指定编译器的方式，任选其一：
#   1) 环境变量 AARCH64_TOOLCHAIN_DIR 指向 Buildroot 工具链的 .../host/bin
#      （例如 /opt/rk3568/host/bin），脚本会自动拼出 aarch64-buildroot-linux-gnu-gcc
#   2) 系统里已有 aarch64-linux-gnu-gcc（如 Debian/Ubuntu 的 gcc-aarch64-linux-gnu）
#
# 若板子上的 C 库是 musl，请把 CROSS_PREFIX 改成 aarch64-linux-musl-

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(DEFINED ENV{AARCH64_TOOLCHAIN_DIR})
    set(CROSS_DIR "$ENV{AARCH64_TOOLCHAIN_DIR}")
    # Buildroot 工具链常见前缀，按实际目录二选一
    if(EXISTS "${CROSS_DIR}/aarch64-buildroot-linux-gnu-gcc")
        set(CROSS_PREFIX "aarch64-buildroot-linux-gnu-")
    else()
        set(CROSS_PREFIX "aarch64-linux-gnu-")
    endif()
    set(CMAKE_C_COMPILER   "${CROSS_DIR}/${CROSS_PREFIX}gcc")
    set(CMAKE_CXX_COMPILER "${CROSS_DIR}/${CROSS_PREFIX}g++")
    set(CMAKE_SYSROOT      "${CROSS_DIR}/../aarch64-buildroot-linux-gnu/sysroot")
else()
    set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
