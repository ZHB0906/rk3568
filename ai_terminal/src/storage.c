#define _POSIX_C_SOURCE 200809L

#include "storage.h"
#include "json_util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define HISTORY_FILE "history.jsonl"
#define SETTINGS_FILE "settings.json"

static int path_join(char *dst, size_t size, const char *name)
{
    int n = snprintf(dst, size, "%s/%s", g_data_dir, name);

    return (n < 0 || (size_t)n >= size) ? -1 : 0;
}

static char *read_whole_file(const char *path, size_t *out_len)
{
    FILE *fp;
    long size;
    char *buf;
    size_t n;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);

    buf = (char *)malloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }
    n = fread(buf, 1, (size_t)size, fp);
    fclose(fp);
    buf[n] = '\0';
    if (out_len != NULL) {
        *out_len = n;
    }
    return buf;
}

static int mkdir_p(const char *path)
{
    char tmp[CFG_PATH_LEN];
    size_t len;
    char *p;

    len = strlen(path);
    if (len == 0 || len >= sizeof(tmp)) {
        return -1;
    }
    memcpy(tmp, path, len + 1);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p != '\0'; p++) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            struct stat st;
            if (stat(tmp, &st) != 0 || !S_ISDIR(st.st_mode)) {
                return -1;
            }
        }
        *p = '/';
    }

    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        struct stat st;
        if (stat(tmp, &st) != 0 || !S_ISDIR(st.st_mode)) {
            return -1;
        }
    }
    return 0;
}

int storage_init(void)
{
    const char *env = getenv("AI_DATA_DIR");
    size_t len;

    if (env != NULL && env[0] != '\0') {
        snprintf(g_data_dir, sizeof(g_data_dir), "%s", env);
    } else {
        snprintf(g_data_dir, sizeof(g_data_dir), "%s", APP_DATA_DIR_DEFAULT);
    }

    len = strlen(g_data_dir);
    while (len > 1 && g_data_dir[len - 1] == '/') {
        g_data_dir[--len] = '\0';
    }
    if (len == 0) {
        snprintf(g_data_dir, sizeof(g_data_dir), "%s", APP_DATA_DIR_DEFAULT);
    }

    if (mkdir_p(g_data_dir) != 0) {
        fprintf(stderr, "[storage] 无法创建数据目录: %s\n", g_data_dir);
        return -1;
    }
    return 0;
}

int config_load(app_config_t *cfg)
{
    char path[CFG_PATH_LEN];
    char tmp[CFG_STR_LEN];
    char *json;
    long long num;

    if (cfg == NULL) {
        return -1;
    }
    if (path_join(path, sizeof(path), SETTINGS_FILE) != 0) {
        return -1;
    }
    json = read_whole_file(path, NULL);
    if (json == NULL) {
        return -1;
    }

    if (json_get_string(json, "server_ip", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0') {
        snprintf(cfg->server_ip, sizeof(cfg->server_ip), "%s", tmp);
    }
    if (json_get_int(json, "server_port", &num) == 0 && num > 0 && num <= 65535) {
        cfg->server_port = (int)num;
    }
    if (json_get_string(json, "model", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0') {
        snprintf(cfg->model, sizeof(cfg->model), "%s", tmp);
    }
    if (json_get_string(json, "account", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0') {
        snprintf(cfg->account, sizeof(cfg->account), "%s", tmp);
    }
    if (json_get_string(json, "password", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0') {
        snprintf(cfg->password, sizeof(cfg->password), "%s", tmp);
    }
    if (json_get_int(json, "context_len", &num) == 0 && num > 0 && num <= 1048576) {
        cfg->context_len = (int)num;
    }
    if (json_get_int(json, "font_size", &num) == 0 && num >= 8 && num <= 72) {
        cfg->font_size = (int)num;
    }
    if (json_get_string(json, "game_host", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0') {
        snprintf(cfg->game_host, sizeof(cfg->game_host), "%s", tmp);
    }
    if (json_get_int(json, "game_port", &num) == 0 && num > 0 && num <= 65535) {
        cfg->game_port = (int)num;
    }

    free(json);
    return 0;
}

int config_save(const app_config_t *cfg)
{
    char path[CFG_PATH_LEN];
    char tmp_path[CFG_PATH_LEN + 8];
    char *ip = NULL;
    char *model = NULL;
    char *account = NULL;
    char *password = NULL;
    char *ghost = NULL;
    FILE *fp = NULL;
    int rc = -1;

    if (cfg == NULL) {
        return -1;
    }
    if (path_join(path, sizeof(path), SETTINGS_FILE) != 0) {
        return -1;
    }
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    ip = json_escape_dup(cfg->server_ip);
    model = json_escape_dup(cfg->model);
    account = json_escape_dup(cfg->account);
    password = json_escape_dup(cfg->password);
    ghost = json_escape_dup(cfg->game_host);
    if (ip == NULL || model == NULL || account == NULL || password == NULL || ghost == NULL) {
        goto cleanup;
    }

    fp = fopen(tmp_path, "wb");
    if (fp == NULL) {
        goto cleanup;
    }

    if (fprintf(fp,
                "{\n"
                "  \"server_ip\": \"%s\",\n"
                "  \"server_port\": %d,\n"
                "  \"model\": \"%s\",\n"
                "  \"account\": \"%s\",\n"
                "  \"password\": \"%s\",\n"
                "  \"context_len\": %d,\n"
                "  \"font_size\": %d,\n"
                "  \"game_host\": \"%s\",\n"
                "  \"game_port\": %d\n"
                "}\n",
                ip, cfg->server_port, model, account, password,
                cfg->context_len, cfg->font_size, ghost, cfg->game_port) < 0) {
        fclose(fp);
        fp = NULL;
        remove(tmp_path);
        goto cleanup;
    }

    if (fclose(fp) != 0) {
        fp = NULL;
        remove(tmp_path);
        goto cleanup;
    }
    fp = NULL;

    if (rename(tmp_path, path) != 0) {
        remove(tmp_path);
        goto cleanup;
    }
    rc = 0;

cleanup:
    if (fp != NULL) {
        fclose(fp);
    }
    free(ip);
    free(model);
    free(account);
    free(password);
    free(ghost);
    return rc;
}

int history_add(const char *user, const char *assistant)
{
    char path[CFG_PATH_LEN];
    char *u;
    char *a;
    char stamp[32];
    time_t now;
    struct tm tmv;
    FILE *fp;
    int rc = 0;

    if (path_join(path, sizeof(path), HISTORY_FILE) != 0) {
        return -1;
    }
    u = json_escape_dup((user != NULL) ? user : "");
    a = json_escape_dup((assistant != NULL) ? assistant : "");
    if (u == NULL || a == NULL) {
        free(u);
        free(a);
        return -1;
    }

    now = time(NULL);
    memset(&tmv, 0, sizeof(tmv));
    localtime_r(&now, &tmv);
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);

    fp = fopen(path, "ab");
    if (fp == NULL) {
        free(u);
        free(a);
        return -1;
    }
    if (fprintf(fp, "{\"time\":\"%s\",\"user\":\"%s\",\"assistant\":\"%s\"}\n",
                stamp, u, a) < 0) {
        rc = -1;
    }
    if (fclose(fp) != 0) {
        rc = -1;
    }
    free(u);
    free(a);
    return rc;
}

static void truncate_utf8(char *s, size_t max_bytes, size_t buf_size)
{
    size_t len = strlen(s);
    size_t cut;

    if (len <= max_bytes) {
        return;
    }
    cut = max_bytes;
    while (cut > 0 && (((unsigned char)s[cut]) & 0xC0u) == 0x80u) {
        cut--;
    }
    if (cut + 4 > buf_size) {
        cut = (buf_size > 4) ? buf_size - 4 : 0;
    }
    memcpy(s + cut, "...", 4);
}

int history_load_recent(int n, char ***out, int *count)
{
    char path[CFG_PATH_LEN];
    char *data;
    char **lines;
    char **items;
    int filled = 0;
    int start = 0;
    int produced = 0;
    int i;
    char *save = NULL;
    char *line;

    if (out == NULL || count == NULL) {
        return -1;
    }
    *out = NULL;
    *count = 0;
    if (n <= 0) {
        return 0;
    }
    if (path_join(path, sizeof(path), HISTORY_FILE) != 0) {
        return -1;
    }
    data = read_whole_file(path, NULL);
    if (data == NULL) {
        /* 文件不存在视为空历史 */
        return 0;
    }

    lines = (char **)calloc((size_t)n, sizeof(char *));
    if (lines == NULL) {
        free(data);
        return -1;
    }

    for (line = strtok_r(data, "\n", &save); line != NULL;
         line = strtok_r(NULL, "\n", &save)) {
        char *copy;
        int idx;

        while (*line == '\r' || *line == ' ' || *line == '\t') {
            line++;
        }
        if (*line == '\0') {
            continue;
        }
        copy = strdup(line);
        if (copy == NULL) {
            continue;
        }
        if (filled < n) {
            idx = start + filled;
            filled++;
        } else {
            idx = start;
            start = (start + 1) % n;
        }
        free(lines[idx]);
        lines[idx] = copy;
    }
    free(data);

    items = (char **)calloc((size_t)filled + 1, sizeof(char *));
    if (items == NULL) {
        for (i = 0; i < n; i++) {
            free(lines[i]);
        }
        free(lines);
        return -1;
    }

    for (i = 0; i < filled; i++) {
        const char *src = lines[(start + i) % n];
        char u[192] = "";
        char a[256] = "";
        char *item;
        size_t need;

        if (src == NULL) {
            continue;
        }
        json_get_string(src, "user", u, sizeof(u));
        json_get_string(src, "assistant", a, sizeof(a));
        truncate_utf8(u, 60, sizeof(u));
        truncate_utf8(a, 80, sizeof(a));

        need = strlen(u) + strlen(a) + 16;
        item = (char *)malloc(need);
        if (item == NULL) {
            continue;
        }
        snprintf(item, need, "我: %s\nAI: %s",
                 (u[0] != '\0') ? u : "(空)",
                 (a[0] != '\0') ? a : "(空)");
        items[produced++] = item;
    }

    for (i = 0; i < n; i++) {
        free(lines[i]);
    }
    free(lines);

    *out = items;
    *count = produced;
    return 0;
}

void history_free(char **items, int count)
{
    int i;

    if (items == NULL) {
        return;
    }
    for (i = 0; i < count; i++) {
        free(items[i]);
    }
    free(items);
}

int history_clear(void)
{
    char path[CFG_PATH_LEN];
    FILE *fp;

    if (path_join(path, sizeof(path), HISTORY_FILE) != 0) {
        return -1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        return -1;
    }
    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}
