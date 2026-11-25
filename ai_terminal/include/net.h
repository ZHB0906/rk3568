#ifndef AI_TERMINAL_NET_H
#define AI_TERMINAL_NET_H

/* 流式聊天接口。所有回调都在 LVGL 主线程（lv_timer_handler）中被调用。 */

typedef void (*net_delta_cb)(const char *delta, void *user);
typedef void (*net_done_cb)(void *user);
typedef void (*net_error_cb)(const char *message, void *user);

/* 发起一次流式请求：
 *   POST /api/chat
 *   {"model":...,"messages":[{"role":"user","content":...}],"stream":true}
 * 成功把任务抛到后台线程返回 0；已有任务在跑时返回 -1。 */
int net_chat_start(const char *prompt,
                   net_delta_cb on_delta,
                   net_done_cb on_done,
                   net_error_cb on_error,
                   void *user);

/* 请求取消当前任务（异步生效）。 */
void net_chat_cancel(void);

/* 是否有任务在跑。 */
int net_is_busy(void);

#endif /* AI_TERMINAL_NET_H */
