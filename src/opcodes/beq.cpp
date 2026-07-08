#include "opcodes.h"

void op_beq(cpu_group& group, int active)
{
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);
    __m256i flags = field8(cpu, reg_flags);

    __m256i z = _mm256_and_si256(flags, _mm256_set1_epi64x(flag_z));
    int taken = _mm256_movemask_pd(_mm256_castsi256_pd(
        _mm256_cmpeq_epi64(z, _mm256_set1_epi64x(flag_z))
    ));

    branch_relative(group, active, active & taken);
}