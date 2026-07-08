#pragma once
#include "cluster65.h"

void op_illegal(cpu_group&, int);
void op_nop(cpu_group&, int);

void op_jmp_abs(cpu_group&, int);

void op_lda_imm(cpu_group&, int);
void op_ldx_imm(cpu_group&, int);
void op_ldy_imm(cpu_group&, int);

void op_tax(cpu_group&, int);
void op_tay(cpu_group&, int);
void op_txa(cpu_group&, int);
void op_tya(cpu_group&, int);

void op_inx(cpu_group&, int);
void op_iny(cpu_group&, int);
void op_dex(cpu_group&, int);
void op_dey(cpu_group&, int);

void op_sta_abs(cpu_group&, int);

void op_beq(cpu_group&, int);
void op_bne(cpu_group&, int);

void op_clc(cpu_group&, int);
void op_sec(cpu_group&, int);