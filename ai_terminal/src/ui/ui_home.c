#include "ui_home.h"
#include "ui_theme.h"
#include "ui_chat.h"
#include "ui_history.h"
#include "ui_settings.h"
#include "ui_about.h"
#include "ui_game.h"
#include "app.h"

#include <stdio.h>

static void home_go_chat(lv_event_t *e)
{
    (void)e;
    ui_nav_to(ui_chat_create);
}

static void home_go_history(lv_event_t *e)
{
    (void)e;
    ui_nav_to(ui_history_create);
}

static void home_go_settings(lv_event_t *e)
{
    (void)e;
    ui_nav_to(ui_settings_create);
}

static void home_go_game(lv_event_t *e)
{
    (void)e;
    ui_nav_to(ui_game_create);
}

static void home_go_about(lv_event_t *e)
{
    (void)e;
    ui_nav_to(ui_about_create);
}

lv_obj_t *ui_home_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *body;
    lv_obj_t *grid;
    lv_obj_t *btn;
    lv_obj_t *spacer;
    lv_obj_t *info;
    char buf[192];

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 0, 0);

    ui_create_header(scr, "AI 终端", NULL);

    body = lv_obj_create(scr);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 24, 0);
    lv_obj_set_style_pad_row(body, 18, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    grid = lv_obj_create(body);
    lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 20, 0);
    lv_obj_set_style_pad_column(grid, 20, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    btn = ui_make_button(grid, "AI 聊天", 300, 130);
    lv_obj_add_event_cb(btn, home_go_chat, LV_EVENT_CLICKED, NULL);

    btn = ui_make_button(grid, "历史记录", 300, 130);
    lv_obj_add_event_cb(btn, home_go_history, LV_EVENT_CLICKED, NULL);

    btn = ui_make_button(grid, "设置", 300, 130);
    lv_obj_add_event_cb(btn, home_go_settings, LV_EVENT_CLICKED, NULL);

    btn = ui_make_button(grid, "贪吃蛇", 300, 130);
    lv_obj_add_event_cb(btn, home_go_game, LV_EVENT_CLICKED, NULL);

    btn = ui_make_button(grid, "关于", 300, 130);
    lv_obj_add_event_cb(btn, home_go_about, LV_EVENT_CLICKED, NULL);

    spacer = lv_obj_create(body);
    lv_obj_set_width(spacer, lv_pct(100));
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    snprintf(buf, sizeof(buf), "服务器：%s:%d    模型：%s",
             g_cfg.server_ip, g_cfg.server_port, g_cfg.model);
    info = ui_make_label(body, buf);
    lv_obj_set_style_text_color(info, lv_color_hex(UI_C_MUTED), 0);

    return scr;
}
