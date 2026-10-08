# step-0-minimal (C) — waker.S：V3F 唤醒器
# H417 双核启动的关键：Boot ROM 只拉起 V3F（coreid 0），V5F 保持复位，
# 需要 V3F 在 PFIC 里登记 V5F 入口地址后 CPU 才被释放（序列与 Zephyr
# soc/wch/ch32v/ch32h41x/soc_v3f.c 的已验证实现一致）。
# 纯汇编、无数据段：链接到 0x00000000，重启后第一条指令就是它。
# 每步自包含：后续各 step 目录内各带一份内容相同的副本。

    .section .init
    .global _start

_start:
    li      sp, 0x20102800        # V3F 侧临时栈顶（本程序无函数调用，仅占位）

    li      t0, 0x08010000        # V5F 镜像入口（Flash 别名窗口，物理同 0x00010000）
    li      t1, 0xe000e724        # PFIC->WAKEIP[1]
    sw      t0, 0(t1)

    li      t1, 0xe000ed10        # PFIC->SCTLR
    lw      t3, 0(t1)
    li      t2, (1 << 5)          # SLEEPONEXIT：唤醒 V5F 后 V3F 自动入睡
    or      t3, t3, t2
    sw      t3, 0(t1)

1:  wfi                           # 此后 V5F 接管，V3F 永眠
    j       1b
