/**
 * @file pages_registry.c
 * 19 个硬件测试页面的注册表。
 */
#include "pages.h"

static const page_desc_t s_pages[] = {
    { "RTC 时钟",   page_rtc_create },
    { "LED 灯",     page_led_create },
    { "蜂鸣器",     page_buzzer_create },
    { "背光",       page_backlight_create },
    { "按键",       page_key_create },
    { "光线传感器", page_lightsensor_create },
    { "重力传感器", page_gsensor_create },
    { "ADC",        page_adc_create },
    { "音频",       page_audio_create },
    { "触摸屏",     page_touch_create },
    { "U 盘",       page_udisk_create },
    { "TF 卡",      page_tfcard_create },
    { "EEPROM",     page_eeprom_create },
    { "串口",       page_uart_create },
    { "CAN",        page_can_create },
    { "以太网",     page_eth_create },
    { "4G 模块",    page_4g_create },
    { "WiFi",       page_wifi_create },
    { "蓝牙",       page_bt_create },
};

const page_desc_t *pages_registry(int *count)
{
    if(count != NULL) *count = (int)(sizeof(s_pages) / sizeof(s_pages[0]));
    return s_pages;
}
