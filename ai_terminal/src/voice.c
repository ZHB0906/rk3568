#include "voice.h"

#include <stdio.h>

int voice_init(void)
{
    return 0;
}

int voice_start_asr(void)
{
    printf("[voice] 语音功能未实现：无法开始录音识别\n");
    fflush(stdout);
    return -1;
}

int voice_stop_asr(void)
{
    printf("[voice] 语音功能未实现：无法停止录音识别\n");
    fflush(stdout);
    return -1;
}

int voice_tts_speak(const char *text)
{
    printf("[voice] 语音功能未实现：无法朗读 \"%s\"\n", (text != NULL) ? text : "");
    fflush(stdout);
    return -1;
}
