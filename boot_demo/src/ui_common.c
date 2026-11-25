/**
 * @file ui_common.c
 * 主界面框架、页面公共控件与异步任务支持。
 */
#include "ui_common.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*==========================================================================
 * label 工具
 *========================================================================*/
void ui_label_set_text_fmt(lv_obj_t *label, const char *fmt, ...)
{
    char buf[2048];
    va_list ap;

    if(label == NULL || fmt == NULL) return;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    lv_label_set_text(label, buf);
}

void ui_set_state(lv_obj_t *label, int ok, const char *fmt, ...)
{
    char body[1900];
    char buf[2048];
    va_list ap;

    if(label == NULL || fmt == NULL) return;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    snprintf(buf, sizeof(buf), "%s: %s", ok ? "成功" : "失败", body);
    lv_obj_set_style_text_color(label, lv_color_hex(ok ? UI_COLOR_OK : UI_COLOR_ERR), 0);
    lv_label_set_text(label, buf);
}

/*==========================================================================
 * 页面公共控件
 *========================================================================*/
lv_obj_t *ui_page_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label;

    if(parent == NULL) return NULL;
    label = lv_label_create(parent);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_ACCENT), 0);
    return label;
}

lv_obj_t *ui_page_status(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label;

    if(parent == NULL) return NULL;
    label = lv_label_create(parent);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    return label;
}

lv_obj_t *ui_page_button_row(lv_obj_t *parent)
{
    lv_obj_t *row;

    if(parent == NULL) return NULL;
    row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(row, 8, 0);
    lv_obj_set_style_pad_column(row, 8, 0);
    return row;
}

lv_obj_t *ui_page_button(lv_obj_t *row, const char *text, lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn;
    lv_obj_t *label;

    if(row == NULL) return NULL;
    btn = lv_button_create(row);
    lv_obj_set_height(btn, 36);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_pad_hor(btn, 14, 0);
    lv_obj_set_style_pad_ver(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x24313f), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_ACCENT), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    label = lv_label_create(btn);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_center(label);

    if(cb != NULL) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    return btn;
}

lv_obj_t *ui_page_result(lv_obj_t *parent, const char *text)
{
    lv_obj_t *box;
    lv_obj_t *label;

    if(parent == NULL) return NULL;
    box = lv_obj_create(parent);
    lv_obj_set_width(box, lv_pct(100));
    lv_obj_set_flex_grow(box, 1);
    lv_obj_set_style_bg_color(box, lv_color_hex(UI_COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, 6, 0);
    lv_obj_set_style_pad_all(box, 8, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_AUTO);

    label = lv_label_create(box);
    lv_label_set_text(label, (text != NULL) ? text : "");
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_TEXT), 0);
    return label;
}

/*==========================================================================
 * 定时器随对象销毁
 *========================================================================*/
static void ui_timer_delete_event_cb(lv_event_t *e)
{
    lv_timer_t *t = (lv_timer_t *)lv_event_get_user_data(e);

    if(t != NULL) lv_timer_delete(t);
}

lv_timer_t *ui_timer_attach(lv_obj_t *obj, uint32_t period_ms, lv_timer_cb_t cb, void *user_data)
{
    lv_timer_t *t;

    if(obj == NULL || cb == NULL) return NULL;
    t = lv_timer_create(cb, period_ms, user_data);
    if(t == NULL) return NULL;
    lv_obj_add_event_cb(obj, ui_timer_delete_event_cb, LV_EVENT_DELETE, t);
    return t;
}

/*==========================================================================
 * 线程安全结果容器
 *========================================================================*/
void ui_result_init(ui_result_t *r)
{
    if(r == NULL) return;
    pthread_mutex_init(&r->lock, NULL);
    r->text[0] = '\0';
    r->seq = 1;
    r->busy = 0;
}

void ui_result_begin(ui_result_t *r)
{
    if(r == NULL) return;
    pthread_mutex_lock(&r->lock);
    snprintf(r->text, sizeof(r->text), "执行中，请稍候...");
    r->busy = 1;
    r->seq++;
    pthread_mutex_unlock(&r->lock);
}

void ui_result_write(ui_result_t *r, const char *fmt, ...)
{
    va_list ap;

    if(r == NULL || fmt == NULL) return;
    pthread_mutex_lock(&r->lock);
    va_start(ap, fmt);
    vsnprintf(r->text, sizeof(r->text), fmt, ap);
    va_end(ap);
    r->seq++;
    pthread_mutex_unlock(&r->lock);
}

unsigned ui_result_snapshot(ui_result_t *r, char *out, size_t len, int *busy)
{
    unsigned seq;

    if(r == NULL || out == NULL || len == 0) return 0;
    pthread_mutex_lock(&r->lock);
    strncpy(out, r->text, len - 1);
    out[len - 1] = '\0';
    seq = r->seq;
    if(busy != NULL) *busy = r->busy;
    pthread_mutex_unlock(&r->lock);
    return seq;
}

/*==========================================================================
 * 异步任务
 *========================================================================*/
typedef struct {
    ui_result_t *res;
    ui_task_fn fn;
    void *arg;
} ui_task_ctx_t;

static void *ui_task_thread(void *p)
{
    ui_task_ctx_t *ctx = (ui_task_ctx_t *)p;
    char out[1600];

    out[0] = '\0';
    ctx->fn(out, sizeof(out), ctx->arg);
    if(out[0] == '\0') snprintf(out, sizeof(out), "完成（无输出）");
    ui_result_write(ctx->res, "%s", out);

    pthread_mutex_lock(&ctx->res->lock);
    ctx->res->busy = 0;
    pthread_mutex_unlock(&ctx->res->lock);

    free(ctx);
    return NULL;
}

int ui_task_start(ui_result_t *r, ui_task_fn fn, void *arg)
{
    pthread_t tid;
    ui_task_ctx_t *ctx;
    int busy;

    if(r == NULL || fn == NULL) return -1;

    pthread_mutex_lock(&r->lock);
    busy = r->busy;
    pthread_mutex_unlock(&r->lock);
    if(busy) return -1;

    ui_result_begin(r);

    ctx = (ui_task_ctx_t *)malloc(sizeof(*ctx));
    if(ctx == NULL) {
        ui_result_write(r, "内存不足，任务未启动");
        pthread_mutex_lock(&r->lock);
        r->busy = 0;
        pthread_mutex_unlock(&r->lock);
        return -1;
    }
    ctx->res = r;
    ctx->fn = fn;
    ctx->arg = arg;

    if(pthread_create(&tid, NULL, ui_task_thread, ctx) != 0) {
        free(ctx);
        ui_result_write(r, "线程创建失败");
        pthread_mutex_lock(&r->lock);
        r->busy = 0;
        pthread_mutex_unlock(&r->lock);
        return -1;
    }
    pthread_detach(tid);
    return 0;
}

/*==========================================================================
 * 结果轮询
 *========================================================================*/
typedef struct {
    lv_obj_t *label;
    ui_result_t *res;
    unsigned last_seq;
} ui_bind_ctx_t;

static void ui_free_delete_cb(lv_event_t *e)
{
    free(lv_event_get_user_data(e));
}

static void ui_bind_timer_cb(lv_timer_t *t)
{
    ui_bind_ctx_t *ctx = (ui_bind_ctx_t *)lv_timer_get_user_data(t);
    char buf[2048];
    unsigned seq;
    int busy = 0;

    if(ctx == NULL || ctx->label == NULL) return;
    seq = ui_result_snapshot(ctx->res, buf, sizeof(buf), &busy);
    if(seq != ctx->last_seq) {
        ctx->last_seq = seq;
        lv_label_set_text(ctx->label, buf);
    }
    LV_UNUSED(busy);
}

void ui_bind_result(lv_obj_t *label, ui_result_t *r, uint32_t period_ms)
{
    ui_bind_ctx_t *ctx;

    if(label == NULL || r == NULL) return;
    ctx = (ui_bind_ctx_t *)malloc(sizeof(*ctx));
    if(ctx == NULL) return;
    ctx->label = label;
    ctx->res = r;
    ctx->last_seq = 0;

    /* ctx 由 label 的删除回调释放；timer 若创建失败不会轮询，但不会泄漏 */
    lv_obj_add_event_cb(label, ui_free_delete_cb, LV_EVENT_DELETE, ctx);
    (void)ui_timer_attach(label, period_ms, ui_bind_timer_cb, ctx);
}

void ui_format_size(char *buf, size_t len, unsigned long long bytes)
{
    if(buf == NULL || len == 0) return;
    if(bytes >= (1024ULL * 1024ULL * 1024ULL)) {
        snprintf(buf, len, "%.2f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
    else if(bytes >= (1024ULL * 1024ULL)) {
        snprintf(buf, len, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    }
    else {
        snprintf(buf, len, "%.0f KB", (double)bytes / 1024.0);
    }
}

void ui_str_append(char *buf, size_t len, size_t *used, const char *fmt, ...)
{
    va_list ap;
    int n;
    size_t avail;

    if(buf == NULL || used == NULL || fmt == NULL) return;
    if(*used >= len) return;

    avail = len - *used;
    va_start(ap, fmt);
    n = vsnprintf(buf + *used, avail, fmt, ap);
    va_end(ap);

    if(n > 0) {
        *used += ((size_t)n < avail) ? (size_t)n : (avail - 1);
    }
}

/*==========================================================================
 * 主界面
 *========================================================================*/
#define UI_NAV_W     196
#define UI_HEADER_H  52
#define UI_MAX_PAGES 32

static lv_obj_t *s_content;
static lv_obj_t *s_nav_buttons[UI_MAX_PAGES];
static lv_obj_t *s_clock_label;
static int s_page_count;
static int s_cur_page = -1;

static void ui_clock_refresh(void)
{
    time_t now;
    struct tm tmv;
    char buf[64];

    if(s_clock_label == NULL) return;
    now = time(NULL);
    if(localtime_r(&now, &tmv) == NULL) return;
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d  %02d:%02d:%02d",
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    lv_label_set_text(s_clock_label, buf);
}

static void ui_clock_timer_cb(lv_timer_t *t)
{
    LV_UNUSED(t);
    ui_clock_refresh();
}

static void ui_nav_highlight(int index)
{
    int i;

    for(i = 0; i < s_page_count && i < UI_MAX_PAGES; i++) {
        lv_obj_t *btn = s_nav_buttons[i];
        if(btn == NULL) continue;
        lv_obj_set_style_bg_color(btn,
                                  lv_color_hex(i == index ? UI_COLOR_ACCENT : UI_COLOR_PANEL), 0);
    }
}

static void ui_switch_page(int index)
{
    const page_desc_t *pages;
    int count;

    if(index < 0 || s_content == NULL) return;
    pages = pages_registry(&count);
    if(pages == NULL || index >= count) return;
    if(index == s_cur_page) return;

    s_cur_page = index;
    lv_obj_clean(s_content);          /* 删除旧页面全部子对象（含其定时器） */
    ui_nav_highlight(index);
    pages[index].create_cb(s_content);
}

static void ui_nav_click_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e) - 1;

    ui_switch_page(index);
}

void ui_main_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_display_t *disp = lv_display_get_default();
    int32_t w = (disp != NULL) ? lv_display_get_horizontal_resolution(disp) : 1024;
    int32_t h = (disp != NULL) ? lv_display_get_vertical_resolution(disp) : 600;
    lv_obj_t *header;
    lv_obj_t *title;
    lv_obj_t *nav;
    lv_obj_t *content;
    const page_desc_t *pages;
    int count = 0;
    int i;

    if(scr == NULL) return;

    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(UI_COLOR_TEXT), 0);

    /* 顶部标题栏 */
    header = lv_obj_create(scr);
    lv_obj_remove_style_all(header);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, w, UI_HEADER_H);
    lv_obj_set_style_bg_color(header, lv_color_hex(UI_COLOR_HEADER), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    title = lv_label_create(header);
    lv_label_set_text(title, "rk3568 硬件测试");
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 16, 0);

    s_clock_label = lv_label_create(header);
    lv_label_set_text(s_clock_label, "--");
    lv_obj_set_style_text_color(s_clock_label, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_align(s_clock_label, LV_ALIGN_RIGHT_MID, -16, 0);

    /* 左侧导航 */
    nav = lv_obj_create(scr);
    lv_obj_remove_style_all(nav);
    lv_obj_set_pos(nav, 0, UI_HEADER_H);
    lv_obj_set_size(nav, UI_NAV_W, h - UI_HEADER_H);
    lv_obj_set_style_bg_color(nav, lv_color_hex(UI_COLOR_HEADER), 0);
    lv_obj_set_style_bg_opa(nav, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(nav, 6, 0);
    lv_obj_set_style_pad_row(nav, 4, 0);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(nav, LV_SCROLLBAR_MODE_AUTO);

    /* 右侧内容区 */
    content = lv_obj_create(scr);
    lv_obj_remove_style_all(content);
    lv_obj_set_pos(content, UI_NAV_W, UI_HEADER_H);
    lv_obj_set_size(content, w - UI_NAV_W, h - UI_HEADER_H);
    lv_obj_set_style_bg_color(content, lv_color_hex(UI_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(content, 12, 0);
    lv_obj_set_style_pad_row(content, 8, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    s_content = content;

    /* 导航按钮 */
    pages = pages_registry(&count);
    s_page_count = (count > UI_MAX_PAGES) ? UI_MAX_PAGES : count;
    for(i = 0; i < s_page_count; i++) {
        lv_obj_t *btn = lv_button_create(nav);
        lv_obj_t *lbl;
        lv_obj_set_size(btn, lv_pct(100), 34);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_PANEL), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 2, 0);

        lbl = lv_label_create(btn);
        lv_label_set_text(lbl, pages[i].name);
        lv_obj_set_style_text_color(lbl, lv_color_hex(UI_COLOR_TEXT), 0);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, ui_nav_click_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)(i + 1));
        s_nav_buttons[i] = btn;
    }

    ui_clock_refresh();
    lv_timer_create(ui_clock_timer_cb, 1000, NULL);

    ui_switch_page(0);
}
