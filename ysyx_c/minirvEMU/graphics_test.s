.global _start
_start:
    # ============================================
    # 测试基本指令
    # ============================================
    
    # 1. LUI指令测试
    lui x1, 0x10000        # x1 = 0x10000000
    
    # 2. ADDI指令测试
    addi x2, x0, 0x123     # x2 = 0x123
    addi x3, x0, 0x456     # x3 = 0x456
    
    # 3. ADD指令测试
    add x4, x2, x3         # x4 = 0x579
    
    # 4. 存储指令测试
    addi x5, x0, 0x100     # 内存地址 0x100
    sw x4, 0(x5)           # 存储到内存
    
    # 5. 加载指令测试
    lw x6, 0(x5)           # 从内存加载到x6
    
    # ============================================
    # 初始化Framebuffer地址
    # ============================================
    lui x7, 0x20000        # 设置FB_BASE = 0x20000000
    addi x7, x7, 0x0       # x7 = 0x20000000 (framebuffer基地址)
    
    # ============================================
    # 绘制彩色方块
    # ============================================
    
    # 1. 绘制红色方块 (左上角 50x50)
    li x8, 0xFF0000FF      # 红色: ARGB = 0xFF0000FF
    li x9, 50              # 高度循环计数器
draw_red_outer:
    li x10, 50             # 宽度循环计数器
    li x11, 0              # 当前列索引
    
draw_red_inner:
    # 计算像素偏移: (y * 400 + x) * 4
    # x11 = 列 (0-49), x12 = 当前行
    addi x12, x9, -50      # x12 = -50 ~ -1
    neg x12, x12           # x12 = 0 ~ 49 (行索引)
    
    li x13, 400            # 屏幕宽度
    mul x14, x12, x13      # y * 400
    add x14, x14, x11      # y * 400 + x
    slli x14, x14, 2       # 乘以4 (每个像素4字节)
    
    # 计算像素地址
    add x15, x7, x14       # 像素地址 = FB_BASE + 偏移
    
    # 写入红色像素
    sw x8, 0(x15)
    
    addi x11, x11, 1
    addi x10, x10, -1
    bnez x10, draw_red_inner
    
    addi x9, x9, -1
    bnez x9, draw_red_outer
    
    # 2. 绘制绿色方块 (右上角 50x50)
    li x8, 0xFF00FF00      # 绿色: ARGB = 0xFF00FF00
    li x9, 50              # 高度循环计数器
draw_green_outer:
    li x10, 50             # 宽度循环计数器
    li x11, 350            # 列起始位置 (400-50=350)
    
draw_green_inner:
    # 计算像素偏移
    addi x12, x9, -50      # x12 = -50 ~ -1
    neg x12, x12           # x12 = 0 ~ 49 (行索引)
    
    li x13, 400            # 屏幕宽度
    mul x14, x12, x13      # y * 400
    add x14, x14, x11      # y * 400 + x
    slli x14, x14, 2       # 乘以4
    
    # 计算像素地址
    add x15, x7, x14       # 像素地址 = FB_BASE + 偏移
    
    # 写入绿色像素
    sw x8, 0(x15)
    
    addi x11, x11, 1
    addi x10, x10, -1
    bnez x10, draw_green_inner
    
    addi x9, x9, -1
    bnez x9, draw_green_outer
    
    # 3. 绘制蓝色方块 (左下角 50x50)
    li x8, 0xFFFF0000      # 蓝色: ARGB = 0xFFFF0000
    li x9, 50              # 高度循环计数器
draw_blue_outer:
    li x10, 50             # 宽度循环计数器
    li x11, 0              # 列起始位置
    
draw_blue_inner:
    # 计算像素偏移 (行从250开始)
    addi x12, x9, -50      # x12 = -50 ~ -1
    neg x12, x12           # x12 = 0 ~ 49
    addi x12, x12, 250     # 行 = 250 ~ 299
    
    li x13, 400            # 屏幕宽度
    mul x14, x12, x13      # y * 400
    add x14, x14, x11      # y * 400 + x
    slli x14, x14, 2       # 乘以4
    
    # 计算像素地址
    add x15, x7, x14       # 像素地址 = FB_BASE + 偏移
    
    # 写入蓝色像素
    sw x8, 0(x15)
    
    addi x11, x11, 1
    addi x10, x10, -1
    bnez x10, draw_blue_inner
    
    addi x9, x9, -1
    bnez x9, draw_blue_outer
    
    # 4. 绘制黄色方块 (右下角 50x50)
    li x8, 0xFFFFFF00      # 黄色: ARGB = 0xFFFFFF00
    li x9, 50              # 高度循环计数器
draw_yellow_outer:
    li x10, 50             # 宽度循环计数器
    li x11, 350            # 列起始位置 (400-50=350)
    
draw_yellow_inner:
    # 计算像素偏移 (行从250开始)
    addi x12, x9, -50      # x12 = -50 ~ -1
    neg x12, x12           # x12 = 0 ~ 49
    addi x12, x12, 250     # 行 = 250 ~ 299
    
    li x13, 400            # 屏幕宽度
    mul x14, x12, x13      # y * 400
    add x14, x14, x11      # y * 400 + x
    slli x14, x14, 2       # 乘以4
    
    # 计算像素地址
    add x15, x7, x14       # 像素地址 = FB_BASE + 偏移
    
    # 写入黄色像素
    sw x8, 0(x15)
    
    addi x11, x11, 1
    addi x10, x10, -1
    bnez x10, draw_yellow_inner
    
    addi x9, x9, -1
    bnez x9, draw_yellow_outer
    
    # ============================================
    # 绘制白色中心方块
    # ============================================
    li x8, 0xFFFFFFFF      # 白色: ARGB = 0xFFFFFFFF
    li x9, 100             # 高度循环计数器 (100x100方块)
draw_white_outer:
    li x10, 100            # 宽度循环计数器
    li x11, 150            # 列起始位置 (居中: (400-100)/2=150)
    
draw_white_inner:
    # 计算像素偏移 (行从100开始)
    addi x12, x9, -100     # x12 = -100 ~ -1
    neg x12, x12           # x12 = 0 ~ 99
    addi x12, x12, 100     # 行 = 100 ~ 199
    
    li x13, 400            # 屏幕宽度
    mul x14, x12, x13      # y * 400
    add x14, x14, x11      # y * 400 + x
    slli x14, x14, 2       # 乘以4
    
    # 计算像素地址
    add x15, x7, x14       # 像素地址 = FB_BASE + 偏移
    
    # 写入白色像素
    sw x8, 0(x15)
    
    addi x11, x11, 1
    addi x10, x10, -1
    bnez x10, draw_white_inner
    
    addi x9, x9, -1
    bnez x9, draw_white_outer
    
    # ============================================
    # 测试结束，无限循环
    # ============================================
    j done

done:
    # 保持循环，不退出
    j done

# 辅助宏定义
.macro li reg, imm
    lui \reg, (\imm) >> 12
    addi \reg, \reg, (\imm) & 0xFFF
.endm
