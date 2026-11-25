/**
 * @file page_tfcard.c
 * TF 卡：检查 /dev/mmcblk0p1、/dev/mmcblk1p1，挂载到 /mnt/tfcard 并显示容量。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

#define TF_MNT "/mnt/tfcard"

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;
static char s_part[64];

static void tf_choose_part(void)
{
    static const char *cands[] = {
        "/dev/mmcblk0p1",
        "/dev/mmcblk1p1",
        "/dev/mmcblk0",
        "/dev/mmcblk1",
    };
    size_t i;

    s_part[0] = '\0';
    for(i = 0; i < sizeof(cands) / sizeof(cands[0]); i++) {
        if(hw_path_exists(cands[i])) {
            snprintf(s_part, sizeof(s_part), "%s", cands[i]);
            return;
        }
    }
}

static void tf_refresh_status(void)
{
    char buf[800];
    char mntbuf[256];
    size_t used = 0;

    tf_choose_part();

    ui_str_append(buf, sizeof(buf), &used, "分区检测:\n");
    ui_str_append(buf, sizeof(buf), &used, "  /dev/mmcblk0p1 : %s\n",
                  hw_path_exists("/dev/mmcblk0p1") ? "存在" : "未找到");
    ui_str_append(buf, sizeof(buf), &used, "  /dev/mmcblk1p1 : %s\n",
                  hw_path_exists("/dev/mmcblk1p1") ? "存在" : "未找到");
    ui_str_append(buf, sizeof(buf), &used, "候选分区: %s\n",
                  (s_part[0] != '\0') ? s_part : "无");

    if(hw_run_cmd("grep \" " TF_MNT " \" /proc/mounts 2>&1", mntbuf, sizeof(mntbuf)) == HW_OK) {
        ui_str_append(buf, sizeof(buf), &used, "挂载状态: 已挂载\n%s", mntbuf);
    }
    else {
        ui_str_append(buf, sizeof(buf), &used, "挂载状态: 未挂载");
    }
    lv_label_set_text(s_status, buf);
}

static void tf_mount_task(char *out, size_t outlen, void *arg)
{
    char cmd[320];
    char buf[512];
    char sizebuf[48];
    size_t used = 0;
    struct statvfs vfs;
    const char *part = (const char *)arg;

    out[0] = '\0';
    if(part == NULL || part[0] == '\0') {
        snprintf(out, outlen, "未找到 TF 卡设备节点（/dev/mmcblk0p1、/dev/mmcblk1p1 均不存在）");
        return;
    }

    snprintf(cmd, sizeof(cmd),
             "mkdir -p " TF_MNT " 2>&1; mount -t vfat %s " TF_MNT " 2>&1 || mount %s " TF_MNT " 2>&1",
             part, part);
    if(hw_run_cmd(cmd, buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used, "挂载 %s 失败:\n%s", part,
                      (buf[0] != '\0') ? buf : "(命令无输出)");
        return;
    }

    if(statvfs(TF_MNT, &vfs) == 0) {
        ui_format_size(sizebuf, sizeof(sizebuf),
                       (unsigned long long)vfs.f_blocks * (unsigned long long)vfs.f_frsize);
        ui_str_append(out, outlen, &used, "已挂载 %s -> " TF_MNT "\n总容量: %s\n", part, sizebuf);
        ui_format_size(sizebuf, sizeof(sizebuf),
                       (unsigned long long)vfs.f_bavail * (unsigned long long)vfs.f_frsize);
        ui_str_append(out, outlen, &used, "可用空间: %s", sizebuf);
    }
    else {
        ui_str_append(out, outlen, &used, "挂载成功，但读取 " TF_MNT " 容量失败");
    }
}

static void tf_umount_task(char *out, size_t outlen, void *arg)
{
    char buf[256];

    LV_UNUSED(arg);
    out[0] = '\0';
    if(hw_run_cmd("umount " TF_MNT " 2>&1", buf, sizeof(buf)) == HW_OK) {
        snprintf(out, outlen, "已卸载 " TF_MNT);
    }
    else {
        snprintf(out, outlen, "卸载失败:\n%s", (buf[0] != '\0') ? buf : "(设备未挂载)");
    }
}

static void tf_mount_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, tf_mount_task, s_part) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void tf_umount_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, tf_umount_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void tf_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    tf_refresh_status();
    if(s_part[0] != '\0') ui_set_state(s_result, 1, "检测到 TF 卡分区 %s", s_part);
    else ui_set_state(s_result, 0, "未找到 TF 卡分区");
}

void page_tfcard_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "TF 卡读写");
    s_status = ui_page_status(parent, "正在检测 TF 卡...");
    s_part[0] = '\0';

    row = ui_page_button_row(parent);
    ui_page_button(row, "挂载", tf_mount_cb, NULL);
    ui_page_button(row, "卸载", tf_umount_cb, NULL);
    ui_page_button(row, "刷新", tf_refresh_cb, NULL);

    s_result = ui_page_result(parent, "挂载点为 " TF_MNT "。");
    ui_bind_result(s_result, &s_async, 300);

    tf_refresh_status();
}
