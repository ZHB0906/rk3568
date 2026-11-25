#define _POSIX_C_SOURCE 200809L

#include "net.h"
#include "app.h"
#include "json_util.h"
#include "lvgl.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#define NET_CONNECT_TIMEOUT 10
#define NET_READ_TIMEOUT    10
#define NET_MAX_HEADER      (64 * 1024)
#define NET_MAX_LINE        (256 * 1024)

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} buf_t;

typedef struct {
    char *prompt;
    char *model;
    char *host;
    int port;
    net_delta_cb on_delta;
    net_done_cb on_done;
    net_error_cb on_error;
    void *user;
} chat_job_t;

typedef struct {
    int type; /* 0 增量, 1 完成, 2 错误 */
    char *text;
    net_delta_cb on_delta;
    net_done_cb on_done;
    net_error_cb on_error;
    void *user;
} chat_event_t;

typedef struct {
    chat_job_t *job;
    buf_t line;
    int got_done;
    int got_content;
    char err[256];
} body_ctx_t;

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static int s_busy = 0;
static volatile int s_cancel = 0;

/* ---------------- 缓冲区 ---------------- */

static int buf_reserve(buf_t *b, size_t extra)
{
    size_t need;
    size_t ncap;
    char *p;

    need = b->len + extra + 1;
    if (need <= b->cap) {
        return 0;
    }
    ncap = (b->cap > 0) ? b->cap : 4096;
    while (ncap < need) {
        ncap *= 2;
    }
    p = (char *)realloc(b->data, ncap);
    if (p == NULL) {
        return -1;
    }
    b->data = p;
    b->cap = ncap;
    return 0;
}

static int buf_append(buf_t *b, const char *data, size_t len)
{
    if (len == 0) {
        return 0;
    }
    if (buf_reserve(b, len) != 0) {
        return -1;
    }
    memcpy(b->data + b->len, data, len);
    b->len += len;
    b->data[b->len] = '\0';
    return 0;
}

static void buf_consume(buf_t *b, size_t len)
{
    if (len >= b->len) {
        b->len = 0;
        if (b->data != NULL) {
            b->data[0] = '\0';
        }
        return;
    }
    memmove(b->data, b->data + len, b->len - len);
    b->len -= len;
    b->data[b->len] = '\0';
}

static char *find_bytes(const char *hay, size_t hlen, const char *needle, size_t nlen)
{
    size_t i;

    if (nlen == 0 || hlen < nlen) {
        return NULL;
    }
    for (i = 0; i + nlen <= hlen; i++) {
        if (hay[i] == needle[0] && memcmp(hay + i, needle, nlen) == 0) {
            return (char *)(hay + i);
        }
    }
    return NULL;
}

static int ci_prefix(const char *a, const char *b, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
    }
    return 1;
}

/* ---------------- 主线程事件 ---------------- */

static void chat_event_cb(void *arg)
{
    chat_event_t *ev = (chat_event_t *)arg;

    if (ev == NULL) {
        return;
    }
    switch (ev->type) {
    case 0:
        if (ev->on_delta != NULL) {
            ev->on_delta((ev->text != NULL) ? ev->text : "", ev->user);
        }
        break;
    case 1:
        if (ev->on_done != NULL) {
            ev->on_done(ev->user);
        }
        break;
    default:
        if (ev->on_error != NULL) {
            ev->on_error((ev->text != NULL) ? ev->text : "未知错误", ev->user);
        }
        break;
    }
    free(ev->text);
    free(ev);
}

static void post_event(chat_event_t *ev)
{
    if (ev == NULL) {
        return;
    }
    if (lv_async_call(chat_event_cb, ev) != LV_RESULT_OK) {
        free(ev->text);
        free(ev);
    }
}

static chat_event_t *make_event(int type, const char *text, const chat_job_t *job)
{
    chat_event_t *ev = (chat_event_t *)calloc(1, sizeof(*ev));

    if (ev == NULL) {
        return NULL;
    }
    ev->type = type;
    if (text != NULL) {
        ev->text = strdup(text);
        if (ev->text == NULL) {
            free(ev);
            return NULL;
        }
    }
    ev->on_delta = job->on_delta;
    ev->on_done = job->on_done;
    ev->on_error = job->on_error;
    ev->user = job->user;
    return ev;
}

static void post_delta(const chat_job_t *job, const char *text)
{
    post_event(make_event(0, text, job));
}

/* ---------------- NDJSON 行解析 ---------------- */

static int process_line(body_ctx_t *bc, const char *raw)
{
    const char *line = raw;
    char *content;
    size_t cap;
    int done = 0;
    char msgbuf[256];

    if (line == NULL) {
        return 0;
    }
    while (*line == ' ' || *line == '\t' || *line == '\r') {
        line++;
    }
    if (*line == '\0' || strcmp(line, "[DONE]") == 0) {
        return 0;
    }

    if (json_get_bool(line, "done", &done) == 0 && done) {
        bc->got_done = 1;
    }

    cap = strlen(line) + 1;
    content = (char *)malloc(cap);
    if (content != NULL) {
        if (json_get_string(line, "content", content, cap) == 0 && content[0] != '\0') {
            post_delta(bc->job, content);
            bc->got_content = 1;
        }
        free(content);
    }

    if (json_get_string(line, "error", msgbuf, sizeof(msgbuf)) == 0) {
        snprintf(bc->err, sizeof(bc->err), "%s",
                 (msgbuf[0] != '\0') ? msgbuf : "服务器返回错误");
        return -1;
    }
    return 0;
}

static int feed_bytes(body_ctx_t *bc, const char *data, size_t len)
{
    size_t i = 0;

    while (i < len) {
        const char *nl = (const char *)memchr(data + i, '\n', len - i);
        size_t chunk;
        int rc;

        if (nl == NULL) {
            if (buf_append(&bc->line, data + i, len - i) != 0) {
                return -1;
            }
            break;
        }
        chunk = (size_t)(nl - (data + i));
        if (buf_append(&bc->line, data + i, chunk) != 0) {
            return -1;
        }
        rc = process_line(bc, (bc->line.data != NULL) ? bc->line.data : "");
        bc->line.len = 0;
        if (bc->line.data != NULL) {
            bc->line.data[0] = '\0';
        }
        if (rc != 0) {
            return rc;
        }
        i += chunk + 1;
    }

    if (bc->line.len > NET_MAX_LINE) {
        snprintf(bc->err, sizeof(bc->err), "响应行过长");
        return -1;
    }
    return 0;
}

/* ---------------- 网络基础操作 ---------------- */

static int connect_to(const char *host, int port, char *err, size_t errlen)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *rp;
    char portstr[16];
    int fd = -1;
    int gai;
    int last_errno = 0;

    if (err != NULL && errlen > 0) {
        err[0] = '\0';
    }
    if (host == NULL || host[0] == '\0') {
        snprintf(err, errlen, "服务器地址为空");
        return -1;
    }
    snprintf(portstr, sizeof(portstr), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    gai = getaddrinfo(host, portstr, &hints, &res);
    if (gai != 0) {
        snprintf(err, errlen, "无法解析服务器地址 %s", host);
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        int flags;
        int soerr = 0;
        socklen_t slen = sizeof(soerr);

        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            last_errno = errno;
            continue;
        }

        flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0) {
            flags = 0;
        }
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != 0) {
            if (errno == EINPROGRESS) {
                fd_set wfds;
                struct timeval tv;
                int rc;

                FD_ZERO(&wfds);
                FD_SET(fd, &wfds);
                tv.tv_sec = NET_CONNECT_TIMEOUT;
                tv.tv_usec = 0;
                rc = select(fd + 1, NULL, &wfds, NULL, &tv);
                if (rc <= 0) {
                    if (rc == 0) {
                        snprintf(err, errlen, "连接服务器超时");
                        last_errno = ETIMEDOUT;
                    } else {
                        snprintf(err, errlen, "连接服务器失败");
                        last_errno = errno;
                    }
                    close(fd);
                    fd = -1;
                    continue;
                }
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &slen) != 0 || soerr != 0) {
                    snprintf(err, errlen, "连接服务器失败");
                    last_errno = soerr;
                    close(fd);
                    fd = -1;
                    continue;
                }
            } else {
                snprintf(err, errlen, "连接服务器失败");
                last_errno = errno;
                close(fd);
                fd = -1;
                continue;
            }
        }

        (void)fcntl(fd, F_SETFL, flags);
        break;
    }
    freeaddrinfo(res);

    if (fd < 0) {
        if (err[0] == '\0') {
            snprintf(err, errlen, "无法连接 %s:%d（错误码 %d）", host, port, last_errno);
        }
        return -1;
    }
    return fd;
}

static int send_all(int fd, const char *data, size_t len)
{
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        sent += (size_t)n;
    }
    return 0;
}

/* 返回 >0 读到的字节数，0 表示 EOF，-1 表示出错/超时，-2 表示已取消 */
static int read_some(int fd, char *buf, size_t cap, char *err, size_t errlen)
{
    for (;;) {
        fd_set rfds;
        struct timeval tv;
        int rc;

        if (s_cancel) {
            return -2;
        }
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec = NET_READ_TIMEOUT;
        tv.tv_usec = 0;
        rc = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            snprintf(err, errlen, "等待服务器响应失败");
            return -1;
        }
        if (rc == 0) {
            snprintf(err, errlen, "等待服务器响应超时");
            return -1;
        }

        ssize_t n = recv(fd, buf, cap, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            snprintf(err, errlen, "读取服务器响应失败");
            return -1;
        }
        return (int)n;
    }
}

/* ---------------- 响应解析 ---------------- */

static int parse_status(const char *hdr, size_t len)
{
    const char *sp;

    if (len < 12 || strncmp(hdr, "HTTP/", 5) != 0) {
        return 0;
    }
    sp = (const char *)memchr(hdr, ' ', len);
    if (sp == NULL) {
        return 0;
    }
    return (int)strtol(sp + 1, NULL, 10);
}

static void parse_headers(const char *hdr, size_t len, long *content_length, int *chunked)
{
    size_t i = 0;

    *content_length = -1;
    *chunked = 0;

    while (i < len) {
        size_t j = i;

        while (j < len && !(hdr[j] == '\r' && j + 1 < len && hdr[j + 1] == '\n')) {
            j++;
        }
        if (j > i) {
            const char *line = hdr + i;
            size_t llen = j - i;

            if (llen > 15 && ci_prefix(line, "content-length:", 15)) {
                *content_length = strtol(line + 15, NULL, 10);
            } else if (llen > 18 && ci_prefix(line, "transfer-encoding:", 18)) {
                size_t k;
                for (k = 18; k + 7 <= llen; k++) {
                    if (ci_prefix(line + k, "chunked", 7)) {
                        *chunked = 1;
                        break;
                    }
                }
            }
        }
        i = j + 2;
    }
}

/* 返回 0 正常结束，-1 出错 */
static int decode_body(int fd, buf_t *in, int chunked, long content_length,
                       body_ctx_t *bc, char *err, size_t errlen)
{
    size_t consumed = 0;
    int chunk_state = 0; /* 0 块大小, 1 块数据, 2 块尾, 3 结束 */
    size_t chunk_left = 0;
    int eof = 0;

    for (;;) {
        int progressed = 1;

        if (bc->got_done) {
            return 0;
        }

        while (progressed && in->len > 0) {
            progressed = 0;

            if (!chunked) {
                size_t take = in->len;

                if (content_length >= 0) {
                    long left = content_length - (long)consumed;
                    if (left <= 0) {
                        return 0;
                    }
                    if ((long)take > left) {
                        take = (size_t)left;
                    }
                }
                if (feed_bytes(bc, in->data, take) != 0) {
                    return -1;
                }
                consumed += take;
                buf_consume(in, take);
                if (content_length >= 0 && (long)consumed >= content_length) {
                    return 0;
                }
                progressed = 1;
            } else if (chunk_state == 0) {
                char *eol = find_bytes(in->data, in->len, "\r\n", 2);
                size_t linelen;
                char sizebuf[32];
                char *endp = NULL;
                unsigned long v;

                if (eol == NULL) {
                    if (in->len > 64) {
                        snprintf(err, errlen, "分块编码格式错误");
                        return -1;
                    }
                    break;
                }
                linelen = (size_t)(eol - in->data);
                if (linelen >= sizeof(sizebuf)) {
                    snprintf(err, errlen, "分块编码格式错误");
                    return -1;
                }
                memcpy(sizebuf, in->data, linelen);
                sizebuf[linelen] = '\0';
                v = strtoul(sizebuf, &endp, 16);
                if (endp == sizebuf) {
                    snprintf(err, errlen, "分块编码格式错误");
                    return -1;
                }
                buf_consume(in, linelen + 2);
                if (v == 0) {
                    return 0;
                }
                chunk_left = (size_t)v;
                chunk_state = 1;
                progressed = 1;
            } else if (chunk_state == 1) {
                size_t take = (in->len < chunk_left) ? in->len : chunk_left;

                if (take > 0) {
                    if (feed_bytes(bc, in->data, take) != 0) {
                        return -1;
                    }
                    buf_consume(in, take);
                    chunk_left -= take;
                }
                if (chunk_left == 0) {
                    chunk_state = 2;
                    progressed = 1;
                }
            } else if (chunk_state == 2) {
                if (in->len < 2) {
                    break;
                }
                buf_consume(in, 2);
                chunk_state = 0;
                progressed = 1;
            } else {
                return 0;
            }
        }

        if (bc->got_done) {
            return 0;
        }
        if (eof) {
            return 0;
        }

        char tmp[4096];
        int n = read_some(fd, tmp, sizeof(tmp), err, errlen);

        if (n == -2) {
            snprintf(err, errlen, "请求已取消");
            return -1;
        }
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            eof = 1;
            if (in->len == 0) {
                return 0;
            }
            continue;
        }
        if (buf_append(in, tmp, (size_t)n) != 0) {
            snprintf(err, errlen, "内存不足");
            return -1;
        }
    }
}

/* ---------------- 线程与对外接口 ---------------- */

static void free_job(chat_job_t *job)
{
    if (job == NULL) {
        return;
    }
    free(job->prompt);
    free(job->model);
    free(job->host);
    free(job);
}

static void set_busy(int busy)
{
    pthread_mutex_lock(&s_lock);
    s_busy = busy;
    pthread_mutex_unlock(&s_lock);
}

static void *chat_thread(void *arg)
{
    chat_job_t *job = (chat_job_t *)arg;
    char err[256] = "";
    char header[512];
    int hlen = 0;
    int failed = 0;
    int cancelled = 0;
    int fd = -1;
    char *body = NULL;
    char *model_esc = NULL;
    char *prompt_esc = NULL;
    buf_t in;
    body_ctx_t bc;
    int http_status = 0;
    long content_length = -1;
    int chunked = 0;

    memset(&in, 0, sizeof(in));
    memset(&bc, 0, sizeof(bc));
    bc.job = job;

    model_esc = json_escape_dup(job->model);
    prompt_esc = json_escape_dup(job->prompt);
    if (model_esc == NULL || prompt_esc == NULL) {
        snprintf(err, sizeof(err), "内存不足");
        failed = 1;
    } else {
        size_t cap = strlen(model_esc) + strlen(prompt_esc) + 96;

        body = (char *)malloc(cap);
        if (body == NULL) {
            snprintf(err, sizeof(err), "内存不足");
            failed = 1;
        } else {
            snprintf(body, cap,
                     "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
                     "\"stream\":true}",
                     model_esc, prompt_esc);
        }
    }
    free(model_esc);
    free(prompt_esc);

    if (!failed) {
        fd = connect_to(job->host, job->port, err, sizeof(err));
        if (fd < 0) {
            failed = 1;
        }
    }

    if (!failed) {
        hlen = snprintf(header, sizeof(header),
                        "POST /api/chat HTTP/1.1\r\n"
                        "Host: %s:%d\r\n"
                        "Content-Type: application/json\r\n"
                        "Accept: application/json\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        job->host, job->port, strlen(body));
        if (hlen < 0 || (size_t)hlen >= sizeof(header)) {
            snprintf(err, sizeof(err), "请求头过长");
            failed = 1;
        }
    }

    if (!failed) {
        if (send_all(fd, header, (size_t)hlen) != 0 ||
            send_all(fd, body, strlen(body)) != 0) {
            snprintf(err, sizeof(err), "发送请求失败");
            failed = 1;
        }
    }
    free(body);
    body = NULL;

    if (!failed) {
        for (;;) {
            char tmp[4096];
            int n;
            char *hdr_end;

            n = read_some(fd, tmp, sizeof(tmp), err, sizeof(err));
            if (n == -2) {
                cancelled = 1;
                break;
            }
            if (n < 0) {
                failed = 1;
                break;
            }
            if (n == 0) {
                snprintf(err, sizeof(err), "服务器关闭了连接");
                failed = 1;
                break;
            }
            if (buf_append(&in, tmp, (size_t)n) != 0) {
                snprintf(err, sizeof(err), "内存不足");
                failed = 1;
                break;
            }
            if (in.len > NET_MAX_HEADER) {
                snprintf(err, sizeof(err), "响应头过大");
                failed = 1;
                break;
            }
            hdr_end = find_bytes(in.data, in.len, "\r\n\r\n", 4);
            if (hdr_end != NULL) {
                size_t hdr_len = (size_t)(hdr_end - in.data);

                http_status = parse_status(in.data, hdr_len);
                parse_headers(in.data, hdr_len, &content_length, &chunked);
                buf_consume(&in, hdr_len + 4);
                break;
            }
        }
    }

    if (!failed && !cancelled) {
        if (decode_body(fd, &in, chunked, content_length, &bc, err, sizeof(err)) != 0) {
            failed = 1;
        } else if (!bc.got_done && bc.line.len > 0) {
            if (process_line(&bc, bc.line.data) != 0) {
                failed = 1;
            }
        }
        if (!failed && bc.err[0] != '\0') {
            snprintf(err, sizeof(err), "%s", bc.err);
            failed = 1;
        }
        if (!failed && http_status != 0 && http_status != 200 && !bc.got_content) {
            snprintf(err, sizeof(err), "服务器返回 HTTP %d", http_status);
            failed = 1;
        }
    }

    if (fd >= 0) {
        close(fd);
    }
    free(in.data);
    free(bc.line.data);

    if (s_cancel) {
        cancelled = 1;
    }

    /* 取消时不再回调，避免旧事件影响新会话 */
    if (!cancelled) {
        chat_event_t *ev;
        if (failed) {
            ev = make_event(2, (err[0] != '\0') ? err : "请求失败", job);
        } else {
            ev = make_event(1, NULL, job);
        }
        set_busy(0);
        post_event(ev);
    } else {
        set_busy(0);
    }

    free_job(job);
    return NULL;
}

int net_chat_start(const char *prompt,
                   net_delta_cb on_delta,
                   net_done_cb on_done,
                   net_error_cb on_error,
                   void *user)
{
    chat_job_t *job;
    pthread_t tid;

    if (prompt == NULL || prompt[0] == '\0') {
        return -1;
    }

    pthread_mutex_lock(&s_lock);
    if (s_busy) {
        pthread_mutex_unlock(&s_lock);
        return -1;
    }
    s_busy = 1;
    s_cancel = 0;
    pthread_mutex_unlock(&s_lock);

    job = (chat_job_t *)calloc(1, sizeof(*job));
    if (job == NULL) {
        set_busy(0);
        return -1;
    }
    job->prompt = strdup(prompt);
    job->model = strdup(g_cfg.model);
    job->host = strdup(g_cfg.server_ip);
    job->port = g_cfg.server_port;
    job->on_delta = on_delta;
    job->on_done = on_done;
    job->on_error = on_error;
    job->user = user;

    if (job->prompt == NULL || job->model == NULL || job->host == NULL) {
        free_job(job);
        set_busy(0);
        return -1;
    }

    if (pthread_create(&tid, NULL, chat_thread, job) != 0) {
        free_job(job);
        set_busy(0);
        return -1;
    }
    pthread_detach(tid);
    return 0;
}

void net_chat_cancel(void)
{
    s_cancel = 1;
}

int net_is_busy(void)
{
    int busy;

    pthread_mutex_lock(&s_lock);
    busy = s_busy;
    pthread_mutex_unlock(&s_lock);
    return busy;
}
