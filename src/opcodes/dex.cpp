#include "opcodes.h"

void op_dex(cpu_group& group, int active)
{
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);

    __m256i value = field8(cpu, reg_x);
    value = _mm256_and_si256(
        _mm256_sub_epi64(value, _mm256_set1_epi64x(1)),
        _mm256_set1_epi64x(mask8)
    );

    __m256i next_cpu = replace_field(cpu, value, x_mask, reg_x);
    next_cpu = set_zn(next_cpu, value);

    store_active_cpu(group, cpu, next_cpu, active);
}