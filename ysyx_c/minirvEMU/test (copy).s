.global _start
_start:
    addi a0, zero, 20      # a0 = 20
    jalr ra, 16(zero)      # 跳转到 fun 函数 (地址 0x10)
    jalr ra, 12(zero)      # 跳转到 halt (地址 0x0c)

halt:
    jalr zero, 12(zero)    # 无限循环

fun:
    addi a0, a0, 10       # a0 += 10
    jalr zero, 0(ra)       # 返回调用者
