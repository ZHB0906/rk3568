/**
 * @file page_4g.c
 * 4G 模块：枚举 /dev/ttyUSB*，通过 ttyUSB2 发送 AT 指令并显示响应。
 */
#include "lvgl.h"
#include "ui_common.h"
#include "hw.h"

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MODEM_DEV_DEFAULT "/dev/ttyUSB2"

static lv_obj_t *s_status;
static lv_obj_t *s_result;
static ui_result_t s_async;
static int s_async_inited;
static char s_dev[64];

static void sanitize(char *s)
{
    for(; *s != '\0'; s++) {
        if(*s == '\r' || *s == '\n') *s = ' ';
    }
}

static int at_cmd(int fd, const char *cmd, char *resp, size_t len)
{
    size_t rlen = 0;
    int i;

    resp[0] = '\0';
    if(write(fd, cmd, strlen(cmd)) != (ssize_t)strlen(cmd)) return -1;

    for(i = 0; i < 60; i++) {
        ssize_t n = read(fd, resp + rlen, len - 1 - rlen);
        if(n > 0) rlen += (size_t)n;
        resp[rlen] = '\0';
        if(strstr(resp, "OK") != NULL || strstr(resp, "ERROR") != NULL) break;
        if(rlen >= len - 1) break;
        usleep(30000);
    }
    resp[rlen] = '\0';
    sanitize(resp);
    return (rlen > 0) ? 0 : -1;
}

static void modem_task(char *out, size_t outlen, void *arg)
{
    const char *dev = (const char *)arg;
    struct termios tio;
    char resp[360];
    size_t used = 0;
    int fd;

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
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &tio);
    tcflush(fd, TCIOFLUSH);

    ui_str_append(out, outlen, &used, "串口: %s（115200 8N1）\n\n", dev);

    if(at_cmd(fd, "AT\r\n", resp, sizeof(resp)) == 0) {
        ui_str_append(out, outlen, &used, "AT: %s\n", resp);
    }
    else {
        ui_str_append(out, outlen, &used, "AT: 无响应（模块未就绪或波特率不匹配）\n");
        close(fd);
        return;
    }

    if(at_cmd(fd, "AT+CPIN?\r\n", resp, sizeof(resp)) == 0) {
        ui_str_append(out, outlen, &used, "AT+CPIN?: %s\n", resp);
    }
    else {
        ui_str_append(out, outlen, &used, "AT+CPIN?: 无响应\n");
    }

    if(at_cmd(fd, "AT+CSQ\r\n", resp, sizeof(resp)) == 0) {
        ui_str_append(out, outlen, &used, "AT+CSQ: %s\n", resp);
    }
    else {
        ui_str_append(out, outlen, &used, "AT+CSQ: 无响应\n");
    }

    if(at_cmd(fd, "AT+COPS?\r\n", resp, sizeof(resp)) == 0) {
        ui_str_append(out, outlen, &used, "AT+COPS?: %s\n", resp);
    }
    else {
        ui_str_append(out, outlen, &used, "AT+COPS?: 无响应\n");
    }

    close(fd);
}

static void modem_refresh_status(void)
{
    glob_t g;
    char buf[700];
    size_t used = 0;
    size_t i;
    int n = 0;

    ui_str_append(buf, sizeof(buf), &used, "4G 模块串口（/dev/ttyUSB*）:\n");
    if(glob("/dev/ttyUSB*", 0, NULL, &g) == 0) {
        for(i = 0; i < g.gl_pathc && i < 8; i++) {
            ui_str_append(buf, sizeof(buf), &used, "  %s\n", g.gl_pathv[i]);
            n++;
        }
        globfree(&g);
    }
    if(n == 0) {
        ui_str_append(buf, sizeof(buf), &used, "  未找到 /dev/ttyUSB*（模块可能未插入或驱动未加载）\n");
    }
    ui_str_append(buf, sizeof(buf), &used, "使用串口: %s", s_dev);
    lv_label_set_text(s_status, buf);
}

static void modem_test_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    if(!hw_path_exists(s_dev)) {
        ui_set_state(s_result, 0, "未找到 4G 模块串口 %s", s_dev);
        return;
    }
    if(ui_task_start(&s_async, modem_task, s_dev) != 0) {
        ui_set_state(s_result, 0, "上一个任务仍在执行，请稍候");
    }
}

static void modem_refresh_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    modem_refresh_status();
    if(hw_path_exists(s_dev)) ui_set_state(s_result, 1, "已刷新，%s 存在", s_dev);
    else ui_set_state(s_result, 0, "未找到 %s", s_dev);
}

void page_4g_create(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(!s_async_inited) {
        ui_result_init(&s_async);
        s_async_inited = 1;
    }

    ui_page_title(parent, "4G 模块");
    s_status = ui_page_status(parent, "正在枚举 /dev/ttyUSB* ...");

    if(hw_path_exists(MODEM_DEV_DEFAULT)) {
        snprintf(s_dev, sizeof(s_dev), "%s", MODEM_DEV_DEFAULT);
    }
    else if(hw_first_glob("/dev", "ttyUSB*", s_dev, sizeof(s_dev)) != HW_OK) {
        snprintf(s_dev, sizeof(s_dev), "%s", MODEM_DEV_DEFAULT);
    }

    row = ui_page_button_row(parent);
    ui_page_button(row, "发送 AT 指令", modem_test_cb, NULL);
    ui_page_button(row, "刷新设备", modem_refresh_cb, NULL);

    s_result = ui_page_result(parent,
                              "依次发送 AT / AT+CPIN? / AT+CSQ / AT+COPS? 并显示响应。");
    ui_bind_result(s_result, &s_async, 300);

    modem_refresh_status();
}
