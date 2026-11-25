#ifndef AI_TERMINAL_JSON_UTIL_H
#define AI_TERMINAL_JSON_UTIL_H

#include <stddef.h>

/*
 * 极简 JSON 工具，只做本程序需要的事情：
 *  - 生成 JSON 字符串字面量内容（转义）
 *  - 从响应文本里按 key 提取字符串 / 整数 / 布尔
 * 不依赖任何第三方库。
 */

/* 把 src 转义为可放进双引号内的内容写入 dst（不含首尾引号）。
 * 成功返回写入字节数（不含结尾 '\0'），空间不足返回 -1。 */
int json_escape(const char *src, char *dst, size_t dst_size);

/* 同 json_escape，但返回 malloc 出来的字符串，调用者 free。失败返回 NULL。 */
char *json_escape_dup(const char *src);

/* 提取 "key":"字符串" 的值，处理 \" \\ \/ \b \f \n \r \t \uXXXX（含代理对）。
 * 成功返回 0 且 out 为 UTF-8 字符串；未找到 / 格式错误 / 空间不足返回 -1。 */
int json_get_string(const char *json, const char *key, char *out, size_t out_size);

/* 提取 "key":123 的数值，成功返回 0。 */
int json_get_int(const char *json, const char *key, long long *out);

/* 提取 "key":true / false，成功返回 0，*out 为 1 或 0。 */
int json_get_bool(const char *json, const char *key, int *out);

#endif /* AI_TERMINAL_JSON_UTIL_H */
