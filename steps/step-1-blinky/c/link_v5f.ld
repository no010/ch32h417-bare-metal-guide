/*
 * step-0-minimal — link_v5f.ld（V5F 核精简链接脚本，C/Rust 两版共用同一布局）
 *
 * 内存布局与 openwch/ch32h417 EVT 的 Link_v5f.ld 一致：
 *   - V5F 用户程序位于 CodeFlash 0x00010000（低 64KB 为 BootLoader/信息区）
 *   - 数据放 DTCM，起点 0x200C0400（前 1KB 为硬件压栈保留）
 * 与官方版的差异：官方把 .text 搬进 ITCM(0x200A0000) 执行以提速；
 * 本脚本让代码在 Flash 原地执行——少一次搬运，多一个教学话题（第 07 章）。
 */
ENTRY(_start)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00010000, LENGTH = 480K
    RAM   (rwx) : ORIGIN = 0x200C0400, LENGTH = 255K
}

SECTIONS
{
    .init : {
        KEEP(*(SORT_NONE(.init)))
    } > FLASH AT> FLASH

    .text : {
        *(.text .text.*)
    } > FLASH AT> FLASH

    .rodata : {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } > FLASH AT> FLASH

    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data .data.*)
        *(.sdata .sdata.*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH
    _sidata = LOADADDR(.data);

    .bss (NOLOAD) : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    _eusrstack = ORIGIN(RAM) + LENGTH(RAM);
}
