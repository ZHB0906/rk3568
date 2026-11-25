/**
 * @file page_touch.c
 * 触摸屏：在画布上触摸画点，显示坐标，支持清屏。
 */
#include "lvgl.h"
#include "ui_common.h"

#include <stdio.h>

#define TOUCH_DOT_MAX 240

static lv_obj_t *s_canvas;
static lv_obj_t *s_status;
static lv_obj_t *s_result;
static int s_dots;

static void touch_add_dot(int32_t x, int32_t y)
{
    lv_obj_t *dot;

    if(s_canvas == NULL) return;
    if(s_dots >= TOUCH_DOT_MAX) return;

    dot = lv_obj_create(s_canvas);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_pos(dot, x - 5, y - 5);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xff5252), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);

    s_dots++;
}

static void touch_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev;
    lv_point_t p;
    lv_area_t area;
    int32_t x;
    int32_t y;

    if(code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) return;

    indev = lv_indev_active();
    if(indev == NULL) return;

    lv_indev_get_point(indev, &p);
    if(s_canvas == NULL) return;
    lv_obj_get_coords(s_canvas, &area);

    if(p.x < area.x1 || p.x > area.x2 || p.y < area.y1 || p.y > area.y2) return;

    x = p.x - area.x1;
    y = p.y - area.y1;

    touch_add_dot(x, y);
    ui_label_set_text_fmt(s_status, "触摸坐标: X=%d  Y=%d\n已绘制 %d 个点（上限 %d）",
                          (int)x, (int)y, s_dots, TOUCH_DOT_MAX);

    if(s_dots >= TOUCH_DOT_MAX) {
        ui_set_state(s_result, 1, "已达到 %d 个点上限，点击“清屏”重置", TOUCH_DOT_MAX);
    }
}

static void touch_clear_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(s_canvas == NULL) return;
    lv_obj_clean(s_canvas);
    s_dots = 0;
    ui_label_set_text(s_status, "已清屏，请在黑色区域触摸/滑动");
    ui_set_state(s_result, 1, "画布已清空");
}

void page_touch_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    ui_page_title(parent, "触摸屏测试");
    s_status = ui_page_status(parent, "请在下方黑色区域触摸或滑动");
    s_dots = 0;

    row = ui_page_button_row(parent);
    ui_page_button(row, "清屏", touch_clear_cb, NULL);

    s_canvas = lv_obj_create(parent);
    lv_obj_set_width(s_canvas, lv_pct(100));
    lv_obj_set_height(s_canvas, 320);
    lv_obj_set_style_bg_color(s_canvas, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_canvas, 1, 0);
    lv_obj_set_style_border_color(s_canvas, lv_color_hex(0x2a3a4a), 0);
    lv_obj_set_style_radius(s_canvas, 6, 0);
    lv_obj_set_style_pad_all(s_canvas, 0, 0);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_canvas, touch_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_canvas, touch_event_cb, LV_EVENT_PRESSING, NULL);

    s_result = ui_page_result(parent, "触摸屏幕，红色圆点会标记触点位置。");
}
