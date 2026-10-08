# step-0-minimal：从上电到 main

> 状态：🧪 可编译（含 V3F 唤醒器）；寄存器已按实测修正，待上板复验 PB1 方波
>
> 对应教程章节：第 02 章（写作中）

## 目标

不做任何"配置"，直接回答四个问题：

1. **双核芯片上电后谁先跑？** Boot ROM 只拉起 V3F；V5F 需由 V3F 唤醒（`c/waker.S`）
2. 芯片上电后，CPU 执行的第一条指令从哪来？（启动代码 + 链接脚本）
3. `main` 函数运行之前，内存里发生了什么？（`.data` 拷贝、`.bss` 清零、栈指针）
4. 怎么用最少的代码让一个 GPIO 翻转？（寄存器直写）

## 构建

```bash
# C（同时产出 V3F 唤醒器）
make -C c                     # 产物: c/waker.bin, c/step0.elf, c/step0.bin

# Rust（零第三方依赖；唤醒器由 C 轨产出，两轨共用）
cd rust && cargo build --release
```

## 烧录（WCH-LinkE + wlink）

> ⚠️ **H417 双核启动坑**：Boot ROM 只拉起 V3F，V5F（本例程运行的核心）默认保持复位。
> 必须先烧本目录的 V3F 唤醒器，再烧应用；两个 bin 分开烧：

```bash
wlink flash -e -a 0x08000000 c/waker.bin    # 唤醒器，带整片擦除
wlink flash    -a 0x08010000 c/step0.bin    # V5F 应用，不再擦
```

或 WCH-LinkUtility 载入 `c/step0.elf`（唤醒器源码见 `c/waker.S`）。

## 预期现象

PB1 输出方波（示波器可见；周期约数百 ms，取决于复位后默认时钟，第 07 章讲时钟树后可精确控制）。

## 本章刻意做的简化

| 简化 | 官方工程的做法 | 详见 |
|------|----------------|------|
| 不设中断向量表 / 不配 `mtvec` | 完整向量表 + PFIC | 第 06 章 |
| `.text` 在 CodeFlash 原地执行 | 拷入 ITCM(0x200A0000) 提速 | 第 07 章 |
| 时钟用复位默认值 | HSE 25M + PLL 配置 | 第 07 章 |
| 不初始化 FPU/中断栈 | V5F 完整初始化 | 第 06/07 章 |

## 验证清单（上板时逐项打勾）

- [x] `make -C c` / `cargo build --release` 均产出固件（含 waker.bin）
- [ ] wlink 双 bin 烧录成功
- [ ] PB1 测到方波
- [x] `RCC_HB2PCENR(0x1C)` / GPIOB 偏移已按上板实测修正（step-1 验证，2026-09-09）
