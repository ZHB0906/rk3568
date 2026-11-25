/**
 * @file page_key.c
 * 按键：非阻塞读取 /dev/input/event0..event6，显示最近 5 条按键事件。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define KEY_DEV_MAX 7
#define KEY_LOG_MAX 5

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static int s_fds[KEY_DEV_MAX];
static char s_dev_name[KEY_DEV_MAX][64];
static int s_dev_cnt;
static char s_log[KEY_LOG_MAX][112];
static int s_log_cnt;

static const char *key_name(unsigned int code)
{
    switch(code) {
        case KEY_POWER:       return "KEY_POWER";
        case KEY_VOLUMEUP:    return "KEY_VOLUMEUP";
        case KEY_VOLUMEDOWN:  return "KEY_VOLUMEDOWN";
        case KEY_HOME:        return "KEY_HOME";
        case KEY_BACK:        return "KEY_BACK";
        case KEY_MENU:        return "KEY_MENU";
        case KEY_ENTER:       return "KEY_ENTER";
        case KEY_UP:          return "KEY_UP";
        case KEY_DOWN:        return "KEY_DOWN";
        case KEY_LEFT:        return "KEY_LEFT";
        case KEY_RIGHT:       return "KEY_RIGHT";
        case KEY_ESC:         return "KEY_ESC";
        case KEY_PLAYPAUSE:   return "KEY_PLAYPAUSE";
        case KEY_WAKEUP:      return "KEY_WAKEUP";
        case KEY_SLEEP:       return "KEY_SLEEP";
        case KEY_MUTE:        return "KEY_MUTE";
        case KEY_CAMERA:      return "KEY_CAMERA";
        default:              return NULL;
    }
}

static void key_refresh_status(void)
{
    char buf[800];
    size_t used = 0;
    int i;

    ui_str_append(buf, sizeof(buf), &used, "已打开的输入设备:");
    if(s_dev_cnt == 0) {
        ui_str_append(buf, sizeof(buf), &used, " 未找到按键输入设备（/dev/input/event0..6）\n");
    }
    else {
        ui_str_append(buf, sizeof(buf), &used, "\n");
        for(i = 0; i < s_dev_cnt; i++) {
            ui_str_append(buf, sizeof(buf), &used, "  event: %s\n", s_dev_name[i]);
        }
    }
    ui_str_append(buf, sizeof(buf), &used, "提示: 触摸事件也来自 evdev，按下屏幕同样会记录。");
    lv_label_set_text(s_status, buf);
}

static void key_render_log(void)
{
    char buf[700];
    size_t used = 0;
    int i;

    if(s_result == NULL) return;
    if(s_log_cnt == 0) {
        ui_label_set_text(s_result, "暂无按键事件，按下板载按键试试。");
        return;
    }

    ui_str_append(buf, sizeof(buf), &used, "最近 %d 条按键事件（新 -> 旧）:\n", s_log_cnt);
    for(i = 0; i < s_log_cnt; i++) {
        ui_str_append(buf, sizeof(buf), &used, "  %s\n", s_log[i]);
    }
    lv_label_set_text(s_result, buf);
}

static void key_add_event(const struct input_event *ev, int dev_idx)
{
    const char *name = key_name(ev->code);
    char line[112];
    int i;

    snprintf(line, sizeof(line), "[%s] type=%d code=%d(%s) value=%d",
             s_dev_name[dev_idx], (int)ev->type, (int)ev->code,
             (name != NULL) ? name : "未知", (int)ev->value);

    for(i = KEY_LOG_MAX - 1; i > 0; i--) {
        memcpy(s_log[i], s_log[i - 1], sizeof(s_log[i]));
    }
    snprintf(s_log[0], sizeof(s_log[0]), "%s", line);
    if(s_log_cnt < KEY_LOG_MAX) s_log_cnt++;

    key_render_log();
}

static void key_poll_cb(lv_timer_t *t)
{
    struct input_event ev;
    int i;

    LV_UNUSED(t);
    for(i = 0; i < s_dev_cnt; i++) {
        while(read(s_fds[i], &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
            if(ev.type == EV_KEY) key_add_event(&ev, i);
        }
    }
}

static void key_delete_cb(lv_event_t *e)
{
    int i;

    LV_UNUSED(e);
    for(i = 0; i < KEY_DEV_MAX; i++) {
        if(s_fds[i] >= 0) {
            close(s_fds[i]);
            s_fds[i] = -1;
        }
    }
    s_dev_cnt = 0;
    s_status = NULL;
    s_result = NULL;
}

static void key_clear_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    s_log_cnt = 0;
    memset(s_log, 0, sizeof(s_log));
    key_render_log();
}

void page_key_create(lv_obj_t *parent)
{
    lv_obj_t *row;
    int i;

    ui_page_title(parent, "按键检测");
    s_status = ui_page_status(parent, "正在打开输入设备...");

    s_dev_cnt = 0;
    s_log_cnt = 0;
    memset(s_log, 0, sizeof(s_log));

    for(i = 0; i < KEY_DEV_MAX; i++) {
        char path[48];
        char name[64];
        int fd;

        s_fds[i] = -1;
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        fd = open(path, O_RDONLY | O_NONBLOCK);
        if(fd < 0) continue;

        memset(name, 0, sizeof(name));
        if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) < 0) {
            snprintf(name, sizeof(name), "event%d", i);
        }
        name[sizeof(name) - 1] = '\0';

        s_fds[s_dev_cnt] = fd;
        snprintf(s_dev_name[s_dev_cnt], sizeof(s_dev_name[0]), "%s (%s)", path, name);
        s_dev_cnt++;
    }

    key_refresh_status();

    row = ui_page_button_row(parent);
    ui_page_button(row, "清空记录", key_clear_cb, NULL);

    s_result = ui_page_result(parent, "暂无按键事件，按下板载按键试试。");
    key_render_log();

    ui_timer_attach(s_status, 40, key_poll_cb, NULL);
    lv_obj_add_event_cb(s_status, key_delete_cb, LV_EVENT_DELETE, NULL);

    if(s_dev_cnt == 0) {
        ui_set_state(s_result, 0, "未找到按键输入设备");
    }
}
