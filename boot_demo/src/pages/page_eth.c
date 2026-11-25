/**
 * @file page_eth.c
 * 以太网：枚举 /sys/class/net，显示 up/down、MAC、IP；一键 up + udhcpc。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <glob.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;

static void eth_refresh_status(void)
{
    glob_t g;
    char buf[1400];
    size_t used = 0;
    size_t i;

    ui_str_append(buf, sizeof(buf), &used, "网卡列表（/sys/class/net）:\n");

    if(glob("/sys/class/net/*", 0, NULL, &g) != 0) {
        ui_str_append(buf, sizeof(buf), &used, "  未找到网卡\n");
        lv_label_set_text(s_status, buf);
        return;
    }

    for(i = 0; i < g.gl_pathc; i++) {
        const char *base = strrchr(g.gl_pathv[i], '/');
        char state_path[192];
        char mac_path[192];
        char state[32] = "unknown";
        char mac[40] = "-";
        char ip[40] = "-";

        base = (base != NULL) ? base + 1 : g.gl_pathv[i];

        snprintf(state_path, sizeof(state_path), "%s/operstate", g.gl_pathv[i]);
        if(hw_read_file(state_path, state, sizeof(state)) != HW_OK) snprintf(state, sizeof(state), "unknown");

        snprintf(mac_path, sizeof(mac_path), "%s/address", g.gl_pathv[i]);
        if(hw_read_file(mac_path, mac, sizeof(mac)) != HW_OK) snprintf(mac, sizeof(mac), "-");

        if(hw_iface_ip(base, ip, sizeof(ip)) != HW_OK) snprintf(ip, sizeof(ip), "-");

        ui_str_append(buf, sizeof(buf), &used, "  %-8s %-7s MAC=%s  IP=%s\n",
                      base, state, mac, ip);
    }
    globfree(&g);

    lv_label_set_text(s_status, buf);
}

static void eth_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    eth_refresh_status();
}

static void eth_dhcp_task(char *out, size_t outlen, void *arg)
{
    char buf[700];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(hw_run_cmd("ip link set eth0 up 2>&1", buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used, "启用 eth0 失败:\n%s",
                      (buf[0] != '\0') ? buf : "(无输出)");
        return;
    }
    ui_str_append(out, outlen, &used, "eth0 已 up，开始 udhcpc 获取地址...\n");

    if(hw_run_cmd("udhcpc -i eth0 -t 3 -q -n 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used, "udhcpc 完成");
    }
    else {
        ui_str_append(out, outlen, &used, "udhcpc 失败或超时");
    }
    if(buf[0] != '\0') ui_str_append(out, outlen, &used, "\n%s", buf);

    if(hw_iface_ip("eth0", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used, "\n当前 eth0 IP: %s", buf);
    }
    else {
        ui_str_append(out, outlen, &used, "\n当前 eth0 无 IPv4 地址");
    }
}

static void eth_dhcp_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, eth_dhcp_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void eth_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    eth_refresh_status();
    ui_set_state(s_result, 1, "网卡状态已刷新");
}

void page_eth_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "以太网");
    s_status = ui_page_status(parent, "正在枚举网卡...");

    row = ui_page_button_row(parent);
    ui_page_button(row, "eth0 启动 + DHCP", eth_dhcp_cb, NULL);
    ui_page_button(row, "刷新", eth_refresh_cb, NULL);

    s_result = ui_page_result(parent, "点击按钮执行: ip link set eth0 up; udhcpc -i eth0 -t 3");
    ui_bind_result(s_result, &s_async, 300);

    eth_refresh_status();
    ui_timer_attach(s_status, 3000, eth_timer_cb, NULL);
}
