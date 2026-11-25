#include "snake.h"

#include <stdlib.h>
#include <string.h>

#define SNAKE_COLOR_HEAD 0x16A34A
#define SNAKE_COLOR_BODY 0x22C55E
#define SNAKE_COLOR_FOOD 0xEF4444
#define SNAKE_COLOR_BG   0x151A23
#define SNAKE_CELL_GAP   2

typedef struct {
    int x;
    int y;
} snake_pt_t;

typedef struct {
    lv_obj_t *board;
    lv_obj_t *cells[SNAKE_ROWS][SNAKE_COLS];
    lv_timer_t *timer;
    snake_pt_t body[SNAKE_COLS * SNAKE_ROWS];
    int len;
    int dir;
    int next_dir;
    snake_pt_t food;
    int score;
    int running;
    int over;
    int32_t cell;
    unsigned rng;
    int grid[SNAKE_ROWS][SNAKE_COLS]; /* 0 空 1 身体 2 头 3 食物 */
    void (*score_cb)(int score);
} snake_state_t;

static snake_state_t s_snake;
static lv_point_t s_press_point;

static void snake_render(void);
static void snake_spawn_food(void);

static void snake_apply_cell(int x, int y, int state)
{
    lv_obj_t *cell = s_snake.cells[y][x];

    if (cell == NULL) {
        return;
    }
    switch (state) {
    case 1:
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(cell, lv_color_hex(SNAKE_COLOR_BODY), 0);
        break;
    case 2:
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(cell, lv_color_hex(SNAKE_COLOR_HEAD), 0);
        break;
    case 3:
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(cell, lv_color_hex(SNAKE_COLOR_FOOD), 0);
        break;
    default:
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        break;
    }
}

static void snake_render(void)
{
    int next[SNAKE_ROWS][SNAKE_COLS];
    int x;
    int y;
    int i;

    for (y = 0; y < SNAKE_ROWS; y++) {
        for (x = 0; x < SNAKE_COLS; x++) {
            next[y][x] = 0;
        }
    }

    for (i = 0; i < s_snake.len; i++) {
        int bx = s_snake.body[i].x;
        int by = s_snake.body[i].y;

        if (bx < 0 || bx >= SNAKE_COLS || by < 0 || by >= SNAKE_ROWS) {
            continue;
        }
        next[by][bx] = (i == 0) ? 2 : 1;
    }
    if (s_snake.food.x >= 0 && s_snake.food.x < SNAKE_COLS &&
        s_snake.food.y >= 0 && s_snake.food.y < SNAKE_ROWS) {
        if (next[s_snake.food.y][s_snake.food.x] == 0) {
            next[s_snake.food.y][s_snake.food.x] = 3;
        }
    }

    for (y = 0; y < SNAKE_ROWS; y++) {
        for (x = 0; x < SNAKE_COLS; x++) {
            if (s_snake.grid[y][x] != next[y][x]) {
                snake_apply_cell(x, y, next[y][x]);
                s_snake.grid[y][x] = next[y][x];
            }
        }
    }
}

static void snake_spawn_food(void)
{
    int free_x[SNAKE_COLS * SNAKE_ROWS];
    int free_y[SNAKE_COLS * SNAKE_ROWS];
    int count = 0;
    int occupied;
    int x;
    int y;
    int i;
    unsigned r;

    for (y = 0; y < SNAKE_ROWS; y++) {
        for (x = 0; x < SNAKE_COLS; x++) {
            occupied = 0;
            for (i = 0; i < s_snake.len; i++) {
                if (s_snake.body[i].x == x && s_snake.body[i].y == y) {
                    occupied = 1;
                    break;
                }
            }
            if (!occupied) {
                free_x[count] = x;
                free_y[count] = y;
                count++;
            }
        }
    }

    if (count == 0) {
        s_snake.food.x = -1;
        s_snake.food.y = -1;
        return;
    }

    s_snake.rng = s_snake.rng * 1103515245u + 12345u;
    r = (s_snake.rng >> 16) % (unsigned)count;
    s_snake.food.x = free_x[r];
    s_snake.food.y = free_y[r];
}

static void snake_game_over(void)
{
    s_snake.running = 0;
    s_snake.over = 1;
    if (s_snake.score_cb != NULL) {
        s_snake.score_cb(s_snake.score);
    }
}

static void snake_tick(lv_timer_t *timer)
{
    int dx = 0;
    int dy = 0;
    int eating;
    int limit;
    int i;
    snake_pt_t head;

    (void)timer;
    if (!s_snake.running || s_snake.over) {
        return;
    }

    s_snake.dir = s_snake.next_dir;
    switch (s_snake.dir) {
    case SNAKE_DIR_UP:    dy = -1; break;
    case SNAKE_DIR_RIGHT: dx = 1;  break;
    case SNAKE_DIR_DOWN:  dy = 1;  break;
    default:              dx = -1; break;
    }

    head.x = s_snake.body[0].x + dx;
    head.y = s_snake.body[0].y + dy;

    if (head.x < 0 || head.x >= SNAKE_COLS || head.y < 0 || head.y >= SNAKE_ROWS) {
        snake_game_over();
        return;
    }

    eating = (head.x == s_snake.food.x && head.y == s_snake.food.y);
    limit = eating ? s_snake.len : s_snake.len - 1;
    for (i = 0; i < limit; i++) {
        if (s_snake.body[i].x == head.x && s_snake.body[i].y == head.y) {
            snake_game_over();
            return;
        }
    }

    if (eating) {
        if (s_snake.len < SNAKE_COLS * SNAKE_ROWS) {
            s_snake.len++;
        }
        s_snake.score += 10;
        if (s_snake.score_cb != NULL) {
            s_snake.score_cb(s_snake.score);
        }
        snake_spawn_food();
    }

    for (i = s_snake.len - 1; i > 0; i--) {
        s_snake.body[i] = s_snake.body[i - 1];
    }
    s_snake.body[0] = head;

    if (s_snake.len >= SNAKE_COLS * SNAKE_ROWS) {
        snake_game_over();
        return;
    }
    snake_render();
}

static void snake_board_pressed(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_active();

    (void)e;
    if (indev != NULL) {
        lv_indev_get_point(indev, &s_press_point);
    }
}

static void snake_board_released(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;
    int dx;
    int dy;

    (void)e;
    if (indev == NULL) {
        return;
    }
    lv_indev_get_point(indev, &p);
    dx = (int)p.x - (int)s_press_point.x;
    dy = (int)p.y - (int)s_press_point.y;
    if (abs(dx) < 24 && abs(dy) < 24) {
        return;
    }
    if (abs(dx) >= abs(dy)) {
        snake_set_direction((dx > 0) ? SNAKE_DIR_RIGHT : SNAKE_DIR_LEFT);
    } else {
        snake_set_direction((dy > 0) ? SNAKE_DIR_DOWN : SNAKE_DIR_UP);
    }
}

static void snake_board_delete(lv_event_t *e)
{
    (void)e;
    if (s_snake.timer != NULL) {
        lv_timer_delete(s_snake.timer);
        s_snake.timer = NULL;
    }
    memset(&s_snake, 0, sizeof(s_snake));
}

lv_obj_t *snake_create(lv_obj_t *parent, int32_t cell_px)
{
    int32_t board_w;
    int32_t board_h;
    int x;
    int y;

    if (cell_px < 8) {
        cell_px = 8;
    }

    memset(&s_snake, 0, sizeof(s_snake));
    s_snake.cell = cell_px;
    s_snake.rng = (unsigned)lv_tick_get() | 1u;

    board_w = SNAKE_COLS * cell_px + 4;
    board_h = SNAKE_ROWS * cell_px + 4;

    s_snake.board = lv_obj_create(parent);
    lv_obj_set_size(s_snake.board, board_w, board_h);
    lv_obj_set_style_bg_color(s_snake.board, lv_color_hex(SNAKE_COLOR_BG), 0);
    lv_obj_set_style_bg_opa(s_snake.board, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_snake.board, 2, 0);
    lv_obj_set_style_border_color(s_snake.board, lv_color_hex(0x2A3446), 0);
    lv_obj_set_style_radius(s_snake.board, 8, 0);
    lv_obj_set_style_pad_all(s_snake.board, 0, 0);
    lv_obj_remove_flag(s_snake.board, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_snake.board, LV_OBJ_FLAG_CLICKABLE);

    for (y = 0; y < SNAKE_ROWS; y++) {
        for (x = 0; x < SNAKE_COLS; x++) {
            lv_obj_t *cell = lv_obj_create(s_snake.board);

            lv_obj_set_size(cell, cell_px - SNAKE_CELL_GAP, cell_px - SNAKE_CELL_GAP);
            lv_obj_set_pos(cell, x * cell_px + 1, y * cell_px + 1);
            lv_obj_set_style_radius(cell, 3, 0);
            lv_obj_set_style_border_width(cell, 0, 0);
            lv_obj_set_style_pad_all(cell, 0, 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
            lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(cell, LV_OBJ_FLAG_CLICKABLE);
            s_snake.cells[y][x] = cell;
        }
    }

    lv_obj_add_event_cb(s_snake.board, snake_board_pressed, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_snake.board, snake_board_released, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_snake.board, snake_board_delete, LV_EVENT_DELETE, NULL);

    s_snake.timer = lv_timer_create(snake_tick, 200, NULL);
    snake_reset();
    return s_snake.board;
}

void snake_reset(void)
{
    int cx;
    int cy;
    int x;
    int y;

    if (s_snake.board == NULL) {
        return;
    }

    memset(s_snake.grid, 0, sizeof(s_snake.grid));
    for (y = 0; y < SNAKE_ROWS; y++) {
        for (x = 0; x < SNAKE_COLS; x++) {
            snake_apply_cell(x, y, 0);
        }
    }

    cx = SNAKE_COLS / 2;
    cy = SNAKE_ROWS / 2;
    s_snake.len = 3;
    s_snake.body[0].x = cx;
    s_snake.body[0].y = cy;
    s_snake.body[1].x = cx - 1;
    s_snake.body[1].y = cy;
    s_snake.body[2].x = cx - 2;
    s_snake.body[2].y = cy;

    s_snake.dir = SNAKE_DIR_RIGHT;
    s_snake.next_dir = SNAKE_DIR_RIGHT;
    s_snake.score = 0;
    s_snake.running = 0;
    s_snake.over = 0;

    snake_spawn_food();
    snake_render();

    if (s_snake.score_cb != NULL) {
        s_snake.score_cb(0);
    }
}

void snake_start(void)
{
    if (s_snake.board == NULL) {
        return;
    }
    if (s_snake.over || s_snake.len <= 0) {
        snake_reset();
    }
    s_snake.running = 1;
    s_snake.over = 0;
}

void snake_set_direction(int dir)
{
    if (dir < SNAKE_DIR_UP || dir > SNAKE_DIR_LEFT) {
        return;
    }
    if (!s_snake.running || s_snake.over) {
        return;
    }
    if ((dir + 2) % 4 == s_snake.dir) {
        return;
    }
    s_snake.next_dir = dir;
}

int snake_score(void)
{
    return s_snake.score;
}

int snake_is_running(void)
{
    return s_snake.running;
}

int snake_is_over(void)
{
    return s_snake.over;
}

void snake_set_score_cb(void (*cb)(int score))
{
    s_snake.score_cb = cb;
}
