#include "ui_theme.h"
#include "ui_login.h"

#include <stdlib.h>

typedef struct {
    void (*cb)(void);
} back_ctx_t;

static void header_delete_cb(lv_event_t *e)
{
    back_ctx_t *ctx = (back_ctx_t *)lv_event_get_user_data(e);

    if (ctx != NULL) {
        free(ctx);
    }
}

static void header_back_clicked(lv_event_t *e)
{
    back_ctx_t *ctx = (back_ctx_t *)lv_event_get_user_data(e);

    if (ctx != NULL && ctx->cb != NULL) {
        ctx->cb();
    }
}

void ui_apply_screen_style(lv_obj_t *scr)
{
    if (scr == NULL) {
        return;
    }
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_C_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, lv_color_hex(UI_C_TEXT), 0);
    lv_obj_set_style_text_font(scr, UI_FONT, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

void apply_theme(void)
{
    ui_apply_screen_style(lv_screen_active());
}

lv_obj_t *ui_create_header(lv_obj_t *parent, const char *title, void (*back_cb)(void))
{
    lv_obj_t *hdr;
    lv_obj_t *title_label;

    hdr = lv_obj_create(parent);
    lv_obj_set_width(hdr, lv_pct(100));
    lv_obj_set_height(hdr, UI_HEADER_H);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(UI_C_PRIMARY), 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_set_style_pad_left(hdr, 16, 0);
    lv_obj_set_style_pad_right(hdr, 16, 0);
    lv_obj_set_style_pad_top(hdr, 0, 0);
    lv_obj_set_style_pad_bottom(hdr, 0, 0);
    lv_obj_set_style_pad_column(hdr, 12, 0);
    lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    if (back_cb != NULL) {
        back_ctx_t *ctx = (back_ctx_t *)malloc(sizeof(*ctx));
        lv_obj_t *btn = lv_button_create(hdr);

        lv_obj_set_size(btn, 96, 42);
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_C_PRIMARY_DK), 0);
        lv_obj_set_style_radius(btn, 8, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, "返回");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(lbl, UI_FONT, 0);
        lv_obj_center(lbl);

        if (ctx != NULL) {
            ctx->cb = back_cb;
            lv_obj_add_event_cb(btn, header_back_clicked, LV_EVENT_CLICKED, ctx);
            lv_obj_add_event_cb(hdr, header_delete_cb, LV_EVENT_DELETE, ctx);
        }
    }

    title_label = lv_label_create(hdr);
    lv_label_set_text(title_label, (title != NULL) ? title : "");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, UI_FONT, 0);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_flex_grow(title_label, 1);

    return hdr;
}

void ui_nav_to(lv_obj_t *(*create_fn)(void))
{
    lv_obj_t *scr;

    if (create_fn == NULL) {
        return;
    }
    scr = create_fn();
    if (scr == NULL) {
        return;
    }
    ui_apply_screen_style(scr);
    lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, true);
}

void ui_app_start(void)
{
    apply_theme();
    ui_nav_to(ui_login_create);
}

lv_obj_t *ui_make_button(lv_obj_t *parent, const char *text, int32_t w, int32_t h)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_t *lbl;

    if (w > 0) {
        lv_obj_set_width(btn, w);
    }
    if (h > 0) {
        lv_obj_set_height(btn, h);
    }
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_C_PRIMARY), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 6, 0);

    lbl = lv_label_create(btn);
    lv_label_set_text(lbl, (text != NULL) ? text : "");
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lbl, UI_FONT, 0);
    lv_obj_center(lbl);

    return btn;
}

lv_obj_t *ui_make_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);

    lv_label_set_text(lbl, (text != NULL) ? text : "");
    lv_obj_set_style_text_font(lbl, UI_FONT, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(UI_C_TEXT), 0);
    return lbl;
}

lv_obj_t *ui_make_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);

    lv_obj_set_style_bg_color(card, lv_color_hex(UI_C_CARD), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_shadow_width(card, 10, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
    return card;
}
