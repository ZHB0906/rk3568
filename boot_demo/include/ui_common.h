/**
 * @file ui_common.h
 * 主界面框架、页面公共控件、线程安全异步任务与结果轮询。
 */
#ifndef BOOT_DEMO_UI_COMMON_H
#define BOOT_DEMO_UI_COMMON_H

#include "lvgl.h"
#include "pages.h"

#include <pthread.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 配色 */
#define UI_COLOR_BG       0x0b1219
#define UI_COLOR_HEADER   0x16212c
#define UI_COLOR_PANEL    0x1b2733
#define UI_COLOR_ACCENT   0x1e88e5
#define UI_COLOR_TEXT     0xe6edf3
#define UI_COLOR_TEXT_DIM 0x9fb0c0
#define UI_COLOR_OK       0x4caf50
#define UI_COLOR_ERR      0xef5350

/* 创建主界面（标题 + 时钟 + 导航 + 内容区） */
void ui_main_create(void);

/* 页面内公共控件 */
lv_obj_t *ui_page_title(lv_obj_t *parent, const char *text);
lv_obj_t *ui_page_status(lv_obj_t *parent, const char *text);
lv_obj_t *ui_page_button_row(lv_obj_t *parent);
lv_obj_t *ui_page_button(lv_obj_t *row, const char *text, lv_event_cb_t cb, void *user_data);
lv_obj_t *ui_page_result(lv_obj_t *parent, const char *text);

/* 安全地设置 label 文本 */
void ui_label_set_text_fmt(lv_obj_t *label, const char *fmt, ...);
/* 带“成功/失败”前缀并着色 */
void ui_set_state(lv_obj_t *label, int ok, const char *fmt, ...);

/* 创建 lv_timer，并在 obj 被删除时自动删除该 timer */
lv_timer_t *ui_timer_attach(lv_obj_t *obj, uint32_t period_ms, lv_timer_cb_t cb, void *user_data);

/*==========================================================================
 * 线程安全结果容器：工作线程写，UI 侧（定时器）读
 *========================================================================*/
typedef struct {
    pthread_mutex_t lock;
    char text[2048];
    unsigned seq;
    int busy;
} ui_result_t;

void ui_result_init(ui_result_t *r);
void ui_result_begin(ui_result_t *r);
void ui_result_write(ui_result_t *r, const char *fmt, ...);
/* 读取快照，返回本次 seq（与上次不同说明有新结果） */
unsigned ui_result_snapshot(ui_result_t *r, char *out, size_t len, int *busy);

/* 工作线程函数：把结果写入 out，不要访问任何 LVGL 对象 */
typedef void (*ui_task_fn)(char *out, size_t outlen, void *arg);

/* 启动 detach 线程执行 fn；若已有任务在跑则返回 -1 */
int ui_task_start(ui_result_t *r, ui_task_fn fn, void *arg);

/* 把 label 绑定到结果容器：每 period_ms 轮询，seq 变化时刷新文本（自动随 label 销毁） */
void ui_bind_result(lv_obj_t *label, ui_result_t *r, uint32_t period_ms);

/* 字节数格式化为 "x.x MB" */
void ui_format_size(char *buf, size_t len, unsigned long long bytes);

/* 向缓冲区追加格式化文本（自动维护 used，越界安全） */
void ui_str_append(char *buf, size_t len, size_t *used, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_DEMO_UI_COMMON_H */
