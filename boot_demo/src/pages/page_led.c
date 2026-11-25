/**
 * @file page_led.c
 * LED：枚举 /sys/class/leds 并可开关，同时支持 GPIO LED 120/121/123/124。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <glob.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LED_MAX      8
#define GPIO_LED_CNT 4

static lv_obj_t *s_status;
static lv_obj_t *s_result;

static char s_led_bright[LED_MAX][144];
static char s_led_name[LED_MAX][40];
static int  s_led_max[LED_MAX];
static int  s_led_on[LED_MAX];
static int  s_led_cnt;
static int  s_btn_cnt;          /* 本次页面实例创建的 LED 按钮数量 */
static lv_obj_t *s_led_btn[LED_MAX];

static const int s_gpio_leds[GPIO_LED_CNT] = { 120, 121, 123, 124 };
static int s_gpio_on[GPIO_LED_CNT];
static int s_gpio_ok[GPIO_LED_CNT];
static lv_obj_t *s_gpio_btn[GPIO_LED_CNT];

static void led_scan(void)
{
    glob_t g;
    size_t i;

    s_led_cnt = 0;
    memset(s_led_max, 0, sizeof(s_led_max));
    memset(s_led_on, 0, sizeof(s_led_on));

    if(glob("/sys/class/leds/*", 0, NULL, &g) != 0) return;

    for(i = 0; i < g.gl_pathc && s_led_cnt < LED_MAX; i++) {
        char path[176];
        char val[32];
        long maxv = 0;
        long curv = 0;
        const char *base = strrchr(g.gl_pathv[i], '/');
        int idx = s_led_cnt;

        base = (base != NULL) ? base + 1 : g.gl_pathv[i];
        snprintf(s_led_bright[idx], sizeof(s_led_bright[idx]), "%s/brightness", g.gl_pathv[i]);
        if(hw_read_file(s_led_bright[idx], val, sizeof(val)) != HW_OK) continue;

        snprintf(path, sizeof(path), "%s/max_brightness", g.gl_pathv[i]);
        if(hw_read_int(path, &maxv) != HW_OK || maxv <= 0) maxv = 1;
        curv = strtol(val, NULL, 10);

        snprintf(s_led_name[idx], sizeof(s_led_name[idx]), "%s", base);
        s_led_max[idx] = (int)maxv;
        s_led_on[idx] = (curv > 0);
        s_led_cnt++;
    }
    globfree(&g);
}

static void led_scan_gpio(void)
{
    int i;

    for(i = 0; i < GPIO_LED_CNT; i++) {
        int v = 0;
        s_gpio_ok[i] = (hw_gpio_export(s_gpio_leds[i], "out") == HW_OK);
        if(s_gpio_ok[i] && hw_gpio_get(s_gpio_leds[i], &v) == HW_OK) s_gpio_on[i] = v;
    }
}

static void led_button_text(int idx)
{
    lv_obj_t *lbl;

    if(idx < 0 || idx >= s_btn_cnt || s_led_btn[idx] == NULL) return;
    lbl = lv_obj_get_child(s_led_btn[idx], 0);
    if(lbl != NULL) {
        ui_label_set_text_fmt(lbl, "%s %s", s_led_name[idx], s_led_on[idx] ? "开" : "关");
    }
}

static void led_gpio_button_text(int idx)
{
    lv_obj_t *lbl;

    if(idx < 0 || idx >= GPIO_LED_CNT || s_gpio_btn[idx] == NULL) return;
    lbl = lv_obj_get_child(s_gpio_btn[idx], 0);
    if(lbl != NULL) {
        ui_label_set_text_fmt(lbl, "GPIO%d %s", s_gpio_leds[idx],
                              s_gpio_ok[idx] ? (s_gpio_on[idx] ? "高" : "低") : "未找到");
    }
}

static void led_refresh_status(void)
{
    char buf[1200];
    size_t used = 0;
    int i;

    ui_str_append(buf, sizeof(buf), &used, "LED 列表（/sys/class/leds）:");
    if(s_led_cnt == 0) {
        ui_str_append(buf, sizeof(buf), &used, " 未找到可用 LED\n");
    }
    else {
        ui_str_append(buf, sizeof(buf), &used, "\n");
        for(i = 0; i < s_led_cnt; i++) {
            char val[32] = "?";
            hw_read_file(s_led_bright[i], val, sizeof(val));
            ui_str_append(buf, sizeof(buf), &used, "  [%d] %s  brightness=%s / max=%d\n",
                          i, s_led_name[i], val, s_led_max[i]);
        }
    }

    ui_str_append(buf, sizeof(buf), &used, "GPIO LED: ");
    for(i = 0; i < GPIO_LED_CNT; i++) {
        if(s_gpio_ok[i]) {
            ui_str_append(buf, sizeof(buf), &used, "GPIO%d=%d  ", s_gpio_leds[i], s_gpio_on[i]);
        }
        else {
            ui_str_append(buf, sizeof(buf), &used, "GPIO%d=未找到  ", s_gpio_leds[i]);
        }
    }
    lv_label_set_text(s_status, buf);
}

static void led_toggle_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e) - 1;
    char v[8];

    if(idx < 0 || idx >= s_led_cnt) return;

    s_led_on[idx] = !s_led_on[idx];
    snprintf(v, sizeof(v), "%d", s_led_on[idx] ? s_led_max[idx] : 0);
    if(hw_write_file(s_led_bright[idx], v) == HW_OK) {
        ui_set_state(s_result, 1, "%s 已设置为%s（brightness=%s）",
                     s_led_name[idx], s_led_on[idx] ? "亮" : "灭", v);
    }
    else {
        s_led_on[idx] = !s_led_on[idx];
        ui_set_state(s_result, 0, "写入 %s 失败", s_led_bright[idx]);
    }
    led_button_text(idx);
    led_refresh_status();
}

static void led_gpio_toggle_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e) - 1;
    int want;

    if(idx < 0 || idx >= GPIO_LED_CNT) return;
    if(!s_gpio_ok[idx]) {
        ui_set_state(s_result, 0, "GPIO%d 未找到，无法操作", s_gpio_leds[idx]);
        return;
    }

    want = s_gpio_on[idx] ? 0 : 1;
    if(hw_gpio_set(s_gpio_leds[idx], want) == HW_OK) {
        s_gpio_on[idx] = want;
        ui_set_state(s_result, 1, "GPIO%d 已输出 %d", s_gpio_leds[idx], want);
    }
    else {
        ui_set_state(s_result, 0, "GPIO%d 写入失败", s_gpio_leds[idx]);
    }
    led_gpio_button_text(idx);
    led_refresh_status();
}

static void led_refresh_cb(lv_event_t *e)
{
    int i;

    LV_UNUSED(e);
    led_scan();
    for(i = 0; i < s_btn_cnt && i < s_led_cnt; i++) led_button_text(i);
    led_refresh_status();
    ui_set_state(s_result, 1, "已重新扫描，共 %d 个 LED", s_led_cnt);
}

void page_led_create(lv_obj_t *parent)
{
    lv_obj_t *row;
    lv_obj_t *sub;
    int i;

    ui_page_title(parent, "LED 指示灯");
    s_status = ui_page_status(parent, "正在扫描 LED...");

    led_scan();
    led_scan_gpio();

    s_btn_cnt = 0;
    row = ui_page_button_row(parent);
    for(i = 0; i < s_led_cnt; i++) {
        s_led_btn[i] = ui_page_button(row, s_led_name[i], led_toggle_cb,
                                      (void *)(intptr_t)(i + 1));
        s_btn_cnt = i + 1;
        led_button_text(i);
    }
    for(i = 0; i < GPIO_LED_CNT; i++) {
        s_gpio_btn[i] = ui_page_button(row, "GPIO", led_gpio_toggle_cb,
                                       (void *)(intptr_t)(i + 1));
        led_gpio_button_text(i);
    }

    sub = ui_page_button_row(parent);
    ui_page_button(sub, "重新扫描", led_refresh_cb, NULL);

    s_result = ui_page_result(parent,
                              "点击按钮切换 LED。\nGPIO LED 使用 /sys/class/gpio/gpio120/121/123/124。");
    led_refresh_status();
}
