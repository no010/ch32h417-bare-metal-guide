# step-1-blinky (C)
# 产物:
#   waker.bin        V3F 唤醒器   (烧到 0x00000000)
#   step1.bin        V5F 闪灯应用 (烧到 0x00010000)
#   step1_full.bin   两者合并镜像 (waker 填充到 64KB 后拼接，一次烧入)
# 工具链: 默认找 CI 同款 riscv64-unknown-elf-，MRS2 用户可
#   make CROSS=riscv-wch-elf- PATH="$(dirname <MRS2 GCC12>/bin):$$PATH"
CROSS   ?= riscv64-unknown-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy

CFLAGS  := -march=rv32imac -mabi=ilp32 -mcmodel=medany \
           -ffreestanding -nostdlib -O2 -Wall -Wextra -g

all: step1_full.bin

# --- V5F 闪灯应用 (0x00010000) ---
step1.elf: main.c startup.S link_v5f.ld
	$(CC) $(CFLAGS) -T link_v5f.ld -o $@ main.c startup.S -Wl,--gc-sections

step1.bin: step1.elf
	$(OBJCOPY) -O binary $< $@

# --- V3F 唤醒器 (0x00000000) ---
waker.elf: waker.S waker.ld
	$(CC) $(CFLAGS) -T waker.ld -o $@ waker.S -Wl,--gc-sections

waker.bin: waker.elf
	$(OBJCOPY) -O binary $< $@

# --- 合并镜像: waker 占满低 64KB，后接 V5F 应用 ---
step1_full.bin: waker.bin step1.bin
	cp waker.bin $@
	truncate -s 65536 $@
	cat step1.bin >> $@

clean:
	rm -f step1.elf step1.bin waker.elf waker.bin waker_pad.bin step1_full.bin

.PHONY: all clean
