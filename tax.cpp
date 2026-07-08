#include "opcodes.h"

void op_tax(cpu_group& group, int active)
{
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);
    __m256i value = field8(cpu, reg_a);

    __m256i next_cpu = replace_field(cpu, value, x_mask, reg_x);
    next_cpu = set_zn(next_cpu, value);

    store_active_cpu(group, cpu, next_cpu, active);
}