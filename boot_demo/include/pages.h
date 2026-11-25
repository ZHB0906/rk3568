/**
 * @file pages.h
 * 19 个硬件测试页面的注册表接口。
 */
#ifndef BOOT_DEMO_PAGES_H
#define BOOT_DEMO_PAGES_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;             /* 导航按钮显示的名字 */
    void (*create_cb)(lv_obj_t *parent); /* 在内容区创建页面 */
} page_desc_t;

/* 取页面注册表（返回静态数组首地址，count 为元素个数） */
const page_desc_t *pages_registry(int *count);

void page_rtc_create(lv_obj_t *parent);
void page_led_create(lv_obj_t *parent);
void page_buzzer_create(lv_obj_t *parent);
void page_backlight_create(lv_obj_t *parent);
void page_key_create(lv_obj_t *parent);
void page_lightsensor_create(lv_obj_t *parent);
void page_gsensor_create(lv_obj_t *parent);
void page_adc_create(lv_obj_t *parent);
void page_audio_create(lv_obj_t *parent);
void page_touch_create(lv_obj_t *parent);
void page_udisk_create(lv_obj_t *parent);
void page_tfcard_create(lv_obj_t *parent);
void page_eeprom_create(lv_obj_t *parent);
void page_uart_create(lv_obj_t *parent);
void page_can_create(lv_obj_t *parent);
void page_eth_create(lv_obj_t *parent);
void page_4g_create(lv_obj_t *parent);
void page_wifi_create(lv_obj_t *parent);
void page_bt_create(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_DEMO_PAGES_H */
