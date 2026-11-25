/**
 * @file hw.h
 * 通用硬件访问帮助函数（sysfs / procfs / ioctl / popen）。
 * 所有函数返回 int：0 成功，负数为失败码，配合 hw_strerror() 得到中文原因。
 */
#ifndef BOOT_DEMO_HW_H
#define BOOT_DEMO_HW_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 统一返回码 */
#define HW_OK           0
#define HW_ERR_ARG     (-1)   /* 参数错误 */
#define HW_ERR_IO      (-2)   /* 打开或读写失败 */
#define HW_ERR_NOTFOUND (-3)  /* 设备/文件不存在 */
#define HW_ERR_CMD     (-4)   /* 外部命令执行失败 */
#define HW_ERR_TIMEOUT (-5)   /* 超时 */
#define HW_ERR_SYS     (-6)   /* 系统调用失败 */

/* 读取文本文件，去掉尾部空白/换行；buf 保证以 '\0' 结尾 */
int hw_read_file(const char *path, char *buf, size_t len);

/* 写入文本（sysfs 属性），不存在时返回 HW_ERR_NOTFOUND */
int hw_write_file(const char *path, const char *text);

/* popen 执行命令并收集输出（去掉尾部换行）；命令非 0 退出返回 HW_ERR_CMD */
int hw_run_cmd(const char *cmd, char *out, size_t outlen);

/* 通过 /sys/class/gpio 导出 GPIO 并设置方向（"in"/"out"），内部带重试 */
int hw_gpio_export(int num, const char *dir);

/* 写 GPIO 电平 0/1，内部带重试 */
int hw_gpio_set(int num, int val);

/* 读 GPIO 电平到 *val，内部带重试 */
int hw_gpio_get(int num, int *val);

/* 路径存在返回 1，否则 0 */
int hw_path_exists(const char *path);

/* 用 ioctl(SIOCGIFADDR) 取 IPv4 地址，失败时 buf 置空 */
int hw_iface_ip(const char *ifname, char *buf, size_t len);

/* 在 dir 下按 pattern 做 glob，取第一个匹配项写入 out */
int hw_first_glob(const char *dir, const char *pattern, char *out, size_t len);

/* 在 /sys/bus/iio/devices/iio:device* 中查找名为 file 的属性，命中的完整路径写入 out */
int hw_find_iio(const char *file, char *out, size_t len);

/* 读取 sysfs 中的十进制整数 */
int hw_read_int(const char *path, long *val);

/* 去除首尾空白字符（原地修改） */
void hw_trim(char *s);

/* 返回码对应的中文原因文本 */
const char *hw_strerror(int code);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_DEMO_HW_H */
