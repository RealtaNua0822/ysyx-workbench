#include <am.h>
#include <klib-macros.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==============================
// Configuration
// ==============================

#define MEM_SIZE        (1024 * 1024)     // 1MB physical memory: 0x0 ~ 0xFFFFF
#define FB_BASE         0x20000000U       // Simulated VRAM base (not used directly)
#define SIM_FB_WIDTH    320
#define SIM_FB_HEIGHT   200
#define SIM_FB_PIXELS   (SIM_FB_WIDTH * SIM_FB_HEIGHT)

// Physical memory and simulated framebuffer
static uint8_t memory[MEM_SIZE];
static uint32_t sim_framebuffer[SIM_FB_PIXELS];

// GPU drawing buffer
#define MAX_GPU_PIXELS (400 * 300)
static uint32_t gpu_buffer[MAX_GPU_PIXELS];

// CPU registers and state
static uint32_t reg[32] = {0};
static uint32_t pc = 0;
static int screen_dirty = 0;

// ==============================
// Load Logisim-style .hex file
// Format:
//   v3.0 hex words addressed
//   00000: 00000413 00093137 ...
// ==============================

static void load_logisim_hex(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: Cannot open %s\n", filename);
        exit(1);
    }

    char line[256];
    // Skip header line
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        printf("File is empty.\n");
        exit(1);
    }

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '\r') continue;

        char *colon = strchr(line, ':');
        if (!colon) continue;

        *colon = '\0';
        uint32_t word_addr = (uint32_t)strtoul(line, NULL, 16);
        uint32_t byte_addr = word_addr * 4;

        if (byte_addr >= MEM_SIZE) {
            printf("Warning: Address 0x%x out of memory range.\n", byte_addr);
            continue;
        }

        char *p = colon + 1;
        while (*p) {
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0' || *p == '\n' || *p == '\r') break;

            char word_str[9] = {0};
            int i = 0;
            while (i < 8 && *p && *p != ' ' && *p != '\n' && *p != '\r') {
                word_str[i++] = *p++;
            }

            uint32_t word = (uint32_t)strtoul(word_str, NULL, 16);

            if (byte_addr + 3 < MEM_SIZE) {
                memory[byte_addr + 0] = (word >>  0) & 0xFF;
                memory[byte_addr + 1] = (word >>  8) & 0xFF;
                memory[byte_addr + 2] = (word >> 16) & 0xFF;
                memory[byte_addr + 3] = (word >> 24) & 0xFF;
            } else {
                printf("Memory overflow at byte address 0x%x\n", byte_addr);
                break;
            }

            byte_addr += 4;
        }
    }

    fclose(fp);
    printf("Loaded Logisim-style hex file: %s\n", filename);
}

// ==============================
// Memory I/O (for direct memory access only)
// ==============================

static void mmio_write(uint32_t addr, uint32_t value, int len) {
    if (addr < MEM_SIZE) {
        switch (len) {
            case 1: memory[addr] = (uint8_t)value; break;
            case 2: *(uint16_t*)&memory[addr] = (uint16_t)value; break;
            case 4: *(uint32_t*)&memory[addr] = value; break;
        }
    }
}

static uint32_t mmio_read(uint32_t addr, int len) {
    if (addr < MEM_SIZE) {
        switch (len) {
            case 1: return memory[addr];
            case 2: return *(uint16_t*)&memory[addr];
            case 4: return *(uint32_t*)&memory[addr];
        }
    }
    return 0;
}

// ==============================
// Execute one RISC-V instruction (RV32I subset)
// ==============================

static void execute_one() {
    if (pc >= MEM_SIZE || (pc & 3)) {
        printf("PC error: 0x%08x\n", pc);
        exit(1);
    }

    uint32_t inst = *(uint32_t*)&memory[pc];
    uint32_t next_pc = pc + 4;

    switch (OPCODE(inst)) {
        case 0x37: // LUI
            reg[RD(inst)] = (IMM_I(inst) << 12);
            break;
        case 0x13: // ADDI
            if (FUNCT3(inst) == 0)
                reg[RD(inst)] = reg[RS1(inst)] + IMM_I(inst);
            break;
        case 0x33: // ADD
            if (FUNCT3(inst) == 0 && FUNCT7(inst) == 0)
                reg[RD(inst)] = reg[RS1(inst)] + reg[RS2(inst)];
            break;
        case 0x03: // LW / LBU
            {
                uint32_t addr = reg[RS1(inst)] + IMM_I(inst);
                if (FUNCT3(inst) == 0x2) {
                    reg[RD(inst)] = mmio_read(addr, 4);
                } else if (FUNCT3(inst) == 0x4) {
                    reg[RD(inst)] = mmio_read(addr, 1);
                }
            }
            break;
        case 0x23: // SW / SB
            {
                uint32_t addr = reg[RS1(inst)] + IMM_S(inst);
                if (FUNCT3(inst) == 0x2) {
                    mmio_write(addr, reg[RS2(inst)], 4);
                } else if (FUNCT3(inst) == 0x0) {
                    mmio_write(addr, reg[RS2(inst)], 1);
                }
            }
            break;
        case 0x67: // JALR
            {
                uint32_t target = (reg[RS1(inst)] + IMM_I(inst)) & ~1U;
                reg[RD(inst)] = next_pc;
                next_pc = target;
            }
            break;
        case 0x73: // EBREAK or CSR (only handle ebreak)
            if (inst == 0x00100073) {
                next_pc = 0xFFFFFFFF; // Terminate
            }
            break;
        default:
            printf("Unsupported instruction: 0x%08x @ PC=0x%08x\n", inst, pc);
            exit(1);
    }

    pc = next_pc;
}

// ==============================
// Draw simulated framebuffer to real GPU via AM
// ==============================

static void draw_sim_fb() {
    if (!screen_dirty) return;

    AM_GPU_CONFIG_T cfg = io_read(AM_GPU_CONFIG);
    int w = cfg.width, h = cfg.height;

    if (w <= 0 || h <= 0 || w * h > MAX_GPU_PIXELS) return;

    for (int i = 0; i < w * h; i++) {
        gpu_buffer[i] = 0xFF000000; // Black background (RGBA)
    }

    int copy_w = (w < SIM_FB_WIDTH) ? w : SIM_FB_WIDTH;
    int copy_h = (h < SIM_FB_HEIGHT) ? h : SIM_FB_HEIGHT;

    for (int y = 0; y < copy_h; y++) {
        for (int x = 0; x < copy_w; x++) {
            gpu_buffer[y * w + x] = sim_framebuffer[y * SIM_FB_WIDTH + x];
        }
    }

    io_write(AM_GPU_FBDRAW, 0, 0, gpu_buffer, w, h, true);
    screen_dirty = 0;
}

// ==============================
// Intercept AM syscalls via ecall emulation (optional, not needed here)
// But your program uses direct function calls, so we don't need ECALL.
// Instead, we rely on the fact that display_image writes to sim_framebuffer
// via io_write(AM_GPU_FBDRAW, ...), which is handled by AM runtime.
//
// However! Your program calls io_write directly as a function,
// and AM's native implementation already handles it.
// So we just need to let the program run and call io_write normally.
//
// BUT WAIT: In your disassembly, display_image calls io_write via jalr.
// That means it's calling a PLT/GOT entry that eventually triggers AM's native handler.
// So as long as we don't block it, it should work.
//
// However, our emulator doesn't simulate function calls to host — but in native AM,
// io_write is linked directly to the host!
//
// Therefore: We must NOT intercept memory writes to simulate framebuffer.
// Instead, we trust that when the guest calls io_write(AM_GPU_FBDRAW, ...),
// the AM native backend will draw it.
//
// But then why do we have sim_framebuffer?
//
// 🔥 CRITICAL REALIZATION:
// Your program does NOT write to physical memory for graphics.
// It calls io_write(AM_GPU_FBDRAW, pixels, ...) directly.
// So our entire sim_framebuffer is UNUSED!
//
// ✅ Therefore: We can REMOVE sim_framebuffer logic entirely.
// And just let the program run — when it calls io_write, AM will show the image.
//
// But wait: The program might be writing pixel data to a buffer in memory,
// then passing that buffer to io_write.
// That buffer is in guest memory (e.g., stack), which is fine.
//
// So the only thing we need is:
// - Correct execution from main()
// - Proper stack
// - Let io_write calls go through to AM
//
// Therefore: We can simplify drastically.
// ==============================

// 🚨 NEW PLAN: Remove manual framebuffer simulation.
// Just run the CPU, and let AM handle GPU via real io_write.

// Redefine draw_sim_fb as no-op (or remove calls)
// But we'll keep minimal version to avoid flicker

static void draw_sim_fb() {
    // Actually, we don't need to do anything.
    // The real drawing happens inside io_write(AM_GPU_FBDRAW)
    // So we just yield control occasionally.
}

// ==============================
// Main function
// ==============================

int main() {
    ioe_init();

    // Initialize memory
    memset(memory, 0, sizeof(memory));

    // Load program
    load_logisim_hex("vga.hex");

    // Initialize all registers to 0
    for (int i = 0; i < 32; i++) {
        reg[i] = 0;
    }

    // Set stack pointer to top of memory (as done in _start)
    reg[2] = MEM_SIZE;  // x2 = sp

    // Start from main() at 0x8c, skipping _trm_init and halt
    pc = 0x8c;

    printf("Starting execution from main() at PC=0x%08x\n", pc);

    // Emulation loop
    while (1) {
        if (pc == 0xFFFFFFFF || pc >= MEM_SIZE) {
            printf("Program terminated.\n");
            break;
        }

        execute_one();
        // No need for draw_sim_fb() — real drawing is done by io_write
    }

    // Keep window alive
    printf("Holding screen...\n");
    while (1) {
        // Do nothing; AM window stays open
    }

    return 0;
}
