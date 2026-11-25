#include "ui_login.h"
#include "ui_theme.h"
#include "ui_home.h"
#include "app.h"

#include <string.h>

static lv_obj_t *s_acc;
static lv_obj_t *s_pwd;
static lv_obj_t *s_status;
static lv_obj_t *s_kb;

static void login_delete_cb(lv_event_t *e)
{
    (void)e;
    s_acc = NULL;
    s_pwd = NULL;
    s_status = NULL;
    s_kb = NULL;
}

static void login_kb_hide_cb(lv_event_t *e)
{
    (void)e;
    if (s_kb != NULL) {
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void login_ta_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);

    if (s_kb == NULL) {
        return;
    }
    lv_keyboard_set_textarea(s_kb, ta);
    lv_obj_remove_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_kb);
}

static void login_ta_blur_cb(lv_event_t *e)
{
    (void)e;
    if (s_kb != NULL) {
        lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void login_clicked(lv_event_t *e)
{
    const char *acc;
    const char *pwd;

    (void)e;
    if (s_acc == NULL || s_pwd == NULL || s_status == NULL) {
        return;
    }
    acc = lv_textarea_get_text(s_acc);
    pwd = lv_textarea_get_text(s_pwd);

    if (strcmp(acc, g_cfg.account) == 0 && strcmp(pwd, g_cfg.password) == 0) {
        g_logged_in = 1;
        lv_label_set_text(s_status, "登录成功，正在进入…");
        lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_OK), 0);
        ui_nav_to(ui_home_create);
    } else {
        lv_label_set_text(s_status, "账号或密码错误");
        lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_DANGER), 0);
    }
}

static lv_obj_t *make_field(lv_obj_t *parent, const char *placeholder, const char *text)
{
    lv_obj_t *ta = lv_textarea_create(parent);

    lv_obj_set_size(ta, 380, 50);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_text(ta, (text != NULL) ? text : "");
    lv_obj_set_style_text_font(ta, UI_FONT, 0);
    lv_obj_add_event_cb(ta, login_ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(ta, login_ta_blur_cb, LV_EVENT_DEFOCUSED, NULL);
    return ta;
}

lv_obj_t *ui_login_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *card;
    lv_obj_t *title;
    lv_obj_t *subtitle;
    lv_obj_t *btn;

    s_acc = NULL;
    s_pwd = NULL;
    s_status = NULL;
    s_kb = NULL;

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_event_cb(scr, login_delete_cb, LV_EVENT_DELETE, NULL);

    card = ui_make_card(scr);
    lv_obj_set_size(card, 480, 400);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 24, 0);
    lv_obj_set_style_pad_row(card, 14, 0);

    title = ui_make_label(card, "AI 终端");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_C_PRIMARY), 0);

    subtitle = ui_make_label(card, "请登录后使用");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(UI_C_MUTED), 0);

    s_acc = make_field(card, "账号", g_cfg.account);
    s_pwd = make_field(card, "密码", g_cfg.password);
    lv_textarea_set_password_mode(s_pwd, true);

    btn = ui_make_button(card, "登 录", 380, 52);
    lv_obj_add_event_cb(btn, login_clicked, LV_EVENT_CLICKED, NULL);

    s_status = ui_make_label(card, " ");
    lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_DANGER), 0);

    s_kb = lv_keyboard_create(scr);
    lv_obj_set_size(s_kb, lv_pct(100), 200);
    lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(s_kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_mode(s_kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(s_kb, login_kb_hide_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_kb, login_kb_hide_cb, LV_EVENT_CANCEL, NULL);

    return scr;
}
