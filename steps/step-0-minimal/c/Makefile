# step-0-minimal (C)
CROSS   ?= riscv64-unknown-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy

CFLAGS  := -march=rv32imac -mabi=ilp32 -mcmodel=medany \
           -ffreestanding -nostdlib -O2 -Wall -Wextra -g

all: step0.bin

step0.elf: main.c startup.S link_v5f.ld
	$(CC) $(CFLAGS) -T link_v5f.ld -o $@ main.c startup.S -Wl,--gc-sections

step0.bin: step0.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f step0.elf step0.bin

.PHONY: all clean
