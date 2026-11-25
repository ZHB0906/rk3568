/**
 * @file hw.c
 * 通用硬件访问帮助函数实现。
 */
#include "hw.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <glob.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void hw_trim(char *s)
{
    size_t len;

    if(s == NULL) return;
    len = strlen(s);
    while(len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    while(*s != '\0' && isspace((unsigned char)*s)) {
        memmove(s, s + 1, strlen(s));
    }
}

const char *hw_strerror(int code)
{
    switch(code) {
        case HW_OK:            return "成功";
        case HW_ERR_ARG:       return "参数错误";
        case HW_ERR_IO:        return "读写失败";
        case HW_ERR_NOTFOUND:  return "未找到设备或节点";
        case HW_ERR_CMD:       return "外部命令执行失败";
        case HW_ERR_TIMEOUT:   return "操作超时";
        case HW_ERR_SYS:       return "系统调用失败";
        default:               return "未知错误";
    }
}

int hw_path_exists(const char *path)
{
    struct stat st;

    if(path == NULL) return 0;
    return stat(path, &st) == 0 ? 1 : 0;
}

int hw_read_file(const char *path, char *buf, size_t len)
{
    FILE *f;
    size_t n;

    if(path == NULL || buf == NULL || len == 0) return HW_ERR_ARG;
    buf[0] = '\0';
    f = fopen(path, "rb");
    if(f == NULL) {
        return (errno == ENOENT || errno == ENOTDIR) ? HW_ERR_NOTFOUND : HW_ERR_IO;
    }
    n = fread(buf, 1, len - 1, f);
    buf[n] = '\0';
    fclose(f);
    hw_trim(buf);
    return HW_OK;
}

int hw_write_file(const char *path, const char *text)
{
    FILE *f;
    int rc;

    if(path == NULL || text == NULL) return HW_ERR_ARG;
    f = fopen(path, "w");
    if(f == NULL) {
        return (errno == ENOENT || errno == ENOTDIR) ? HW_ERR_NOTFOUND : HW_ERR_IO;
    }
    if(fputs(text, f) == EOF) {
        fclose(f);
        return HW_ERR_IO;
    }
    if(fflush(f) != 0) {
        fclose(f);
        return HW_ERR_IO;
    }
    rc = fclose(f);
    return rc == 0 ? HW_OK : HW_ERR_IO;
}

int hw_run_cmd(const char *cmd, char *out, size_t outlen)
{
    FILE *p;
    size_t n = 0;
    int c;
    int status;

    if(cmd == NULL) return HW_ERR_ARG;
    if(out != NULL && outlen > 0) out[0] = '\0';

    p = popen(cmd, "r");
    if(p == NULL) return HW_ERR_CMD;

    while((c = fgetc(p)) != EOF) {
        if(out != NULL && n + 1 < outlen) out[n++] = (char)c;
    }
    if(out != NULL && outlen > 0) {
        out[n] = '\0';
        hw_trim(out);
    }

    status = pclose(p);
    if(status == -1) return HW_ERR_CMD;
    if(WIFEXITED(status) && WEXITSTATUS(status) != 0) return HW_ERR_CMD;
    return HW_OK;
}

static void hw_sleep_ms(int ms)
{
    usleep((useconds_t)ms * 1000U);
}

int hw_gpio_export(int num, const char *dir)
{
    char path[128];
    char val[16];
    int i;

    if(num < 0) return HW_ERR_ARG;
    if(!hw_path_exists("/sys/class/gpio/export")) return HW_ERR_NOTFOUND;

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", num);
    if(!hw_path_exists(path)) {
        snprintf(val, sizeof(val), "%d", num);
        if(hw_write_file("/sys/class/gpio/export", val) != HW_OK) return HW_ERR_IO;
        for(i = 0; i < 10 && !hw_path_exists(path); i++) hw_sleep_ms(20);
        if(!hw_path_exists(path)) return HW_ERR_TIMEOUT;
    }

    if(dir != NULL && dir[0] != '\0') {
        snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", num);
        if(hw_write_file(path, dir) != HW_OK) return HW_ERR_IO;
    }
    return HW_OK;
}

int hw_gpio_set(int num, int val)
{
    char path[128];
    char v[4];
    int i;

    if(num < 0) return HW_ERR_ARG;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", num);
    v[0] = val ? '1' : '0';
    v[1] = '\0';

    for(i = 0; i < 5; i++) {
        if(hw_write_file(path, v) == HW_OK) return HW_OK;
        hw_sleep_ms(10);
    }
    return hw_path_exists(path) ? HW_ERR_IO : HW_ERR_NOTFOUND;
}

int hw_gpio_get(int num, int *val)
{
    char path[128];
    char buf[8];
    int i;
    int rc = HW_ERR_IO;

    if(num < 0 || val == NULL) return HW_ERR_ARG;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", num);
    for(i = 0; i < 5; i++) {
        rc = hw_read_file(path, buf, sizeof(buf));
        if(rc == HW_OK) {
            *val = (buf[0] == '1') ? 1 : 0;
            return HW_OK;
        }
        if(rc == HW_ERR_NOTFOUND) return HW_ERR_NOTFOUND;
        hw_sleep_ms(10);
    }
    return rc;
}

int hw_iface_ip(const char *ifname, char *buf, size_t len)
{
    int fd;
    struct ifreq ifr;
    struct sockaddr_in *sin;

    if(ifname == NULL || buf == NULL || len == 0) return HW_ERR_ARG;
    buf[0] = '\0';

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) return HW_ERR_SYS;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if(ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
        close(fd);
        return HW_ERR_IO;
    }
    sin = (struct sockaddr_in *)&ifr.ifr_addr;
    if(inet_ntop(AF_INET, &sin->sin_addr, buf, (socklen_t)len) == NULL) {
        buf[0] = '\0';
        close(fd);
        return HW_ERR_SYS;
    }
    close(fd);
    return HW_OK;
}

int hw_first_glob(const char *dir, const char *pattern, char *out, size_t len)
{
    char pat[512];
    glob_t g;
    int ret = HW_ERR_NOTFOUND;

    if(dir == NULL || pattern == NULL || out == NULL || len == 0) return HW_ERR_ARG;
    out[0] = '\0';

    snprintf(pat, sizeof(pat), "%s/%s", dir, pattern);
    if(glob(pat, 0, NULL, &g) != 0) return HW_ERR_NOTFOUND;

    if(g.gl_pathc > 0) {
        strncpy(out, g.gl_pathv[0], len - 1);
        out[len - 1] = '\0';
        ret = HW_OK;
    }
    globfree(&g);
    return ret;
}

int hw_find_iio(const char *file, char *out, size_t len)
{
    glob_t g;
    size_t i;
    int ret = HW_ERR_NOTFOUND;

    if(file == NULL || out == NULL || len == 0) return HW_ERR_ARG;
    out[0] = '\0';

    if(glob("/sys/bus/iio/devices/iio:device*", 0, NULL, &g) != 0) return HW_ERR_NOTFOUND;

    for(i = 0; i < g.gl_pathc; i++) {
        snprintf(out, len, "%s/%s", g.gl_pathv[i], file);
        if(hw_path_exists(out)) {
            ret = HW_OK;
            break;
        }
    }
    globfree(&g);

    if(ret != HW_OK) out[0] = '\0';
    return ret;
}

int hw_read_int(const char *path, long *val)
{
    char buf[64];
    char *end = NULL;
    long v;
    int rc;

    rc = hw_read_file(path, buf, sizeof(buf));
    if(rc != HW_OK) return rc;

    errno = 0;
    v = strtol(buf, &end, 10);
    if(end == buf || errno != 0) return HW_ERR_IO;
    if(val != NULL) *val = v;
    return HW_OK;
}
