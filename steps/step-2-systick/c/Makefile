# step-2-systick (C)
# 产物:
#   waker.bin     V3F 唤醒器 (0x00000000)
#   step2.bin     V5F 节拍应用 (0x00010000)
#   measure.bin   测频固件（计数器自由跑，供调试器读 CNT 求速率）
# 用法:
#   make measure.bin TICK_HZ=<x>     # 先测频
#   make TICK_HZ=<实测值>            # 再出节拍固件
CROSS   ?= riscv64-unknown-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy

CFLAGS  := -march=rv32imac -mabi=ilp32 -mcmodel=medany \
           -ffreestanding -nostdlib -O2 -Wall -Wextra -g

# 计数速率（Hz）——上板实测均值（NO_RTC=1 档），测法见 README「测频实录」
TICK_HZ ?= 100720000
# 测频固件的时钟源选择：1=NO_RTC（系统侧高速源），0=RTC 侧
STK_NO_RTC_SEL ?= 1

all: step2.bin waker.bin

# 改 TICK_HZ / STK_NO_RTC_SEL 这类命令行宏不会改变文件时间戳，
# 用 FORCE 让应用始终重编，避免烧到旧镜像。
step2.elf: main.c startup.S link_v5f.ld FORCE
	$(CC) $(CFLAGS) -DTICK_HZ=$(TICK_HZ)UL -T link_v5f.ld -o $@ main.c startup.S -Wl,--gc-sections

step2.bin: step2.elf
	$(OBJCOPY) -O binary $< $@

measure.elf: main.c startup.S link_v5f.ld FORCE
	$(CC) $(CFLAGS) -DSTK_MEASURE -DSTK_NO_RTC_SEL=$(STK_NO_RTC_SEL) -T link_v5f.ld -o $@ main.c startup.S -Wl,--gc-sections

measure.bin: measure.elf
	$(OBJCOPY) -O binary $< $@

waker.elf: waker.S waker.ld
	$(CC) $(CFLAGS) -T waker.ld -o $@ waker.S -Wl,--gc-sections

waker.bin: waker.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f step2.elf step2.bin measure.elf measure.bin waker.elf waker.bin

FORCE:

.PHONY: all clean FORCE
