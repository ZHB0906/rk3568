/**
 * @file page_rtc.c
 * RTC 实时时钟：/dev/rtc0 检测、ioctl 读写时间、hwclock 校验。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <fcntl.h>
#include <linux/rtc.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;

static int rtc_read_time(struct rtc_time *out)
{
    int fd;
    int rc;

    fd = open("/dev/rtc0", O_RDONLY);
    if(fd < 0) return HW_ERR_NOTFOUND;
    rc = ioctl(fd, RTC_RD_TIME, out);
    close(fd);
    return (rc == 0) ? HW_OK : HW_ERR_SYS;
}

static void rtc_refresh(void)
{
    struct rtc_time t;
    char tbuf[96];
    char hwbuf[256];
    char buf[640];
    int rc;
    int hrc;

    if(!hw_path_exists("/dev/rtc0")) {
        ui_label_set_text_fmt(s_status,
                              "未找到 /dev/rtc0\n请确认内核已加载 RTC 驱动（例如 rtc-hym8563）");
        ui_set_state(s_result, 0, "未找到 RTC 设备节点");
        return;
    }

    rc = rtc_read_time(&t);
    if(rc == HW_OK) {
        snprintf(tbuf, sizeof(tbuf), "%04d-%02d-%02d %02d:%02d:%02d (星期%d)",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec, t.tm_wday);
    }
    else {
        snprintf(tbuf, sizeof(tbuf), "读取失败（%s）", hw_strerror(rc));
    }

    hrc = hw_run_cmd("hwclock -r 2>&1 | head -n 1", hwbuf, sizeof(hwbuf));
    if(hrc != HW_OK || hwbuf[0] == '\0') {
        snprintf(hwbuf, sizeof(hwbuf), "不可用（%s）", hw_strerror(hrc));
    }

    snprintf(buf, sizeof(buf),
             "设备节点: /dev/rtc0（存在）\nRTC 时间: %s\nhwclock -r: %s",
             tbuf, hwbuf);
    lv_label_set_text(s_status, buf);

    if(rc == HW_OK) ui_set_state(s_result, 1, "ioctl RTC_RD_TIME 读取成功");
    else ui_set_state(s_result, 0, "ioctl RTC_RD_TIME 失败：%s", hw_strerror(rc));
}

static void rtc_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    rtc_refresh();
}

static void rtc_write_cb(lv_event_t *e)
{
    struct tm tmv;
    struct rtc_time t;
    time_t now;
    int fd;
    int rc;

    LV_UNUSED(e);

    if(!hw_path_exists("/dev/rtc0")) {
        ui_set_state(s_result, 0, "未找到 RTC 设备节点");
        return;
    }

    now = time(NULL);
    if(localtime_r(&now, &tmv) == NULL) {
        ui_set_state(s_result, 0, "获取本地时间失败");
        return;
    }

    memset(&t, 0, sizeof(t));
    t.tm_sec = tmv.tm_sec;
    t.tm_min = tmv.tm_min;
    t.tm_hour = tmv.tm_hour;
    t.tm_mday = tmv.tm_mday;
    t.tm_mon = tmv.tm_mon;
    t.tm_year = tmv.tm_year;
    t.tm_wday = tmv.tm_wday;
    t.tm_yday = tmv.tm_yday;
    t.tm_isdst = tmv.tm_isdst;

    fd = open("/dev/rtc0", O_RDWR);
    if(fd < 0) {
        ui_set_state(s_result, 0, "以读写方式打开 /dev/rtc0 失败（需要 root）");
        return;
    }
    rc = ioctl(fd, RTC_SET_TIME, &t);
    close(fd);

    rtc_refresh();
    if(rc == 0) {
        ui_set_state(s_result, 1, "已写入本机时间 %04d-%02d-%02d %02d:%02d:%02d",
                     t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                     t.tm_hour, t.tm_min, t.tm_sec);
    }
    else {
        ui_set_state(s_result, 0, "ioctl RTC_SET_TIME 失败（需要 root 权限或 RTC 只读）");
    }
}

void page_rtc_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    ui_page_title(parent, "RTC 实时时钟");
    s_status = ui_page_status(parent, "正在检测 RTC 设备...");
    row = ui_page_button_row(parent);
    ui_page_button(row, "刷新时间", rtc_refresh_cb, NULL);
    ui_page_button(row, "写入当前时间", rtc_write_cb, NULL);
    s_result = ui_page_result(parent, "等待操作");
    rtc_refresh();
}
