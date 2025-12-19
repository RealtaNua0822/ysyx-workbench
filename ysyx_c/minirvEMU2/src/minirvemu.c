#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ==============================
// 配置与内存定义
// ==============================

#define MEM_SIZE        (1024 * 1024)           // 1MB RAM
#define EBREAK_OPCODE   0x00100073              // ebreak instruction

uint8_t memory[MEM_SIZE];
uint32_t reg[32] = {0};
uint32_t pc = 0;

// ==============================
// Helper Functions
// ==============================

static inline int32_t sext_12(int32_t imm) {
    return (imm << 20) >> 20;
}

static inline uint32_t get_imm_s(uint32_t inst) {
    int32_t imm = ((inst >> 25) & 0x7F) << 5;
    imm |= (inst >> 7) & 0x1F;
    return (uint32_t)sext_12(imm);
}

// ==============================
// Memory Access
// ==============================

void mmio_write(uint32_t addr, uint32_t value, int len) {
    if (addr < MEM_SIZE) {
        switch (len) {
            case 1: 
                memory[addr] = (uint8_t)(value & 0xFF); 
                break;
            case 2: 
                if (addr + 1 < MEM_SIZE) {
                    *(uint16_t*)(&memory[addr]) = (uint16_t)(value & 0xFFFF);
                }
                break;
            case 4: 
                if (addr + 3 < MEM_SIZE) {
                    *(uint32_t*)(&memory[addr]) = value;
                }
                break;
        }
    }
}

uint32_t mmio_read(uint32_t addr, int len) {
    if (addr < MEM_SIZE) {
        switch (len) {
            case 1: 
                return (uint32_t)memory[addr];
            case 2: 
                if (addr + 1 < MEM_SIZE) {
                    return (uint32_t)*(uint16_t*)(&memory[addr]);
                }
                break;
            case 4: 
                if (addr + 3 < MEM_SIZE) {
                    return *(uint32_t*)(&memory[addr]);
                }
                break;
        }
    }
    return 0;
}

// ==============================
// Instruction Decode Macros
// ==============================

#define OPCODE(inst)  ((inst) & 0x7F)
#define RD(inst)      (((inst) >> 7) & 0x1F)
#define FUNCT3(inst)  (((inst) >> 12) & 0x7)
#define RS1(inst)     (((inst) >> 15) & 0x1F)
#define RS2(inst)     (((inst) >> 20) & 0x1F)
#define FUNCT7(inst)  (((inst) >> 25) & 0x7F)

static inline int32_t get_imm_i(uint32_t inst) {
    return (int32_t)inst >> 20;
}

// ==============================
// Execute One Instruction
// ==============================

void execute_instruction() {
    if (pc >= MEM_SIZE) {
        fprintf(stderr, "PC out of range: 0x%08x\n", pc);
        exit(1);
    }

    uint32_t inst = *(uint32_t*)(&memory[pc]);
    uint32_t next_pc = pc + 4;

    switch (OPCODE(inst)) {
        case 0x37: { // LUI
            if (RD(inst) != 0) {
                reg[RD(inst)] = (inst & 0xFFFFF000);
            }
            break;
        }
        case 0x13: { // ADDI
            if (FUNCT3(inst) == 0) {
                if (RD(inst) != 0) {
                    int32_t imm = get_imm_i(inst);
                    reg[RD(inst)] = reg[RS1(inst)] + imm;
                }
            } else {
                fprintf(stderr, "Unsupported ADDI funct3: 0x%x at PC=0x%08x\n", FUNCT3(inst), pc);
                exit(1);
            }
            break;
        }
        case 0x33: { // ADD
            if (FUNCT3(inst) == 0 && FUNCT7(inst) == 0) {
                if (RD(inst) != 0) {
                    reg[RD(inst)] = reg[RS1(inst)] + reg[RS2(inst)];
                }
            } else {
                fprintf(stderr, "Unsupported ADD funct3/funct7: 0x%x/0x%x at PC=0x%08x\n", 
                        FUNCT3(inst), FUNCT7(inst), pc);
                exit(1);
            }
            break;
        }
        case 0x03: { // Load instructions
            int32_t imm = get_imm_i(inst);
            uint32_t addr = reg[RS1(inst)] + imm;
            
            if (RD(inst) == 0) break;
            
            switch (FUNCT3(inst)) {
                case 0x2: // LW
                    reg[RD(inst)] = mmio_read(addr, 4);
                    break;
                case 0x4: // LBU
                    reg[RD(inst)] = mmio_read(addr, 1);
                    break;
                case 0x5: // LHU
                    reg[RD(inst)] = mmio_read(addr, 2);
                    break;
                case 0x0: // LB
                    reg[RD(inst)] = (uint32_t)(int32_t)(int8_t)mmio_read(addr, 1);
                    break;
                default:
                    fprintf(stderr, "Unsupported load funct3: 0x%x at PC=0x%08x\n", FUNCT3(inst), pc);
                    exit(1);
            }
            break;
        }
        case 0x23: { // Store instructions
            int32_t imm = get_imm_s(inst);
            uint32_t addr = reg[RS1(inst)] + imm;
            
            switch (FUNCT3(inst)) {
                case 0x2: // SW
                    mmio_write(addr, reg[RS2(inst)], 4);
                    break;
                case 0x0: // SB (关键修复)
                    mmio_write(addr, reg[RS2(inst)] & 0xFF, 1);
                    break;
                case 0x1: // SH
                    mmio_write(addr, reg[RS2(inst)] & 0xFFFF, 2);
                    break;
                default:
                    fprintf(stderr, "Unsupported store funct3: 0x%x at PC=0x%08x\n", FUNCT3(inst), pc);
                    exit(1);
            }
            break;
        }
        case 0x67: { // JALR
            int32_t imm = get_imm_i(inst);
            uint32_t base = reg[RS1(inst)];
            uint32_t target = (base + imm) & ~1U;
            
            if (RD(inst) != 0) {
                reg[RD(inst)] = next_pc;
            }
            next_pc = target;
            break;
        }
        default:
            fprintf(stderr, "Unsupported instruction at PC=0x%08x: 0x%08x (opcode=0x%02x)\n", 
                    pc, inst, OPCODE(inst));
            exit(1);
    }

    pc = next_pc;
}

// ==============================
// Binary Loader
// ==============================

void load_binary(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size > MEM_SIZE) {
        fprintf(stderr, "Binary too large (> %d bytes)\n", MEM_SIZE);
        fclose(f);
        exit(1);
    }
    rewind(f);
    size_t bytes_read = fread(memory, 1, size, f);
    if (bytes_read != size) {
        fprintf(stderr, "Failed to read all bytes\n");
        exit(1);
    }
    fclose(f);
    printf("Loaded '%s' (%ld bytes) to memory.\n", filename, size);
}

// ==============================
// Debug Helpers
// ==============================

const char* reg_name(int i) {
    static const char* names[] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    return (i < 32) ? names[i] : "???";
}

void print_instruction(uint32_t inst) {
    uint8_t opcode = OPCODE(inst);
    uint8_t rd = RD(inst);
    uint8_t rs1 = RS1(inst);
    uint8_t rs2 = RS2(inst);
    uint8_t funct3 = FUNCT3(inst);
    int32_t imm;
    
    switch (opcode) {
        case 0x37: 
            printf("LUI   %s, 0x%x", reg_name(rd), (inst & 0xFFFFF000)); 
            break;
        case 0x13: 
            if (funct3 == 0) {
                imm = get_imm_i(inst);
                printf("ADDI  %s, %s, 0x%x", reg_name(rd), reg_name(rs1), imm);
            }
            break;
        case 0x33:
            if (funct3 == 0) 
                printf("ADD   %s, %s, %s", reg_name(rd), reg_name(rs1), reg_name(rs2));
            break;
        case 0x03:
            imm = get_imm_i(inst);
            switch (funct3) {
                case 0x2: printf("LW    %s, 0x%x(%s)", reg_name(rd), imm, reg_name(rs1)); break;
                case 0x4: printf("LBU   %s, 0x%x(%s)", reg_name(rd), imm, reg_name(rs1)); break;
                case 0x5: printf("LHU   %s, 0x%x(%s)", reg_name(rd), imm, reg_name(rs1)); break;
                case 0x0: printf("LB    %s, 0x%x(%s)", reg_name(rd), imm, reg_name(rs1)); break;
                default: printf("LOAD funct3=%d", funct3);
            }
            break;
        case 0x23:
            imm = get_imm_s(inst);
            switch (funct3) {
                case 0x2: printf("SW    %s, 0x%x(%s)", reg_name(rs2), imm, reg_name(rs1)); break;
                case 0x0: printf("SB    %s, 0x%x(%s)", reg_name(rs2), imm, reg_name(rs1)); break;
                case 0x1: printf("SH    %s, 0x%x(%s)", reg_name(rs2), imm, reg_name(rs1)); break;
                default: printf("STORE funct3=%d", funct3);
            }
            break;
        case 0x67:
            imm = get_imm_i(inst);
            printf("JALR  %s, %s, 0x%x", reg_name(rd), reg_name(rs1), imm);
            break;
        default: 
            printf("UNKNOWN opcode=0x%02x", opcode);
    }
}

// ==============================
// Main Function - 增强版
// ==============================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    // Initialize memory
    memset(memory, 0, MEM_SIZE);
    load_binary(argv[1]);

    // 初始化寄存器
    reg[0] = 0;  // x0 必须为0
    
    // Start execution
    pc = 0;
    int max_instructions = 5000;  // 增加到5000条
    int instruction_count = 0;
    uint32_t last_pc = 0xFFFFFFFF;
    int halt_loop_count = 0;
    
    printf("\nStarting execution...\n");
    printf("=====================\n");

    while (instruction_count < max_instructions) {
        instruction_count++;
        
        // 检查PC范围
        if (pc >= MEM_SIZE) {
            printf("\nPC out of RAM range: 0x%08x\n", pc);
            break;
        }

        uint32_t inst = *(uint32_t*)(&memory[pc]);
        
        // 检测无限循环
        if (pc == last_pc) {
            halt_loop_count++;
            if (halt_loop_count > 10) {
                printf("\nDetected loop at PC=0x%08x. Stopping.\n", pc);
                break;
            }
        } else {
            halt_loop_count = 0;
        }
        last_pc = pc;
        
        // 检查 ebreak
        if (inst == EBREAK_OPCODE) {
            printf("\nEBREAK hit at PC=0x%08x. Halting.\n", pc);
            break;
        }

        // 每500条指令打印一次进度
        if (instruction_count % 500 == 0) {
            printf("[%d] Executed %d instructions...\n", instruction_count, instruction_count);
        }
        
        // 详细的指令跟踪（前100条和最后100条）
        if (instruction_count <= 100 || instruction_count > max_instructions - 100) {
            printf("[%04d] PC=0x%08x: ", instruction_count, pc);
            print_instruction(inst);
            printf("\n");
            
            // 保存寄存器状态（用于比较）
            uint32_t old_regs[32];
            memcpy(old_regs, reg, sizeof(reg));
            
            // 执行指令
            execute_instruction();
            
            // 显示变化的寄存器
            int changes = 0;
            for (int i = 1; i < 32; i++) {
                if (reg[i] != old_regs[i]) {
                    if (changes == 0) printf("       Changed: ");
                    else printf(", ");
                    printf("%s:0x%08x", reg_name(i), reg[i]);
                    changes++;
                }
            }
            if (changes > 0) printf("\n");
        } else {
            // 简单执行，不打印细节
            execute_instruction();
        }
    }

    if (instruction_count >= max_instructions) {
        printf("\nReached maximum instruction count (%d).\n", max_instructions);
    }

    // 显示最终结果
    printf("\n=====================\n");
    printf("Execution summary:\n");
    printf("Total instructions: %d\n", instruction_count);
    printf("Final PC: 0x%08x\n", pc);
    
    // 打印所有寄存器
    printf("\nAll registers:\n");
    for (int i = 0; i < 32; i += 4) {
        for (int j = 0; j < 4 && i+j < 32; j++) {
            int idx = i + j;
            printf("x%2d (%5s)=0x%08x  ", idx, reg_name(idx), reg[idx]);
        }
        printf("\n");
    }

    return 0;
}
