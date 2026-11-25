#include "ui_settings.h"
#include "ui_theme.h"
#include "ui_home.h"
#include "app.h"
#include "storage.h"

#include <stdio.h>
#include <stdlib.h>

enum {
    F_SERVER_IP = 0,
    F_SERVER_PORT,
    F_MODEL,
    F_ACCOUNT,
    F_PASSWORD,
    F_CONTEXT_LEN,
    F_FONT_SIZE,
    F_GAME_HOST,
    F_GAME_PORT,
    F_COUNT
};

static lv_obj_t *s_ta[F_COUNT];
static lv_obj_t *s_status;
static lv_obj_t *s_kb;

static void settings_back(void)
{
    ui_nav_to(ui_home_create);
}

static void settings_delete_cb(lv_event_t *e)
{
    int i;

    (void)e;
    for (i = 0; i < F_COUNT; i++) {
        s_ta[i] = NULL;
    }
    s_status = NULL;
    s_kb = NULL;
}

static void settings_set_status(const char *text, uint32_t color)
{
    if (s_status == NULL) {
        return;
    }
    lv_label_set_text(s_status, (text != NULL) ? text : "");
    lv_obj_set_style_text_color(s_status, lv_color_hex(color), 0);
}

static void settings_kb_hide_cb(lv_event_t *e)
{
    (void)e;
    if (s_kb != NULL) {
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void settings_ta_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);

    if (s_kb == NULL) {
        return;
    }
    lv_keyboard_set_textarea(s_kb, ta);
    lv_obj_remove_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_kb);
}

static void settings_ta_blur_cb(lv_event_t *e)
{
    (void)e;
    if (s_kb != NULL) {
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t *make_field(lv_obj_t *parent, int id, const char *name,
                            const char *value, uint32_t max_len)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_t *label;
    lv_obj_t *ta;

    lv_obj_set_size(box, 300, 96);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_pad_row(box, 6, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    label = ui_make_label(box, name);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_C_MUTED), 0);

    ta = lv_textarea_create(box);
    lv_obj_set_width(ta, lv_pct(100));
    lv_obj_set_height(ta, 52);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, max_len);
    lv_textarea_set_text(ta, (value != NULL) ? value : "");
    lv_obj_set_style_text_font(ta, UI_FONT, 0);
    lv_obj_add_event_cb(ta, settings_ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(ta, settings_ta_blur_cb, LV_EVENT_DEFOCUSED, NULL);

    if (id >= 0 && id < F_COUNT) {
        s_ta[id] = ta;
    }
    return box;
}

static void settings_fill_fields(void)
{
    char num[32];

    if (s_ta[F_SERVER_IP] == NULL) {
        return;
    }
    lv_textarea_set_text(s_ta[F_SERVER_IP], g_cfg.server_ip);
    snprintf(num, sizeof(num), "%d", g_cfg.server_port);
    lv_textarea_set_text(s_ta[F_SERVER_PORT], num);
    lv_textarea_set_text(s_ta[F_MODEL], g_cfg.model);
    lv_textarea_set_text(s_ta[F_ACCOUNT], g_cfg.account);
    lv_textarea_set_text(s_ta[F_PASSWORD], g_cfg.password);
    snprintf(num, sizeof(num), "%d", g_cfg.context_len);
    lv_textarea_set_text(s_ta[F_CONTEXT_LEN], num);
    snprintf(num, sizeof(num), "%d", g_cfg.font_size);
    lv_textarea_set_text(s_ta[F_FONT_SIZE], num);
    lv_textarea_set_text(s_ta[F_GAME_HOST], g_cfg.game_host);
    snprintf(num, sizeof(num), "%d", g_cfg.game_port);
    lv_textarea_set_text(s_ta[F_GAME_PORT], num);
}

static int parse_int(const char *text, int min, int max, int *out)
{
    char *end = NULL;
    long value;

    if (text == NULL || text[0] == '\0' || out == NULL) {
        return -1;
    }
    value = strtol(text, &end, 10);
    if (end == text || (end != NULL && *end != '\0')) {
        return -1;
    }
    if (value < (long)min || value > (long)max) {
        return -1;
    }
    *out = (int)value;
    return 0;
}

static void settings_save_clicked(lv_event_t *e)
{
    const char *ip;
    const char *port_text;
    const char *model;
    const char *account;
    const char *password;
    const char *ctx_text;
    const char *font_text;
    const char *host;
    const char *gport_text;
    int port = 0;
    int ctx = 0;
    int font = 0;
    int gport = 0;

    (void)e;
    if (s_ta[F_SERVER_IP] == NULL || s_status == NULL) {
        return;
    }

    ip = lv_textarea_get_text(s_ta[F_SERVER_IP]);
    port_text = lv_textarea_get_text(s_ta[F_SERVER_PORT]);
    model = lv_textarea_get_text(s_ta[F_MODEL]);
    account = lv_textarea_get_text(s_ta[F_ACCOUNT]);
    password = lv_textarea_get_text(s_ta[F_PASSWORD]);
    ctx_text = lv_textarea_get_text(s_ta[F_CONTEXT_LEN]);
    font_text = lv_textarea_get_text(s_ta[F_FONT_SIZE]);
    host = lv_textarea_get_text(s_ta[F_GAME_HOST]);
    gport_text = lv_textarea_get_text(s_ta[F_GAME_PORT]);

    if (ip == NULL || ip[0] == '\0') {
        settings_set_status("服务器地址不能为空", UI_C_DANGER);
        return;
    }
    if (parse_int(port_text, 1, 65535, &port) != 0) {
        settings_set_status("服务器端口需为 1-65535 的整数", UI_C_DANGER);
        return;
    }
    if (model == NULL || model[0] == '\0') {
        settings_set_status("模型名称不能为空", UI_C_DANGER);
        return;
    }
    if (account == NULL || account[0] == '\0') {
        settings_set_status("登录账号不能为空", UI_C_DANGER);
        return;
    }
    if (parse_int(ctx_text, 1, 100000, &ctx) != 0) {
        settings_set_status("上下文长度需为 1-100000 的整数", UI_C_DANGER);
        return;
    }
    if (parse_int(font_text, 8, 48, &font) != 0) {
        settings_set_status("字体大小需为 8-48 的整数", UI_C_DANGER);
        return;
    }
    if (host == NULL || host[0] == '\0') {
        settings_set_status("游戏服务器地址不能为空", UI_C_DANGER);
        return;
    }
    if (parse_int(gport_text, 1, 65535, &gport) != 0) {
        settings_set_status("游戏端口需为 1-65535 的整数", UI_C_DANGER);
        return;
    }

    snprintf(g_cfg.server_ip, sizeof(g_cfg.server_ip), "%s", ip);
    g_cfg.server_port = port;
    snprintf(g_cfg.model, sizeof(g_cfg.model), "%s", model);
    snprintf(g_cfg.account, sizeof(g_cfg.account), "%s", account);
    snprintf(g_cfg.password, sizeof(g_cfg.password), "%s",
             (password != NULL) ? password : "");
    g_cfg.context_len = ctx;
    g_cfg.font_size = font;
    snprintf(g_cfg.game_host, sizeof(g_cfg.game_host), "%s", host);
    g_cfg.game_port = gport;

    if (config_save(&g_cfg) != 0) {
        settings_set_status("保存失败，请检查数据目录权限", UI_C_DANGER);
        return;
    }
    settings_set_status("设置已保存", UI_C_OK);
}

static void settings_reset_clicked(lv_event_t *e)
{
    (void)e;
    if (s_ta[F_SERVER_IP] == NULL) {
        return;
    }
    app_config_defaults(&g_cfg);
    settings_fill_fields();
    if (config_save(&g_cfg) != 0) {
        settings_set_status("已恢复默认值，但写入文件失败", UI_C_DANGER);
        return;
    }
    settings_set_status("已恢复默认设置", UI_C_OK);
}

lv_obj_t *ui_settings_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *body;
    lv_obj_t *grid;
    lv_obj_t *hint;
    lv_obj_t *spacer;
    lv_obj_t *bar;
    lv_obj_t *save_btn;
    lv_obj_t *reset_btn;
    int i;

    for (i = 0; i < F_COUNT; i++) {
        s_ta[i] = NULL;
    }
    s_status = NULL;
    s_kb = NULL;

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_add_event_cb(scr, settings_delete_cb, LV_EVENT_DELETE, NULL);

    ui_create_header(scr, "设置", settings_back);

    body = lv_obj_create(scr);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 20, 0);
    lv_obj_set_style_pad_row(body, 12, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    grid = lv_obj_create(body);
    lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 14, 0);
    lv_obj_set_style_pad_column(grid, 14, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    make_field(grid, F_SERVER_IP, "服务器地址", g_cfg.server_ip, 63);
    make_field(grid, F_SERVER_PORT, "服务器端口", "0", 5);
    make_field(grid, F_MODEL, "模型名称", g_cfg.model, 63);
    make_field(grid, F_ACCOUNT, "登录账号", g_cfg.account, 63);
    make_field(grid, F_PASSWORD, "登录密码", g_cfg.password, 63);
    make_field(grid, F_CONTEXT_LEN, "上下文长度", "0", 6);
    make_field(grid, F_FONT_SIZE, "字体大小", "16", 2);
    make_field(grid, F_GAME_HOST, "游戏服务器", g_cfg.game_host, 63);
    make_field(grid, F_GAME_PORT, "游戏端口", "0", 5);

    if (s_ta[F_PASSWORD] != NULL) {
        lv_textarea_set_password_mode(s_ta[F_PASSWORD], true);
    }
    settings_fill_fields();

    hint = ui_make_label(body, "提示：字体大小写入配置文件；界面中文固定使用 LVGL 内置 16px 字体。");
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, lv_pct(100));
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_C_MUTED), 0);

    spacer = lv_obj_create(body);
    lv_obj_set_width(spacer, lv_pct(100));
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    bar = lv_obj_create(body);
    lv_obj_set_width(bar, lv_pct(100));
    lv_obj_set_height(bar, 52);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_pad_column(bar, 16, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    save_btn = ui_make_button(bar, "保存设置", 180, 48);
    lv_obj_add_event_cb(save_btn, settings_save_clicked, LV_EVENT_CLICKED, NULL);

    reset_btn = ui_make_button(bar, "恢复默认", 160, 48);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(UI_C_MUTED), 0);
    lv_obj_add_event_cb(reset_btn, settings_reset_clicked, LV_EVENT_CLICKED, NULL);

    s_status = ui_make_label(bar, "");
    lv_obj_set_flex_grow(s_status, 1);
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_MUTED), 0);

    s_kb = lv_keyboard_create(scr);
    lv_obj_set_size(s_kb, lv_pct(100), 200);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(s_kb, settings_kb_hide_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_kb, settings_kb_hide_cb, LV_EVENT_CANCEL, NULL);

    return scr;
}
