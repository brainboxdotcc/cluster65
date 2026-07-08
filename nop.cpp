#include "opcodes.h"

void op_nop(cpu_group&, int)
{
    // NOP has already consumed its opcode byte.
    // No operands, no state changes.
}
