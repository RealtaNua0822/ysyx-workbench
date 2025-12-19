.global _start
_start:
    # 非常简单的程序：直接进入死循环
    # 让模拟器自己来绘制图案
    
    # 加载framebuffer地址到a0
    lui a0, 0x20000        # a0 = 0x20000000
    
    # 写入一个蓝色像素到第一个位置
    li a1, 0x000000FF      # 蓝色
    sw a1, 0(a0)
    
    # 写入一个绿色像素到第二个位置
    li a1, 0x0000FF00      # 绿色
    sw a1, 4(a0)
    
    # 写入一个红色像素到第三个位置
    li a1, 0x00FF0000      # 红色
    sw a1, 8(a0)
    
    # 进入死循环
loop:
    j loop
