# tab5-nc2000 移植文档（NC2000 / NC1020 文曲星模拟器 → M5Stack Tab5）

> 本文是 `tab5-nc2000` 的权威移植规范。后续所有移植/调试工作**按本文进行**，
> 改动若与本文冲突，先更新本文再改代码。

- **上游**：https://github.com/wangyu-/NC2000 （克隆在 `tab5-retro/NC2000/`）
- **目标平台**：M5Stack Tab5（ESP32-P4 / ESP-IDF v5.5.2），复用 `tab-nc1020` 的 Tab5 框架
- **当前状态**：`idf.py build` **通过（exit 0）**，app bin ≈ 759 KB，**尚未上机实测**
- **真正交付目录**：`tab5-nc2000/`（`tab-nc2000/` 是更早的脚手架，废弃）

---

## 1. 为什么一套代码能模拟 nc1020 / nc2000 / nc3000 / pc1000

这些文曲星都是**同一代 6502 SoC 硬件家族**，只是外设/存储/时钟规格不同。wangyu 的模拟器是**一套参数化的 6502 SoC 模拟器**，靠 5 层机制覆盖全部型号：

| 层 | 机制 | 代码位置 |
|---|---|---|
| CPU 内核 | 共用一份 6502(W65C02) 执行；只有时钟不同（pc1000=3.69M / nc1020·nc2000=5.12M / nc3000=10.24MHz） | `ansi/w65c02*`, `comm.cpp init_parameters()` |
| 模式开关 | 全局 `nc1020mode/nc2000mode/nc3000mode/pc1000mode`，代码里 `if(nc2000mode){…}` 分支 | `comm.cpp` |
| 硬件规格参数化 | `init_parameters()` 按模式算 `num_nand_pages / num_nor_pages / num_rom_pages / CYCLES_SECOND` | `comm.cpp` |
| 可插拔 IO/CPU-loop | `io_version`(IO_V1/V2/EMUX) + `cpu_loop_version`(RUN1/2/3) 按型号选实现 | `io*.cpp`, `cpu_loop*.cpp` |
| 文件名 = 基名 + 后缀 | 给一个基名拼后缀得实际文件；**后缀标明芯片类型，不是型号** | `settings.cpp`（桌面版）/ 我们的 `main.c` |

**关键结论：型号 = 一组（时钟 + 存储芯片组合 + IO/loop 变体）。后缀只是芯片类型命名。**

每型号的存储芯片组合与参数：

| 模式 | 加载文件（基名+后缀） | NOR | NAND | 独立 ROM | CPU 时钟 |
|---|---|---|---|---|---|
| **NC1020** | `.rom` + `.nor` | 512K | — | 12 MB | 5.12 MHz |
| **PC1000** | `.rom` + `.nor` | 512K | — | 有 | 3.69 MHz |
| **NC2000** | `.nand` + `.nand0` + `.nor` | 512K | ~34 MB (65536×528B) | 无（系统跑在 NAND） | 5.12 MHz |
| **NC3000** | `.nand` + `.nand0` + `.nor` | 1 MB | ~68 MB | 无 | 10.24 MHz |

> 用户现有的 `nc1020.rom`(12MB) + `nc1020.nor`(512KB) = **NC1020 模式**（wangyu 格式，与 rainyx 的 24MB `obj_lu.bin` 不同格式）。**NC1020 模式无 NAND，是最简单的上机验证路径，应优先调通。**

---

## 2. 桌面模拟器 → Tab5 的映射（什么复用、什么替换）

| 桌面版模块 | Tab5 处理 |
|---|---|
| 6502 核心 (`cpu/mem/io/rom/nor/nand/ram` + `ansi/w65c02`) | **原样移植**（少量 BSS→PSRAM 改造，见 §6） |
| `NekoDriverIO.cpp`（IO 端口后端） | **必须编译**（是真实后端，不是桩） |
| `display.cpp` + `lcdstripe/`（SDL 渲染） | **丢弃**，glue 直接 `CopyLcdBuffer` → rgb565 → PPA |
| `main.cpp`/`settings.cpp`（SDL 窗口/命令行） | **替换**为 `apps/nc2000_tab5/main/main.c`（SD 扫描+选型号） |
| `sound.cpp`/`dsp/`（SDL 音频） | **桩成静音**（`tab5_stubs.cpp` + `stub/dsp/dsp.h`） |
| `console/iv_uart/cmd/udp_server/disassembler` | **桩成空操作** |
| `compare/c6502`、`compare/pc1000bus` | 编译但**不执行**（只为满足链接器；我们用 CPU_HANDYPSP+IO_V2 路径） |

显示 **160×80**（与 NC1020 同尺寸）→ Tab5 的 PPA 90° 旋转管线、双 UI（满屏/软键盘）、硬件键盘 `kbd_hw`+触摸、键码表**全部 1:1 复用**（NC2000 `SetKey()` 用的键码与 NC1020 相同）。

---

## 3. 工程结构

```
tab5-nc2000/
├── PORTING.md                      ← 本文
├── partitions_ota.csv              ← nvs + otadata + 4MB factory（无 ROM/storage 分区，SD-only）
├── tools/{build_flash.ps1, gen_nc_cjk.py, mock_layout.py}
├── apps/nc2000_tab5/               ← 应用（main.c = SD 扫描+选型号+运行）
└── components/
    ├── nc2000/                     ← wangyu 核心（移植）
    │   ├── CMakeLists.txt          ← 只编核心子集 + 桩
    │   ├── *.cpp/*.h               ← 原始上游源码
    │   ├── stub/                   ← SDL/dsp 桩头
    │   └── tab5_stubs.cpp          ← 桌面外设空桩
    ├── nc2000_run/                 ← glue（显示/输入/循环 + C↔C++ shim）
    │   ├── nc2000_run.c / .h       ← C glue（面板/输入/主循环）
    │   ├── nc2000_api.{h,cpp}      ← C↔C++ 桥（C glue 调不了 C++ 核心）
    │   └── nc_cjk.{c,h}            ← 24×24 CJK 字库
    ├── kbd_hw, rom_store, odroid, bsp, ppa_engine, audio, app_common …  ← 复用 Tab5 框架
```

---

## 4. 启动与选型号（按后缀）—— `apps/nc2000_tab5/main/main.c`

**SD-only**：rom/nor/nand/state/.kbmode 全在 SD 卡上，flash 不放数据。

`app_main` 流程：
1. `app_init()` → `odroid_sdcard_open("/sd")`（挂载失败则提示并重启）。
2. `scan("/sd", depth≤4)` 递归找主文件，按后缀建候选 `nc2k_cand_t{base, mode, label}`：
   - `*.nand` → **NC2000**（需同名 `.nand0` + `.nor`）
   - `*.rom` → **NC1020**（需同名 `.nor`）
3. `nc2000_pick()`：>1 个候选 → 全屏触摸列表选；=1 → 直接跑；=0 → 报错重启。
4. `nc2000_run(base, mode)` 进入模拟器；退出后存档（`.nor`/state 写回 SD base 旁）。

> NC3000/PC1000 的后缀映射可按 §1 表后续补进 `scan()`。

---

## 5. glue 与 C↔C++ 桥

C glue（`nc2000_run.c`）不能直接调 C++ 核心，`nc2000_api.{h,cpp}` 是边界：

```c
void nc2k_configure(nc2k_mode_t mode, const char *base); // 设 mode + cpu_loop_version=CPU_RUN3
                                                         // + io_version=IO_V2 + 拼 WqxRom 路径
                                                         // + init_parameters() + init_keyitems()
void nc2k_load(void);                                    // LoadNC2k：load rom/nor/nand 并启动
void nc2k_run_slice(uint32_t ms, bool fast);             // 跑一个时间片
int  nc2k_copy_lcd(uint8_t *buf);   // 0=无/1=1bpp(1600B)/2=2bpp灰(3200B)，经 is_grey_mode()
void nc2k_set_key(uint8_t code, bool down);              // 键码同 NC1020
void nc2k_save(void);                                    // 存 NOR+NAND+state
```

**显示 2bpp 灰度**：`present_lcd` 两种都处理——`nc2k_copy_lcd` 返回 1=1bpp / 2=2bpp 灰；
灰度值 `(byte >> (6-2j)) & 3` → `GREY4[]={0xFFFF,0xAD55,0x52AA,0x0000}` 的 rgb565，
再走与 NC1020 相同的 PPA 90° 缩放/旋转管线。

---

## 6. 核心如何编译+链接通过（关键改动，勿回退）

全部在 `components/nc2000/`：

1. **CMake SRCS**（flags `-std=gnu++17 -fpermissive -w -DHANDYPSP -DTAB5_PORT`）：
   `nc2000 comm cpu cpu_loop[_new] mem io io_new rom nor nand ram key key_new NekoDriverIO compare/c6502 compare/pc1000bus ansi/w65c02{cpu,op} tab5_stubs`。`INCLUDE_DIRS "." "stub"`，`REQUIRES fatfs esp_timer`。
2. **`NekoDriverIO.cpp` 是真实 IO 后端**（定义 `ReadPort1/Write*/Read01IntStatus/timer{0,1}ticks/CreateHotlinkMapping` 等，被 io*.cpp 引用）。`-DHANDYPSP` 下其 `#include "ANSI/65C02.h"` 被 `#else` 跳过 → 能编。**必须编译。**
3. **`compare/c6502.cpp` + `pc1000bus.cpp`** 只为满足链接：`init_cpu_new()` 在 `cpu_version==CPU_EMUX / io_version==IO_EMUX` 时 `new C6502()/new BusPC1000()`；我们用 `CPU_HANDYPSP+IO_V2`（走 `new CPUInterface()`），这些类**永不执行**。
4. **SDL 桩**（`stub/`，在 include 路径里）：`SDL2/SDL.h`（GetTicks/GetTicks64/SetWindowTitle）、`SDL_keycode.h`（**完整 SDLK_* 枚举**，让 key*.cpp 编过，但实际不用——glue 直接 `SetKey`）、`SDL_timer.h`、`SDL_keyboard.h`。
5. **`stub/dsp/dsp.h`**（上游缺）：极简静音 `class Dsp{int dspMode; void reset(); int write(int=0,int=0);}`；`io_new.cpp` 用 `extern Dsp dsp;`（实例在 `tab5_stubs.cpp`）。
6. **`tab5_stubs.cpp`** 桩掉被排除的桌面外设（静音/空）：SDL 函数、`Dsp dsp;`、sound（`post_cpu_run_sound_handling/sound_busy=false/…`）、iv_uart（`RCR0` 绑静态字节、read/write_3a-d 等）、cmd/console（`console_on=false`、`is_nc2600_rom=false`、`set_warm_reset_flag`、`handle_cmd`…）、disassembler（→""）。
7. **BSS→PSRAM 堆（曾是链接拦路虎 "Total discarded sections 26MB"）**：
   - `rom.cpp` 的 `uint8_t rom_buff[24MB]`、`nor.cpp` 的 `uint8_t nor_buff[1MB]` 原是静态 BSS → 放不下。
   - 改为 PSRAM 堆指针：`uint8_t *rom_buff=nullptr;` + `init_rom/init_nor` 里 `heap_caps_malloc(SIZE, MALLOC_CAP_SPIRAM)`；`sizeof(buf)` 断言改 `assert(buf)`，`memset(&buf` → `memset(buf`，头文件 extern 改指针。
8. **NAND 按需分页（已做）**：`nand.cpp` 不再用 ~66MB 静态数组；NAND 的 528B 页按需从 SD 文件读/写（NC2000 模式 34MB 装不进 32MB PSRAM）。

---

## 7. 存储模型（SD-only）

- 数据：`<base>.rom / .nor / .nand / .nand0`、`<base>.state`、`.kbmode` 全在 SD。
- 退出存档：`save_flash_on_exit=true` → NOR/NAND/state 写回 SD base 旁。
- **不变量**：不要把 NAND/ROM 放回 flash 分区或静态数组；ROM/NOR 用 PSRAM 堆，NAND 始终 SD 分页。

---

## 8. 构建 / 烧录

```powershell
tools\build_flash.ps1 -NoFlash            # 只编译
tools\build_flash.ps1 -Port COM4          # 编译+烧录（app）
```
分区 = nvs + otadata + **4MB factory**（无 ROM/storage 分区）。app ≈ 759KB（82% 余量）。
> 注意：COM4 串口偶发掉线（USB-CDC 重枚举），见 [[tab-nc1020-standalone]]；掉线就重插/重试。

---

## 9. 剩余工作 = 上机调试（按此顺序）

代码已构建通过，剩下是硬件 bring-up：

1. **先用 NC1020 模式验证内核**（最简单：ROM+NOR，无 NAND）。
   把 `nc1020.rom`/`nc1020.nor` 改名成同基名（如 `/sd/wqx.rom`+`/sd/wqx.nor`）放 SD → 开机应进选单或直接跑。
2. **显示**：确认 160×80 2bpp 灰度 → rgb565 → PPA 90° 输出正确（方向/灰阶）。`GREY4[]` 可调。
3. **输入**：硬件键盘/触摸/软键盘 → `nc2k_set_key` 键码是否对（对照 `key.cpp` SetKey）。
4. **再上 NC2000 模式**：放 `.nand`+`.nand0`+`.nor`，验证 **NAND 分页 I/O 正确性与速度**（SD 随机读 528B 页可能慢 → 必要时加页缓存）。
5. **音频**：当前静音；如需声音，把 `tab5_stubs.cpp` 的 dsp/beeper 接到 Tab5 BSP 音频（参考 tab-nc1020 audio）。
6. **存档**：验证退出写回 `.nor`/`.state`/NAND，重启能续。

---

## 10. 不变量 / 易踩坑

- **保持 `NekoDriverIO.cpp`、`compare/*` 在编译列表**（去掉会链接失败）。
- **不要重新引入大静态数组**（rom_buff/nor_buff/nand）→ 必然超 BSS/PSRAM。
- 模式由 `nc2k_configure` 设 `nc1020mode/nc2000mode` + `cpu_loop_version=CPU_RUN3` + `io_version=IO_V2`，并 `init_parameters()`。
- glue 是 C、核心是 C++ → 一切跨界调用走 `nc2000_api`（`extern "C"`）。
- 现有 **tab-nc1020（rainyx 24MB obj_lu，能用版）与本工程独立**，互不影响。

---

## 11. 参考：关键文件速查

| 想改什么 | 看哪里 |
|---|---|
| 选型号/扫描 SD | `apps/nc2000_tab5/main/main.c` |
| 主循环/显示/输入 | `components/nc2000_run/nc2000_run.c` |
| 模式配置/路径/API 桥 | `components/nc2000_run/nc2000_api.cpp` |
| 每型号参数（时钟/页数） | `components/nc2000/comm.cpp init_parameters()` |
| 内存 bank 映射 | `components/nc2000/mem.cpp` |
| IO 端口 | `components/nc2000/io.cpp / io_new.cpp / NekoDriverIO.cpp` |
| NAND 分页 | `components/nc2000/nand.cpp` |
| ROM/NOR 加载（PSRAM 堆） | `components/nc2000/rom.cpp / nor.cpp` |
| 桌面外设桩 | `components/nc2000/tab5_stubs.cpp`、`stub/` |
| 键码 | `components/nc2000/key.cpp`（SetKey） |
