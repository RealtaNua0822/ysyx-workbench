// 修正后的完整代码
#include <stdio.h>
#include <stdint.h>

// 指令格式定义
#define OP_ADD   0x0  // 00 - ADD RD, RS1, RS2
#define OP_LI    0x1  // 01 - LI RD, imm  
#define OP_BNER0 0x3  // 11 - BNER0 ADDR, RS2

// 提取指令字段
#define GET_OP(inst)   ((inst) >> 6)
#define GET_RD(inst)   (((inst) >> 4) & 0x3)
#define GET_RS1(inst)   (((inst) >> 2) & 0x3)
#define GET_RS2(inst)   ((inst) & 0x3)
#define GET_IMM(inst) ((inst) & 0xF)
#define GET_ADDR(inst)   (((inst) >> 2) & 0xF)

int main() {
    // 初始化状态
    uint8_t PC = 0;
    uint8_t R[4] = {0, 0, 0, 0};
    uint8_t M[16] = {0};

    uint8_t program[] = {
        // 初始化：r1=10, r2=0, r3=1, r0=0
        0x5A,   // 10 01 1010: LI r1, 10
        0x60,   // 10 10 0000: LI r2, 0
        0x71,   // 10 11 0001: LI r3, 1
        0x40,   // 10 00 0000: LI r0, 0
        
        // 循环体开始 (PC=4)
        0x03,   // 00 00 00 11: ADD r0, r0, r3  // counter += 1
        0x28,   // 00 10 10 00: ADD r2, r2, r0  // sum += counter
        0xD1,   // 11 01 00 01: BNER0 ADDR, r1 // 如果 counter != 10
        
        // 如果BNER0不跳转，继续执行下一条
        0x00    // NOP - 程序结束
    };
    
    // 加载程序
    for (int i = 0; i < sizeof(program); i++) {
        M[i] = program[i];
    }
    
    printf("8-bit ISA Simulator - Corrected Version\n");
    printf("PC starts at 0\n\n");
    
    // 执行
    int max_cycles = 50;
    for (int cycle = 1; cycle <= max_cycles; cycle++) {
        uint8_t inst = M[PC];
        uint8_t op = GET_OP(inst);
        
        printf("Cycle %2d: PC=%2d, Inst=0x%02X -> ", cycle, PC, inst);
        
        switch (op) {
            case OP_ADD: {
                uint8_t RD = GET_RD(inst);
                uint8_t RS1 = GET_RS1(inst);
                uint8_t RS2 = GET_RS2(inst);
                R[RD] = R[RS1] + R[RS2];
                printf("ADD r%d=r%d+r%d=%d\n", RD, RS1, RS2, R[RD]);
                PC = (PC + 1) & 0xF;
                break;
            }
            case OP_LI: {
                uint8_t RD = GET_RD(inst);
                uint8_t IMM = GET_IMM(inst);
                R[RD] = IMM;
                printf("LI r%d=%d\n", RD, IMM);
                PC = (PC + 1) & 0xF;
                break;
            }
            case OP_BNER0: {
                uint8_t ADDR = GET_ADDR(inst);
                uint8_t RS2 = GET_RS2(inst);

                printf("BNER0 %d %d, counter是R[0]=%d\n", 
                        ADDR, RS2 , R[0]);
                
                if (R[0] != R[RS2]) {
                    PC = ADDR;
                    printf("TAKEN, PC=%d\n", PC);
                } else {
                    PC = (PC + 1) & 0xF;
                    printf("NOT taken, PC=%d\n", PC);
                }
                break;
            }
            default:
                printf("NOP\n");
                PC = (PC + 1) & 0xF;
                break;
        }
        
        // 显示寄存器状态
        printf("  Regs: [%d %d %d %d]\n\n", R[0], R[1], R[2], R[3]);
        
        // 停止条件：当循环完成时停止
        if (PC == 7) {
            printf("Program completed: counter reached 10\n");
            break;
        }
        
        // 安全停止条件
        if (cycle >= max_cycles - 1) {
            printf("Max cycles reached, stopping.\n");
            break;
        }
    }
    
    printf("\nFinal result: r2 = %d\n", R[2]);
    
    return 0;
}
