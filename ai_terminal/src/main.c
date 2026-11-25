#define _POSIX_C_SOURCE 200809L

#include "lvgl.h"

#if defined(__has_include)
#  if __has_include("src/drivers/display/fb/lv_linux_fbdev.h")
#    include "src/drivers/display/fb/lv_linux_fbdev.h"
#  elif __has_include("lvgl/src/drivers/display/fb/lv_linux_fbdev.h")
#    include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#  endif
#  if __has_include("src/drivers/evdev/lv_evdev.h")
#    include "src/drivers/evdev/lv_evdev.h"
#  elif __has_include("lvgl/src/drivers/evdev/lv_evdev.h")
#    include "lvgl/src/drivers/evdev/lv_evdev.h"
#  endif
#else
#  include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#  include "lvgl/src/drivers/evdev/lv_evdev.h"
#endif

#include "app.h"
#include "storage.h"
#include "voice.h"
#include "pinyin.h"
#include "ui_theme.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int env_int(const char *name, int fallback)
{
    const char *value = getenv(name);
    int v;

    if (value == NULL || value[0] == '\0') {
        return fallback;
    }
    v = atoi(value);
    return (v > 0) ? v : fallback;
}

int main(void)
{
    const char *fb_device = getenv("LV_LINUX_FBDEV_DEVICE");
    const char *touch_device = getenv("LV_TOUCH_DEVICE");
    int screen_w = env_int("LV_TOUCH_W", UI_SCREEN_W);
    int screen_h = env_int("LV_TOUCH_H", UI_SCREEN_H);
    lv_display_t *disp;
    lv_indev_t *indev;

    if (fb_device == NULL || fb_device[0] == '\0') {
        fb_device = "/dev/fb0";
    }
    if (touch_device == NULL || touch_device[0] == '\0') {
        touch_device = "/dev/input/event0";
    }

    lv_init();

    storage_init();
    app_config_defaults(&g_cfg);
    config_load(&g_cfg);
    voice_init();
    pinyin_init();

    disp = lv_linux_fbdev_create();
    if (disp == NULL) {
        fprintf(stderr, "[main] 无法创建帧缓冲显示设备\n");
        return 1;
    }
    lv_linux_fbdev_set_file(disp, fb_device);
    lv_display_set_resolution(disp, screen_w, screen_h);
    lv_display_set_default(disp);

    indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, touch_device);
    if (indev == NULL) {
        fprintf(stderr, "[main] 无法打开触摸设备: %s\n", touch_device);
    }

    ui_app_start();

    while (1) {
        lv_timer_handler();
        lv_tick_inc(5);
        usleep(5000);
    }

    return 0;
}
