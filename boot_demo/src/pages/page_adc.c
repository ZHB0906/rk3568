/**
 * @file page_adc.c
 * ADC：扫描 iio in_voltage0..7_raw 八个通道并显示原始值。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>

#define ADC_CHANNELS 8

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static int s_found;

static void adc_refresh(void)
{
    char buf[1024];
    size_t used = 0;
    int ch;
    int found = 0;

    s_found = 0;
    ui_str_append(buf, sizeof(buf), &used, "ADC 通道原始值（in_voltageN_raw）:\n");

    for(ch = 0; ch < ADC_CHANNELS; ch++) {
        char fname[40];
        char path[192];
        long val = 0;

        snprintf(fname, sizeof(fname), "in_voltage%d_raw", ch);
        if(hw_find_iio(fname, path, sizeof(path)) != HW_OK) continue;
        if(hw_read_int(path, &val) != HW_OK) continue;

        ui_str_append(buf, sizeof(buf), &used, "  CH%d: %-8ld  %s\n", ch, val, path);
        found++;
    }

    if(found == 0) {
        ui_str_append(buf, sizeof(buf), &used,
                      "未找到 ADC 通道\n"
                      "已尝试 /sys/bus/iio/devices/iio:device*/in_voltage0_raw .. in_voltage7_raw");
    }
    else {
        s_found = 1;
        ui_str_append(buf, sizeof(buf), &used, "共 %d 个通道可用", found);
    }
    lv_label_set_text(s_status, buf);
}

static void adc_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    adc_refresh();
    if(s_found) ui_set_state(s_result, 1, "ADC 数据已刷新");
    else ui_set_state(s_result, 0, "未找到 ADC 通道");
}

static void adc_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    adc_refresh();
}

void page_adc_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    ui_page_title(parent, "ADC 采样");
    s_status = ui_page_status(parent, "正在扫描 ADC 通道...");
    s_found = 0;

    row = ui_page_button_row(parent);
    ui_page_button(row, "刷新", adc_refresh_cb, NULL);

    s_result = ui_page_result(parent, "点击“刷新”重新读取 8 个通道，页面打开期间每秒自动刷新。");

    adc_refresh();
    if(s_found) ui_set_state(s_result, 1, "ADC 通道扫描完成");
    else ui_set_state(s_result, 0, "未找到 ADC 通道");
    ui_timer_attach(s_status, 1000, adc_timer_cb, NULL);
}
