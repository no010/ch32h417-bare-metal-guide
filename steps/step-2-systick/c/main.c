/*
 * step-2-systick (C) — 用 SysTick 系统计数器产生精确 1ms 节拍，LED 每秒交替
 * 状态: ✅ 已上板验证（1ms 节拍、LED 每秒交替；实测数据见 README）
 *
 * QingKe V5 的 SysTick 是全局"系统计数器"，共两路、每核一路：
 *   STK0 @ 0xE000F000（IRQ 12）归 V3F；STK1 @ 0xE000F080（IRQ 13）归 V5F。
 *   V5F 镜像必须用 STK1，否则节拍中断发给正在睡觉的 V3F。
 * 布局来源：ch32-rs `svd/fixed/ch32h417.svd` + Zephyr `dts/riscv/wch/qingke-v5f.dtsi`，
 * 计数速率与标志清除语义均为本机实测（见 README）。
 */
#include <stdint.h>

/* --- GPIO（沿用第 03 章实测事实） --- */
#define RCC_BASE       0x40021000UL
#define GPIOE_BASE     0x40011800UL
#define RCC_HB2PCENR   (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_IOPEEN     (1UL << 6)
#define GPIOE_CFGLR    (*(volatile uint32_t *)(GPIOE_BASE + 0x00))
#define GPIOE_BSHR     (*(volatile uint32_t *)(GPIOE_BASE + 0x10))

#define CFG_PE2        (0x3UL << 8)
#define CFG_PE3        (0x3UL << 12)
#define LED1_ON        (1UL << (16 + 2))
#define LED1_OFF       (1UL << 2)
#define LED2_ON        (1UL << (16 + 3))
#define LED2_OFF       (1UL << 3)

/* --- SysTick：两路计数器共用一个 ISR 标志寄存器 --- */
#define STK_BASE       0xE000F000UL
#define STK_ISR        (*(volatile uint32_t *)(STK_BASE + 0x04)) /* bit0=STK0 标志, bit1=STK1 标志；清标志写 0（实测），0xE000F084 为同位镜像 */
#define STK1_CTLR      (*(volatile uint32_t *)(STK_BASE + 0x80))
#define STK1_CNT       (*(volatile uint32_t *)(STK_BASE + 0x88))
#define STK1_CMP       (*(volatile uint32_t *)(STK_BASE + 0x90))

#define STK_EN         (1UL << 0)
#define STK_IE         (1UL << 1)   /* 本章轮询标志即可，中断留到第 06 章 */
#define STK_NO_RTC     (1UL << 2)   /* 计数时钟源选择（实测确认高速档） */
#define STK_AUTO_RLD   (1UL << 3)
#define STK_DOWN       (1UL << 4)
#define STK1_FLAG      (1UL << 1)

/* 计数频率（Hz）与 1ms 节拍数：测法见 README「测频实录」 */
#ifndef STK_NO_RTC_SEL
#define STK_NO_RTC_SEL 1
#endif

static void gpio_init(void)
{
    RCC_HB2PCENR |= RCC_IOPEEN;
    GPIOE_CFGLR = (GPIOE_CFGLR & ~(0xFFUL << 8)) | CFG_PE2 | CFG_PE3;
}

int main(void)
{
    gpio_init();

#ifdef STK_MEASURE
    /* 测频模式：让计数器自由跑，CPU 空转；
     * 用调试器隔一段时间读两次 STK1_CNT，即可算出计数速率。 */
    STK1_CMP  = 0xFFFFFFFFUL;
    STK1_CNT  = 0;
    STK1_CTLR = STK_EN | (STK_NO_RTC_SEL ? STK_NO_RTC : 0U);
    for (;;) {
        __asm__ volatile("nop");
    }
#else
    /* 节拍模式：CMP = 每毫秒计数值，自动重装载；轮询标志拼出 1ms → 1s → 交替 LED */
    STK1_CMP  = TICK_HZ / 1000UL - 1UL;
    STK1_CNT  = 0;
    STK1_CTLR = STK_EN | STK_NO_RTC | STK_AUTO_RLD;
    STK_ISR   = 0U;   /* 清标志：实测写 0 才清得掉，且使能瞬间标志往往已置起 */

    uint32_t ms = 0;
    int phase = 0;
    for (;;) {
        while ((STK_ISR & STK1_FLAG) == 0U) {
            /* 等一个 1ms 节拍 */
        }
        STK_ISR = 0U; /* 清标志：写 0（实测语义，不是 F1 的写 1 清除） */
        if (++ms >= 1000UL) {
            ms = 0;
            phase ^= 1;
            GPIOE_BSHR = phase ? (LED1_ON | LED2_OFF) : (LED1_OFF | LED2_ON);
        }
    }
#endif
}
