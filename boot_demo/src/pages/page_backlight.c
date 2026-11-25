/**
 * @file page_backlight.c
 * 背光：枚举 /sys/class/backlight/*，滑块调节 brightness。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static lv_obj_t *s_slider;
static lv_obj_t *s_value_label;
static char s_bright_path[160];
static int s_max;
static int s_ready;

static void backlight_read_status(void)
{
    char val[32] = "?";
    int cur = -1;
    long curv = 0;
    char buf[512];

    if(!s_ready) {
        ui_label_set_text_fmt(s_status,
                              "未找到背光设备（/sys/class/backlight 下没有可用的 brightness）");
        return;
    }

    hw_read_file(s_bright_path, val, sizeof(val));
    if(hw_read_int(s_bright_path, &curv) == HW_OK) cur = (int)curv;

    snprintf(buf, sizeof(buf),
             "设备: %s\nmax_brightness: %d\n当前 brightness: %s",
             s_bright_path, s_max, val);
    lv_label_set_text(s_status, buf);

    if(s_slider != NULL && cur >= 0) {
        lv_slider_set_value(s_slider, cur, LV_ANIM_OFF);
    }
    if(s_value_label != NULL && cur >= 0) {
        int pct = (s_max > 0) ? (cur * 100 / s_max) : 0;
        ui_label_set_text_fmt(s_value_label, "当前亮度: %d / %d (%d%%)", cur, s_max, pct);
    }
}

static void backlight_slider_cb(lv_event_t *e)
{
    int value;
    char v[16];

    LV_UNUSED(e);
    if(!s_ready || s_slider == NULL) return;

    value = (int)lv_slider_get_value(s_slider);
    if(value < 0) value = 0;
    if(value > s_max) value = s_max;

    snprintf(v, sizeof(v), "%d", value);
    if(hw_write_file(s_bright_path, v) == HW_OK) {
        int pct = (s_max > 0) ? (value * 100 / s_max) : 0;
        if(s_value_label != NULL) {
            ui_label_set_text_fmt(s_value_label, "当前亮度: %d / %d (%d%%)", value, s_max, pct);
        }
        ui_set_state(s_result, 1, "已设置 brightness=%d", value);
    }
    else {
        ui_set_state(s_result, 0, "写入 %s 失败", s_bright_path);
    }
}

static void backlight_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    backlight_read_status();
    if(s_ready) ui_set_state(s_result, 1, "已刷新背光状态");
    else ui_set_state(s_result, 0, "未找到背光设备");
}

void page_backlight_create(lv_obj_t *parent)
{
    char dir[160];
    char max_path[192];
    long maxv = 0;
    lv_obj_t *row;

    ui_page_title(parent, "背光调节");
    s_status = ui_page_status(parent, "正在检测背光设备...");
    s_slider = NULL;
    s_value_label = NULL;
    s_ready = 0;
    s_max = 100;

    if(hw_first_glob("/sys/class/backlight", "*", dir, sizeof(dir)) == HW_OK) {
        snprintf(s_bright_path, sizeof(s_bright_path), "%s/brightness", dir);
        snprintf(max_path, sizeof(max_path), "%s/max_brightness", dir);
        if(hw_path_exists(s_bright_path)) {
            s_ready = 1;
            if(hw_read_int(max_path, &maxv) == HW_OK && maxv > 0) s_max = (int)maxv;
        }
    }

    row = ui_page_button_row(parent);
    ui_page_button(row, "刷新", backlight_refresh_cb, NULL);

    s_slider = lv_slider_create(parent);
    lv_obj_set_width(s_slider, lv_pct(100));
    lv_slider_set_range(s_slider, 0, s_max);
    lv_slider_set_value(s_slider, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(UI_COLOR_ACCENT), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(UI_COLOR_ACCENT), LV_PART_KNOB);
    lv_obj_add_event_cb(s_slider, backlight_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);
    if(!s_ready) lv_obj_add_state(s_slider, LV_STATE_DISABLED);

    s_value_label = ui_page_status(parent, "当前亮度: -");

    s_result = ui_page_result(parent, s_ready ? "拖动滑块调节背光亮度。" : "未找到背光设备。");
    backlight_read_status();
}
