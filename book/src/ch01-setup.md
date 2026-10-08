# 01 环境搭建：工具链与下载器

本章目标：装好 C 与 Rust 两套工具链，让 CI 同款构建命令在你本机可用。

## 硬件清单

- CH32H417 EVT 评估板
- WCH-LinkE 下载器
- USB 线 ×2（一条供电 + 烧录，一条看串口，取决于板卡配置）

## C 工具链（二选一）

### 路线 A：MounRiver Studio 2（推荐，最省事）

沁恒官方 IDE，内置 riscv-none-elf 工具链、WCH-Link 支持与工程模板。从 [MounRiver 官网](http://www.mounriver.com/) 下载安装即可。本教程的 C 示例是纯命令行工程（Makefile），MounRiver 仅作为工具链来源：

```bash
export PATH="$PATH:/opt/MounRiver/MounRiver_Studio2/toolchain/RISC-V Embedded GCC12/bin"   # Linux/macOS 示例
```

### 路线 B：独立工具链

Ubuntu 直接 `sudo apt install gcc-riscv64-unknown-elf`（CI 用的就是它），或从 [xPack](https://xpack-dev-tools.github.io/riscv-none-elf-gcc-xpack/) 安装。要求支持 `-march=rv32imac -mabi=ilp32`。

验证：

```bash
riscv64-unknown-elf-gcc --version
```

## Rust 工具链

安装 rustup 后，**本仓库根目录的 `rust-toolchain.toml` 会自动**安装 stable 工具链与 `riscv32imac-unknown-none-elf` target——任何 `cargo build` 首次运行时自动完成，无需手动干预。

验证：

```bash
rustc --version
rustup target list --installed | grep riscv32
```

> 为什么选 `riscv32imac`？V5F 的完整 ISA 是 RV32IMABCF（含 B/F 扩展），但 step-0 阶段用不到 B 与 F；imac 目标在 stable Rust 直接可用。后续章节讲到硬件浮点时，再引入自定义 target spec。

## 下载器（调试器）

CH32H417 的调试口走沁恒私有的 2 线 SDI 协议（引脚 PB9=SWDIO、PB8=SWCLK——虽然叫 SWDIO/SWCLK，但**不是标准 SWD**）。因此只能用 WCH-Link/LinkE：DAPLink、J-Link 等标准 CMSIS-DAP/JTAG 调试器不会说 SDI 协议，无法烧录和调试这颗芯片。

烧录工具二选一：

| 工具 | 平台 | 特点 |
|------|------|------|
| [WCH-LinkUtility](https://www.wch.cn/downloads/WCH-LinkUtility_ZIP.html) | Win | 官方 GUI，功能全；LinkE 固件偏旧时先用它升级（新芯片需要较新固件） |
| [wlink](https://github.com/ch32-rs/wlink) | Win/Linux/macOS | Rust 编写的 CLI，`wlink flash step0.bin` 一条命令 |

调试（断点 / 单步）：C、Rust 两条轨道统一用 MounRiver Studio 自带的 OpenOCD（`adapter driver wlinke` + `transport select sdi`，双核配置可参考 Zephyr 上游 ch32h417evt 板的 openocd.cfg）+ GDB。[probe-rs](https://probe.rs/) 上游已收录 CH32H4 目标定义与 flash 算法，但实测 v0.32.0 对 WCH-LinkE（固件 v2.18）attach 失败，暂不能作为依赖。

## 上电与连接排障

EVT 板若由 WCH-LinkE 的电源引脚供电（未接板载 USB），注意两点：

- **LinkE 重新插拔后电源输出默认是关的**，芯片没电自然连不上。先开电：

  ```bash
  wlink set-power enable3v3
  ```

- 若 `wlink` / OpenOCD 报 `0x55: failed to connect with riscvchip`：芯片大概率被上一次调试留在 halt 状态。**远程断电重启即可解救，不必按复位键**（断电→上电就是一次硬复位）：

  ```bash
  wlink set-power disable3v3 && wlink set-power enable3v3
  ```

调试时遵守一条规则：**halt / 单步都在同一个 OpenOCD 会话内完成**（gdb 常驻连接 3333/3334 端口），退出前 resume；不要让芯片带着 halt 状态跨会话。

## 串口终端

EVT 板的 USART 经 WCH-Link 虚拟串口或 USB 转串口连到电脑。任选：PuTTY / MobaXterm / `minicom` / `tio`，115200-8-N-1 起步（第 05 章会从寄存器层配出这个波特率）。

## 检查清单

- [ ] `riscv64-unknown-elf-gcc --version` 或 MounRiver 工具链可用
- [ ] `cargo build` 能自动拉起 riscv32imac target
- [ ] WCH-LinkE 插上后能被识别（设备管理器 / `wlink --help`）
- [ ] 串口终端能打开端口

就绪后进入第 02 章：把一段最小的固件从源码变成板上运行的指令。
