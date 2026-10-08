//! step-1-blinky (Rust) — 双 LED 交替闪烁，零第三方依赖。
//!
//! 状态: 🧪 可编译，待上板验证（LED1=PE2、LED2=PE3，低电平有效——来自用户板 J3 实际跳线）。
//! 地址来源: ch32-riscv-ug/ch32-device-data evidence/memory_map.csv（F1 风格布局）。
//!
//! 双核启动: V5F 不会被 Boot ROM 拉起，需要 V3F 唤醒器先行（见 ../c/waker.S，
//! 两轨共用同一 waker.bin）。合并与烧录步骤见本目录上级 README。
#![no_std]
#![no_main]

use core::arch::{asm, global_asm};
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

// 寄存器已上板验证（2026-09-09）：RCC HB2PCENR(0x1C)、GPIOE F1 风格寄存器组
const RCC_HB2PCENR: *mut u32 = 0x4002_101C as *mut u32;
const RCC_IOPEEN: u32 = 1 << 6;
const GPIOE_CFGLR: *mut u32 = 0x4001_1800 as *mut u32;
const GPIOE_BSHR: *mut u32 = 0x4001_1810 as *mut u32;

/* BSHR: 低 16 位 = 置一（拉高，灯灭）；高 16 位 = 清零（拉低，灯亮） */
const LED1_ON: u32 = 1 << (16 + 2); // PE2 拉低
const LED1_OFF: u32 = 1 << 2; // PE2 拉高
const LED2_ON: u32 = 1 << (16 + 3); // PE3 拉低
const LED2_OFF: u32 = 1 << 3; // PE3 拉高

#[no_mangle]
pub extern "C" fn main() -> ! {
    unsafe {
        write_volatile(RCC_HB2PCENR, read_volatile(RCC_HB2PCENR) | RCC_IOPEEN);

        /* PE2/PE3 推挽输出: MODE=11 CNF=00，每脚占 CFGLR 中 4 位 */
        let cfglr = read_volatile(GPIOE_CFGLR);
        write_volatile(GPIOE_CFGLR, (cfglr & !(0xFF << 8)) | (0x3 << 8) | (0x3 << 12));
    }

    loop {
        unsafe {
            write_volatile(GPIOE_BSHR, LED1_ON | LED2_OFF);
        }
        delay(150_000);
        unsafe {
            write_volatile(GPIOE_BSHR, LED1_OFF | LED2_ON);
        }
        delay(150_000);
    }
}

fn delay(n: u32) {
    for _ in 0..n {
        unsafe { asm!("nop") }
    }
}

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    loop {
        core::hint::spin_loop();
    }
}
