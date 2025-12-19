#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// ==============================
// 1. 配置与内存定义
// ==============================

#define MEM_SIZE        (1024 * 1024)           // 1MB RAM: 0x00000000 ~ 0x000FFFFF
#define FB_BASE         0x20000000              // Framebuffer base address
#define WIDTH           320
#define HEIGHT          200
#define FB_SIZE         (WIDTH * HEIGHT * sizeof(uint32_t))  // 320*200*4 = 256KB
#define EBREAK_OPCODE   0x00100073              // ebreak instruction

// Physical RAM (only for [0, MEM_SIZE))
uint8_t memory[MEM_SIZE];

// Dedicated framebuffer (not part of 'memory')
uint32_t framebuffer[WIDTH * HEIGHT];

// Register file x0-x31 (x0 is hardwired to 0)
uint32_t reg[32] = {0};

// Program Counter
uint32_t pc = 0;

// Dirty flag for screen update
int screen_dirty = 0;

// ==============================
// 2. Helper Functions
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
// 3. Memory Access with MMIO
// ==============================

void mmio_write(uint32_t addr, uint32_t value, int len) {
    // Framebuffer region: [FB_BASE, FB_BASE + FB_SIZE)
    if (addr >= FB_BASE && addr < FB_BASE + FB_SIZE) {
        if (len == 4) {
            uint32_t offset = addr - FB_BASE;
            if (offset < FB_SIZE && (offset & 3) == 0) {
                int idx = offset / 4;
                if (idx < WIDTH * HEIGHT) {
                    framebuffer[idx] = value;
                    screen_dirty = 1;
                }
            }
        } else if (len == 1) {
            // Optional: support byte writes to framebuffer (e.g., sb)
            // For simplicity, we ignore byte writes to FB in this demo
        }
        return;
    }

    // Normal RAM write (only within [0, MEM_SIZE))
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
    // Framebuffer read
    if (addr >= FB_BASE && addr < FB_BASE + FB_SIZE) {
        if (len == 4) {
            uint32_t offset = addr - FB_BASE;
            if (offset < FB_SIZE && (offset & 3) == 0) {
                int idx = offset / 4;
                if (idx < WIDTH * HEIGHT) {
                    return framebuffer[idx];
                }
            }
        }
        return 0;
    }

    // RAM read
    if (addr < MEM_SIZE) {
        switch (len) {
            case 1:
                return memory[addr];
            case 2:
                if (addr + 1 < MEM_SIZE) {
                    return *(uint16_t*)(&memory[addr]);
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
// 4. Instruction Decode Macros
// ==============================

#define OPCODE(inst)  ((inst) & 0x7F)
#define RD(inst)      (((inst) >> 7) & 0x1F)
#define FUNCT3(inst)  (((inst) >> 12) & 0x7)
#define RS1(inst)     (((inst) >> 15) & 0x1F)
#define RS2(inst)     (((inst) >> 20) & 0x1F)
#define FUNCT7(inst)  (((inst) >> 25) & 0x7F)

// 修复：正确的立即数提取
static inline int32_t get_imm_i(uint32_t inst) {
    return (int32_t)inst >> 20;  // 符号扩展的12位立即数
}

// ==============================
// 5. Execute One Instruction
// ==============================

void execute_instruction() {
    if (pc >= MEM_SIZE || (pc & 3) != 0) {
        fprintf(stderr, "PC misaligned or out of range: 0x%08x\n", pc);
        exit(1);
    }

    uint32_t inst = *(uint32_t*)(&memory[pc]);
    uint32_t next_pc = pc + 4;

    switch (OPCODE(inst)) {
        case 0x37: { // LUI
            if (RD(inst) != 0) {
                reg[RD(inst)] = (inst & 0xFFFFF000);  // 高20位
            }
            break;
        }
        case 0x13: { // ADDI
            if (FUNCT3(inst) == 0) {
                if (RD(inst) != 0) {
                    int32_t imm = get_imm_i(inst);
                    reg[RD(inst)] = reg[RS1(inst)] + imm;
                }
            }
            break;
        }
        case 0x33: { // ADD
            if (FUNCT3(inst) == 0 && FUNCT7(inst) == 0) {
                if (RD(inst) != 0) {
                    reg[RD(inst)] = reg[RS1(inst)] + reg[RS2(inst)];
                }
            }
            break;
        }
        case 0x03: { // LW / LBU
            int32_t imm = get_imm_i(inst);
            uint32_t addr = reg[RS1(inst)] + imm;
            if (FUNCT3(inst) == 0x2) { // LW
                if (RD(inst) != 0) {
                    reg[RD(inst)] = mmio_read(addr, 4);
                }
            } else if (FUNCT3(inst) == 0x4) { // LBU
                if (RD(inst) != 0) {
                    reg[RD(inst)] = mmio_read(addr, 1);
                }
            }
            break;
        }
        case 0x23: { // SW / SB
            int32_t imm = ((inst >> 25) & 0x7F) << 5;
            imm |= (inst >> 7) & 0x1F;
            imm = sext_12(imm);
            
            uint32_t addr = reg[RS1(inst)] + imm;
            if (FUNCT3(inst) == 0x2) { // SW
                mmio_write(addr, reg[RS2(inst)], 4);
            } else if (FUNCT3(inst) == 0x0) { // SB
                mmio_write(addr, reg[RS2(inst)], 1);
            }
            break;
        }
        case 0x67: { // JALR
            int32_t imm = get_imm_i(inst);
            uint32_t base = reg[RS1(inst)];
            uint32_t target = (base + imm) & ~1U; // 清除最低位
            
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
// 6. AM Display Simulation
// ==============================

void am_draw_screen() {
    if (!screen_dirty) return;
    printf("[AM] Screen updated! (320x200, %d pixels)\n", WIDTH * HEIGHT);
}

// ==============================
// 7. Binary Loader
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
// 8. Main Function
// ==============================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    // Initialize memory and framebuffer
    memset(memory, 0, MEM_SIZE);
    memset(framebuffer, 0xFF, sizeof(framebuffer));

    // Load program
    load_binary(argv[1]);

    // 初始化寄存器
    reg[0] = 0;  // x0 必须为0

    // Start execution from 0x0
    pc = 0;

    // Execution loop
    int max_instructions = 20;
    int instruction_count = 0;
    uint32_t last_pc = 0xFFFFFFFF;

    printf("\nStarting execution...\n");
    printf("=====================\n");

    while (1) {
        // 检查指令数量限制
        if (instruction_count++ >= max_instructions) {
            printf("\nReached maximum instruction count (%d). Stopping.\n", max_instructions);
            break;
        }
        
        // 检查PC范围
        if (pc >= MEM_SIZE) {
            printf("\nPC out of RAM range: 0x%08x\n", pc);
            break;
        }

        uint32_t inst = *(uint32_t*)(&memory[pc]);
        
        // 检测无限循环
        if (pc == last_pc) {
            printf("\nPC stalled at 0x%08x. Halting.\n", pc);
            break;
        }
        last_pc = pc;
        
        // 检查 ebreak
        if (inst == EBREAK_OPCODE) {
            printf("\nEBREAK hit at PC=0x%08x. Halting.\n", pc);
            break;
        }

        // 打印执行前的状态
        printf("[%02d] PC=0x%08x, inst=0x%08x, ", instruction_count, pc, inst);
        
        // 执行指令
        uint32_t old_a0 = reg[10];
        uint32_t old_ra = reg[1];
        
        execute_instruction();
        
        // 打印执行后的状态
        printf("a0:0x%08x->0x%08x, ra:0x%08x->0x%08x\n", 
               old_a0, reg[10], old_ra, reg[1]);
        
        am_draw_screen();
    }

    // 打印最终状态
    printf("\n=====================\n");
    printf("Final state:\n");
    printf("PC        = 0x%08x\n", pc);
    printf("a0 (x10)  = 0x%08x (%d)\n", reg[10], reg[10]);
    printf("ra (x1)   = 0x%08x\n", reg[1]);
    printf("zero (x0) = 0x%08x\n", reg[0]);
    
    // 验证结果
    if (reg[10] == 30) {
        printf("\n✓ Test PASSED! a0 = 20 + 10 = 30\n");
    } else {
        printf("\n✗ Test FAILED! Expected a0 = 30, got %d\n", reg[10]);
    }

    return 0;
}
