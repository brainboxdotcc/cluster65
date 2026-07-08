#include "opcodes.h"

void op_sec(cpu_group& group, int active)
{
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);
    __m256i flags = field8(cpu, reg_flags);

    flags = _mm256_or_si256(flags, _mm256_set1_epi64x(flag_c));

    __m256i next_cpu = replace_field(cpu, flags, flags_mask, reg_flags);
    store_active_cpu(group, cpu, next_cpu, active);
}