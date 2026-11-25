cmake_minimum_required(VERSION 3.16)

project(boot_demo C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# LVGL 9
#   - 如果传入 -DLVGL_DIR=/path/to/lvgl 则直接使用该源码树
#   - 否则用 FetchContent 拉取 LVGL 源码
# 说明：LVGL v9.2.2 的源码树中没有 Source Han Sans CJK 字体
#       （该字体从 v9.3.0 才随源码提供），lv_conf.h 会自动检测并回退到
#       内置 simsun_16_cjk。若想使用 Source Han Sans，可加
#       -DLVGL_TAG=v9.3.0。
# ---------------------------------------------------------------------------
set(LVGL_TAG "v9.2.2" CACHE STRING "LVGL git tag, e.g. v9.2.2 or v9.3.0")

if(DEFINED LVGL_DIR AND EXISTS "${LVGL_DIR}/CMakeLists.txt")
    message(STATUS "boot_demo: 使用本地 LVGL 源码树 ${LVGL_DIR}")
    add_subdirectory("${LVGL_DIR}" "${CMAKE_BINARY_DIR}/lvgl")
else()
    include(FetchContent)
    set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
    set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
    set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(lvgl
        GIT_REPOSITORY https://github.com/lvgl/lvgl.git
        GIT_TAG ${LVGL_TAG}
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(lvgl)
    message(STATUS "boot_demo: 使用 FetchContent 获取 LVGL ${LVGL_TAG}")
endif()

if(NOT TARGET lvgl)
    message(FATAL_ERROR "boot_demo: 未找到 LVGL 目标，请检查 -DLVGL_DIR 或网络")
endif()

# 让 LVGL 自身的源码也能通过 "#include \"lv_conf.h\"" 找到本工程的配置
target_include_directories(lvgl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# 缺少 Source Han Sans 字体时给出提示（v9.2.2 属于这种情况）
if(DEFINED lvgl_SOURCE_DIR)
    if(NOT EXISTS "${lvgl_SOURCE_DIR}/src/font/lv_font_source_han_sans_sc_16_cjk.c")
        message(STATUS "boot_demo: LVGL ${LVGL_TAG} 不含 Source Han Sans 16 CJK 字体，"
                       "lv_conf.h 将回退到 simsun_16_cjk（-DLVGL_TAG=v9.3.0 可获得 Source Han Sans）")
    endif()
endif()

# ---------------------------------------------------------------------------
# 源文件（新增页面时在此登记）
# ---------------------------------------------------------------------------
set(BOOT_DEMO_SOURCES
    src/main.c
    src/hw.c
    src/ui_common.c
    src/pages_registry.c
    src/pages/page_rtc.c
    src/pages/page_led.c
    src/pages/page_buzzer.c
    src/pages/page_backlight.c
    src/pages/page_key.c
    src/pages/page_lightsensor.c
    src/pages/page_gsensor.c
    src/pages/page_adc.c
    src/pages/page_audio.c
    src/pages/page_touch.c
    src/pages/page_udisk.c
    src/pages/page_tfcard.c
    src/pages/page_eeprom.c
    src/pages/page_uart.c
    src/pages/page_can.c
    src/pages/page_eth.c
    src/pages/page_4g.c
    src/pages/page_wifi.c
    src/pages/page_bt.c
)

add_executable(boot_demo ${BOOT_DEMO_SOURCES})

target_include_directories(boot_demo PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(boot_demo PRIVATE
    LV_CONF_INCLUDE_SIMPLE=1
    LV_LVGL_H_INCLUDE_SIMPLE=1)

target_link_libraries(boot_demo PRIVATE lvgl pthread m)
