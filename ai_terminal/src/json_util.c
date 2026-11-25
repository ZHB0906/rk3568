#include "json_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_ws(const char *p)
{
    while (p != NULL && *p != '\0' &&
           (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
        p++;
    }
    return p;
}

/* p 指向字符串起始引号，返回结束引号之后的位置；格式错误返回 NULL。 */
static const char *skip_string(const char *p)
{
    p++; /* 跳过起始引号 */
    while (*p != '\0') {
        if (*p == '\\') {
            p++;
            if (*p == '\0') {
                return NULL;
            }
            p++;
        } else if (*p == '"') {
            return p + 1;
        } else {
            p++;
        }
    }
    return NULL;
}

/* 查找 "key" 对应的值起始位置（跳过冒号和空白）。 */
static const char *find_value(const char *json, const char *key)
{
    const char *p;
    size_t klen;

    if (json == NULL || key == NULL) {
        return NULL;
    }
    klen = strlen(key);
    p = json;
    while (*p != '\0') {
        if (*p != '"') {
            p++;
            continue;
        }
        const char *end = skip_string(p);
        if (end == NULL) {
            return NULL;
        }
        if ((size_t)(end - p - 2) == klen && memcmp(p + 1, key, klen) == 0) {
            const char *q = skip_ws(end);
            if (*q == ':') {
                return skip_ws(q + 1);
            }
        }
        p = end;
    }
    return NULL;
}

static int hex4(const char *p, unsigned *out)
{
    unsigned v = 0;
    int i;

    for (i = 0; i < 4; i++) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') {
            v |= (unsigned)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            v |= (unsigned)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            v |= (unsigned)(c - 'A' + 10);
        } else {
            return -1;
        }
    }
    *out = v;
    return 0;
}

static int utf8_encode(unsigned cp, char *out)
{
    if (cp < 0x80u) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800u) {
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp < 0x10000u) {
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    if (cp <= 0x10FFFFu) {
        out[0] = (char)(0xF0u | (cp >> 18));
        out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (char)(0x80u | (cp & 0x3Fu));
        return 4;
    }
    out[0] = '?';
    return 1;
}

int json_escape(const char *src, char *dst, size_t dst_size)
{
    const unsigned char *p;
    size_t o = 0;

    if (dst == NULL || dst_size == 0) {
        return -1;
    }
    if (src == NULL) {
        src = "";
    }
    for (p = (const unsigned char *)src; *p != '\0'; p++) {
        const char *rep = NULL;
        char uni[8];
        size_t n;

        switch (*p) {
        case '"':  rep = "\\\""; break;
        case '\\': rep = "\\\\"; break;
        case '\b': rep = "\\b";  break;
        case '\f': rep = "\\f";  break;
        case '\n': rep = "\\n";  break;
        case '\r': rep = "\\r";  break;
        case '\t': rep = "\\t";  break;
        default:
            if (*p < 0x20u) {
                snprintf(uni, sizeof(uni), "\\u%04x", (unsigned)*p);
                rep = uni;
            }
            break;
        }

        if (rep != NULL) {
            n = strlen(rep);
            if (o + n >= dst_size) {
                return -1;
            }
            memcpy(dst + o, rep, n);
            o += n;
        } else {
            if (o + 1 >= dst_size) {
                return -1;
            }
            dst[o++] = (char)*p;
        }
    }
    dst[o] = '\0';
    return (int)o;
}

char *json_escape_dup(const char *src)
{
    size_t len;
    char *buf;

    if (src == NULL) {
        src = "";
    }
    len = strlen(src);
    buf = (char *)malloc(len * 6 + 1);
    if (buf == NULL) {
        return NULL;
    }
    if (json_escape(src, buf, len * 6 + 1) < 0) {
        free(buf);
        return NULL;
    }
    return buf;
}

int json_get_string(const char *json, const char *key, char *out, size_t out_size)
{
    const char *p;
    size_t o = 0;
    char *tmp;

    if (out == NULL || out_size == 0) {
        return -1;
    }
    out[0] = '\0';
    p = find_value(json, key);
    if (p == NULL || *p != '"') {
        return -1;
    }
    p++;

    while (*p != '\0' && *p != '"') {
        unsigned char c = (unsigned char)*p;

        if (c == '\\') {
            char enc[4];
            int n = 1;

            p++;
            switch (*p) {
            case '"':  enc[0] = '"';  break;
            case '\\': enc[0] = '\\'; break;
            case '/':  enc[0] = '/';  break;
            case 'b':  enc[0] = '\b'; break;
            case 'f':  enc[0] = '\f'; break;
            case 'n':  enc[0] = '\n'; break;
            case 'r':  enc[0] = '\r'; break;
            case 't':  enc[0] = '\t'; break;
            case 'u': {
                unsigned cp = 0;
                if (hex4(p + 1, &cp) != 0) {
                    out[0] = '\0';
                    return -1;
                }
                p += 4;
                if (cp >= 0xD800u && cp <= 0xDBFFu && p[1] == '\\' && p[2] == 'u') {
                    unsigned lo = 0;
                    if (hex4(p + 3, &lo) == 0 && lo >= 0xDC00u && lo <= 0xDFFFu) {
                        cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
                        p += 6;
                    }
                }
                if (cp == 0u) {
                    enc[0] = '?';
                    n = 1;
                } else {
                    n = utf8_encode(cp, enc);
                }
                break;
            }
            case '\0':
                out[0] = '\0';
                return -1;
            default:
                out[0] = '\0';
                return -1;
            }
            if (o + (size_t)n >= out_size) {
                out[0] = '\0';
                return -1;
            }
            tmp = out + o;
            memcpy(tmp, enc, (size_t)n);
            o += (size_t)n;
            p++;
        } else {
            if (o + 1 >= out_size) {
                out[0] = '\0';
                return -1;
            }
            out[o++] = (char)c;
            p++;
        }
    }

    if (*p != '"') {
        out[0] = '\0';
        return -1;
    }
    out[o] = '\0';
    return 0;
}

int json_get_int(const char *json, const char *key, long long *out)
{
    const char *p;
    char *endp = NULL;
    long long v;

    if (out == NULL) {
        return -1;
    }
    p = find_value(json, key);
    if (p == NULL) {
        return -1;
    }
    if (!(*p == '-' || (*p >= '0' && *p <= '9'))) {
        return -1;
    }
    v = strtoll(p, &endp, 10);
    if (endp == p) {
        return -1;
    }
    *out = v;
    return 0;
}

int json_get_bool(const char *json, const char *key, int *out)
{
    const char *p;

    if (out == NULL) {
        return -1;
    }
    p = find_value(json, key);
    if (p == NULL) {
        return -1;
    }
    if (strncmp(p, "true", 4) == 0) {
        *out = 1;
        return 0;
    }
    if (strncmp(p, "false", 5) == 0) {
        *out = 0;
        return 0;
    }
    return -1;
}
