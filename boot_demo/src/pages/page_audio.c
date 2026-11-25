/**
 * @file page_audio.c
 * 音频：显示 /proc/asound/cards，aplay 播放测试音，arecord 录音 3 秒（独立线程）。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define TEST_WAV "/usr/share/sounds/test.wav"
#define REC_WAV  "/tmp/test.wav"

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;

static void audio_play_task(char *out, size_t outlen, void *arg)
{
    char buf[512];
    size_t used = 0;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(!hw_path_exists(TEST_WAV)) {
        snprintf(out, outlen, "未找到测试音频 %s\n可先点“录音 3 秒”生成 /tmp/test.wav", TEST_WAV);
        return;
    }

    if(hw_run_cmd("timeout 10 aplay -q " TEST_WAV " 2>&1", buf, sizeof(buf)) == HW_OK) {
        snprintf(out, outlen, "已播放 %s\n命令输出: %s", TEST_WAV, buf[0] ? buf : "无");
    }
    else {
        ui_str_append(out, outlen, &used, "播放失败（aplay 返回错误）:\n%s", buf);
    }
}

static void audio_record_task(char *out, size_t outlen, void *arg)
{
    char buf[512];
    size_t used = 0;
    struct stat st;

    LV_UNUSED(arg);
    out[0] = '\0';

    if(hw_run_cmd("timeout 10 arecord -d 3 -f cd -t wav " REC_WAV " 2>&1", buf, sizeof(buf)) != HW_OK) {
        ui_str_append(out, outlen, &used, "录音失败（arecord 返回错误）:\n%s", buf);
        return;
    }

    if(stat(REC_WAV, &st) == 0) {
        ui_str_append(out, outlen, &used, "录音完成: %s\n大小: %ld 字节，时长 3 秒", REC_WAV, (long)st.st_size);
    }
    else {
        ui_str_append(out, outlen, &used, "录音命令已执行，但未生成 %s", REC_WAV);
    }
    if(buf[0] != '\0') ui_str_append(out, outlen, &used, "\n命令输出: %s", buf);
}

static void audio_play_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, audio_play_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个音频任务仍在执行，请稍候");
    }
}

static void audio_record_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, audio_record_task, NULL) != 0) {
        ui_set_state(s_result, 0, "上一个音频任务仍在执行，请稍候");
    }
}

static void audio_cards_refresh(void)
{
    char buf[600];

    if(hw_read_file("/proc/asound/cards", buf, sizeof(buf)) != HW_OK) {
        ui_label_set_text_fmt(s_status, "未找到声卡信息（/proc/asound/cards 不存在）");
        return;
    }
    if(buf[0] == '\0') {
        ui_label_set_text_fmt(s_status, "声卡列表为空（未检测到 ALSA 声卡）");
        return;
    }
    ui_label_set_text_fmt(s_status, "声卡列表（/proc/asound/cards）:\n%s", buf);
}

static void audio_cards_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    audio_cards_refresh();
    ui_set_state(s_result, 1, "已刷新声卡列表");
}

void page_audio_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "音频测试");
    s_status = ui_page_status(parent, "正在读取声卡...");

    row = ui_page_button_row(parent);
    ui_page_button(row, "播放测试音", audio_play_cb, NULL);
    ui_page_button(row, "录音 3 秒", audio_record_cb, NULL);
    ui_page_button(row, "刷新声卡", audio_cards_cb, NULL);

    s_result = ui_page_result(parent, "播放使用 aplay，录音使用 arecord（输出到 /tmp/test.wav）。");
    ui_bind_result(s_result, &s_async, 300);

    audio_cards_refresh();
}
