.global _start
_start:
    # 初始化，将framebuffer基地址加载到寄存器
    lui a0, 0x20000        # 将0x20000000的高20位加载到a0 (0x20000 << 12 = 0x20000000)
    
    # 设置颜色值: 蓝色 (0x0000FF)
    li a1, 0xFF           # 加载蓝色值到a1的低8位
    
    # 设置循环计数器 (400*300 = 120000)
    li a2, 120000         # 像素总数
    
loop:
    # 将颜色写入framebuffer
    sw a1, 0(a0)          # 将a1的值写入a0指向的地址
    
    # 更新地址指针
    addi a0, a0, 4        # 移动到下一个像素(4字节)
    
    # 递减计数器
    addi a2, a2, -1       # 计数器减1
    
    # 检查是否继续循环
    bnez a2, loop         # 如果a2 != 0，跳转到loop
    
    # 程序结束
    ebreak

# 对齐到4字节边界
.align 4
