/**
 * @file page_wifi.c
 * WiFi：启动 wlan0，扫描并列出最多 5 个 SSID，显示当前 IP。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>

#define WIFI_SSID_MAX 5

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;

static void wifi_refresh_status(void)
{
    char buf[600];
    size_t used = 0;
    char ip[40] = "-";
    char state[32] = "unknown";
    char mac[40] = "-";

    if(!hw_path_exists("/sys/class/net/wlan0")) {
        ui_label_set_text_fmt(s_status, "未找到 wlan0 网卡（/sys/class/net/wlan0）");
        return;
    }

    hw_read_file("/sys/class/net/wlan0/operstate", state, sizeof(state));
    hw_read_file("/sys/class/net/wlan0/address", mac, sizeof(mac));
    if(hw_iface_ip("wlan0", ip, sizeof(ip)) != HW_OK) snprintf(ip, sizeof(ip), "无 IPv4");

    ui_str_append(buf, sizeof(buf), &used, "wlan0 状态: %s\nMAC: %s\n当前 IP: %s",
                  state, mac, ip);
    lv_label_set_text(s_status, buf);
}

static int parse_iw_ssid(const char *txt, char ids[][40], int max)
{
    const char *p = txt;
    int n = 0;

    while((p = strstr(p, "SSID: ")) != NULL) {
        const char *e;
        size_t len;

        p += 6;
        e = strchr(p, '\n');
        len = (e != NULL) ? (size_t)(e - p) : strlen(p);
        if(len > 39) len = 39;
        if(len > 0 && n < max) {
            memcpy(ids[n], p, len);
            ids[n][len] = '\0';
            n++;
        }
        if(e == NULL) break;
        p = e + 1;
    }
    return n;
}

static int parse_wpa_ssid(const char *txt, char ids[][40], int max)
{
    const char *p = txt;
    int n = 0;

    while(*p != '\0') {
        const char *nl = strchr(p, '\n');
        const char *end = (nl != NULL) ? nl : p + strlen(p);
        const char *q = p;
        int tabs = 0;
        size_t len;

        while(q < end && tabs < 4) {
            if(*q == '\t') tabs++;
            q++;
        }
        if(tabs == 4 && q < end) {
            len = (size_t)(end - q);
            if(len > 39) len = 39;
            if(len > 0 && n < max) {
                memcpy(ids[n], q, len);
                ids[n][len] = '\0';
                n++;
            }
        }
        if(nl == NULL) break;
        p = nl + 1;
    }
    return n;
}

static void wifi_scan_task(char *out, size_t outlen, void *arg)
{
    char buf[3000];
    char ssid[WIFI_SSID_MAX][40];
    size_t used = 0;
    int n = 0;
    int i;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(!hw_path_exists("/sys/class/net/wlan0")) {
        snprintf(out, outlen, "未找到 wlan0 网卡，无法扫描");
        return;
    }

    hw_run_cmd("ip link set wlan0 up 2>&1", buf, sizeof(buf));

    if(hw_run_cmd("timeout 10 iw dev wlan0 scan 2>&1", buf, sizeof(buf)) == HW_OK &&
       strstr(buf, "SSID:") != NULL) {
        n = parse_iw_ssid(buf, ssid, WIFI_SSID_MAX);
    }
    else if(hw_run_cmd("timeout 10 wpa_cli -i wlan0 scan_results 2>&1", buf, sizeof(buf)) == HW_OK) {
        n = parse_wpa_ssid(buf, ssid, WIFI_SSID_MAX);
    }
    else {
        ui_str_append(out, outlen, &used,
                      "扫描失败（iw / wpa_cli 均不可用或返回错误）:\n%s",
                      (buf[0] != '\0') ? buf : "(无输出)");
        return;
    }

    if(n == 0) {
        ui_str_append(out, outlen, &used, "未扫描到 SSID（可能周围无热点或权限不足）");
        return;
    }

    ui_str_append(out, outlen, &used, "扫描到 %d 个 SSID（最多显示 %d 个）:\n", n, WIFI_SSID_MAX);
    for(i = 0; i < n; i++) {
        ui_str_append(out, outlen, &used, "  %d. %s\n", i + 1, ssid[i]);
    }
}

static void wifi_scan_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, wifi_scan_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void wifi_up_cb(lv_event_t *e)
{
    char buf[256];

    LV_UNUSED(e);
    if(!hw_path_exists("/sys/class/net/wlan0")) {
        ui_set_state(s_result, 0, "未找到 wlan0 网卡");
        return;
    }
    if(hw_run_cmd("ip link set wlan0 up 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_set_state(s_result, 1, "wlan0 已 up");
    }
    else {
        ui_set_state(s_result, 0, "wlan0 up 失败: %s", buf);
    }
    wifi_refresh_status();
}

static void wifi_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    wifi_refresh_status();
    ui_set_state(s_result, 1, "WiFi 状态已刷新");
}

void page_wifi_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "WiFi");
    s_status = ui_page_status(parent, "正在检测 wlan0...");

    row = ui_page_button_row(parent);
    ui_page_button(row, "启动 wlan0", wifi_up_cb, NULL);
    ui_page_button(row, "扫描", wifi_scan_cb, NULL);
    ui_page_button(row, "刷新", wifi_refresh_cb, NULL);

    s_result = ui_page_result(parent, "扫描使用 iw dev wlan0 scan，失败时回退 wpa_cli。");
    ui_bind_result(s_result, &s_async, 300);

    wifi_refresh_status();
}
