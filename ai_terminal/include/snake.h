#ifndef AI_TERMINAL_SNAKE_H
#define AI_TERMINAL_SNAKE_H

#include "lvgl.h"

#define SNAKE_COLS 20
#define SNAKE_ROWS 15

#define SNAKE_DIR_UP    0
#define SNAKE_DIR_RIGHT 1
#define SNAKE_DIR_DOWN  2
#define SNAKE_DIR_LEFT  3

/* 在 parent 里创建 20x15 的棋盘（每格 cell_px 像素），返回棋盘对象。
 * 同时创建 200ms 定时器并重置游戏。 */
lv_obj_t *snake_create(lv_obj_t *parent, int32_t cell_px);

/* 开始 / 重置 / 设置方向 / 查询状态 */
void snake_start(void);
void snake_reset(void);
void snake_set_direction(int dir);

int snake_score(void);
int snake_is_running(void);
int snake_is_over(void);

/* 分数变化回调（在主线程被调用）。 */
void snake_set_score_cb(void (*cb)(int score));

#endif /* AI_TERMINAL_SNAKE_H */
