#define _POSIX_C_SOURCE 200809L

#include "ui_chat.h"
#include "ui_theme.h"
#include "ui_home.h"
#include "net.h"
#include "storage.h"
#include "pinyin.h"
#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAT_PY_CANDS 4

typedef struct {
    lv_obj_t *scr;
    lv_obj_t *list;
    lv_obj_t *input;
    lv_obj_t *send_btn;
    lv_obj_t *clear_btn;
    lv_obj_t *status;
    lv_obj_t *ai_label;
    lv_obj_t *ai_row;
    lv_obj_t *kb;
    lv_obj_t *py_panel;
    lv_obj_t *py_ta;
    lv_obj_t *py_btns[CHAT_PY_CANDS];
    char *ai_text;
    size_t ai_len;
    size_t ai_cap;
    char *prompt;
    int streaming;
} chat_ctx_t;

static chat_ctx_t s_chat;

static void chat_free_buffers(void)
{
    free(s_chat.ai_text);
    s_chat.ai_text = NULL;
    s_chat.ai_len = 0;
    s_chat.ai_cap = 0;
    free(s_chat.prompt);
    s_chat.prompt = NULL;
}

static void chat_delete_cb(lv_event_t *e)
{
    (void)e;
    chat_free_buffers();
    memset(&s_chat, 0, sizeof(s_chat));
}

static int chat_ai_append(const char *delta)
{
    size_t add = strlen(delta);
    size_t need = s_chat.ai_len + add + 1;
    char *p;

    if (need > s_chat.ai_cap) {
        size_t cap = (s_chat.ai_cap > 0) ? s_chat.ai_cap : 256;
        while (cap < need) {
            cap *= 2;
        }
        p = (char *)realloc(s_chat.ai_text, cap);
        if (p == NULL) {
            return -1;
        }
        s_chat.ai_text = p;
        s_chat.ai_cap = cap;
    }
    memcpy(s_chat.ai_text + s_chat.ai_len, delta, add + 1);
    s_chat.ai_len = need - 1;
    return 0;
}

static void chat_scroll_bottom(void)
{
    int32_t target;

    if (s_chat.list == NULL) {
        return;
    }
    lv_obj_update_layout(s_chat.list);
    target = lv_obj_get_scroll_y(s_chat.list) + lv_obj_get_scroll_bottom(s_chat.list);
    if (target > 0) {
        lv_obj_scroll_to_y(s_chat.list, target, LV_ANIM_OFF);
    }
}

static lv_obj_t *chat_add_bubble(const char *text, int is_user, lv_obj_t **row_out)
{
    lv_obj_t *row = lv_obj_create(s_chat.list);
    lv_obj_t *bubble;
    lv_obj_t *label;

    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row,
                          is_user ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    bubble = lv_obj_create(row);
    lv_obj_set_width(bubble, lv_pct(76));
    lv_obj_set_height(bubble, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(bubble, lv_color_hex(is_user ? UI_C_USER : UI_C_AI), 0);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bubble, 12, 0);
    lv_obj_set_style_border_width(bubble, 0, 0);
    lv_obj_set_style_pad_all(bubble, 12, 0);
    lv_obj_remove_flag(bubble, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(bubble);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(UI_C_TEXT), 0);
    lv_obj_set_style_text_font(label, UI_FONT, 0);

    if (row_out != NULL) {
        *row_out = row;
    }
    return label;
}

static void chat_set_status(const char *text, uint32_t color)
{
    if (s_chat.status == NULL) {
        return;
    }
    lv_label_set_text(s_chat.status, (text != NULL) ? text : "");
    lv_obj_set_style_text_color(s_chat.status, lv_color_hex(color), 0);
}

/* ---------------- 拼音输入法面板 ---------------- */

static void chat_py_update(void)
{
    const char **cands = NULL;
    const char *text;
    int count = 0;
    int i;

    if (s_chat.py_ta == NULL) {
        return;
    }
    text = lv_textarea_get_text(s_chat.py_ta);
    if (text != NULL && text[0] != '\0') {
        cands = pinyin_input_commit(text);
        count = pinyin_candidate_count();
    } else {
        pinyin_init();
    }

    for (i = 0; i < CHAT_PY_CANDS; i++) {
        lv_obj_t *btn = s_chat.py_btns[i];
        lv_obj_t *lbl;

        if (btn == NULL) {
            continue;
        }
        lbl = lv_obj_get_child(btn, 0);
        if (cands != NULL && i < count && cands[i] != NULL && cands[i][0] != '\0') {
            if (lbl != NULL) {
                lv_label_set_text(lbl, cands[i]);
            }
            lv_obj_remove_state(btn, LV_STATE_DISABLED);
        } else {
            if (lbl != NULL) {
                lv_label_set_text(lbl, "—");
            }
            lv_obj_add_state(btn, LV_STATE_DISABLED);
        }
    }
}

static void chat_py_value_cb(lv_event_t *e)
{
    (void)e;
    chat_py_update();
}

static void chat_py_cand_cb(lv_event_t *e)
{
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
    const char **cands;
    const char *text;
    int count;
    int index = -1;
    int i;

    if (s_chat.py_ta == NULL || s_chat.input == NULL) {
        return;
    }
    for (i = 0; i < CHAT_PY_CANDS; i++) {
        if (s_chat.py_btns[i] == target) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        return;
    }
    text = lv_textarea_get_text(s_chat.py_ta);
    if (text == NULL || text[0] == '\0') {
        return;
    }
    cands = pinyin_input_commit(text);
    count = pinyin_candidate_count();
    if (cands == NULL || index >= count || cands[index] == NULL) {
        return;
    }
    lv_textarea_add_text(s_chat.input, cands[index]);
    lv_textarea_set_text(s_chat.py_ta, "");
    pinyin_init();
    chat_py_update();
}

static void chat_py_toggle_cb(lv_event_t *e)
{
    (void)e;
    if (s_chat.py_panel == NULL) {
        return;
    }
    if (lv_obj_has_flag(s_chat.py_panel, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_remove_flag(s_chat.py_panel, LV_OBJ_FLAG_HIDDEN);
        chat_py_update();
    } else {
        lv_obj_add_flag(s_chat.py_panel, LV_OBJ_FLAG_HIDDEN);
        if (s_chat.kb != NULL) {
            lv_obj_add_flag(s_chat.kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void chat_on_delta(const char *delta, void *user)
{
    (void)user;
    if (s_chat.scr == NULL || delta == NULL) {
        return;
    }
    if (chat_ai_append(delta) != 0) {
        return;
    }
    if (s_chat.ai_label != NULL) {
        lv_label_set_text(s_chat.ai_label, s_chat.ai_text);
    }
    chat_scroll_bottom();
}

static void chat_on_done(void *user)
{
    (void)user;
    if (s_chat.scr == NULL) {
        return;
    }
    s_chat.streaming = 0;
    if (s_chat.send_btn != NULL) {
        lv_obj_remove_state(s_chat.send_btn, LV_STATE_DISABLED);
    }
    if (s_chat.ai_label != NULL &&
        (s_chat.ai_text == NULL || s_chat.ai_text[0] == '\0')) {
        lv_label_set_text(s_chat.ai_label, "（没有返回内容）");
    }
    chat_set_status("回答完成", UI_C_OK);
    if (s_chat.prompt != NULL) {
        history_add(s_chat.prompt, (s_chat.ai_text != NULL) ? s_chat.ai_text : "");
    }
    chat_scroll_bottom();
}

static void chat_on_error(const char *message, void *user)
{
    (void)user;
    if (s_chat.scr == NULL) {
        return;
    }
    s_chat.streaming = 0;
    if (s_chat.send_btn != NULL) {
        lv_obj_remove_state(s_chat.send_btn, LV_STATE_DISABLED);
    }
    chat_set_status(((message != NULL) && (message[0] != '\0')) ? message : "请求失败",
                    UI_C_DANGER);
    if (s_chat.ai_label != NULL &&
        (s_chat.ai_text == NULL || s_chat.ai_text[0] == '\0')) {
        lv_label_set_text(s_chat.ai_label, "（请求失败）");
    }
    chat_scroll_bottom();
}

static void chat_send_clicked(lv_event_t *e)
{
    const char *text;
    char *prompt_copy;

    (void)e;
    if (s_chat.streaming || net_is_busy()) {
        return;
    }
    if (s_chat.input == NULL) {
        return;
    }
    text = lv_textarea_get_text(s_chat.input);
    if (text == NULL || text[0] == '\0') {
        chat_set_status("请先输入内容", UI_C_DANGER);
        return;
    }
    prompt_copy = strdup(text);
    if (prompt_copy == NULL) {
        chat_set_status("内存不足", UI_C_DANGER);
        return;
    }

    chat_add_bubble(prompt_copy, 1, NULL);

    chat_free_buffers();
    s_chat.ai_label = chat_add_bubble("思考中…", 0, &s_chat.ai_row);
    s_chat.prompt = prompt_copy;

    lv_textarea_set_text(s_chat.input, "");
    lv_obj_add_state(s_chat.send_btn, LV_STATE_DISABLED);
    s_chat.streaming = 1;
    chat_set_status("思考中…", UI_C_MUTED);
    chat_scroll_bottom();

    if (net_chat_start(s_chat.prompt, chat_on_delta, chat_on_done, chat_on_error, NULL) != 0) {
        s_chat.streaming = 0;
        lv_obj_remove_state(s_chat.send_btn, LV_STATE_DISABLED);
        chat_set_status("无法发起请求，请稍后重试", UI_C_DANGER);
        if (s_chat.ai_label != NULL) {
            lv_label_set_text(s_chat.ai_label, "（请求未发送）");
        }
    }
}

static void chat_clear_clicked(lv_event_t *e)
{
    (void)e;
    if (s_chat.streaming || net_is_busy()) {
        chat_set_status("当前正在回复，请稍后再清空", UI_C_DANGER);
        return;
    }
    if (s_chat.list != NULL) {
        lv_obj_clean(s_chat.list);
    }
    chat_free_buffers();
    s_chat.ai_label = NULL;
    s_chat.ai_row = NULL;
    chat_set_status("对话已清空", UI_C_MUTED);
}

static void chat_back_to_home(void)
{
    if (s_chat.streaming || net_is_busy()) {
        net_chat_cancel();
    }
    ui_nav_to(ui_home_create);
}

static void chat_kb_hide_cb(lv_event_t *e)
{
    (void)e;
    if (s_chat.kb != NULL) {
        lv_obj_add_flag(s_chat.kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void chat_ta_focus_cb(lv_event_t *e)
{
    lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);

    if (s_chat.kb == NULL) {
        return;
    }
    lv_keyboard_set_textarea(s_chat.kb, ta);
    lv_obj_remove_flag(s_chat.kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_chat.kb);
    chat_scroll_bottom();
}

static void chat_ta_blur_cb(lv_event_t *e)
{
    (void)e;
    if (s_chat.kb != NULL) {
        lv_obj_add_flag(s_chat.kb, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_chat_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *status_bar;
    lv_obj_t *bar;
    lv_obj_t *py_label;
    lv_obj_t *py_btn;
    int i;

    memset(&s_chat, 0, sizeof(s_chat));

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    s_chat.scr = scr;
    lv_obj_add_event_cb(scr, chat_delete_cb, LV_EVENT_DELETE, NULL);

    ui_create_header(scr, "AI 聊天", chat_back_to_home);

    status_bar = lv_obj_create(scr);
    lv_obj_set_width(status_bar, lv_pct(100));
    lv_obj_set_height(status_bar, 32);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_pad_left(status_bar, 20, 0);
    lv_obj_set_style_pad_right(status_bar, 20, 0);
    lv_obj_set_style_pad_top(status_bar, 6, 0);
    lv_obj_set_style_pad_bottom(status_bar, 0, 0);
    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    s_chat.status = ui_make_label(status_bar, "输入问题后点击发送");
    lv_obj_set_style_text_color(s_chat.status, lv_color_hex(UI_C_MUTED), 0);

    s_chat.py_panel = lv_obj_create(scr);
    lv_obj_set_width(s_chat.py_panel, lv_pct(100));
    lv_obj_set_height(s_chat.py_panel, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(s_chat.py_panel, lv_color_hex(UI_C_CARD), 0);
    lv_obj_set_style_bg_opa(s_chat.py_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_chat.py_panel, 1, 0);
    lv_obj_set_style_border_side(s_chat.py_panel, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(s_chat.py_panel, lv_color_hex(UI_C_LINE), 0);
    lv_obj_set_style_radius(s_chat.py_panel, 0, 0);
    lv_obj_set_style_pad_all(s_chat.py_panel, 10, 0);
    lv_obj_set_style_pad_column(s_chat.py_panel, 8, 0);
    lv_obj_set_flex_flow(s_chat.py_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_chat.py_panel, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(s_chat.py_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_chat.py_panel, LV_OBJ_FLAG_HIDDEN);

    py_label = ui_make_label(s_chat.py_panel, "拼音");
    lv_obj_set_style_text_color(py_label, lv_color_hex(UI_C_MUTED), 0);

    s_chat.py_ta = lv_textarea_create(s_chat.py_panel);
    lv_obj_set_size(s_chat.py_ta, 220, 44);
    lv_textarea_set_one_line(s_chat.py_ta, true);
    lv_textarea_set_max_length(s_chat.py_ta, 32);
    lv_textarea_set_placeholder_text(s_chat.py_ta, "输入拼音，如 nihao");
    lv_obj_set_style_text_font(s_chat.py_ta, UI_FONT, 0);
    lv_obj_add_event_cb(s_chat.py_ta, chat_ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(s_chat.py_ta, chat_ta_blur_cb, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_event_cb(s_chat.py_ta, chat_py_value_cb, LV_EVENT_VALUE_CHANGED, NULL);

    for (i = 0; i < CHAT_PY_CANDS; i++) {
        lv_obj_t *cand_btn = ui_make_button(s_chat.py_panel, "—", 100, 36);

        lv_obj_add_event_cb(cand_btn, chat_py_cand_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_state(cand_btn, LV_STATE_DISABLED);
        s_chat.py_btns[i] = cand_btn;
    }

    s_chat.list = lv_obj_create(scr);
    lv_obj_set_width(s_chat.list, lv_pct(100));
    lv_obj_set_flex_grow(s_chat.list, 1);
    lv_obj_set_style_bg_opa(s_chat.list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_chat.list, 0, 0);
    lv_obj_set_style_pad_all(s_chat.list, 16, 0);
    lv_obj_set_style_pad_row(s_chat.list, 10, 0);
    lv_obj_set_flex_flow(s_chat.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_chat.list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(s_chat.list, LV_DIR_VER);

    bar = lv_obj_create(scr);
    lv_obj_set_width(bar, lv_pct(100));
    lv_obj_set_height(bar, 88);
    lv_obj_set_style_bg_color(bar, lv_color_hex(UI_C_CARD), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(UI_C_LINE), 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 14, 0);
    lv_obj_set_style_pad_column(bar, 12, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    s_chat.input = lv_textarea_create(bar);
    lv_obj_set_height(s_chat.input, 56);
    lv_obj_set_flex_grow(s_chat.input, 1);
    lv_textarea_set_one_line(s_chat.input, true);
    lv_textarea_set_max_length(s_chat.input, 1000);
    lv_textarea_set_placeholder_text(s_chat.input, "输入消息，按发送键提交");
    lv_obj_set_style_text_font(s_chat.input, UI_FONT, 0);
    lv_obj_add_event_cb(s_chat.input, chat_ta_focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(s_chat.input, chat_ta_blur_cb, LV_EVENT_DEFOCUSED, NULL);

    s_chat.send_btn = ui_make_button(bar, "发送", 116, 56);
    lv_obj_add_event_cb(s_chat.send_btn, chat_send_clicked, LV_EVENT_CLICKED, NULL);

    py_btn = ui_make_button(bar, "拼音", 84, 56);
    lv_obj_set_style_bg_color(py_btn, lv_color_hex(UI_C_MUTED), 0);
    lv_obj_add_event_cb(py_btn, chat_py_toggle_cb, LV_EVENT_CLICKED, NULL);

    s_chat.clear_btn = ui_make_button(bar, "清空", 96, 56);
    lv_obj_set_style_bg_color(s_chat.clear_btn, lv_color_hex(UI_C_MUTED), 0);
    lv_obj_add_event_cb(s_chat.clear_btn, chat_clear_clicked, LV_EVENT_CLICKED, NULL);

    s_chat.kb = lv_keyboard_create(scr);
    lv_obj_set_size(s_chat.kb, lv_pct(100), 200);
    lv_obj_align(s_chat.kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(s_chat.kb, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(s_chat.kb, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_mode(s_chat.kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_event_cb(s_chat.kb, chat_kb_hide_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_chat.kb, chat_kb_hide_cb, LV_EVENT_CANCEL, NULL);

    return scr;
}
