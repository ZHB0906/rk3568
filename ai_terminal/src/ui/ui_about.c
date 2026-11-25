#include "ui_about.h"
#include "ui_theme.h"
#include "ui_home.h"
#include "app.h"

#include <stdio.h>

static void about_back(void)
{
    ui_nav_to(ui_home_create);
}

static void about_add_line(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = ui_make_label(parent, text);

    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, lv_pct(100));
}

lv_obj_t *ui_about_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *body;
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *footer;
    char buf[256];

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 0, 0);

    ui_create_header(scr, "关于", about_back);

    body = lv_obj_create(scr);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 24, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    card = ui_make_card(body);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_style_pad_all(card, 24, 0);
    lv_obj_set_style_pad_row(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    title = ui_make_label(card, APP_NAME "  v" APP_VERSION);
    lv_obj_set_style_text_color(title, lv_color_hex(UI_C_PRIMARY), 0);

    about_add_line(card, "基于 LVGL 9.2.2 与纯 C11 构建，不依赖 cJSON / FreeType / SDL。");
    about_add_line(card, "运行平台：RK3568，Buildroot aarch64，1024x600 帧缓冲与 evdev 触摸。");
    about_add_line(card, "功能：登录、流式 AI 聊天、历史记录、设置、拼音输入法、贪吃蛇。");
    about_add_line(card, "语音识别 / 语音合成：接口已预留，当前版本返回未实现。");
    about_add_line(card, "联机乒乓：接口已预留，当前版本返回未实现。");

    snprintf(buf, sizeof(buf), "数据目录：%s", g_data_dir);
    about_add_line(card, buf);

    snprintf(buf, sizeof(buf), "聊天服务：%s:%d，模型 %s",
             g_cfg.server_ip, g_cfg.server_port, g_cfg.model);
    about_add_line(card, buf);

    snprintf(buf, sizeof(buf), "游戏服务：%s:%d", g_cfg.game_host, g_cfg.game_port);
    about_add_line(card, buf);

    snprintf(buf, sizeof(buf), "当前账号：%s", g_cfg.account);
    about_add_line(card, buf);

    footer = ui_make_label(body, "AI Terminal · RK3568");
    lv_obj_set_style_text_color(footer, lv_color_hex(UI_C_MUTED), 0);

    return scr;
}
