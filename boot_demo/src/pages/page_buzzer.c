/**
 * @file page_buzzer.c
 * GPIO 蜂鸣器：默认 GPIO111，按钮响 1 秒（用 lv_timer 翻转），可手动开关。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <stdlib.h>

#define BUZZER_GPIO_DEFAULT 111

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static lv_timer_t *s_timer;
static int s_gpio = BUZZER_GPIO_DEFAULT;
static int s_ready;
static int s_on;

static void buzzer_report(void)
{
    if(!s_ready) {
        ui_label_set_text_fmt(s_status,
                              "未找到 GPIO%d 蜂鸣器\n可通过环境变量 BUZZER_GPIO 指定其他 GPIO",
                              s_gpio);
        return;
    }
    ui_label_set_text_fmt(s_status,
                          "蜂鸣器 GPIO: %d（/sys/class/gpio/gpio%d）\n方向: output\n当前电平: %s",
                          s_gpio, s_gpio, s_on ? "高（响）" : "低（静音）");
}

static void buzzer_apply(int val)
{
    if(!s_ready) return;
    if(hw_gpio_set(s_gpio, val) != HW_OK) {
        ui_set_state(s_result, 0, "GPIO%d 写入失败", s_gpio);
        return;
    }
    s_on = val;
    buzzer_report();
}

static void buzzer_tick_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    buzzer_apply(0);
    if(s_timer != NULL) lv_timer_pause(s_timer);
    ui_set_state(s_result, 1, "1 秒结束，蜂鸣器已关闭");
}

static void buzzer_beep_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(!s_ready) {
        ui_set_state(s_result, 0, "未找到 GPIO%d，无法驱动蜂鸣器", s_gpio);
        return;
    }
    buzzer_apply(1);
    if(s_timer != NULL) lv_timer_resume(s_timer);
    ui_set_state(s_result, 1, "蜂鸣器开始响，1 秒后自动关闭");
}

static void buzzer_toggle_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(!s_ready) {
        ui_set_state(s_result, 0, "未找到 GPIO%d，无法驱动蜂鸣器", s_gpio);
        return;
    }
    buzzer_apply(s_on ? 0 : 1);
    ui_set_state(s_result, 1, "蜂鸣器已%s", s_on ? "打开" : "关闭");
}

static void buzzer_refresh_cb(lv_event_t *e)
{
    int v = 0;

    LV_UNUSED(e);
    if(s_ready && hw_gpio_get(s_gpio, &v) == HW_OK) s_on = v;
    buzzer_report();
    if(s_ready) ui_set_state(s_result, 1, "已刷新，当前电平 %d", s_on);
    else ui_set_state(s_result, 0, "蜂鸣器 GPIO 不可用");
}

static void buzzer_delete_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
    if(s_ready && s_on) hw_gpio_set(s_gpio, 0);
    s_on = 0;
    s_status = NULL;
    s_result = NULL;
}

void page_buzzer_create(lv_obj_t *parent)
{
    const char *env = getenv("BUZZER_GPIO");
    lv_obj_t *row;
    int v = 0;

    if(env != NULL && env[0] != '\0') {
        int n = atoi(env);
        if(n > 0) s_gpio = n;
    }

    ui_page_title(parent, "蜂鸣器");
    s_status = ui_page_status(parent, "正在检测蜂鸣器 GPIO...");

    s_ready = (hw_gpio_export(s_gpio, "out") == HW_OK);
    if(s_ready) {
        hw_gpio_set(s_gpio, 0);
        if(hw_gpio_get(s_gpio, &v) == HW_OK) s_on = v;
        else s_on = 0;
    }

    row = ui_page_button_row(parent);
    ui_page_button(row, "响 1 秒", buzzer_beep_cb, NULL);
    ui_page_button(row, "开/关切换", buzzer_toggle_cb, NULL);
    ui_page_button(row, "刷新状态", buzzer_refresh_cb, NULL);

    s_result = ui_page_result(parent, "点击“响 1 秒”测试蜂鸣器。");

    s_timer = lv_timer_create(buzzer_tick_cb, 1000, NULL);
    if(s_timer != NULL) lv_timer_pause(s_timer);
    /* 页面内容区只清理子对象，因此把清理钩子挂在页面自己的 status label 上 */
    lv_obj_add_event_cb(s_status, buzzer_delete_cb, LV_EVENT_DELETE, NULL);

    buzzer_report();
    if(!s_ready) ui_set_state(s_result, 0, "未找到 GPIO%d 蜂鸣器", s_gpio);
}
