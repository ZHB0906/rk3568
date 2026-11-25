/**
 * @file page_can.c
 * CAN：启动 can0（500kbps）、查看详情、发送一帧（cansend）。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;

static void can_refresh_status(void)
{
    char detail[900];
    char buf[1100];

    if(!hw_path_exists("/sys/class/net/can0")) {
        ui_label_set_text_fmt(s_status,
                              "未找到 can0 网卡（/sys/class/net/can0）\n"
                              "请确认 CAN 控制器驱动与设备树配置");
        return;
    }

    if(hw_run_cmd("ip -details link show can0 2>&1", detail, sizeof(detail)) != HW_OK) {
        snprintf(detail, sizeof(detail), "读取详细信息失败");
    }
    snprintf(buf, sizeof(buf), "can0 存在\n%s", detail);
    lv_label_set_text(s_status, buf);
}

static void can_up_task(char *out, size_t outlen, void *arg)
{
    char buf[512];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(hw_run_cmd("ip link set can0 down 2>/dev/null; "
                  "ip link set can0 up type can bitrate 500000 2>&1", buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used, "启动 can0 失败:\n%s",
                      (buf[0] != '\0') ? buf : "(无输出)");
        return;
    }
    ui_str_append(out, outlen, &used, "can0 已启动，bitrate=500000");
    if(buf[0] != '\0') ui_str_append(out, outlen, &used, "\n命令输出: %s", buf);

    if(hw_run_cmd("ip -details link show can0 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used, "\n%s", buf);
    }
}

static void can_send_task(char *out, size_t outlen, void *arg)
{
    char buf[512];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(hw_run_cmd("cansend can0 123#1122334455667788 2>&1", buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used,
                      "发送失败（cansend）:\n%s\n请先启动 can0，并确认 can-utils 已安装",
                      (buf[0] != '\0') ? buf : "(无输出)");
        return;
    }
    ui_str_append(out, outlen, &used, "已发送 CAN 帧: ID=0x123 数据=1122334455667788");
}

static void can_up_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, can_up_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void can_send_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, can_send_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void can_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    can_refresh_status();
    if(hw_path_exists("/sys/class/net/can0")) ui_set_state(s_result, 1, "can0 状态已刷新");
    else ui_set_state(s_result, 0, "未找到 can0");
}

void page_can_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "CAN 总线");
    s_status = ui_page_status(parent, "正在读取 can0 状态...");

    row = ui_page_button_row(parent);
    ui_page_button(row, "启动 can0 (500k)", can_up_cb, NULL);
    ui_page_button(row, "发送一帧", can_send_cb, NULL);
    ui_page_button(row, "刷新状态", can_refresh_cb, NULL);

    s_result = ui_page_result(parent, "发送帧: cansend can0 123#1122334455667788");
    ui_bind_result(s_result, &s_async, 300);

    can_refresh_status();
}
