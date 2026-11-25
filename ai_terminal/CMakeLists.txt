cmake_minimum_required(VERSION 3.16)
project(ai_terminal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(AI_TERMINAL_SOURCES
    src/main.c
    src/app.c
    src/json_util.c
    src/storage.c
    src/net.c
    src/lan_game.c
    src/voice.c
    src/pinyin.c
    src/snake.c
    src/ui/ui_theme.c
    src/ui/ui_login.c
    src/ui/ui_home.c
    src/ui/ui_chat.c
    src/ui/ui_history.c
    src/ui/ui_settings.c
    src/ui/ui_about.c
    src/ui/ui_game.c
)

add_executable(ai_terminal ${AI_TERMINAL_SOURCES})

# lv_conf.h 位于本工程根目录，通过 LV_CONF_PATH 告知 LVGL（LVGL 会把该目录加入 include）
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "Path of lv_conf.h" FORCE)

if(DEFINED LVGL_DIR)
    add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl)
else()
    include(FetchContent)
    # 只编译 LVGL 本体，避免拉入 examples/demos/thorvg（thorvg 需要 C++ 编译器）
    set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
    set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
    set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(lvgl
        GIT_REPOSITORY https://github.com/lvgl/lvgl.git
        GIT_TAG v9.2.2
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(lvgl)
endif()

if(NOT TARGET lvgl)
    message(FATAL_ERROR "ai_terminal: 未找到 LVGL 目标，请检查 -DLVGL_DIR 或网络")
endif()

target_compile_definitions(ai_terminal PRIVATE LV_CONF_INCLUDE_SIMPLE=1 LV_LVGL_H_INCLUDE_SIMPLE=1)
target_include_directories(ai_terminal PRIVATE include . ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ai_terminal PRIVATE lvgl pthread m)
