#include "app.h"

#include <stdio.h>
#include <string.h>

app_config_t g_cfg;
int g_logged_in = 0;
char g_data_dir[CFG_PATH_LEN] = APP_DATA_DIR_DEFAULT;

static void copy_str(char *dst, size_t size, const char *src)
{
    if (size == 0) {
        return;
    }
    snprintf(dst, size, "%s", (src != NULL) ? src : "");
}

void app_config_defaults(app_config_t *cfg)
{
    if (cfg == NULL) {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    copy_str(cfg->server_ip, sizeof(cfg->server_ip), "192.168.137.1");
    cfg->server_port = 11434;
    copy_str(cfg->model, sizeof(cfg->model), "ds-chat");
    copy_str(cfg->account, sizeof(cfg->account), "admin");
    copy_str(cfg->password, sizeof(cfg->password), "123456");
    cfg->context_len = 4096;
    cfg->font_size = 16;
    copy_str(cfg->game_host, sizeof(cfg->game_host), "192.168.137.1");
    cfg->game_port = 9000;
}
