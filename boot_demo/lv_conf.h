/**
 * @file lv_conf.h
 * LVGL 9.2.2 配置文件 —— rk3568 开机硬件自检桌面
 *
 * 该文件通过 -DLV_CONF_INCLUDE_SIMPLE=1 被 LVGL 以 #include "lv_conf.h" 找到。
 */
#if 1 /* 1: 启用本配置文件 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   颜色设置
 *====================*/
/* 1: I1, 8: L8, 16: RGB565, 24: RGB888, 32: XRGB8888
 * RK3568 的 /dev/fb0 通常为 32bpp，故使用 32 */
#define LV_COLOR_DEPTH 32

/*=========================
   标准库封装
 *=========================*/
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
    /* LVGL 内部堆大小 */
    #define LV_MEM_SIZE (512U * 1024U)
    #define LV_MEM_POOL_EXPAND_SIZE 0
    #define LV_MEM_ADR 0
    #if LV_MEM_ADR == 0
        #undef LV_MEM_POOL_INCLUDE
        #undef LV_MEM_POOL_ALLOC
    #endif
#endif

/*====================
   HAL 设置
 *====================*/
#define LV_DEF_REFR_PERIOD  16      /* [ms] */
#define LV_DPI_DEF          130     /* [px/inch] */

/*=================
   操作系统
 *=================*/
/* 主线程单线程跑 LVGL，慢速命令用独立 pthread，故 LVGL 内部不需要 OS 支持 */
#define LV_USE_OS   LV_OS_NONE

/*========================
   渲染配置
 *========================*/
#define LV_DRAW_BUF_STRIDE_ALIGN    1
#define LV_DRAW_BUF_ALIGN           4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (24 * 1024)
#define LV_DRAW_THREAD_STACK_SIZE   (8 * 1024)

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     1
    #define LV_DRAW_SW_SUPPORT_RGB888       1
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1
    #define LV_DRAW_SW_SUPPORT_L8           1
    #define LV_DRAW_SW_SUPPORT_AL88         1
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           1
    #define LV_DRAW_SW_DRAW_UNIT_CNT        1
    #define LV_USE_DRAW_ARM2D_SYNC          0
    #define LV_USE_NATIVE_HELIUM_ASM        0
    #define LV_DRAW_SW_COMPLEX              1
    #if LV_DRAW_SW_COMPLEX == 1
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif
    #define LV_USE_DRAW_SW_ASM     LV_DRAW_SW_ASM_NONE
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif

#define LV_USE_DRAW_VGLITE 0
#define LV_USE_PXP 0
#define LV_USE_DRAW_DAVE2D 0
#define LV_USE_DRAW_SDL 0
#define LV_USE_DRAW_VG_LITE 0

/*=======================
   功能配置
 *=======================*/
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /* 断言时停机，而不是继续跑坏状态 */

#define LV_USE_REFR_DEBUG 0
#define LV_USE_LAYER_DEBUG 0
#define LV_USE_PARALLEL_DRAW_DEBUG 0

#define LV_ENABLE_GLOBAL_CUSTOM 0
#define LV_CACHE_DEF_SIZE       0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_GRADIENT_MAX_STOPS   2
#define LV_COLOR_MIX_ROUND_OFS  0
#define LV_OBJ_STYLE_CACHE      0
#define LV_USE_OBJ_ID           0
#define LV_OBJ_ID_AUTO_ASSIGN   LV_USE_OBJ_ID
#define LV_USE_OBJ_ID_BUILTIN   1
#define LV_USE_OBJ_PROPERTY 0
#define LV_USE_OBJ_PROPERTY_NAME 1
#define LV_USE_VG_LITE_THORVG  0

/*=====================
   编译器设置
 *=====================*/
#define LV_BIG_ENDIAN_SYSTEM 0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_USE_LARGE_COORD 0

/*=====================
   字体设置
 *=====================*/
/* 需求：使用 LVGL 内置 Source Han Sans 16 CJK 作为默认字体。
 * 但该字体文件从 LVGL v9.3.0 才随源码提供，v9.2.2 源码树里没有。
 * 这里编译期探测字体源文件是否存在，缺失时自动回退到 lv_font_simsun_16_cjk，
 * 保证工程在任何 9.x 版本上都能编译通过。 */
#if defined(__has_include)
#  if __has_include("src/font/lv_font_source_han_sans_sc_16_cjk.c")
#    define BOOT_DEMO_HAS_HAN_FONT 1
#  elif __has_include("lvgl/src/font/lv_font_source_han_sans_sc_16_cjk.c")
#    define BOOT_DEMO_HAS_HAN_FONT 1
#  else
#    define BOOT_DEMO_HAS_HAN_FONT 0
#  endif
#else
#  define BOOT_DEMO_HAS_HAN_FONT 0
#endif

#if BOOT_DEMO_HAS_HAN_FONT
    #define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK 1
    #define LV_FONT_DEFAULT &lv_font_source_han_sans_sc_16_cjk
#else
    #define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK 0
    #define LV_FONT_SIMSUN_16_CJK 1
    #define LV_FONT_DEFAULT &lv_font_simsun_16_cjk
#endif

#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_14_CJK 0
#define LV_FONT_UNSCII_8 0
#define LV_FONT_UNSCII_16 0
#define LV_FONT_CUSTOM_DECLARE

/* 字形缺失时画占位方框，避免显示为空白 */
#define LV_USE_FONT_PLACEHOLDER 1
#define LV_FONT_FMT_TXT_LARGE 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_SUBPX 0
#if LV_USE_FONT_SUBPX
    #define LV_FONT_SUBPX_BGR 0
#endif

/*=================
   文本设置
 *=================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD "#"
#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
   控件
 *==================*/
#define LV_WIDGETS_HAS_DEFAULT_VALUE 1
#define LV_USE_ANIMIMG    1
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BUTTON     1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR   0
#define LV_USE_CANVAS     1
#define LV_USE_CHART      0
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   0
#define LV_USE_IMAGE      1
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD   0
#define LV_USE_LABEL      1
#define LV_USE_LED        1
#define LV_USE_LINE       1
#define LV_USE_LIST       1
#define LV_USE_MENU       0
#define LV_USE_MSGBOX     1
#define LV_USE_ROLLER     0
#define LV_USE_SCALE      0
#define LV_USE_SLIDER     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    0
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      1
#define LV_USE_TABVIEW    0
#define LV_USE_TEXTAREA   1
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0
#define LV_USE_3DTEXTURE  0
#define LV_LABEL_TEXT_SELECTION 1

/*==================
   主题
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 1
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif
#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO 0

/*==================
   布局
 *==================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
   第三方库（全部关闭，不引入额外依赖）
 *====================*/
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0
#define LV_USE_FS_MEMFS 0
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG 0
#define LV_USE_BMP 0
#define LV_USE_TJPGD 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_FREETYPE 0
#define LV_USE_TINY_TTF 0
#define LV_USE_RLOTTIE 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0

/*==================
   其他
 *==================*/
#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_PROFILER 0
#define LV_USE_OBSERVER 1
#define LV_USE_GRIDNAV 0
#define LV_USE_FRAGMENT 0

/*==================
   驱动设备
 *==================*/
#define LV_USE_SDL 0
#define LV_USE_X11 0
#define LV_USE_WAYLAND 0
/* /dev/fb0 显示 */
#define LV_USE_LINUX_FBDEV 1
#define LV_LINUX_FBDEV_BSD 0
#define LV_LINUX_FBDEV_RENDER_MODE LV_DISPLAY_RENDER_MODE_PARTIAL
#define LV_LINUX_FBDEV_BUFFER_SIZE 60
#define LV_LINUX_FBDEV_BUFFER_COUNT 1
/* evdev 输入（触摸屏） */
#define LV_USE_EVDEV 1
#define LV_USE_LIBINPUT 0
#define LV_USE_NUTTX 0
#define LV_USE_LINUX_DRM 0
#define LV_USE_TFT_ESPI 0
#endif /* LV_CONF_H */
#endif /* 1 */
