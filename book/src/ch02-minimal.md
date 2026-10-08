# 02 最小固件：从上电到 `main`

本章目标：不借助任何库、不用一行"外设库函数"，让一段最小固件在 CH32H417 的 V5F 核上跑起来。这个过程要回答三个问题：

1. **双核芯片上电后谁先跑？**
2. 芯片上电后，CPU 执行的第一条指令从哪来？
3. `main` 函数运行之前，内存里发生了什么？

代码在 [`steps/step-0-minimal`](../../steps/step-0-minimal)，C 与 Rust 双轨、零第三方依赖。

## 先修课：双核芯片上电后谁先跑

单核芯片（比如 STM32）上电后 CPU 直接从复位地址取指。**H417 不是**：

> **Boot ROM 只会拉起 V3F（coreid 0）。V5F（coreid 1，本例程运行的核心）上电后一直躺在复位里，等别人来唤醒。**

这是双核芯片与单核芯片最大的差异，也是"程序烧了却不跑"的头号原因。释放 V5F 的开关在 PFIC（QingKe 的中断控制器）里，V3F 要按顺序做三件事：

```asm
li      t0, 0x08010000        # V5F 入口地址（Flash 别名窗口）
li      t1, 0xe000e724        # PFIC->WAKEIP[1]
sw      t0, 0(t1)             # ① 登记 V5F 的入口地址

li      t1, 0xe000ed10        # PFIC->SCTLR
lw      t3, 0(t1)
li      t2, (1 << 5)          # ② SLEEPONEXIT：唤醒 V5F 后 V3F 自动入睡
or      t3, t3, t2
sw      t3, 0(t1)

1:  wfi                       # ③ V3F 功成身退
```

这三步就是 `steps/step-0-minimal/c/waker.S` 的全部（每步自包含，后续每章目录里都带一份副本）。两个细节：

- 入口写的是 `0x08010000` 而不是 `0x00010000`。H417 的 Flash 有两套地址：物理窗口从 `0x00000000` 起、别名窗口从 `0x08000000` 起，指向同一块存储——唤醒寄存器要的是别名。
- 该序列与 Zephyr 上游 `soc/wch/ch32v/ch32h41x/soc_v3f.c` 的实现一致，我们上板实测有效（验证方法见后文）。

V3F 唤醒 V5F 后自己睡去（`SLEEPONEXIT`），此后所有代码只跑在 V5F 上。

## 内存地图（V5F 视角）

| 区域 | 地址 | 用途 |
|------|------|------|
| V3F 入口 | `0x00000000` | Boot ROM 从这启动 V3F（唤醒器住在这里） |
| V5F 应用区 | `0x00010000`（别名 `0x08010000`） | 本章固件，480KB |
| ITCM | `0x200A0000` | 高速指令 RAM（官方把 `.text` 搬这里提速，第 07 章） |
| DTCM | `0x200C0400` 起 | 数据 RAM，前 1KB 留给硬件压栈 |
| 栈顶 | `0x20100000` | 本例程初始 `sp`（栈向下生长） |

来源：官方 EVT 链接脚本 + 社区考据数据（ch32-riscv-ug），已上板验证。

## 启动代码：`_start` 到 `main` 之间

C 语言有个隐含契约：进入 `main` 之前，运行时环境必须就绪。裸机上没人替你干，所以启动代码要亲手做三件事：

```asm
_start:
    la      sp, _eusrstack        # ① 立起栈指针
    # ② 把 .data 从 Flash 拷到 RAM（_sidata → _sdata.._edata）
    # ③ 把 .bss 清零（_sbss.._ebss）
    call    main                  # ④ 进 main
```

- **栈指针**：函数调用、局部变量都依赖 `sp`。`_eusrstack` 指向 RAM 顶端，由链接脚本算出。
- **`.data` 拷贝**：有初值的全局/静态变量（`int x = 5;`）初值存在 Flash，运行时必须拷进 RAM——RAM 掉电不保留、Flash 又不可写。
- **`.bss` 清零**：没写初值的全局变量按 C 标准应为 0，但 RAM 里是上电残留。不清零的 bug 最阴险：同块板子重启十次九次正常，第十次出鬼。
- `main` 永不返回（我们的 `main` 是死循环，`-> !`）。

Rust 轨用 `global_asm!` 内嵌了同一段逻辑（`rust/src/main.rs`）——对照读会发现，"启动"这件事与语言无关，只与 ABI 约定有关。

## 链接脚本：给链接器画一张地图

`link_v5f.ld` 告诉链接器每样东西放哪里：

```ld
MEMORY {
    FLASH (rx)  : ORIGIN = 0x00010000, LENGTH = 480K   /* V5F 应用区 */
    RAM   (rwx) : ORIGIN = 0x200C0400, LENGTH = 255K   /* DTCM，前 1KB 硬件压栈保留 */
}
```

关键点逐条：

- `.init` 段 `KEEP()`：即使"没人调用"，`_start` 也不能被 `--gc-sections` 回收。
- `.data > RAM AT> FLASH`：**运行地址在 RAM、加载地址在 Flash**——这就是启动代码要拷贝的原因；`_sidata = LOADADDR(.data)` 正是 Flash 侧的那个源地址。
- `.bss (NOLOAD)`：不占 Flash 空间，只占 RAM。
- `_eusrstack = ORIGIN(RAM) + LENGTH(RAM)`：栈顶 = RAM 最高地址。
- 与官方工程的差异：官方把 `.text` 搬进 ITCM 执行提速，我们让代码在 Flash 原地跑——少一层搬运，多一个话题（第 07 章）。

## `main`：最少的寄存器操作

`main.c`（与 Rust 版 `main.rs`）做的事极少——使能 GPIOB 时钟、把 PB1 配成推挽输出、循环翻转：

```c
RCC_HB2PCENR |= RCC_IOPBEN;                                   /* 外设时钟先开 */
GPIOB_CFGLR = (GPIOB_CFGLR & ~(0xFUL << 4)) | (0x3UL << 4);   /* PB1 推挽输出 */
for (;;) { GPIOB_OUTDR ^= (1UL << 1); delay(1000000); }
```

> 注意 `RCC_HB2PCENR` 在 `RCC+0x1C`——**不是 STM32 F1 的 `0x18`**。这个坑够写一页排障笔记，第 03 章细讲。

## 构建与烧录

```bash
# C（同时产出 V3F 唤醒器 c/waker.bin）
make -C steps/step-0-minimal/c            # MRS2 用户：make CROSS=riscv-wch-elf-（见第 01 章）

# Rust
cd steps/step-0-minimal/rust && cargo build --release
```

烧录**必须两步**（两个 bin 分开烧）：

```bash
wlink flash -e -a 0x08000000 c/waker.bin    # 唤醒器，带整片擦除
wlink flash    -a 0x08010000 c/step0.bin    # V5F 应用，不再擦
```

为什么不做合并镜像一次烧？实测：65KB 的合并大文件走 `fastprogram` 会报 `[41,01,01,05]`，两个 bin 分开烧最稳。完整的坑与救援办法都记在 `steps/step-0-minimal/README.md` 的排障表里。

## 怎么知道它真的在跑

PB1 在板上是裸引脚（板载用户 LED 接在 PE2/PE3，第 03 章才用到），最直接的验证是示波器。没有示波器也有一条替代路线——**用调试器 halt 内核、直接读 GPIOB 寄存器**，多点采样看 `OUTDR`/`INDR` 的 bit1 翻转，同时核对 `HB2PCENR` 的时钟位与 `CFGLR` 的输出配置：

```
pc (/32): 0x0801008a
0x4002101c:	0x00000008   # HB2PCENR: IOPBEN(bit3)=1 —— GPIOB 时钟已开
0x40010c00:	0x44444434   # CFGLR: PB1 半字节=3 —— 推挽输出
0x40010c08:	0x0000031a   # INDR:  PB1=1 —— 引脚实际电平为高
0x40010c0c:	0x00000002   # OUTDR: PB1=1 —— 输出寄存器为高
```

连续采样中 `OUTDR` bit1 出现 `0 0 1 0 0 1` 的交替，`INDR` 同步跟随——输出值真的到了引脚上。完整命令、原始输出、以及两个附带踩出来的坑（这套 OpenOCD 的单步会把 PC 写飞、读-改-写会带入复位残值）见 [`step-0-minimal/README.md`](../../steps/step-0-minimal/README.md) 的「上板验证实录」。

## 本章刻意做的简化

| 简化 | 官方工程的做法 | 在哪补 |
|------|----------------|--------|
| 不设中断向量表 / 不配 `mtvec` | 完整向量表 + PFIC | 第 06 章 |
| `.text` 在 Flash 原地执行 | 拷入 ITCM 提速 | 第 07 章 |
| 时钟用复位默认值 | HSE + PLL 到 400MHz | 第 07 章 |
| 忙等延时（100 万次空转） | 定时器 / SysTick | 第 04 章 |

## 小结

- 双核启动的钥匙在 PFIC：`WAKEIP[1]` 登记 V5F 入口 + `SLEEPONEXIT` 让 V3F 退场（`waker.S` 三步曲）。
- 启动代码 + 链接脚本构成 C 运行时的"隐形契约"；Rust 借同一套契约，两轨代码几乎逐行对应。
- 烧录两个 bin：唤醒器住低 64KB，应用住 `0x00010000`。
- 验证不止示波器一条路：调试器读寄存器同样能"看见"翻转。

下一章：把 PB1 换成人看得见的信号——GPIO 与两颗用户 LED。
