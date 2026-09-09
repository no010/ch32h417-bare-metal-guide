//! step-0-minimal (Rust) — 最小可运行固件，零第三方依赖。
//!
//! 状态: 🧪 可编译，寄存器用法待上板验证（PB1 与官方 GPIO_Toggle 例程同脚位）。
//! 地址来源: ch32-riscv-ug/ch32-device-data evidence/memory_map.csv（取自 WCH 设备头文件）。
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

// TODO(verif): 偏移/位段按 F1 风格布局推测，上板时需对照 CH32H417RM 逐位核实
const RCC_APB2PCENR: *mut u32 = 0x4002_1018 as *mut u32;
const RCC_IOPBEN: u32 = 1 << 3;
const GPIOB_CFGLR: *mut u32 = 0x4001_0C00 as *mut u32;
const GPIOB_OUTDR: *mut u32 = 0x4001_0C0C as *mut u32;
const PB1_OUT: u32 = 1 << 1;

#[no_mangle]
pub extern "C" fn main() -> ! {
    unsafe {
        write_volatile(RCC_APB2PCENR, read_volatile(RCC_APB2PCENR) | RCC_IOPBEN);

        let cfglr = read_volatile(GPIOB_CFGLR);
        write_volatile(GPIOB_CFGLR, (cfglr & !(0xF << 4)) | (0x3 << 4));
    }

    loop {
        unsafe {
            let out = read_volatile(GPIOB_OUTDR);
            write_volatile(GPIOB_OUTDR, out ^ PB1_OUT);
        }
        delay(1_000_000);
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
