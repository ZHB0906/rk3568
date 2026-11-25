/**
 * @file page_uart.c
 * 串口：/dev/ttyS3（可用 UART_DEVICE 环境变量覆盖），115200 8N1 发送并回读 500ms。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define UART_DEV_DEFAULT "/dev/ttyS3"

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;
static char s_dev[64];

static void uart_task(char *out, size_t outlen, void *arg)
{
    const char *dev = (const char *)arg;
    const char *msg = "RK3568 UART TEST\n";
    struct termios tio;
    char rx[256];
    size_t rlen = 0;
    size_t used = 0;
    int fd;
    int i;

    out[0] = '\0';

    fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd < 0) {
        snprintf(out, outlen, "打开 %s 失败: %s", dev, strerror(errno));
        return;
    }

    memset(&tio, 0, sizeof(tio));
    if(tcgetattr(fd, &tio) != 0) {
        snprintf(out, outlen, "%s 不是有效串口: %s", dev, strerror(errno));
        close(fd);
        return;
    }

    cfmakeraw(&tio);
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag &= ~PARENB;
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CRTSCTS;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    if(tcsetattr(fd, TCSANOW, &tio) != 0) {
        snprintf(out, outlen, "配置 %s 为 115200 8N1 失败: %s", dev, strerror(errno));
        close(fd);
        return;
    }

    tcflush(fd, TCIOFLUSH);

    if(write(fd, msg, strlen(msg)) != (ssize_t)strlen(msg)) {
        snprintf(out, outlen, "向 %s 写数据失败: %s", dev, strerror(errno));
        close(fd);
        return;
    }
    tcdrain(fd);

    /* 约 500ms 回读窗口 */
    for(i = 0; i < 50; i++) {
        ssize_t n = read(fd, rx + rlen, sizeof(rx) - 1 - rlen);
        if(n > 0) rlen += (size_t)n;
        if(rlen >= sizeof(rx) - 1) break;
        usleep(10000);
    }
    close(fd);

    ui_str_append(out, outlen, &used, "设备: %s（115200 8N1）\n发送: %s", dev, msg);

    if(rlen == 0) {
        ui_str_append(out, outlen, &used,
                      "接收: 0 字节（如需回环，请短接该串口的 TX 与 RX）");
        return;
    }

    rx[rlen] = '\0';
    ui_str_append(out, outlen, &used, "接收: %u 字节\n内容: %s\nHEX: ",
                  (unsigned)rlen, rx);
    for(i = 0; i < (int)rlen && i < 64; i++) {
        ui_str_append(out, outlen, &used, "%02X ", (unsigned char)rx[i]);
    }
}

static void uart_test_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(ui_task_start(&s_async, uart_task, s_dev) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void uart_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(hw_path_exists(s_dev)) {
        ui_label_set_text_fmt(s_status, "串口设备: %s\n状态: 存在\n参数: 115200 8N1", s_dev);
        ui_set_state(s_result, 1, "已刷新串口状态");
    }
    else {
        ui_label_set_text_fmt(s_status, "未找到串口设备 %s\n可用环境变量 UART_DEVICE 覆盖", s_dev);
        ui_set_state(s_result, 0, "未找到串口设备 %s", s_dev);
    }
}

void page_uart_create(lv_obj_t *parent)
{
    const char *env = getenv("UART_DEVICE");
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    snprintf(s_dev, sizeof(s_dev), "%s",
             (env != NULL && env[0] != '\0') ? env : UART_DEV_DEFAULT);

    ui_page_title(parent, "串口测试");
    s_status = ui_page_status(parent, "正在检测串口设备...");

    row = ui_page_button_row(parent);
    ui_page_button(row, "发送并接收", uart_test_cb, NULL);
    ui_page_button(row, "刷新状态", uart_refresh_cb, NULL);

    s_result = ui_page_result(parent, "发送 \"RK3568 UART TEST\\n\" 后回读 500ms。");
    ui_bind_result(s_result, &s_async, 300);

    if(hw_path_exists(s_dev)) {
        ui_label_set_text_fmt(s_status, "串口设备: %s\n状态: 存在\n参数: 115200 8N1", s_dev);
    }
    else {
        ui_label_set_text_fmt(s_status, "未找到串口设备 %s\n可用环境变量 UART_DEVICE 覆盖", s_dev);
        ui_set_state(s_result, 0, "未找到串口设备 %s", s_dev);
    }
}
