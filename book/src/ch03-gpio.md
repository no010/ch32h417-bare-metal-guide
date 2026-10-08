# 03 GPIO：点亮一颗 LED

本章目标：让 EVT 板上的两颗蓝色用户 LED 交替闪烁——这是系列教程里第一次让代码产生"看得见"的现象。工程在 [`steps/step-1-blinky`](../../steps/step-1-blinky)，C 与 Rust 双轨。

## 板级事实：先把灯找对

板上 LED 不止一颗，先分清谁是谁（对照丝印）：

| 丝印 | 引脚 | 属性 |
|------|------|------|
| LED1（用户） | **PE2** | 蓝色，低电平有效 |
| LED2（用户） | **PE3** | 蓝色，低电平有效 |
| LED1 RED / LED3 GREEN | — | 电源指示灯（5V / 3.3V），不可编程 |

用户灯经 J3 插针跳线接到 MCU。"低电平有效"是最常见的灌电流接法：引脚输出低电平，电流从 3V3 经限流电阻、LED 流入引脚——灯亮；引脚拉高，灯灭。

## GPIO 是一排"配置格子"（F1 风格）

H417 的 GPIO 寄存器布局与 STM32 F1 同源：每根引脚占用配置寄存器里的 4 个位（MODE×2 + CNF×2）。先记住这张表：

| 寄存器 | 偏移 | 作用 |
|--------|------|------|
| `CFGLR` / `CFGHR` | 0x00 / 0x04 | 引脚 0–7 / 8–15 的模式与类型 |
| `INDR` | 0x08 | 读引脚电平 |
| `OUTDR` | 0x0C | 输出数据（读-改-写，非原子） |
| `BSHR` | 0x10 | **原子置位/复位**：低 16 位写 1 置位，高 16 位写 1 复位 |
| `BCR` | 0x14 | 原子复位（只清位） |

基地址从 `GPIOA=0x40010800` 起，每组 +0x400：GPIOB `0x40010C00`、GPIOC `0x40011000`、GPIOD `0x40011400`、**GPIOE `0x40011800`**。（来源：社区考据数据 + 上板实测。）

输出模式的编码：`MODE=11`（输出、50MHz）+ `CNF=00`（推挽）→ 半字节写 `0x3`。

## 第一个坑：时钟门不开，寄存器写得再对也不亮

外设默认没有时钟（省电），要先在 RCC 里"开门"。STM32 F1 的 APB2 使能寄存器在 `RCC+0x18`（`APB2PCENR`）——把这行照抄到 H417，代码会**静默地不工作**：

```c
/* 错误示范（F1 惯性写法） */
#define RCC_APB2PCENR  (*(volatile uint32_t *)(RCC_BASE + 0x18))
```

症状极具欺骗性：写入成功、读回正确、逻辑全对、**灯就是不亮**。原因：H417 把 F1 式的 APB2 使能挪到了 `RCC+0x1C`，并改名叫 `HB2PCENR`；`0x18` 是另一个寄存器（`HBPCENR`）——往别人家里放东西，当然没用。

```c
/* 正确姿势 */
#define RCC_HB2PCENR  (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_IOPEEN    (1UL << 6)      /* GPIOE 时钟位（GPIOB 是 bit3） */
RCC_HB2PCENR |= RCC_IOPEEN;
```

这个坑是怎么定位的？完整链路：灯不亮 → 调试器读寄存器，发现所有写入"看起来都对" → 怀疑时钟门 → 对照 ch32-rs 仓库的补丁版 SVD（`svd/fixed/ch32h417.svd`）逐位核对 → 发现偏移搬家 → 改 `0x1C` → 灯亮。**考据数据先于盲目试错**——这也是本教程反复强调的方法论。

## 点亮两颗灯：寄存器直写

PE2/PE3 配成推挽输出（`CFGLR` 里 PE2 占 bit[11:8]、PE3 占 bit[15:12]，各写 `0x3`）：

```c
GPIOE_CFGLR = (GPIOE_CFGLR & ~(0xFFUL << 8)) | (0x3UL << 8) | (0x3UL << 12);
```

翻转用 `BSHR` 而不是改 `OUTDR`：

```c
/* BSHR：低 16 位写 1 = 拉高（灯灭）；高 16 位写 1 = 拉低（灯亮） */
GPIOE_BSHR = LED1_ON | LED2_OFF;    /* (1UL<<18) | (1UL<<3)  */
delay(500000);
GPIOE_BSHR = LED1_OFF | LED2_ON;    /* (1UL<<2)  | (1UL<<19) */
delay(500000);
```

为什么偏爱 `BSHR`？`OUTDR ^= mask` 是"读-改-写"三步走，可能被中断（第 06 章）或多核竞争（第 09 章）拦腰打断，把别人的修改冲掉；`BSHR` 是硬件原子操作，一条写指令完成"只动这两根引脚"。单核裸机时差异不明显，双核芯片上是纪律。

Rust 轨完全同构，`write_volatile(GPIOE_BSHR, LED1_ON | LED2_OFF)`——顺便还能看到两门语言的性格差异：Rust 盯着 `u32` 的位运算语义（debug 构建溢出即 panic），C 那边就靠 `UL` 后缀自觉。

## 构建与烧录

与第 02 章同一套双核流程（唤醒器住 `0x08000000`，应用住 `0x08010000`）：

```bash
make -C steps/step-1-blinky/c                            # C: waker.bin + step1.bin
cd steps/step-1-blinky/rust && cargo build --release     # Rust 轨（objcopy 提取 bin）

wlink flash -e -a 0x08000000 c/waker.bin
wlink flash    -a 0x08010000 c/step1.bin
```

C 与 Rust 两版烧的是同一地址，板上只能跑一个。想区分当前跑的是哪版？我们给 Rust 版设了更快的节奏（延时 `150000` vs C 版 `500000`）——**闪得快的是 Rust 版**。

## 排障实录（上板踩过的坑）

| 现象 | 根因 | 解法 |
|------|------|------|
| 烧完毫无反应 | `RCC+0x18`（HBPCENR）不是 F1 的 APB2 使能，`0x1C`（HB2PCENR）才是 | 写 `RCC+0x1C`，IOPEEN=bit6 |
| 65KB 合并镜像 fastprogram 报 `[41,01,01,05]` | wlink 对大文件 / 跨区 fastprogram 不稳 | 两个 bin 分开烧 |
| 连不上：`0x55: failed to connect with riscvchip` | 芯片被上一次调试留在 halt 状态 | `wlink set-power disable3v3 && enable3v3` 断电重启 |

更多坑（包括 OpenOCD 单步会把 PC 写飞、`wlink status` 查询会把芯片留在 halt）见两个 step 目录的 README。

## 小结

- GPIO = F1 风格配置格子：每脚 4 位，`CFGLR/CFGHR` 配置，`BSHR` 原子置位/复位。
- H417 的坑：外设时钟使能寄存器搬了家（`0x1C`），照抄 F1 会"静默不工作"。
- 低电平有效 = 灌电流接法；`BSHR` 高 16 位写 1 清零，正好拉低点亮。
- 双核流程每章复用：waker + app 两个 bin；验证可以是示波器，也可以是调试器读寄存器。

下一章：把 `delay(500000)` 这种"数圈儿"的忙等，换成真正的系统时钟——SysTick。
