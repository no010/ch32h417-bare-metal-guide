# step-0-minimal：从上电到 main

> 状态：✅ 已上板验证（PB1 翻转，调试器读寄存器复核；实录见下）
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

PB1 输出方波（示波器可见；复位默认时钟下翻转较慢、周期在秒级，第 07 章讲时钟树后可精确控制）。

## 上板验证实录：不用示波器，读寄存器验证 PB1（2026-09-29）

PB1 在板上是裸引脚（板载 LED 接在 PE2/PE3，属第 03 章），没有示波器也能验：
**halt 内核，直接读 GPIOB 寄存器**，多点采样看电平翻转。

### 方法

1. 双 bin 烧录（见上节；烧录收尾自动复位，应用即运行）
2. 起 OpenOCD（MRS2 自带，双核配置 `h417-dualcore.cfg`，gdb 口 V3F:3333 / V5F:3334）
3. gdb 批量采样（每次：halt → 读 PC 与寄存器 → resume → detach），脚本本体：

   ```
   target extended-remote localhost:3334
   monitor halt
   monitor reg pc
   x/1xw 0x4002101C        # RCC HB2PCENR
   x/6xw 0x40010C00        # GPIOB: CFGLR CFGHR INDR OUTDR BSHR BCR
   monitor resume
   detach
   ```

   ```bash
   for i in 1 2 3 4 5 6; do riscv-none-embed-gdb -q -batch -x pb1-verify.gdb; sleep 0.4; done
   ```

### 实测输出（PB1=1 时刻的一次采样）

```
pc (/32): 0x0801008a
0x4002101c:	0x00000008   # HB2PCENR: IOPBEN(bit3)=1 —— GPIOB 时钟已开（0x1C 修正生效）
0x40010c00:	0x44444434   # CFGLR: PB1 半字节=3 —— 推挽输出
0x40010c04:	0x44444444   # CFGHR
0x40010c08:	0x0000031a   # INDR:  PB1(bit1)=1 —— 引脚实际电平为高
0x40010c0c:	0x00000002   # OUTDR: PB1(bit1)=1 —— 输出寄存器为高
```

连续 6 次采样的 `OUTDR` bit1：`0 0 1 0 0 1`（0x0/0x2），`INDR` bit1 同步跟随——
**输出值真的到达了引脚**，PB1 在翻转。PC 每次都停在 `0x0801008a`：
延时循环的第一条指令（循环体 4 条：`nop/mv/addi/bnez`，见反汇编）。

### 两个坑（实录附带）

- **这套 OpenOCD 驱动的单步是坏的**：`monitor step 1` 报
  `Written PC (0x1) does not match read back value (0x0)`，并把 V5F 的 PC 写飞——
  之后 PC 停在 `0x00000034`（V3F 唤醒器里 `wfi` 之后的自跳转指令），V5F 误执行
  唤醒器代码后睡死，寄存器数值全部冻结。**恢复：一次 `wlink reset` 复活**
  （复位后采样：PC 回到 `0x0801008a`，OUTDR 继续 0/1 交替）。
  教训：这套驱动上只做 halt/读内存/resume，别碰 step。
- **读-改-写会带入复位残值**：重启后 CFGLR 非 PB1 位从 `0x44444434` 变成 `0x00000030`
  （两次复位路径的初值不同，被 RMW 带进了结果）。PB1 半字节两次都为 3，结论不受影响；
  但配置引脚时不要依赖其他位的复位值。
- 另：`wlink status` 查询会把芯片留在 halt 状态，查询后补一次 `wlink reset`。

（附注：MRS2 的 gdb 8.2 在 detach 后会打印一段 internal-error，属收尾噪音，不影响读值。）

## 本章刻意做的简化

| 简化 | 官方工程的做法 | 详见 |
|------|----------------|------|
| 不设中断向量表 / 不配 `mtvec` | 完整向量表 + PFIC | 第 06 章 |
| `.text` 在 CodeFlash 原地执行 | 拷入 ITCM(0x200A0000) 提速 | 第 07 章 |
| 时钟用复位默认值 | HSE 25M + PLL 配置 | 第 07 章 |
| 不初始化 FPU/中断栈 | V5F 完整初始化 | 第 06/07 章 |

## 验证清单（上板时逐项打勾）

- [x] `make -C c` / `cargo build --release` 均产出固件（含 waker.bin）
- [x] wlink 双 bin 烧录成功（2026-09-29）
- [x] PB1 翻转（调试器读寄存器复核，实录见上；示波器复核可选）
- [x] `RCC_HB2PCENR(0x1C)` / GPIOB 偏移已按上板实测修正（step-1 验证，2026-09-09）
