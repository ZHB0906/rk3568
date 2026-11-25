#ifndef AI_TERMINAL_APP_H
#define AI_TERMINAL_APP_H

#include <stddef.h>

#define APP_NAME             "AI 终端"
#define APP_VERSION          "1.0.0"
#define APP_DATA_DIR_DEFAULT "/root/ai/data"

#define CFG_STR_LEN  64
#define CFG_PATH_LEN 256

/* 应用配置，字段顺序与原程序 settings.json 一致 */
typedef struct {
    char server_ip[CFG_STR_LEN];
    int  server_port;
    char model[CFG_STR_LEN];
    char account[CFG_STR_LEN];
    char password[CFG_STR_LEN];
    int  context_len;
    int  font_size;
    char game_host[CFG_STR_LEN];
    int  game_port;
} app_config_t;

extern app_config_t g_cfg;
extern int g_logged_in;
extern char g_data_dir[CFG_PATH_LEN];

/* 填充默认配置：192.168.137.1:11434 / ds-chat / admin:123456 */
void app_config_defaults(app_config_t *cfg);

#endif /* AI_TERMINAL_APP_H */
