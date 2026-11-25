#ifndef AI_TERMINAL_PINYIN_H
#define AI_TERMINAL_PINYIN_H

/* 内置静态拼音词表（约 100 个常用词），不依赖任何第三方库。 */

/* 初始化（清空当前候选状态）。 */
void pinyin_init(void);

/* 输入拼音串，返回候选词数组（以 NULL 结尾，内部静态存储，不要释放）。
 * 无匹配时返回一个首元素为 NULL 的数组。 */
const char **pinyin_input_commit(const char *pinyin);

/* 在当前候选里前后移动，返回新的候选下标；无候选时返回 -1。 */
int pinyin_prev(void);
int pinyin_next(void);

/* 当前候选数量 / 当前候选词（无候选时返回 NULL）。 */
int pinyin_candidate_count(void);
const char *pinyin_current(void);

#endif /* AI_TERMINAL_PINYIN_H */
