#ifndef AI_TERMINAL_UI_THEME_H
#define AI_TERMINAL_UI_THEME_H

#include "lvgl.h"
#include "app.h"

/* 颜色常量（0xRRGGBB） */
#define UI_C_BG         0xF3F5F9
#define UI_C_CARD       0xFFFFFF
#define UI_C_PRIMARY    0x2F6FEB
#define UI_C_PRIMARY_DK 0x1D4FBF
#define UI_C_PRIMARY_LT 0xE8F0FE
#define UI_C_TEXT       0x1F2430
#define UI_C_MUTED      0x6B7280
#define UI_C_LINE       0xDDE1E8
#define UI_C_USER       0xD6E4FF
#define UI_C_AI         0xECEEF2
#define UI_C_DANGER     0xD93838
#define UI_C_OK         0x1A9C5B
#define UI_C_BOARD      0x151A23

/* 中文界面字体：LVGL 9.2.x 内置 SimSun-16；9.3+ 内置 Source Han Sans SC 16。
 * 两者都是 LVGL 自带字体，不产生任何额外字体数据。 */
#if (LVGL_VERSION_MAJOR > 9) || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR >= 3)
#  define UI_FONT (&lv_font_source_han_sans_sc_16_cjk)
#else
#  define UI_FONT (&lv_font_simsun_16_cjk)
#endif

#define UI_SCREEN_W 1024
#define UI_SCREEN_H 600
#define UI_HEADER_H 60

/* 全局主题：默认屏幕底色 / 字体 / 文字颜色 */
void apply_theme(void);

/* 给某个屏幕套用统一的底色、字体、禁止滚动 */
void ui_apply_screen_style(lv_obj_t *scr);

/* 顶部标题栏，back_cb 为 NULL 时不显示返回按钮 */
lv_obj_t *ui_create_header(lv_obj_t *parent, const char *title, void (*back_cb)(void));

/* 页面切换：create_fn 创建新屏幕并带左移动画加载，旧屏幕自动删除 */
void ui_nav_to(lv_obj_t *(*create_fn)(void));

/* 程序入口：应用主题并进入登录页 */
void ui_app_start(void);

/* 常用控件封装 */
lv_obj_t *ui_make_button(lv_obj_t *parent, const char *text, int32_t w, int32_t h);
lv_obj_t *ui_make_label(lv_obj_t *parent, const char *text);
lv_obj_t *ui_make_card(lv_obj_t *parent);

#endif /* AI_TERMINAL_UI_THEME_H */
