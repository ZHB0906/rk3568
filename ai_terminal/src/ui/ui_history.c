#include "ui_history.h"
#include "ui_theme.h"
#include "ui_home.h"
#include "storage.h"

#include <stdio.h>

#define HISTORY_MAX_ITEMS 50

static lv_obj_t *s_list;
static lv_obj_t *s_status;
static lv_obj_t *s_clear_btn;
static char **s_items;
static int s_count;
static int s_confirm;

static void history_back(void)
{
    ui_nav_to(ui_home_create);
}

static void history_free_items(void)
{
    if (s_items != NULL) {
        history_free(s_items, s_count);
        s_items = NULL;
    }
    s_count = 0;
}

static void history_set_status(const char *text, uint32_t color)
{
    if (s_status == NULL) {
        return;
    }
    lv_label_set_text(s_status, (text != NULL) ? text : "");
    lv_obj_set_style_text_color(s_status, lv_color_hex(color), 0);
}

static void history_set_clear_text(const char *text)
{
    lv_obj_t *lbl;

    if (s_clear_btn == NULL) {
        return;
    }
    lbl = lv_obj_get_child(s_clear_btn, 0);
    if (lbl != NULL) {
        lv_label_set_text(lbl, (text != NULL) ? text : "");
    }
}

static void history_reload(void)
{
    char buf[96];
    int i;

    if (s_list == NULL) {
        return;
    }
    lv_obj_clean(s_list);
    history_free_items();

    if (history_load_recent(HISTORY_MAX_ITEMS, &s_items, &s_count) != 0) {
        s_items = NULL;
        s_count = 0;
    }
    if (s_items == NULL || s_count <= 0) {
        lv_obj_t *empty = ui_make_label(s_list, "暂无历史记录，先去 AI 聊天里问点什么吧。");
        lv_obj_set_style_text_color(empty, lv_color_hex(UI_C_MUTED), 0);
        return;
    }

    for (i = 0; i < s_count; i++) {
        lv_obj_t *card = ui_make_card(s_list);
        lv_obj_t *lbl;

        lv_obj_set_width(card, lv_pct(100));
        lv_obj_set_height(card, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(card, 12, 0);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        lbl = ui_make_label(card, s_items[i]);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, lv_pct(100));
    }

    snprintf(buf, sizeof(buf), "共 %d 条记录（最多显示最近 %d 条）",
             s_count, HISTORY_MAX_ITEMS);
    history_set_status(buf, UI_C_MUTED);
}

static void history_clear_clicked(lv_event_t *e)
{
    (void)e;
    if (!s_confirm) {
        s_confirm = 1;
        history_set_clear_text("确认清空");
        history_set_status("再次点击“确认清空”将删除全部历史记录", UI_C_DANGER);
        return;
    }

    s_confirm = 0;
    history_set_clear_text("清空历史");
    if (history_clear() != 0) {
        history_set_status("清空失败，请检查数据目录权限", UI_C_DANGER);
        return;
    }
    history_reload();
    history_set_status("历史记录已清空", UI_C_OK);
}

static void history_delete_cb(lv_event_t *e)
{
    (void)e;
    history_free_items();
    s_list = NULL;
    s_status = NULL;
    s_clear_btn = NULL;
    s_confirm = 0;
}

lv_obj_t *ui_history_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *toolbar;

    s_list = NULL;
    s_status = NULL;
    s_clear_btn = NULL;
    s_items = NULL;
    s_count = 0;
    s_confirm = 0;

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_add_event_cb(scr, history_delete_cb, LV_EVENT_DELETE, NULL);

    ui_create_header(scr, "历史记录", history_back);

    toolbar = lv_obj_create(scr);
    lv_obj_set_width(toolbar, lv_pct(100));
    lv_obj_set_height(toolbar, 56);
    lv_obj_set_style_bg_opa(toolbar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(toolbar, 0, 0);
    lv_obj_set_style_pad_left(toolbar, 20, 0);
    lv_obj_set_style_pad_right(toolbar, 20, 0);
    lv_obj_set_style_pad_top(toolbar, 10, 0);
    lv_obj_set_style_pad_bottom(toolbar, 6, 0);
    lv_obj_set_style_pad_column(toolbar, 16, 0);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(toolbar, LV_OBJ_FLAG_SCROLLABLE);

    s_status = ui_make_label(toolbar, "");
    lv_obj_set_flex_grow(s_status, 1);
    lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_MUTED), 0);

    s_clear_btn = ui_make_button(toolbar, "清空历史", 160, 40);
    lv_obj_set_style_bg_color(s_clear_btn, lv_color_hex(UI_C_DANGER), 0);
    lv_obj_add_event_cb(s_clear_btn, history_clear_clicked, LV_EVENT_CLICKED, NULL);

    s_list = lv_obj_create(scr);
    lv_obj_set_width(s_list, lv_pct(100));
    lv_obj_set_flex_grow(s_list, 1);
    lv_obj_set_style_bg_opa(s_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_list, 0, 0);
    lv_obj_set_style_pad_left(s_list, 20, 0);
    lv_obj_set_style_pad_right(s_list, 20, 0);
    lv_obj_set_style_pad_top(s_list, 6, 0);
    lv_obj_set_style_pad_bottom(s_list, 20, 0);
    lv_obj_set_style_pad_row(s_list, 12, 0);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);

    history_reload();
    return scr;
}
