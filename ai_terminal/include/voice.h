#ifndef AI_TERMINAL_VOICE_H
#define AI_TERMINAL_VOICE_H

/* 语音模块：本次只保留接口，均不实现实际功能。 */

/* 初始化语音模块，始终返回 0。 */
int voice_init(void);

/* 开始录音识别，未实现，返回 -1。 */
int voice_start_asr(void);

/* 停止录音识别，未实现，返回 -1。 */
int voice_stop_asr(void);

/* 朗读文本，未实现，返回 -1。 */
int voice_tts_speak(const char *text);

#endif /* AI_TERMINAL_VOICE_H */
