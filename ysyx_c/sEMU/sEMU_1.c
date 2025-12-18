#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
// 指令格式定义
#define OP_ADD    0x0  // ADD RD, RS1, RS2
#define OP_LI     0x1  // LI RD, imm
#define OP_BNER0  0x3  // BNER0 ADDR, RS2
#define OP_OUT    0x2  // OUT RD

// 提取指令字段
#define GET_OP(inst)   ((inst) >> 6)
#define GET_RD(inst)   (((inst) >> 4) & 0x3)
#define GET_RS1(inst)  (((inst) >> 2) & 0x3)
#define GET_RS2(inst)  ((inst) & 0x3)
#define GET_IMM(inst)  ((inst) & 0xF)
#define GET_ADDR(inst) (((inst) >> 2) & 0xF)

int main(int argc, char *argv[]) {
	if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n < 0 || n > 15) {
        fprintf(stderr, "Error: n must be between 0 and 15 (4-bit immediate limit)\n");
        return 1;
    }
    	uint8_t PC = 0;
    	uint8_t R[4] = {0};
    	uint8_t M[16] = {0};
	
    // 数列求和程序：计算 1+2+...+10 = 55，并通过 OUT 输出 r2
    uint8_t program[] = {
        0x5A,   // LI r1, 10
        0x60,   // LI r2, 0
        0x71,   // LI r3, 1
        0x40,   // LI r0, 0
        0x03,   // ADD r0, r0, r3   (counter++)
        0x28,   // ADD r2, r2, r0   (sum += counter)
        0xD1,   // BNER0 4, r1      (if r0 != r1, goto 4)
        0xA0,   // OUT r2
        0x00    // NOP
    };
	program[0] = (OP_LI << 6) | (1 << 4) | (n & 0xF);
	
    // 加载程序
    for (size_t i = 0; i < sizeof(program); i++) {
        M[i] = program[i];
    }
    printf("8-bit ISA Simulator - Corrected Version\n");
    printf("PC starts at 0\n\n");

    // 执行
    for (int cycle = 0; cycle < 50; cycle++) {
        uint8_t inst = M[PC];
        uint8_t op = GET_OP(inst);

        switch (op) {
            case OP_ADD: {
                uint8_t RD = GET_RD(inst);
                uint8_t RS1 = GET_RS1(inst);
                uint8_t RS2 = GET_RS2(inst);
                R[RD] = R[RS1] + R[RS2];
                PC = (PC + 1) & 0xF;
                break;
            }
            case OP_LI: {
                uint8_t RD = GET_RD(inst);
                uint8_t IMM = GET_IMM(inst);
                R[RD] = IMM;
                PC = (PC + 1) & 0xF;
                break;
            }
            case OP_BNER0: {
                uint8_t ADDR = GET_ADDR(inst);
                uint8_t RS2 = GET_RS2(inst);
                if (R[0] != R[RS2]) {
                    PC = ADDR;
                } else {
                    PC = (PC + 1) & 0xF;
                }
                break;
            }
            case OP_OUT: {
                uint8_t RD = GET_RD(inst);
                printf("%d\n", R[RD]);
                return 0;               
            }
            default:
                PC = (PC + 1) & 0xF;
                break;
        }
    }

    // 安全兜底（正常不会走到这里）
    return 0;
}



