# RK3568 板端 LVGL 应用

面向 RK3568 开发板的两套 C 应用，基于 **LVGL 9.2.2**，直接操作 `/dev/fb0` 并读取 evdev 触摸事件，不依赖 X11 / Wayland。

- 目标平台：RK3568（aarch64），Buildroot / Linux 4.19
- 屏幕：1024x600，32bpp 帧缓冲
- 触摸：`/dev/input/event0`（evdev）

---

## 目录结构

```
.
├── README.md
├── cmake/
│   └── aarch64-linux-gnu.cmake   交叉编译工具链配置
├── ai_terminal/                  AI 终端应用
│   ├── CMakeLists.txt
│   ├── lv_conf.h
│   ├── include/
│   └── src/
└── boot_demo/                    硬件自检桌面
    ├── CMakeLists.txt
    ├── lv_conf.h
    ├── include/
    └── src/
```

---

## 一、ai_terminal

### 功能

| 模块 | 内容 |
|---|---|
| 登录 | 默认账号 `admin` / `123456`，可在设置页修改 |
| 主页 | 聊天、历史、设置、贪吃蛇、关于 |
| AI 聊天 | HTTP POST `/api/chat`（Ollama 风格），流式逐字显示、自动滚底、取消、错误提示 |
| 历史记录 | `history.jsonl` 追加，展示最近 50 条，支持清空 |
| 设置 | 服务器 IP/端口、模型、账号密码、上下文长度、字号、游戏地址，保存到 `settings.json` |
| 贪吃蛇 | 20x15 网格，方向按钮 + 棋盘滑动，计分与重开 |
| 拼音输入法 | 内置常用词表，候选栏插入输入框 |
| 中文字体 | LVGL 内置 SimSun-16 CJK（LVGL ≥ 9.3 时切换为思源黑体） |

语音识别/合成与联机对战目前只保留接口骨架（`src/voice.c`、`src/lan_game.c`，调用返回 -1）。

### 构建

```bash
cd ai_terminal
cmake -B build \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux-gnu.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

产物：`ai_terminal/build/ai_terminal`

### 运行

```bash
scp build/ai_terminal root@<板子IP>:/root/ai_terminal/
ssh root@<板子IP>
mkdir -p /root/ai/data
/root/ai_terminal/ai_terminal
```

运行前请先停掉板端已有的自启动程序，避免抢占 `/dev/fb0`。

---

## 二、boot_demo

顶部标题 + 时钟，左侧 19 个测试页按钮，右侧内容区。

| 页面 | 实现方式 |
|---|---|
| RTC | `/dev/rtc0` ioctl `RTC_RD_TIME` / `RTC_SET_TIME` |
| LED | `/sys/class/leds/*` 亮度读写 + GPIO 翻转 |
| 蜂鸣器 | GPIO 输出，定时器响 1 秒 |
| 背光 | `/sys/class/backlight/*` 滑块调亮度 |
| 按键 | `/dev/input/event*` 非阻塞读 `EV_KEY`，显示最近 5 条 |
| 光线传感器 | IIO `in_illuminance_raw` / `in_intensity_raw`，1s 刷新 |
| 重力传感器 | IIO `in_accel_{x,y,z}_raw` + scale，500ms 刷新 |
| ADC | IIO `in_voltage0..7_raw`，1s 刷新 |
| 音频 | `/proc/asound/cards`、`aplay` 播放、`arecord` 录 3 秒 |
| 触摸屏 | 画布画点并显示坐标，可清屏 |
| U 盘 | `/dev/sd*` 挂载 `/mnt/udisk` + `statvfs` 容量 |
| TF 卡 | `/dev/mmcblk0p1` / `mmcblk1p1` 挂载 + 容量 |
| EEPROM | `i2cdetect -y -r 0..4`，`i2cget` / `i2cset` 读写 0x50 |
| UART | `/dev/ttyS3` 115200 8N1 自发自收 |
| CAN | `ip link set can0 up type can bitrate 500000`、`cansend` |
| 以太网 | `/sys/class/net` 状态/MAC/IP，`eth0 up` + `udhcpc` |
| 4G | `/dev/ttyUSB2` 发送 `AT+CPIN?` / `AT+CSQ` / `AT+COPS?` |
| WiFi | `iw dev wlan0 scan`（回退 `wpa_cli scan_results`） |
| 蓝牙 | `hciconfig` / `hcitool scan` |

耗时命令放在 pthread 工作线程执行，结果通过互斥结构体 + LVGL 定时器回主线程刷新，工作线程不直接操作 LVGL 对象。

### 构建

```bash
cd boot_demo
cmake -B build \
      -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux-gnu.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

产物：`boot_demo/build/boot_demo`

### 运行

```bash
scp build/boot_demo root@<板子IP>:/usr/bin/
ssh root@<板子IP> "/usr/bin/boot_demo"
```

挂载、`ip link`、`hciconfig`、`i2cdetect`、GPIO 导出等操作需要 root 权限。

---

## 三、构建依赖

| 依赖 | 说明 |
|---|---|
| CMake ≥ 3.16 | 构建 |
| aarch64 交叉编译器 | Buildroot 工具链，或 `apt install gcc-aarch64-linux-gnu` |
| LVGL 9.2.2 | 默认由 CMake FetchContent 自动下载；离线环境用 `-DLVGL_DIR=/path/to/lvgl` 指定本地源码 |
| pthread / libm | 板端系统自带 |

工具链配置二选一：

```bash
# A. Buildroot 工具链
export AARCH64_TOOLCHAIN_DIR=/opt/rk3568/host/bin

# B. 系统交叉编译器
# apt install gcc-aarch64-linux-gnu
```

首次构建需要能访问 GitHub 拉取 LVGL；内网环境可以先手动克隆：

```bash
git clone -b v9.2.2 --depth 1 https://github.com/lvgl/lvgl.git
# 构建时加 -DLVGL_DIR=/path/to/lvgl
```

---

## 四、运行时环境变量

| 变量 | 默认值 | 作用 |
|---|---|---|
| `LV_LINUX_FBDEV_DEVICE` | `/dev/fb0` | 帧缓冲设备 |
| `LV_TOUCH_DEVICE` | `/dev/input/event0` | 触摸设备 |
| `LV_TOUCH_W` / `LV_TOUCH_H` | `1024` / `600` | 分辨率 |
| `AI_DATA_DIR` | `/root/ai/data` | 配置与历史目录（仅 ai_terminal） |

AI 终端默认服务器为 `192.168.137.1:11434`（Ollama 风格），可在设置页修改，写入 `settings.json`。

---

## 五、注意事项

1. **字体**：LVGL 9.2.2 源码树只带 SimSun-16 CJK；思源黑体从 9.3 起随源码提供。两套工程的 `lv_conf.h` 会自动探测并回退。
2. **帧缓冲位深**：`lv_conf.h` 默认 `LV_COLOR_DEPTH 32`；若板端 fb 为 16bpp，请改为 16 后重新编译，否则花屏。
3. **界面差异**：本工程为独立实现，交互与原板端程序接近，但不保证像素级一致，且未包含原机壁纸等图片资源。
