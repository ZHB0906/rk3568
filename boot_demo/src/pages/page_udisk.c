/**
 * @file page_udisk.c
 * U 盘：枚举 /dev/sd*，挂载到 /mnt/udisk 并显示挂载结果与容量。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

#define UDISK_MNT "/mnt/udisk"

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;
static char s_part[64];

static void udisk_choose_part(void)
{
    glob_t g;

    s_part[0] = '\0';
    if(hw_path_exists("/dev/sda1")) {
        snprintf(s_part, sizeof(s_part), "/dev/sda1");
        return;
    }
    if(glob("/dev/sd*", 0, NULL, &g) != 0) return;
    if(g.gl_pathc > 0) {
        size_t i;
        for(i = 0; i < g.gl_pathc; i++) {
            const char *p = g.gl_pathv[i];
            size_t len = strlen(p);
            if(len > 0 && p[len - 1] >= '1' && p[len - 1] <= '9') {
                snprintf(s_part, sizeof(s_part), "%s", p);
                break;
            }
        }
        if(s_part[0] == '\0') snprintf(s_part, sizeof(s_part), "%s", g.gl_pathv[0]);
    }
    globfree(&g);
}

static void udisk_refresh_status(void)
{
    glob_t g;
    char buf[900];
    char mntbuf[256];
    size_t used = 0;
    size_t i;
    int n = 0;

    udisk_choose_part();

    ui_str_append(buf, sizeof(buf), &used, "块设备（/dev/sd*）:\n");
    if(glob("/dev/sd*", 0, NULL, &g) == 0) {
        for(i = 0; i < g.gl_pathc && i < 8; i++) {
            ui_str_append(buf, sizeof(buf), &used, "  %s\n", g.gl_pathv[i]);
            n++;
        }
        globfree(&g);
    }
    if(n == 0) {
        ui_str_append(buf, sizeof(buf), &used, "  未找到 U 盘设备节点\n");
    }

    ui_str_append(buf, sizeof(buf), &used, "待挂载分区: %s\n",
                  (s_part[0] != '\0') ? s_part : "无");

    if(hw_run_cmd("grep \" " UDISK_MNT " \" /proc/mounts 2>&1", mntbuf, sizeof(mntbuf)) == HW_OK) {
        ui_str_append(buf, sizeof(buf), &used, "挂载状态: 已挂载\n%s", mntbuf);
    }
    else {
        ui_str_append(buf, sizeof(buf), &used, "挂载状态: 未挂载");
    }

    lv_label_set_text(s_status, buf);
}

static void udisk_mount_task(char *out, size_t outlen, void *arg)
{
    char cmd[320];
    char buf[512];
    char sizebuf[48];
    size_t used = 0;
    struct statvfs vfs;
    const char *part = (const char *)arg;

    out[0] = '\0';
    if(part == NULL || part[0] == '\0') {
        snprintf(out, outlen, "未找到 U 盘设备节点（/dev/sd*）");
        return;
    }

    snprintf(cmd, sizeof(cmd),
             "mkdir -p " UDISK_MNT " 2>&1; mount -t vfat %s " UDISK_MNT " 2>&1 || mount %s " UDISK_MNT " 2>&1",
             part, part);
    if(hw_run_cmd(cmd, buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used, "挂载 %s 失败:\n%s", part,
                      (buf[0] != '\0') ? buf : "(命令无输出，请确认设备为 FAT 分区)");
        return;
    }

    if(statvfs(UDISK_MNT, &vfs) == 0) {
        ui_format_size(sizebuf, sizeof(sizebuf),
                       (unsigned long long)vfs.f_blocks * (unsigned long long)vfs.f_frsize);
        ui_str_append(out, outlen, &used, "已挂载 %s -> " UDISK_MNT "\n总容量: %s\n", part, sizebuf);
        ui_format_size(sizebuf, sizeof(sizebuf),
                       (unsigned long long)vfs.f_bavail * (unsigned long long)vfs.f_frsize);
        ui_str_append(out, outlen, &used, "可用空间: %s", sizebuf);
    }
    else {
        ui_str_append(out, outlen, &used, "挂载成功，但读取 " UDISK_MNT " 容量失败");
    }
}

static void udisk_umount_task(char *out, size_t outlen, void *arg)
{
    char buf[256];

    LV_UNUSED(arg);
    out[0] = '\0';
    if(hw_run_cmd("umount " UDISK_MNT " 2>&1", buf, sizeof(buf)) == HW_OK) {
        snprintf(out, outlen, "已卸载 " UDISK_MNT);
    }
    else {
        snprintf(out, outlen, "卸载失败:\n%s", (buf[0] != '\0') ? buf : "(设备未挂载)");
    }
}

static void udisk_mount_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, udisk_mount_task, s_part) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void udisk_umount_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, udisk_umount_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void udisk_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    udisk_refresh_status();
    ui_set_state(s_result, (s_part[0] != '\0'),
                 (s_part[0] != '\0') ? "已刷新，检测到 U 盘分区 %s" : "未找到 U 盘设备 %s", s_part);
}

void page_udisk_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "U 盘读写");
    s_status = ui_page_status(parent, "正在扫描 /dev/sd* ...");
    s_part[0] = '\0';

    row = ui_page_button_row(parent);
    ui_page_button(row, "挂载", udisk_mount_cb, NULL);
    ui_page_button(row, "卸载", udisk_umount_cb, NULL);
    ui_page_button(row, "刷新", udisk_refresh_cb, NULL);

    s_result = ui_page_result(parent, "挂载点为 " UDISK_MNT "，默认优先使用 /dev/sda1。");
    ui_bind_result(s_result, &s_async, 300);

    udisk_refresh_status();
}
