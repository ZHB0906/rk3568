#include "ui_game.h"
#include "ui_theme.h"
#include "ui_home.h"
#include "snake.h"

#include <stdio.h>

#define GAME_CELL_PX 30

static lv_obj_t *s_score;
static lv_obj_t *s_status;

static void game_back(void)
{
    ui_nav_to(ui_home_create);
}

static void game_score_cb(int score)
{
    char buf[48];

    if (s_score == NULL) {
        return;
    }
    snprintf(buf, sizeof(buf), "分数：%d", score);
    lv_label_set_text(s_score, buf);

    if (s_status != NULL && snake_is_over()) {
        lv_label_set_text(s_status, "游戏结束，点击“开始 / 重开”再来一局");
        lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_DANGER), 0);
    }
}

static void game_start_clicked(lv_event_t *e)
{
    (void)e;
    snake_reset();
    snake_start();
    if (s_status != NULL) {
        lv_label_set_text(s_status, "游戏进行中，滑动棋盘或点击方向按钮控制");
        lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_OK), 0);
    }
}

static void game_up_clicked(lv_event_t *e)
{
    (void)e;
    snake_set_direction(SNAKE_DIR_UP);
}

static void game_down_clicked(lv_event_t *e)
{
    (void)e;
    snake_set_direction(SNAKE_DIR_DOWN);
}

static void game_left_clicked(lv_event_t *e)
{
    (void)e;
    snake_set_direction(SNAKE_DIR_LEFT);
}

static void game_right_clicked(lv_event_t *e)
{
    (void)e;
    snake_set_direction(SNAKE_DIR_RIGHT);
}

static void game_delete_cb(lv_event_t *e)
{
    (void)e;
    snake_set_score_cb(NULL);
    s_score = NULL;
    s_status = NULL;
}

static lv_obj_t *make_pad_btn(lv_obj_t *parent, const char *text,
                              lv_event_cb_t cb)
{
    lv_obj_t *btn = ui_make_button(parent, text, 88, 56);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

static lv_obj_t *make_spacer(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_create(parent);

    lv_obj_set_size(obj, 88, 56);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

lv_obj_t *ui_game_create(void)
{
    lv_obj_t *scr;
    lv_obj_t *body;
    lv_obj_t *left;
    lv_obj_t *board;
    lv_obj_t *panel;
    lv_obj_t *start_btn;
    lv_obj_t *pad;
    lv_obj_t *tip;

    s_score = NULL;
    s_status = NULL;

    scr = lv_obj_create(NULL);
    ui_apply_screen_style(scr);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(scr, 0, 0);
    lv_obj_add_event_cb(scr, game_delete_cb, LV_EVENT_DELETE, NULL);

    ui_create_header(scr, "贪吃蛇", game_back);

    body = lv_obj_create(scr);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 16, 0);
    lv_obj_set_style_pad_column(body, 20, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    left = lv_obj_create(body);
    lv_obj_set_size(left, 620, lv_pct(100));
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 0, 0);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_remove_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    board = snake_create(left, GAME_CELL_PX);
    lv_obj_center(board);

    panel = ui_make_card(body);
    lv_obj_set_height(panel, lv_pct(100));
    lv_obj_set_flex_grow(panel, 1);
    lv_obj_set_style_pad_all(panel, 16, 0);
    lv_obj_set_style_pad_row(panel, 12, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    s_score = ui_make_label(panel, "分数：0");
    lv_obj_set_style_text_color(s_score, lv_color_hex(UI_C_PRIMARY), 0);

    s_status = ui_make_label(panel, "点击“开始 / 重开”启动游戏");
    lv_label_set_long_mode(s_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_status, lv_pct(100));
    lv_obj_set_style_text_color(s_status, lv_color_hex(UI_C_MUTED), 0);

    start_btn = ui_make_button(panel, "开始 / 重开", 0, 52);
    lv_obj_set_width(start_btn, lv_pct(100));
    lv_obj_add_event_cb(start_btn, game_start_clicked, LV_EVENT_CLICKED, NULL);

    pad = lv_obj_create(panel);
    lv_obj_set_width(pad, lv_pct(100));
    lv_obj_set_height(pad, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pad, 0, 0);
    lv_obj_set_style_pad_all(pad, 0, 0);
    lv_obj_set_style_pad_row(pad, 8, 0);
    lv_obj_set_style_pad_column(pad, 8, 0);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(pad, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(pad, LV_OBJ_FLAG_SCROLLABLE);

    make_spacer(pad);
    make_pad_btn(pad, "↑", game_up_clicked);
    make_spacer(pad);
    make_pad_btn(pad, "←", game_left_clicked);
    make_pad_btn(pad, "↓", game_down_clicked);
    make_pad_btn(pad, "→", game_right_clicked);

    tip = ui_make_label(panel, "棋盘上按住并滑动，也可以控制方向。");
    lv_label_set_long_mode(tip, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(tip, lv_pct(100));
    lv_obj_set_style_text_color(tip, lv_color_hex(UI_C_MUTED), 0);

    snake_set_score_cb(game_score_cb);

    return scr;
}
