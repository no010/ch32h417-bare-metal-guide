# step-0-minimal (C) — startup.S
# 改编自 openwch/ch32h417 EVT startup_ch32h417_v5f.S（版权归南京沁恒微电子）
# 为教学目的裁剪：仅设栈指针、拷贝 .data、清零 .bss、跳入 main。
# 不设向量表/mtvec——本章不含中断，第 06 章补齐。

    .section .init
    .global _start

_start:
    la      sp, _eusrstack

    # .data: 从 Flash(_sidata) 拷到 RAM(_sdata.._edata)
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
    # .bss: 清零 _sbss.._ebss
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
