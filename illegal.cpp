#include "opcodes.h"

void op_illegal(cpu_group&, int)
{
    std::puts("illegal opcode");
}