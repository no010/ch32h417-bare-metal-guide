/*
 * step-0-minimal (C) — main.c
 * 状态: 🧪 可编译，寄存器用法待上板验证（PB1 与官方 GPIO_Toggle 例程同脚位）
 * 地址来源: ch32-riscv-ug/ch32-device-data evidence/memory_map.csv（取自 WCH 设备头文件）
 * 偏移/位段: 按 F1 风格布局推测，上板时需对照 CH32H417RM 逐位核实
 */
#include <stdint.h>

#define RCC_BASE      0x40021000UL
#define GPIOB_BASE    0x40010C00UL

#define RCC_APB2PCENR (*(volatile uint32_t *)(RCC_BASE + 0x18))
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
    /* 使能 GPIOB 时钟 */
    RCC_APB2PCENR |= RCC_IOPBEN;

    /* PB1 推挽输出: MODE=11(输出) CNF=00(推挽)，每脚占 CFGLR 中 4 位 */
    GPIOB_CFGLR = (GPIOB_CFGLR & ~(0xFUL << 4)) | (0x3UL << 4);

    for (;;) {
        GPIOB_OUTDR ^= (1UL << 1);
        delay(1000000);
    }
}
