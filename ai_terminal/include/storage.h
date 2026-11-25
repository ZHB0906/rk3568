#ifndef AI_TERMINAL_STORAGE_H
#define AI_TERMINAL_STORAGE_H

#include "app.h"

/* 初始化数据目录：环境变量 AI_DATA_DIR，默认 /root/ai/data，逐级创建。
 * 成功返回 0。 */
int storage_init(void);

/* 读取 settings.json 到 cfg（缺失字段保持原值，未知字段忽略）。
 * 成功返回 0；文件不存在返回 -1。 */
int config_load(app_config_t *cfg);

/* 写 settings.json。成功返回 0。 */
int config_save(const app_config_t *cfg);

/* 追加一条历史记录到 history.jsonl。成功返回 0。 */
int history_add(const char *user, const char *assistant);

/* 读取最近 n 条历史，每条格式化为 "我: ...\nAI: ..." 的堆字符串。
 * *out 为 malloc 的指针数组（需 history_free 释放），*count 为条数。
 * 无文件 / 空文件时返回 0 且 *count = 0。 */
int history_load_recent(int n, char ***out, int *count);

/* 释放 history_load_recent 返回的数组。 */
void history_free(char **items, int count);

/* 清空历史文件。成功返回 0。 */
int history_clear(void);

#endif /* AI_TERMINAL_STORAGE_H */
