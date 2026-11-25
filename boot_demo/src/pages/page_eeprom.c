/**
 * @file page_eeprom.c
 * EEPROM：i2cdetect 扫描 i2c-0..4，i2cget/i2cset 读写 0x50 的 0x00 字节。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;

static int i2c_collect(const char *txt, unsigned char *found, int max)
{
    const char *p = txt;
    int n = 0;

    while(*p != '\0') {
        const char *nl = strchr(p, '\n');
        size_t linelen = (nl != NULL) ? (size_t)(nl - p) : strlen(p);
        const char *colon = (const char *)memchr(p, ':', linelen);

        if(colon != NULL) {
            const char *q = colon + 1;
            const char *end = p + linelen;

            while(q < end) {
                while(q < end && *q == ' ') q++;
                if(q + 1 < end && isxdigit((unsigned char)q[0]) && isxdigit((unsigned char)q[1]) &&
                   (q + 2 >= end || q[2] == ' ')) {
                    char tmp[3];
                    int v;
                    int i;
                    int exists = 0;

                    tmp[0] = q[0];
                    tmp[1] = q[1];
                    tmp[2] = '\0';
                    v = (int)strtol(tmp, NULL, 16);
                    for(i = 0; i < n; i++) {
                        if(found[i] == (unsigned char)v) exists = 1;
                    }
                    if(!exists && n < max) found[n++] = (unsigned char)v;
                    q += 2;
                }
                else {
                    q++;
                }
            }
        }
        if(nl == NULL) break;
        p = nl + 1;
    }
    return n;
}

static void eeprom_scan_task(char *out, size_t outlen, void *arg)
{
    size_t used = 0;
    int bus;

    LV_UNUSED(arg);
    out[0] = '\0';
    ui_str_append(out, outlen, &used, "i2cdetect -y -r 0..4:\n");

    for(bus = 0; bus < 5; bus++) {
        char cmd[64];
        char buf[1024];
        unsigned char addrs[32];
        int n;
        int i;

        snprintf(cmd, sizeof(cmd), "i2cdetect -y -r %d 2>&1", bus);
        if(hw_run_cmd(cmd, buf, sizeof(buf)) != HW_OK && strstr(buf, "00:") == NULL) {
            ui_str_append(out, outlen, &used, "  i2c-%d: 不可用\n", bus);
            continue;
        }

        n = i2c_collect(buf, addrs, (int)(sizeof(addrs) / sizeof(addrs[0])));
        if(n == 0) {
            ui_str_append(out, outlen, &used, "  i2c-%d: 无设备响应\n", bus);
            continue;
        }
        ui_str_append(out, outlen, &used, "  i2c-%d: ", bus);
        for(i = 0; i < n; i++) ui_str_append(out, outlen, &used, "0x%02x ", addrs[i]);
        ui_str_append(out, outlen, &used, "\n");
    }

    ui_str_append(out, outlen, &used, "\nEEPROM 常见于 i2c-0，地址 0x50");
}

static void eeprom_read_task(char *out, size_t outlen, void *arg)
{
    char buf[512];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';
    if(hw_run_cmd("i2cget -y 0 0x50 0x00 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used, "读取 i2c-0 0x50 寄存器 0x00:\n  %s", buf);
    }
    else {
        ui_str_append(out, outlen, &used, "读取失败（i2cget）:\n  %s",
                      (buf[0] != '\0') ? buf : "无输出，请确认 EEPROM 在 i2c-0 且地址为 0x50");
    }
}

static void eeprom_write_task(char *out, size_t outlen, void *arg)
{
    char buf[512];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';
    if(hw_run_cmd("i2cset -y 0 0x50 0x00 0xAA 2>&1", buf, sizeof(buf)) == HW_OK) {
        ui_str_append(out, outlen, &used,
                      "已向 i2c-0 0x50 寄存器 0x00 写入 0xAA\n可点“读 0x00”验证是否生效");
        if(buf[0] != '\0') ui_str_append(out, outlen, &used, "\n命令输出: %s", buf);
    }
    else {
        ui_str_append(out, outlen, &used, "写入失败（i2cset）:\n  %s",
                      (buf[0] != '\0') ? buf : "无输出");
    }
}

static void eeprom_scan_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, eeprom_scan_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void eeprom_read_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, eeprom_read_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void eeprom_write_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, eeprom_write_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

void page_eeprom_create(lv_obj_t *parent)
{
    lv_obj_t *row;
    lv_obj_t *sub;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "EEPROM 读写");
    s_status = ui_page_status(parent,
                              "使用 i2cdetect / i2cget / i2cset 访问 I2C 总线。\n"
                              "扫描会依次尝试 i2c-0 到 i2c-4。");

    row = ui_page_button_row(parent);
    ui_page_button(row, "扫描 I2C", eeprom_scan_cb, NULL);
    ui_page_button(row, "读 0x50:0x00", eeprom_read_cb, NULL);
    sub = ui_page_button_row(parent);
    ui_page_button(sub, "写 0xAA 到 0x50:0x00", eeprom_write_cb, NULL);

    s_result = ui_page_result(parent, "打开页面后自动扫描一次。");
    ui_bind_result(s_result, &s_async, 300);

    ui_task_start(&s_async, eeprom_scan_task, NULL);
}
