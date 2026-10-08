/*
 * step-0-minimal (C) — main.c
 * 状态: 🧪 可编译，待上板复验 PB1 方波（PB1 与官方 GPIO_Toggle 例程同脚位）
 * 地址来源: ch32-riscv-ug/ch32-device-data evidence/memory_map.csv（取自 WCH 设备头文件）
 * 寄存器: RCC 使能 = HB2PCENR(RCC+0x1C)——F1 风格 APB2 使能在 H417 搬了家，
 *         0x18 是 HBPCENR；GPIO 寄存器组保持 F1 风格（均经 step-1 上板实测）
 */
#include <stdint.h>

#define RCC_BASE      0x40021000UL
#define GPIOB_BASE    0x40010C00UL

#define RCC_HB2PCENR  (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_IOPBEN    (1UL << 3)
#define GPIOB_CFGLR   (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OUTDR   (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

static void delay(volatile uint32_t n)
{
    while (n--) {
        __asm__ volatile ("nop");
    }
}

int main(void)
{
    /* 使能 GPIOB 时钟（HB2PCENR=0x1C；写成 0x18 是 HBPCENR，时钟门不开） */
    RCC_HB2PCENR |= RCC_IOPBEN;

    /* PB1 推挽输出: MODE=11(输出) CNF=00(推挽)，每脚占 CFGLR 中 4 位 */
    GPIOB_CFGLR = (GPIOB_CFGLR & ~(0xFUL << 4)) | (0x3UL << 4);

    for (;;) {
        GPIOB_OUTDR ^= (1UL << 1);
        delay(1000000);
    }
}
