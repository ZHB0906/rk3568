#include "pinyin.h"

#include <ctype.h>
#include <string.h>

#define PINYIN_MAX_CAND 4

typedef struct {
    const char *pinyin;
    const char *words[3];
} pinyin_entry_t;

/* 静态词表：键为全拼，值为候选词（按常用度排序） */
static const pinyin_entry_t s_dict[] = {
    {"nihao",          {"你好", "您好", NULL}},
    {"ninhao",         {"您好", NULL, NULL}},
    {"shijie",         {"世界", NULL, NULL}},
    {"zhongguo",       {"中国", "中国人", NULL}},
    {"tianqi",         {"天气", NULL, NULL}},
    {"yinyue",         {"音乐", NULL, NULL}},
    {"shezhi",         {"设置", NULL, NULL}},
    {"liaotian",       {"聊天", NULL, NULL}},
    {"lishi",          {"历史", NULL, NULL}},
    {"youxi",          {"游戏", NULL, NULL}},
    {"xiexie",         {"谢谢", NULL, NULL}},
    {"zaijian",        {"再见", NULL, NULL}},
    {"zaoshanghao",    {"早上好", NULL, NULL}},
    {"wanan",          {"晚安", NULL, NULL}},
    {"jintian",        {"今天", NULL, NULL}},
    {"mingtian",       {"明天", NULL, NULL}},
    {"zuotian",        {"昨天", NULL, NULL}},
    {"xianzai",        {"现在", NULL, NULL}},
    {"shijian",        {"时间", NULL, NULL}},
    {"pengyou",        {"朋友", NULL, NULL}},
    {"laoshi",         {"老师", NULL, NULL}},
    {"xuesheng",       {"学生", NULL, NULL}},
    {"xuexiao",        {"学校", NULL, NULL}},
    {"gongzuo",        {"工作", NULL, NULL}},
    {"gongsi",         {"公司", NULL, NULL}},
    {"diannao",        {"电脑", NULL, NULL}},
    {"shouji",         {"手机", NULL, NULL}},
    {"wangluo",        {"网络", NULL, NULL}},
    {"fuwuqi",         {"服务器", NULL, NULL}},
    {"moxing",         {"模型", NULL, NULL}},
    {"zhanghao",       {"账号", NULL, NULL}},
    {"mima",           {"密码", NULL, NULL}},
    {"denglu",         {"登录", NULL, NULL}},
    {"tuichu",         {"退出", NULL, NULL}},
    {"kaishi",         {"开始", NULL, NULL}},
    {"jieshu",         {"结束", NULL, NULL}},
    {"jixu",           {"继续", NULL, NULL}},
    {"quxiao",         {"取消", NULL, NULL}},
    {"queding",        {"确定", NULL, NULL}},
    {"fanhui",         {"返回", NULL, NULL}},
    {"zhuye",          {"主页", NULL, NULL}},
    {"guanyu",         {"关于", NULL, NULL}},
    {"bangzhu",        {"帮助", NULL, NULL}},
    {"beijing",        {"北京", NULL, NULL}},
    {"shanghai",       {"上海", NULL, NULL}},
    {"guangzhou",      {"广州", NULL, NULL}},
    {"shenzhen",       {"深圳", NULL, NULL}},
    {"meishi",         {"美食", NULL, NULL}},
    {"dianying",       {"电影", NULL, NULL}},
    {"dushu",          {"读书", NULL, NULL}},
    {"xuexi",          {"学习", NULL, NULL}},
    {"zhishi",         {"知识", NULL, NULL}},
    {"wenti",          {"问题", NULL, NULL}},
    {"huida",          {"回答", NULL, NULL}},
    {"buyong",         {"不用", NULL, NULL}},
    {"duibuqi",        {"对不起", NULL, NULL}},
    {"meiguanxi",      {"没关系", NULL, NULL}},
    {"jiayou",         {"加油", NULL, NULL}},
    {"piaoliang",      {"漂亮", NULL, NULL}},
    {"keai",           {"可爱", NULL, NULL}},
    {"gaoxing",        {"高兴", NULL, NULL}},
    {"kaixin",         {"开心", NULL, NULL}},
    {"nanguo",         {"难过", NULL, NULL}},
    {"xiuxi",          {"休息", NULL, NULL}},
    {"shuijiao",       {"睡觉", NULL, NULL}},
    {"qichuang",       {"起床", NULL, NULL}},
    {"chifan",         {"吃饭", NULL, NULL}},
    {"heshui",         {"喝水", NULL, NULL}},
    {"kafei",          {"咖啡", NULL, NULL}},
    {"shuiguo",        {"水果", NULL, NULL}},
    {"pingguo",        {"苹果", NULL, NULL}},
    {"xigua",          {"西瓜", NULL, NULL}},
    {"dongwu",         {"动物", NULL, NULL}},
    {"chuntian",       {"春天", NULL, NULL}},
    {"xiatian",        {"夏天", NULL, NULL}},
    {"qiutian",        {"秋天", NULL, NULL}},
    {"dongtian",       {"冬天", NULL, NULL}},
    {"xiayu",          {"下雨", NULL, NULL}},
    {"xiaxue",         {"下雪", NULL, NULL}},
    {"taiyang",        {"太阳", NULL, NULL}},
    {"yueliang",       {"月亮", NULL, NULL}},
    {"xingxing",       {"星星", NULL, NULL}},
    {"dahai",          {"大海", NULL, NULL}},
    {"gaoshan",        {"高山", NULL, NULL}},
    {"chengshi",       {"城市", NULL, NULL}},
    {"qiche",          {"汽车", NULL, NULL}},
    {"huoche",         {"火车", NULL, NULL}},
    {"feiji",          {"飞机", NULL, NULL}},
    {"lvxing",         {"旅行", NULL, NULL}},
    {"gushi",          {"故事", NULL, NULL}},
    {"xinwen",         {"新闻", NULL, NULL}},
    {"xiaoxi",         {"消息", NULL, NULL}},
    {"tupian",         {"图片", NULL, NULL}},
    {"shipin",         {"视频", NULL, NULL}},
    {"shengyin",       {"声音", NULL, NULL}},
    {"yanse",          {"颜色", NULL, NULL}},
    {"hongse",         {"红色", NULL, NULL}},
    {"lanse",          {"蓝色", NULL, NULL}},
    {"lvse",           {"绿色", NULL, NULL}},
    {"baise",          {"白色", NULL, NULL}},
    {"heise",          {"黑色", NULL, NULL}},
    {"shengrikuaile",  {"生日快乐", NULL, NULL}},
    {"xinniankuaile",  {"新年快乐", NULL, NULL}},
    {"chunjie",        {"春节", NULL, NULL}},
    {"zhongqiu",       {"中秋", NULL, NULL}},
    {"qingrenjie",     {"情人节", NULL, NULL}},
    {"nihaoma",        {"你好吗", NULL, NULL}},
    {"wohenhao",       {"我很好", NULL, NULL}},
    {"zhongwen",       {"中文", NULL, NULL}},
    {"yingwen",        {"英文", NULL, NULL}},
    {"shuxue",         {"数学", NULL, NULL}},
    {"yisheng",        {"医生", NULL, NULL}},
    {"haizi",          {"孩子", NULL, NULL}},
    {"jiating",        {"家庭", NULL, NULL}},
    {"shenghuo",       {"生活", NULL, NULL}},
};

#define PINYIN_DICT_LEN ((int)(sizeof(s_dict) / sizeof(s_dict[0])))

static const char *s_cands[PINYIN_MAX_CAND + 1];
static int s_cand_count = 0;
static int s_cand_index = -1;
static char s_last_input[64] = "";

void pinyin_init(void)
{
    s_cands[0] = NULL;
    s_cand_count = 0;
    s_cand_index = -1;
    s_last_input[0] = '\0';
}

const char **pinyin_input_commit(const char *pinyin)
{
    size_t len;
    int i;

    s_cand_count = 0;
    s_cand_index = -1;
    s_cands[0] = NULL;

    if (pinyin == NULL || pinyin[0] == '\0') {
        s_last_input[0] = '\0';
        return s_cands;
    }

    len = strlen(pinyin);
    if (len >= sizeof(s_last_input)) {
        len = sizeof(s_last_input) - 1;
    }
    for (i = 0; (size_t)i < len; i++) {
        s_last_input[i] = (char)tolower((unsigned char)pinyin[i]);
    }
    s_last_input[len] = '\0';

    for (i = 0; i < PINYIN_DICT_LEN; i++) {
        int w;

        if (strcmp(s_dict[i].pinyin, s_last_input) != 0) {
            continue;
        }
        for (w = 0; w < 3 && s_cand_count < PINYIN_MAX_CAND; w++) {
            if (s_dict[i].words[w] == NULL) {
                break;
            }
            s_cands[s_cand_count++] = s_dict[i].words[w];
        }
        break;
    }

    s_cands[s_cand_count] = NULL;
    if (s_cand_count > 0) {
        s_cand_index = 0;
    }
    return s_cands;
}

int pinyin_prev(void)
{
    if (s_cand_count <= 0) {
        return -1;
    }
    s_cand_index = (s_cand_index + s_cand_count - 1) % s_cand_count;
    return s_cand_index;
}

int pinyin_next(void)
{
    if (s_cand_count <= 0) {
        return -1;
    }
    s_cand_index = (s_cand_index + 1) % s_cand_count;
    return s_cand_index;
}

int pinyin_candidate_count(void)
{
    return s_cand_count;
}

const char *pinyin_current(void)
{
    if (s_cand_count <= 0 || s_cand_index < 0 || s_cand_index >= s_cand_count) {
        return NULL;
    }
    return s_cands[s_cand_index];
}
