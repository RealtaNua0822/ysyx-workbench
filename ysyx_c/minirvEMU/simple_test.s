.global _start
_start:
    lui x10, 0x1
    addi x10, x10, 0x123
    addi x11, x0, 0x456
    add x12, x10, x11
    addi x2, x0, 0x10
    sw x12, 0(x2)
    lw x13, 0(x2)
    addi x1, x0, 0x30
    jalr x0, 0(x1)
    ebreak
    .org 0x30
    addi x14, x0, 0x789
    ebreak
