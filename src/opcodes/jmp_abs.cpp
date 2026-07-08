#include "opcodes.h"

void op_jmp_abs(cpu_group& group, int active)
{
    __m256i addr = fetch16(group, active);
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);

    __m256i next_cpu = replace_field(cpu, addr, pc_mask, reg_pc);

    store_active_cpu(group, cpu, next_cpu, active);
}