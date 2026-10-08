/*
 * step-1-blinky (C) — main.c
 * 状态: 🧪 可编译，待上板验证（LED1=PE2、LED2=PE3，低电平有效——来自用户板 J3 实际跳线）
 * 地址来源: ch32-riscv-ug/ch32-device-data evidence/memory_map.csv（F1 风格布局）
 * 偏移/位段: RCC/GPIO 沿用 step-0 的推测布局，本次上板即是对它们的检验
 */
#include <stdint.h>

#define RCC_BASE      0x40021000UL
#define GPIOE_BASE    0x40011800UL    /* GPIOA-F 依次 0x40010800 + n*0x400 */

#define RCC_HB2PCENR  (*(volatile uint32_t *)(RCC_BASE + 0x1C))  /* SVD: F1 式 APB2 使能在 H417 落在 0x1C，0x18 是 HBPCENR */
#define RCC_IOPEEN    (1UL << 6)      /* IOPEEN = HB2PCENR bit6 */
#define GPIOE_CFGLR   (*(volatile uint32_t *)(GPIOE_BASE + 0x00))
#define GPIOE_BSHR    (*(volatile uint32_t *)(GPIOE_BASE + 0x10))  /* 高 16 位清零(灭->亮)，低 16 位置一 */

/* 引脚在 CFGLR 中的 4 位组: PE2 -> bit8, PE3 -> bit12 */
#define CFG_PE2       (0x3UL << 8)    /* MODE=11(输出) CNF=00(推挽) */
#define CFG_PE3       (0x3UL << 12)
#define LED1_ON       (1UL << (16 + 2))  /* BSHR.BR2: PE2 拉低 = LED1 亮 */
#define LED1_OFF      (1UL << 2)         /* BSHR.BS2: PE2 拉高 = LED1 灭 */
#define LED2_ON       (1UL << (16 + 3))  /* BSHR.BR3: PE3 拉低 = LED2 亮 */
#define LED2_OFF      (1UL << 3)         /* BSHR.BS3: PE3 拉高 = LED2 灭 */

static void delay(volatile uint32_t n)
{
    while (n--) {
        __asm__ volatile ("nop");
    }
}

int main(void)
{
    /* 使能 GPIOE 时钟 */
    RCC_HB2PCENR |= RCC_IOPEEN;

    /* PE2/PE3 推挽输出，其余位保持复位值 */
    GPIOE_CFGLR = (GPIOE_CFGLR & ~(0xFFUL << 8)) | CFG_PE2 | CFG_PE3;

    for (;;) {
        GPIOE_BSHR = LED1_ON | LED2_OFF;
        delay(500000);
        GPIOE_BSHR = LED1_OFF | LED2_ON;
        delay(500000);
    }
}
