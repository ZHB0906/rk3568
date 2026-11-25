/**
 * @file page_lightsensor.c
 * 光线传感器：读取 iio in_illuminance_raw / in_intensity_raw，每秒刷新。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static char s_path[192];
static int s_found;

static void iio_dir_of(const char *path, char *out, size_t len)
{
    const char *slash = strrchr(path, '/');
    size_t n = (slash != NULL) ? (size_t)(slash - path) : 0;

    if(n >= len) n = len - 1;
    memcpy(out, path, n);
    out[n] = '\0';
}

static void light_refresh(void)
{
    long raw = 0;
    char dir[192];
    char name_path[256];
    char name[80] = "未知";
    char buf[640];

    if(hw_find_iio("in_illuminance_raw", s_path, sizeof(s_path)) != HW_OK &&
       hw_find_iio("in_intensity_raw", s_path, sizeof(s_path)) != HW_OK) {
        s_found = 0;
        ui_label_set_text_fmt(s_status,
                              "未找到光线传感器\n"
                              "已尝试 /sys/bus/iio/devices/iio:device*/in_illuminance_raw 与 in_intensity_raw");
        return;
    }

    s_found = 1;
    iio_dir_of(s_path, dir, sizeof(dir));
    snprintf(name_path, sizeof(name_path), "%s/name", dir);
    if(hw_read_file(name_path, name, sizeof(name)) != HW_OK) snprintf(name, sizeof(name), "未知");

    if(hw_read_int(s_path, &raw) != HW_OK) {
        ui_label_set_text_fmt(s_status, "设备: %s\n传感器: %s\n读取原始值失败", s_path, name);
        return;
    }

    snprintf(buf, sizeof(buf), "设备: %s\n传感器: %s\n原始照度: %ld", s_path, name, raw);
    lv_label_set_text(s_status, buf);
}

static void light_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    light_refresh();
}

static void light_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    light_refresh();
    if(s_found) ui_set_state(s_result, 1, "已刷新光线传感器数据");
    else ui_set_state(s_result, 0, "未找到光线传感器");
}

void page_lightsensor_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    ui_page_title(parent, "光线传感器");
    s_status = ui_page_status(parent, "正在检测...");
    s_found = 0;
    s_path[0] = '\0';

    row = ui_page_button_row(parent);
    ui_page_button(row, "刷新", light_refresh_cb, NULL);

    s_result = ui_page_result(parent, "原始值每秒自动刷新一次。");

    light_refresh();
    if(s_found) {
        ui_set_state(s_result, 1, "已找到光线传感器");
        ui_timer_attach(s_status, 1000, light_timer_cb, NULL);
    }
    else {
        ui_set_state(s_result, 0, "未找到光线传感器");
    }
}
