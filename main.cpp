#include <cstdint>
#include <cstdio>
#include <cstring>
#include <immintrin.h>
#include <memory>
#include "cluster65.h"
#include "opcodes.h"

static opcode_handler handlers[256];

static void init_handlers()
{
    for (int i = 0; i < 256; i++) {
        handlers[i] = op_illegal;
    }

    handlers[0xea] = op_nop;
    handlers[0x4c] = op_jmp_abs;

    handlers[0xa9] = op_lda_imm;
    handlers[0xa2] = op_ldx_imm;
    handlers[0xa0] = op_ldy_imm;

    handlers[0xaa] = op_tax;
    handlers[0xa8] = op_tay;
    handlers[0x8a] = op_txa;
    handlers[0x98] = op_tya;

    handlers[0xe8] = op_inx;
    handlers[0xc8] = op_iny;
    handlers[0xca] = op_dex;
    handlers[0x88] = op_dey;

    handlers[0x8d] = op_sta_abs;

    handlers[0xf0] = op_beq;
    handlers[0xd0] = op_bne;

    handlers[0x18] = op_clc;
    handlers[0x38] = op_sec;
}

static inline void step(cpu_group& group)
{
    alignas(32) uint64_t op64[4] = {};
    uint8_t order[4] = {};
    int seen[256] = {};
    int count = 0;

    // All four CPUs fetch their opcode.
    __m256i opcode = fetch8(group, 0xf);
    _mm256_store_si256((__m256i*)op64, opcode);

    // Group lanes by opcode.
    //
    // If CPU0 and CPU3 both fetched JMP, seen[0x4c] becomes 1001b.
    for (int lane = 0; lane < 4; lane++) {
        uint8_t op = op64[lane] & 0xff;

        if (!seen[op]) {
            order[count++] = op;
        }

        seen[op] |= 1 << lane;
    }

    // Execute each unique opcode once, with a lane mask.
    for (int i = 0; i < count; i++) {
        uint8_t op = order[i];
        handlers[op](group, seen[op]);
    }
}

static void init(cpu_group& group)
{
    std::memset(&group, 0, sizeof(group));
    init_handlers();

    // Set up simple test programs on each CPU. Make each of the 4 CPUs different, so we can see different output on each.
    for (int lane = 0; lane < 4; lane++) {
        mem(group, 0xfffc, lane) = 0x00;
        mem(group, 0xfffd, lane) = 0x03;

        uint16_t addr = 0x0300;

        if (lane == 2) {
            mem(group, addr++, lane) = 0xa2; // LDX #&05
            mem(group, addr++, lane) = 0x05;

            mem(group, addr++, lane) = 0xca; // DEX

            mem(group, addr++, lane) = 0xd0; // BNE &0302
            mem(group, addr++, lane) = 0xfd; // -3 from PC after operand

            mem(group, addr++, lane) = 0x4c; // JMP &0300
            mem(group, addr++, lane) = 0x00;
            mem(group, addr++, lane) = 0x03;
        }
        else {
            int nops = lane;

            if (lane == 3) {
                nops = 0;
            }

            for (int i = 0; i < nops; i++) {
                mem(group, addr++, lane) = 0xea; // NOP
            }

            mem(group, addr++, lane) = 0x4c; // JMP &0300
            mem(group, addr++, lane) = 0x00;
            mem(group, addr++, lane) = 0x03;
        }

        // Get reset vector from &FFFC
        uint16_t reset = mem(group, 0xfffc, lane) | (mem(group, 0xfffd, lane) << 8);

        group.cpu[lane] = (uint64_t(0xfd) << reg_s) | (uint64_t(flag_i | flag_u) << reg_flags) | (uint64_t(reset) << reg_pc);
    }
}

inline void print_flags(uint8_t flags)
{
    std::printf("%c%c%c%c%c%c%c%c",
        (flags & flag_n) ? 'N' : 'n',
        (flags & flag_v) ? 'V' : 'v',
        (flags & flag_u) ? 'U' : 'u',
        (flags & flag_b) ? 'B' : 'b',
        (flags & flag_d) ? 'D' : 'd',
        (flags & flag_i) ? 'I' : 'i',
        (flags & flag_z) ? 'Z' : 'z',
        (flags & flag_c) ? 'C' : 'c');
}

int main()
{
    auto group = std::make_unique<cpu_group>();
    init(*group);

    for (int i = 0; i < 20; i++) {
        std::printf("step %d:", i);

        for (int lane = 0; lane < 4; lane++) {
            std::printf(" cpu%d.pc=&%04x flags=", lane, get_pc(group->cpu[lane]));
            print_flags(get_flags(group->cpu[lane]));
        }

        std::printf("\n");

        step(*group);
    }

    return 0;
}