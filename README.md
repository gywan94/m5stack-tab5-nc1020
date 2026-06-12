# m5stack-tab5-nc1020

文曲星 NC1020 / NC2000 模拟器,移植到 **M5Stack Tab5**(ESP32-P4)。

模拟器核心来自 [wangyu-/NC2000](https://github.com/wangyu-/NC2000)(6502 wqx
多机型模拟器,GPL),显示/输入/SD 框架复用本人的 tab5 系列移植框架
(PPA 硬件缩放旋转 + DSI 720×1280 面板 + M5Tab5 磁吸键盘 + 触摸软键盘)。

## 功能

- **NC1020**(`.rom` + `.nor`)与 **NC2000**(`.nand` + `.nand0` + `.nor`)双机型,
  开机文件浏览器从 SD 卡选择,按后缀自动判断机型
- 160×80 LCD,1bpp 黑白 + 2bpp 四级灰度,PPA 硬件 8× 放大
- 双 UI:硬件键盘模式(全屏 LCD + 功能键条)/ 触摸软键盘模式(完整 QWERTY 面板),
  运行中可切换,选择持久化
- M5Tab5 磁吸硬件键盘(I2C 0x6D)完整映射
- NOR/存档直接写回 SD 卡;NC2000 的 32MB NAND 从 SD 按需分页(LRU 块缓存)
- USB 手柄辅助输入

## 编译 / 烧录

ESP-IDF v5.5.2,目标 esp32p4:

```powershell
.\tools\build_flash.ps1 [-Port COM4] [-NoFlash]
```

或:

```powershell
cd apps\nc2000_tab5
idf.py -p COM4 -b 460800 build flash
```

## SD 卡文件

把数据文件放在 SD 卡任意目录(开机浏览器可进子目录):

| 机型 | 必需文件 | 说明 |
|---|---|---|
| NC1020 | `<名字>.rom`(12MB) + `<名字>.nor`(512KB) | wangyu 格式 dump |
| NC2000 | `<名字>.nand`(~33MB) + `<名字>.nand0` + `<名字>.nor` | NAND 按需分页 |

`.nor` 缺失时会以全空白启动并在保存时创建;存档 `.state`、UI 模式 `.kbmode`
都写在同名基名旁边。

## 移植说明

详见 [PORTING.md](PORTING.md)(架构、踩坑记录、性能优化原理)。

要点:
- 上游核心是参数化 6502 SoC 模拟器(nc1020/nc2000/nc3000/pc1000 共用),
  IO_V2 + CPU_RUN3 路径
- 大缓冲(24MB rom / 1MB nor / NAND 缓存)全部走 PSRAM `heap_caps_malloc`
- `CPU_PEEK/CPU_POKE` 内联直读 `memmap`(绕过分支函数链)+ `-O3`,
  从 6fps 提到满速
- IV 中断系统(RTC 2Hz / 闹钟 / 采样)必须用真实 `iv_uart.cpp`,
  存根会导致睡眠后无法唤醒

## License

模拟器核心(`components/nc2000/`)源自 wangyu-/NC2000,遵循其原始许可。
其余移植代码 MIT。
