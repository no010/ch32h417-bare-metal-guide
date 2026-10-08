# step-1-blinky：双 LED 交替闪烁

> 状态：✅ 双轨均已上板验证（C 2026-09-09；Rust 2026-09-10，交替更快便于区分当前运行版本）
>
> 对应教程章节：第 03 章（写作中）

## 目标

在 step-0"从上电到 main"的基础上回答新问题：**怎么驱动真实板上的外设？**
——点亮 EVT 板上的两颗用户 LED（引脚来自板上 J3 实际跳线）。

双核启动（V3F 唤醒器、PFIC 释放 V5F）已在 step-0 讲清；本步直接复用其唤醒器，
两轨共用同一 `c/waker.bin`（每步自包含：各 step 目录内各带副本）。

## 板级事实（本板实测）

| 项目 | 值 |
|------|-----|
| LED1 | PE2，低电平有效（拉低点亮） |
| LED2 | PE3，低电平有效 |
| 颜色 | 两颗蓝色 |
| 接法 | 经 J3 插针跳线接至 MCU IO |

## 布局

```
0x00000000  waker.bin   V3F 唤醒器（waker.S，纯汇编，两轨共用）
0x00010000  step1.bin   V5F 闪灯应用（C 与 Rust 各一版）
```

唤醒序列（与 Zephyr `soc/wch/ch32v/ch32h41x/soc_v3f.c` 已验证实现一致）：

```
PFIC->WAKEIP[1] (0xE000E724) = 0x08010000   # V5F 入口（Flash 别名地址）
PFIC->SCTLR    (0xE000ED10) |= BIT(5)       # SLEEPONEXIT，V3F 唤醒后入睡
```

## 构建

```bash
# C 轨（MRS2 用户替换 CROSS=riscv-wch-elf-；CI 用 riscv64-unknown-elf-）
make -C c CROSS=riscv-wch-elf-

# Rust 轨（waker.bin 由 C 轨产出，两轨共用）
cd rust && cargo build --release
```

## 烧录（WCH-LinkE + wlink）

双核镜像**分开烧**（合并大文件 fastprogram 会报错）：

```bash
wlink flash -e -a 0x08000000 c/waker.bin    # 唤醒器，带整片擦除
wlink flash    -a 0x08010000 c/step1.bin    # V5F 应用，不再擦
```

Rust 版把 `step1.bin` 换成 `rust/step1.bin`（用 objcopy 从 ELF 提取）。

## 排障实录（上板踩过的坑）

| 现象 | 根因 | 解法 |
|------|------|------|
| 烧完毫无反应 | H417 的 F1 式 APB2 使能寄存器搬到了 **RCC+0x1C（HB2PCENR）**，按 F1 地址写 0x18 实际写的是 HBPCENR，GPIOE 时钟门永远不开 | 使能写 `RCC+0x1C`，IOPEEN=bit6（经 ch32-rs 补丁 SVD 核实） |
| 65KB 合并镜像 fastprogram 报错 | wlink 对大文件/跨区 fastprogram 不稳 | 改为两个 bin 分开烧 |
| 连接报 `0x55: failed to connect with riscvchip` | 芯片被留在 halt 状态 | `wlink set-power disable3v3 && wlink set-power enable3v3`（断电重启=硬复位） |

## 验证清单

- [x] `make` / `cargo build` 均产出固件
- [x] 双 bin 分开烧录成功
- [x] 两颗蓝灯交替闪烁（V5F 应用运行于 PE2/PE3）
- [x] RCC HB2PCENR(0x1C)/IOPEEN(bit6) 实测有效
- [x] Rust 版烧录复核（2026-09-10；延时 150k 快版，用于区分当前运行固件）
