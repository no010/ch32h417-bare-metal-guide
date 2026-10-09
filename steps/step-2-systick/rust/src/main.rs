//! step-2-systick (Rust) — SysTick1 精确 1ms 节拍，LED 每秒交替，零第三方依赖。
//!
//! 状态: ✅ 已上板验证（与 C 轨同一实测）。STK 布局来源：ch32-rs `svd/fixed/ch32h417.svd` +
//! Zephyr `dts/riscv/wch/qingke-v5f.dtsi`；H417 有两路全局系统计数器，每核一路：
//! STK0 @0xE000F000（IRQ12）归 V3F，STK1 @0xE000F080（IRQ13）归 V5F——
//! V5F 镜像必须用 STK1，否则节拍中断发给正在睡觉的 V3F。计数速率实测见 README。
#![no_std]
#![no_main]

use core::arch::global_asm;
use core::ptr::{read_volatile, write_volatile};

// 极简启动：与 C 版 startup.S 逐行对应。
global_asm!(
    r"
    .section .init
    .global _start
_start:
    la      sp, _eusrstack

    la      t0, _sidata
    la      t1, _sdata
    la      t2, _edata
1:
    bgeu    t1, t2, 2f
    lw      t3, 0(t0)
    sw      t3, 0(t1)
    addi    t0, t0, 4
    addi    t1, t1, 4
    j       1b
2:
    la      t0, _sbss
    la      t1, _ebss
3:
    bgeu    t0, t1, 4f
    sw      zero, 0(t0)
    addi    t0, t0, 4
    j       3b
4:
    call    main
5:
    j       5b
    "
);

/* GPIO：沿用第 03 章实测事实（RCC 使能 = HB2PCENR 0x1C，GPIOE = 0x40011800） */
const RCC_HB2PCENR: *mut u32 = 0x4002_101C as *mut u32;
const RCC_IOPEEN: u32 = 1 << 6;
const GPIOE_CFGLR: *mut u32 = 0x4001_1800 as *mut u32;
const GPIOE_BSHR: *mut u32 = 0x4001_1810 as *mut u32;

const LED1_ON: u32 = 1 << (16 + 2);
const LED1_OFF: u32 = 1 << 2;
const LED2_ON: u32 = 1 << (16 + 3);
const LED2_OFF: u32 = 1 << 3;

/* SysTick1（V5F 专用）：CTLR/CNT/CMP 在 0x80 窗口内；标志位 STK1 在 ISR bit1，清标志写 0（实测） */
const STK_ISR: *mut u32 = 0xE000_F004 as *mut u32;
const STK1_CTLR: *mut u32 = 0xE000_F080 as *mut u32;
const STK1_CNT: *mut u32 = 0xE000_F088 as *mut u32;
const STK1_CMP: *mut u32 = 0xE000_F090 as *mut u32;

const STK_EN: u32 = 1 << 0;
const STK_NO_RTC: u32 = 1 << 2;
const STK_AUTO_RLD: u32 = 1 << 3;
const STK1_FLAG: u32 = 1 << 1;

/* 实测：NO_RTC=1 档约 100.72 MHz（逐段 100.2~101.0 MHz 的均值，见 README 测频实录） */
const TICK_HZ: u32 = 100_720_000;
const TICKS_PER_MS: u32 = TICK_HZ / 1000;

#[no_mangle]
pub extern "C" fn main() -> ! {
    unsafe {
        write_volatile(RCC_HB2PCENR, read_volatile(RCC_HB2PCENR) | RCC_IOPEEN);
        let cfglr = read_volatile(GPIOE_CFGLR);
        write_volatile(GPIOE_CFGLR, (cfglr & !(0xFF << 8)) | (0x3 << 8) | (0x3 << 12));

        write_volatile(STK1_CMP, TICKS_PER_MS - 1);
        write_volatile(STK1_CNT, 0);
        write_volatile(STK1_CTLR, STK_EN | STK_NO_RTC | STK_AUTO_RLD);
        write_volatile(STK_ISR, 0);
    }

    let mut ms = 0u32;
    let mut phase = 0u32;
    loop {
        unsafe {
            while read_volatile(STK_ISR) & STK1_FLAG == 0 {
                // 等一个 1ms 节拍
            }
            write_volatile(STK_ISR, 0);
        }
        ms += 1;
        if ms >= 1000 {
            ms = 0;
            phase ^= 1;
            unsafe {
                write_volatile(
                    GPIOE_BSHR,
                    if phase != 0 {
                        LED1_ON | LED2_OFF
                    } else {
                        LED1_OFF | LED2_ON
                    },
                );
            }
        }
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}
