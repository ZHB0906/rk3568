/**
 * @file page_gsensor.c
 * 重力/加速度传感器：读取 iio in_accel_x/y/z_raw，定时刷新并换算 g 值。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static int s_found;

static void gsensor_refresh(void)
{
    char xp[192];
    char yp[192];
    char zp[192];
    char sp[192];
    char scale_txt[32] = "";
    long x = 0;
    long y = 0;
    long z = 0;
    double scale = 1.0;
    char buf[720];

    if(hw_find_iio("in_accel_x_raw", xp, sizeof(xp)) != HW_OK ||
       hw_find_iio("in_accel_y_raw", yp, sizeof(yp)) != HW_OK ||
       hw_find_iio("in_accel_z_raw", zp, sizeof(zp)) != HW_OK) {
        s_found = 0;
        ui_label_set_text_fmt(s_status,
                              "未找到重力传感器\n"
                              "已尝试 /sys/bus/iio/devices/iio:device*/in_accel_x_raw");
        return;
    }

    s_found = 1;

    if(hw_read_int(xp, &x) != HW_OK || hw_read_int(yp, &y) != HW_OK ||
       hw_read_int(zp, &z) != HW_OK) {
        ui_label_set_text_fmt(s_status, "设备: %s\n读取加速度原始值失败", xp);
        return;
    }

    if(hw_find_iio("in_accel_scale", sp, sizeof(sp)) == HW_OK &&
       hw_read_file(sp, scale_txt, sizeof(scale_txt)) == HW_OK) {
        scale = atof(scale_txt);
        if(scale <= 0.0) scale = 1.0;
    }
    else {
        snprintf(scale_txt, sizeof(scale_txt), "未找到");
    }

    snprintf(buf, sizeof(buf),
             "设备: %s\nX: raw=%ld  %.4f g\nY: raw=%ld  %.4f g\nZ: raw=%ld  %.4f g\n"
             "scale: %s",
             xp, x, (double)x * scale, y, (double)y * scale, z, (double)z * scale,
             scale_txt);
    lv_label_set_text(s_status, buf);
}

static void gsensor_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    gsensor_refresh();
}

static void gsensor_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    gsensor_refresh();
    if(s_found) ui_set_state(s_result, 1, "已刷新加速度数据");
    else ui_set_state(s_result, 0, "未找到重力传感器");
}

void page_gsensor_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    ui_page_title(parent, "重力传感器");
    s_status = ui_page_status(parent, "正在检测...");
    s_found = 0;

    row = ui_page_button_row(parent);
    ui_page_button(row, "刷新", gsensor_refresh_cb, NULL);

    s_result = ui_page_result(parent, "数据每 500ms 自动刷新。");

    gsensor_refresh();
    if(s_found) {
        ui_set_state(s_result, 1, "已找到重力传感器");
        ui_timer_attach(s_status, 500, gsensor_timer_cb, NULL);
    }
    else {
        ui_set_state(s_result, 0, "未找到重力传感器");
    }
}
