#include "opcodes.h"

void op_sta_abs(cpu_group& group, int active)
{
    __m256i addr = fetch16(group, active);
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);
    __m256i value = field8(cpu, reg_a);

    write8(group, addr, value, active);
}