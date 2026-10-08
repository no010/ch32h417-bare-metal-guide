/*
 * step-0-minimal — waker.ld（V3F 唤醒器，链接到 0x00000000）
 * 低 64KB 与 V5F 应用区（0x00010000 起）相接，LENGTH=64K 起越界保护作用。
 * 无 .data/.bss：唤醒器只用寄存器。每步自包含：后续各 step 各带一份副本。
 */
ENTRY(_start)

MEMORY
{
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
}

SECTIONS
{
    .init : {
        KEEP(*(SORT_NONE(.init)))
        *(.text .text.*)
    } > FLASH AT> FLASH

    /DISCARD/ : { *(.data .data.*) *(.bss .bss.*) *(.rodata .rodata.*) *(.sdata*) *(.sbss*) *(.comment) *(.riscv.attributes) }
}
