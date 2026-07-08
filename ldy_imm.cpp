#include "opcodes.h"

void op_ldy_imm(cpu_group& group, int active)
{
    __m256i value = fetch8(group, active);
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);

    __m256i next_cpu = replace_field(cpu, value, y_mask, reg_y);
    next_cpu = set_zn(next_cpu, value);

    store_active_cpu(group, cpu, next_cpu, active);
}