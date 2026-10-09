# ch32h417-bare-metal-guide

> 面向 WCH **CH32H417** 双核 RISC-V MCU 的裸机教程系列 —— **C + Rust 双轨对照**，从零 bring-up，拒绝黑盒。

受到 [cpq/bare-metal-programming-guide](https://github.com/cpq/bare-metal-programming-guide)（STM32 裸机编程指南）启发，但不是翻译：我们把「从零手写、无 HAL、无 RTOS、每一步可跑」的方法论，搬到一颗**双核 + USB 3.0 + 百兆以太网**的新 RISC-V MCU 上，并且**每一章都同时提供 C 与 Rust 两套实现**。

## 为什么是 CH32H417

- **够新**：2026 年初发布，官方与社区生态刚起步。你能亲眼看到一颗芯片从「资料稀缺」到「资料齐全」的全过程——这也是最好的学习时机。
- **够硬**：
  - 双核 QingKe **V5F @ 400MHz** + **V3F @ 160MHz**（RV32IMABCF / RV32IMAFCB）
  - 960KB CodeFlash + 896KB SRAM（128KB ITCM / 256KB DTCM）
  - USB 3.2 Gen1（自带 PHY）、10/100M 以太网（MAC + PHY）、500MB/s UHSIF、SerDes、Type-C/PD
  - 3×CAN、LTDC、I3C、双 12-bit ADC（5Msps）+ 20Msps HSADC、双 DAC、3×OPA
- **够典型**：QingKe 内核的 PFIC 中断控制器、专用向量表、ITCM/DTCM 架构是国产 RISC-V MCU 的通行设计。吃透一颗，理解一片。

## 与参考项目的差异（原创点）

| | [cpq 裸机指南](https://github.com/cpq/bare-metal-programming-guide) | 本教程 |
|---|---|---|
| 芯片 | STM32F429（Cortex-M4 单核） | CH32H417（RISC-V 双核） |
| 语言 | C | **C + Rust 双轨**，同章对照 |
| 启动与中断 | CMSIS 资料遍地 | PFIC / 向量表 / 硬件中断栈，逐行考据 |
| Rust 生态 | — | 生态正在 bring-up，**缺口本身就是教材**（见第 11 章） |
| 多核 | — | HSEM 硬件信号量、双核分工专题章 |
| 高速外设 | — | USB 3.0 / 以太网 / UHSIF 进阶篇（对照官方 SDK） |

## 章节路线图

| # | 主题 | 示例工程 | 状态 |
|---|------|---------|------|
| 00 | 写在前面：方法论与定位 | — | ✅ 初稿 |
| 01 | 环境搭建：工具链与下载器 | — | ✅ 初稿 |
| 02 | 最小固件：从上电到 `main` | `steps/step-0-minimal` | ✅ 已上板（PB1 翻转，调试器读寄存器复核）；正文初稿 |
| 03 | GPIO：点亮一颗 LED | `steps/step-1-blinky` | ✅ 双轨已上板；正文初稿 |
| 04 | SysTick 与精确延时 | `steps/step-2-systick` | ✅ 双轨已上板（1ms 节拍、LED 每秒交替）；正文初稿 |
| 05 | UART 与 `printf` / `log` | `steps/step-3-uart` | ⏳ |
| 06 | 中断：PFIC 与向量表 | `steps/step-4-interrupt` | ⏳ |
| 07 | 时钟树：HSE、双 PLL 与分频 | `steps/step-5-clock` | ⏳ |
| 08 | 定时器与 DMA | `steps/step-6-timer-dma` | ⏳ |
| 09 | 双核协同：HSEM 与核间通信 | `steps/step-7-dualcore` | ⏳ |
| 10 | 进阶：USB 3.0 / 以太网 / UHSIF（对照官方 SDK） | `steps/step-8-*` | ⏳ |
| 11 | Rust 生态专栏：bring-up 实录与回馈上游 | — | 📝 |

状态图例：✅ 已完成　📝 写作中　🧪 可编译、待真机验证　⏳ 计划中

详细里程碑、每章考据点与验收标准见 [ROADMAP.md](ROADMAP.md)。

## 目录结构

```
book/                  # 教程正文（Markdown）
steps/                 # 每章示例工程，独立可构建
  step-0-minimal/
    c/                 # C 版：Makefile + 手写启动 + 链接脚本
    rust/              # Rust 版：零依赖 no_std + 手写启动 + 同款链接脚本
.github/workflows/     # CI：所有示例每 push 编译一次
```

每一步都是**完整工程**：不做模板引用，不藏代码，可以单独 checkout 任意一步对照阅读（与参考项目的做法一致）。

## 环境要求

| 用途 | 工具 | 说明 |
|------|------|------|
| 硬件 | CH32H417 EVT 评估板 + WCH-Link | 官方 EVT，约 $20 |
| C 工具链 | MounRiver Studio 2（或独立 `riscv-none-elf-gcc`） | 详见 book/ch01 |
| Rust 工具链 | `rustup`（stable）+ `riscv32imac-unknown-none-elf` target | 仓库自带 `rust-toolchain.toml` 自动安装 |
| 烧录 | WCH-LinkUtility 或 [wlink](https://github.com/ch32-rs/wlink) | wlink 为 Rust 实现 |

## 快速开始

```bash
git clone https://github.com/no010/ch32h417-bare-metal-guide.git
cd ch32h417-bare-metal-guide

# C 版（含 V3F 唤醒器；双核机器两个 bin 都要烧）
make -C steps/step-0-minimal/c          # 产物: waker.bin / step0.elf / step0.bin

# Rust 版
cd steps/step-0-minimal/rust && cargo build --release   # 产物: target/.../step0
```

## 诚实声明

- 所有示例标注验证状态。标注 🧪 的代码**能编译、寄存器用法待上板验证**——我们不做没跑过的"教程式伪代码"。
- Rust 侧的 qingke-rt 尚未支持 V5 内核（[ch32-rs #24](https://github.com/ch32-rs/ch32-rs/pull/24) 已合入 PAC，[ch32-hal #180](https://github.com/ch32-rs/ch32-hal/pull/180) bring-up 进行中）。前期章节 Rust 版**手写启动与寄存器访问、零第三方依赖**——这正好是裸机教程该有的样子。

## 致谢与来源

- [cpq/bare-metal-programming-guide](https://github.com/cpq/bare-metal-programming-guide)（MIT）—— 方法论参考
- [openwch/ch32h417](https://github.com/openwch/ch32h417) —— 官方 EVT 工程与启动文件/链接脚本参考（step-0 中有改编并注明）
- [ch32-riscv-ug](https://github.com/ch32-riscv-ug/CH32H417) —— 社区寄存器/地址映射考据数据
- [ch32-rs](https://github.com/ch32-rs) —— WCH 芯片 Rust 生态（qingke / ch32-hal / wlink）

## License

本仓库教程与代码以 [MIT](LICENSE) 发布；step-0 中改编自 WCH 官方 EVT 的启动文件与链接脚本，版权归南京沁恒微电子所有，文件头已注明。
