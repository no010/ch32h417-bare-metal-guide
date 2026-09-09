# step-0-minimal：从上电到 main

> 状态：🧪 可编译，寄存器用法待上板验证（PB1 脚位与官方 `GPIO_Toggle` 例程一致）
>
> 对应教程章节：第 02 章（写作中）

## 目标

不做任何"配置"，直接回答三个问题：

1. 芯片上电后，CPU 执行的第一条指令从哪来？（启动代码 + 链接脚本）
2. `main` 函数运行之前，内存里发生了什么？（`.data` 拷贝、`.bss` 清零、栈指针）
3. 怎么用最少的代码让一个 GPIO 翻转？（寄存器直写）

## 构建

```bash
# C
make -C c                     # 产物: c/step0.elf, c/step0.bin

# Rust（零第三方依赖）
cd rust && cargo build --release
```

## 烧录（WCH-Link）

```bash
# wlink
wlink flash c/step0.bin

# 或 WCH-LinkUtility 载入 c/step0.elf
```

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

- [ ] `make -C c` / `cargo build --release` 均产出固件
- [ ] wlink 烧录成功
- [ ] PB1 测到方波
- [ ] 核对 `RCC_APB2PCENR`/`GPIOB` 偏移与 CH32H417RM 一致（当前按 F1 风格布局推测）
