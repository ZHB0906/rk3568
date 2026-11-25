/**
 * @file page_bt.c
 * 蓝牙：hciconfig -a、hciconfig hci0 up、hcitool scan --flush（15 秒超时）。
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

static void bt_refresh_status(void)
{
    char buf[600];
    char info[600];

    if(!hw_path_exists("/sys/class/bluetooth/hci0")) {
        ui_label_set_text_fmt(s_status,
                              "未找到蓝牙适配器（/sys/class/bluetooth/hci0）\n"
                              "请确认 BT 驱动与 hciattach/hciuart 已启动");
        return;
    }

    if(hw_run_cmd("hciconfig -a 2>&1", info, sizeof(info)) != HW_OK) {
        snprintf(info, sizeof(info), "hciconfig 不可用（bluez 未安装）");
    }
    snprintf(buf, sizeof(buf), "蓝牙适配器 hci0 存在\n%s", info);
    lv_label_set_text(s_status, buf);
}

static void bt_scan_task(char *out, size_t outlen, void *arg)
{
    char buf[1400];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(hw_run_cmd("hciconfig -a 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used, "hciconfig -a:\n%s\n\n", buf);
    }
    else {
        ui_str_append(out, outlen, &used, "hciconfig -a 执行失败:\n%s\n\n",
                      (buf[0] != '\0') ? buf : "(无输出，可能未安装 bluez)");
    }

    if(hw_run_cmd("hciconfig hci0 up 2>&1", buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used, "hci0 up 失败:\n%s",
                      (buf[0] != '\0') ? buf : "(无输出，可能没有 hci0)");
        return;
    }
    ui_str_append(out, outlen, &used, "hci0 已上电\n开始扫描 15 秒...\n");

    if(hw_run_cmd("timeout 15 hcitool scan --flush 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used, "扫描结果:\n%s",
                      (buf[0] != '\0') ? buf : "未发现设备");
    }
    else {
        ui_str_append(out, outlen, &used, "扫描失败或超时:\n%s",
                      (buf[0] != '\0') ? buf : "(无输出)");
    }
}

static void bt_scan_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(!hw_path_exists("/sys/class/bluetooth/hci0")) {
        ui_set_state(s_result, 0, "未找到蓝牙适配器 hci0");
        return;
    }
    if(ui_task_start(&s_async, bt_scan_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void bt_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    bt_refresh_status();
    if(hw_path_exists("/sys/class/bluetooth/hci0")) ui_set_state(s_result, 1, "hci0 状态已刷新");
    else ui_set_state(s_result, 0, "未找到蓝牙适配器 hci0");
}

void page_bt_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "蓝牙");
    s_status = ui_page_status(parent, "正在读取蓝牙状态...");

    row = ui_page_button_row(parent);
    ui_page_button(row, "上电并扫描", bt_scan_cb, NULL);
    ui_page_button(row, "刷新", bt_refresh_cb, NULL);

    s_result = ui_page_result(parent, "扫描最多耗时 15 秒，期间界面可继续操作。");
    ui_bind_result(s_result, &s_async, 300);

    bt_refresh_status();
}
