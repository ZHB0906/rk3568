/**
 * @file main.c
 * rk3568 开机硬件自检桌面：fbdev 显示 + evdev 触摸 + LVGL 主界面。
 */
#include "lvgl.h"
#include "ui_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if LV_USE_LINUX_FBDEV
#  if defined(__has_include)
#    if __has_include("src/drivers/display/fb/lv_linux_fbdev.h")
#      include "src/drivers/display/fb/lv_linux_fbdev.h"
#    elif __has_include("lvgl/src/drivers/display/fb/lv_linux_fbdev.h")
#      include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#    endif
#  else
#    include "src/drivers/display/fb/lv_linux_fbdev.h"
#  endif
#endif

#if LV_USE_EVDEV
#  if defined(__has_include)
#    if __has_include("src/drivers/evdev/lv_evdev.h")
#      include "src/drivers/evdev/lv_evdev.h"
#    elif __has_include("lvgl/src/drivers/evdev/lv_evdev.h")
#      include "lvgl/src/drivers/evdev/lv_evdev.h"
#    endif
#  else
#    include "src/drivers/evdev/lv_evdev.h"
#  endif
#endif

int main(void)
{
    const char *fb_dev = getenv("LV_LINUX_FBDEV_DEVICE");
    const char *touch_dev = getenv("LV_TOUCH_DEVICE");
    lv_display_t *disp;

    setvbuf(stdout, NULL, _IOLBF, 0);

    if(fb_dev == NULL || fb_dev[0] == '\0') fb_dev = "/dev/fb0";
    if(touch_dev == NULL || touch_dev[0] == '\0') touch_dev = "/dev/input/event0";

    lv_init();

#if LV_USE_LINUX_FBDEV
    disp = lv_linux_fbdev_create();
    if(disp == NULL) {
        fprintf(stderr, "boot_demo: 创建 fbdev 显示设备失败\n");
        return 1;
    }
    lv_linux_fbdev_set_file(disp, fb_dev);
    printf("boot_demo: 显示设备 %s (%dx%d)\n", fb_dev,
           (int)lv_display_get_horizontal_resolution(disp),
           (int)lv_display_get_vertical_resolution(disp));
#else
    fprintf(stderr, "boot_demo: 未启用 LV_USE_LINUX_FBDEV\n");
    return 1;
#endif

#if LV_USE_EVDEV
    if(lv_evdev_create(LV_INDEV_TYPE_POINTER, touch_dev) == NULL) {
        fprintf(stderr, "boot_demo: 触摸设备 %s 打开失败，界面仍可显示但无法点击\n", touch_dev);
    }
    else {
        printf("boot_demo: 触摸设备 %s\n", touch_dev);
    }
#else
    fprintf(stderr, "boot_demo: 未启用 LV_USE_EVDEV\n");
#endif

    ui_main_create();

    while(1) {
        lv_timer_handler();
        lv_tick_inc(5);
        usleep(5000);
    }

    return 0;
}
