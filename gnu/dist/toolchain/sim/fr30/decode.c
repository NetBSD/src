/* Simulator instruction decoder for fr30bf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#define WANT_CPU fr30bf
#define WANT_CPU_FR30BF

#include "sim-main.h"
#include "sim-assert.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC fr30bf_insn_data[FR30BF_INSN_MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem fr30bf_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, FR30BF_INSN_X_INVALID, FR30BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, FR30BF_INSN_X_AFTER, FR30BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, FR30BF_INSN_X_BEFORE, FR30BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, FR30BF_INSN_X_CTI_CHAIN, FR30BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, FR30BF_INSN_X_CHAIN, FR30BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, FR30BF_INSN_X_BEGIN, FR30BF_SFMT_EMPTY },
  { FR30_INSN_ADD, FR30BF_INSN_ADD, FR30BF_SFMT_ADD },
  { FR30_INSN_ADDI, FR30BF_INSN_ADDI, FR30BF_SFMT_ADDI },
  { FR30_INSN_ADD2, FR30BF_INSN_ADD2, FR30BF_SFMT_ADD2 },
  { FR30_INSN_ADDC, FR30BF_INSN_ADDC, FR30BF_SFMT_ADDC },
  { FR30_INSN_ADDN, FR30BF_INSN_ADDN, FR30BF_SFMT_ADDN },
  { FR30_INSN_ADDNI, FR30BF_INSN_ADDNI, FR30BF_SFMT_ADDNI },
  { FR30_INSN_ADDN2, FR30BF_INSN_ADDN2, FR30BF_SFMT_ADDN2 },
  { FR30_INSN_SUB, FR30BF_INSN_SUB, FR30BF_SFMT_ADD },
  { FR30_INSN_SUBC, FR30BF_INSN_SUBC, FR30BF_SFMT_ADDC },
  { FR30_INSN_SUBN, FR30BF_INSN_SUBN, FR30BF_SFMT_ADDN },
  { FR30_INSN_CMP, FR30BF_INSN_CMP, FR30BF_SFMT_CMP },
  { FR30_INSN_CMPI, FR30BF_INSN_CMPI, FR30BF_SFMT_CMPI },
  { FR30_INSN_CMP2, FR30BF_INSN_CMP2, FR30BF_SFMT_CMP2 },
  { FR30_INSN_AND, FR30BF_INSN_AND, FR30BF_SFMT_AND },
  { FR30_INSN_OR, FR30BF_INSN_OR, FR30BF_SFMT_AND },
  { FR30_INSN_EOR, FR30BF_INSN_EOR, FR30BF_SFMT_AND },
  { FR30_INSN_ANDM, FR30BF_INSN_ANDM, FR30BF_SFMT_ANDM },
  { FR30_INSN_ANDH, FR30BF_INSN_ANDH, FR30BF_SFMT_ANDH },
  { FR30_INSN_ANDB, FR30BF_INSN_ANDB, FR30BF_SFMT_ANDB },
  { FR30_INSN_ORM, FR30BF_INSN_ORM, FR30BF_SFMT_ANDM },
  { FR30_INSN_ORH, FR30BF_INSN_ORH, FR30BF_SFMT_ANDH },
  { FR30_INSN_ORB, FR30BF_INSN_ORB, FR30BF_SFMT_ANDB },
  { FR30_INSN_EORM, FR30BF_INSN_EORM, FR30BF_SFMT_ANDM },
  { FR30_INSN_EORH, FR30BF_INSN_EORH, FR30BF_SFMT_ANDH },
  { FR30_INSN_EORB, FR30BF_INSN_EORB, FR30BF_SFMT_ANDB },
  { FR30_INSN_BANDL, FR30BF_INSN_BANDL, FR30BF_SFMT_BANDL },
  { FR30_INSN_BORL, FR30BF_INSN_BORL, FR30BF_SFMT_BANDL },
  { FR30_INSN_BEORL, FR30BF_INSN_BEORL, FR30BF_SFMT_BANDL },
  { FR30_INSN_BANDH, FR30BF_INSN_BANDH, FR30BF_SFMT_BANDL },
  { FR30_INSN_BORH, FR30BF_INSN_BORH, FR30BF_SFMT_BANDL },
  { FR30_INSN_BEORH, FR30BF_INSN_BEORH, FR30BF_SFMT_BANDL },
  { FR30_INSN_BTSTL, FR30BF_INSN_BTSTL, FR30BF_SFMT_BTSTL },
  { FR30_INSN_BTSTH, FR30BF_INSN_BTSTH, FR30BF_SFMT_BTSTL },
  { FR30_INSN_MUL, FR30BF_INSN_MUL, FR30BF_SFMT_MUL },
  { FR30_INSN_MULU, FR30BF_INSN_MULU, FR30BF_SFMT_MULU },
  { FR30_INSN_MULH, FR30BF_INSN_MULH, FR30BF_SFMT_MULH },
  { FR30_INSN_MULUH, FR30BF_INSN_MULUH, FR30BF_SFMT_MULH },
  { FR30_INSN_DIV0S, FR30BF_INSN_DIV0S, FR30BF_SFMT_DIV0S },
  { FR30_INSN_DIV0U, FR30BF_INSN_DIV0U, FR30BF_SFMT_DIV0U },
  { FR30_INSN_DIV1, FR30BF_INSN_DIV1, FR30BF_SFMT_DIV1 },
  { FR30_INSN_DIV2, FR30BF_INSN_DIV2, FR30BF_SFMT_DIV2 },
  { FR30_INSN_DIV3, FR30BF_INSN_DIV3, FR30BF_SFMT_DIV3 },
  { FR30_INSN_DIV4S, FR30BF_INSN_DIV4S, FR30BF_SFMT_DIV4S },
  { FR30_INSN_LSL, FR30BF_INSN_LSL, FR30BF_SFMT_LSL },
  { FR30_INSN_LSLI, FR30BF_INSN_LSLI, FR30BF_SFMT_LSLI },
  { FR30_INSN_LSL2, FR30BF_INSN_LSL2, FR30BF_SFMT_LSLI },
  { FR30_INSN_LSR, FR30BF_INSN_LSR, FR30BF_SFMT_LSL },
  { FR30_INSN_LSRI, FR30BF_INSN_LSRI, FR30BF_SFMT_LSLI },
  { FR30_INSN_LSR2, FR30BF_INSN_LSR2, FR30BF_SFMT_LSLI },
  { FR30_INSN_ASR, FR30BF_INSN_ASR, FR30BF_SFMT_LSL },
  { FR30_INSN_ASRI, FR30BF_INSN_ASRI, FR30BF_SFMT_LSLI },
  { FR30_INSN_ASR2, FR30BF_INSN_ASR2, FR30BF_SFMT_LSLI },
  { FR30_INSN_LDI8, FR30BF_INSN_LDI8, FR30BF_SFMT_LDI8 },
  { FR30_INSN_LDI20, FR30BF_INSN_LDI20, FR30BF_SFMT_LDI20 },
  { FR30_INSN_LDI32, FR30BF_INSN_LDI32, FR30BF_SFMT_LDI32 },
  { FR30_INSN_LD, FR30BF_INSN_LD, FR30BF_SFMT_LD },
  { FR30_INSN_LDUH, FR30BF_INSN_LDUH, FR30BF_SFMT_LD },
  { FR30_INSN_LDUB, FR30BF_INSN_LDUB, FR30BF_SFMT_LD },
  { FR30_INSN_LDR13, FR30BF_INSN_LDR13, FR30BF_SFMT_LDR13 },
  { FR30_INSN_LDR13UH, FR30BF_INSN_LDR13UH, FR30BF_SFMT_LDR13 },
  { FR30_INSN_LDR13UB, FR30BF_INSN_LDR13UB, FR30BF_SFMT_LDR13 },
  { FR30_INSN_LDR14, FR30BF_INSN_LDR14, FR30BF_SFMT_LDR14 },
  { FR30_INSN_LDR14UH, FR30BF_INSN_LDR14UH, FR30BF_SFMT_LDR14UH },
  { FR30_INSN_LDR14UB, FR30BF_INSN_LDR14UB, FR30BF_SFMT_LDR14UB },
  { FR30_INSN_LDR15, FR30BF_INSN_LDR15, FR30BF_SFMT_LDR15 },
  { FR30_INSN_LDR15GR, FR30BF_INSN_LDR15GR, FR30BF_SFMT_LDR15GR },
  { FR30_INSN_LDR15DR, FR30BF_INSN_LDR15DR, FR30BF_SFMT_LDR15DR },
  { FR30_INSN_LDR15PS, FR30BF_INSN_LDR15PS, FR30BF_SFMT_LDR15PS },
  { FR30_INSN_ST, FR30BF_INSN_ST, FR30BF_SFMT_ST },
  { FR30_INSN_STH, FR30BF_INSN_STH, FR30BF_SFMT_ST },
  { FR30_INSN_STB, FR30BF_INSN_STB, FR30BF_SFMT_ST },
  { FR30_INSN_STR13, FR30BF_INSN_STR13, FR30BF_SFMT_STR13 },
  { FR30_INSN_STR13H, FR30BF_INSN_STR13H, FR30BF_SFMT_STR13 },
  { FR30_INSN_STR13B, FR30BF_INSN_STR13B, FR30BF_SFMT_STR13 },
  { FR30_INSN_STR14, FR30BF_INSN_STR14, FR30BF_SFMT_STR14 },
  { FR30_INSN_STR14H, FR30BF_INSN_STR14H, FR30BF_SFMT_STR14H },
  { FR30_INSN_STR14B, FR30BF_INSN_STR14B, FR30BF_SFMT_STR14B },
  { FR30_INSN_STR15, FR30BF_INSN_STR15, FR30BF_SFMT_STR15 },
  { FR30_INSN_STR15GR, FR30BF_INSN_STR15GR, FR30BF_SFMT_STR15GR },
  { FR30_INSN_STR15DR, FR30BF_INSN_STR15DR, FR30BF_SFMT_STR15DR },
  { FR30_INSN_STR15PS, FR30BF_INSN_STR15PS, FR30BF_SFMT_STR15PS },
  { FR30_INSN_MOV, FR30BF_INSN_MOV, FR30BF_SFMT_MOV },
  { FR30_INSN_MOVDR, FR30BF_INSN_MOVDR, FR30BF_SFMT_MOVDR },
  { FR30_INSN_MOVPS, FR30BF_INSN_MOVPS, FR30BF_SFMT_MOVPS },
  { FR30_INSN_MOV2DR, FR30BF_INSN_MOV2DR, FR30BF_SFMT_MOV2DR },
  { FR30_INSN_MOV2PS, FR30BF_INSN_MOV2PS, FR30BF_SFMT_MOV2PS },
  { FR30_INSN_JMP, FR30BF_INSN_JMP, FR30BF_SFMT_JMP },
  { FR30_INSN_JMPD, FR30BF_INSN_JMPD, FR30BF_SFMT_JMP },
  { FR30_INSN_CALLR, FR30BF_INSN_CALLR, FR30BF_SFMT_CALLR },
  { FR30_INSN_CALLRD, FR30BF_INSN_CALLRD, FR30BF_SFMT_CALLR },
  { FR30_INSN_CALL, FR30BF_INSN_CALL, FR30BF_SFMT_CALL },
  { FR30_INSN_CALLD, FR30BF_INSN_CALLD, FR30BF_SFMT_CALL },
  { FR30_INSN_RET, FR30BF_INSN_RET, FR30BF_SFMT_RET },
  { FR30_INSN_RET_D, FR30BF_INSN_RET_D, FR30BF_SFMT_RET },
  { FR30_INSN_INT, FR30BF_INSN_INT, FR30BF_SFMT_INT },
  { FR30_INSN_INTE, FR30BF_INSN_INTE, FR30BF_SFMT_INTE },
  { FR30_INSN_RETI, FR30BF_INSN_RETI, FR30BF_SFMT_RETI },
  { FR30_INSN_BRAD, FR30BF_INSN_BRAD, FR30BF_SFMT_BRAD },
  { FR30_INSN_BRA, FR30BF_INSN_BRA, FR30BF_SFMT_BRAD },
  { FR30_INSN_BNOD, FR30BF_INSN_BNOD, FR30BF_SFMT_BNOD },
  { FR30_INSN_BNO, FR30BF_INSN_BNO, FR30BF_SFMT_BNOD },
  { FR30_INSN_BEQD, FR30BF_INSN_BEQD, FR30BF_SFMT_BEQD },
  { FR30_INSN_BEQ, FR30BF_INSN_BEQ, FR30BF_SFMT_BEQD },
  { FR30_INSN_BNED, FR30BF_INSN_BNED, FR30BF_SFMT_BEQD },
  { FR30_INSN_BNE, FR30BF_INSN_BNE, FR30BF_SFMT_BEQD },
  { FR30_INSN_BCD, FR30BF_INSN_BCD, FR30BF_SFMT_BCD },
  { FR30_INSN_BC, FR30BF_INSN_BC, FR30BF_SFMT_BCD },
  { FR30_INSN_BNCD, FR30BF_INSN_BNCD, FR30BF_SFMT_BCD },
  { FR30_INSN_BNC, FR30BF_INSN_BNC, FR30BF_SFMT_BCD },
  { FR30_INSN_BND, FR30BF_INSN_BND, FR30BF_SFMT_BND },
  { FR30_INSN_BN, FR30BF_INSN_BN, FR30BF_SFMT_BND },
  { FR30_INSN_BPD, FR30BF_INSN_BPD, FR30BF_SFMT_BND },
  { FR30_INSN_BP, FR30BF_INSN_BP, FR30BF_SFMT_BND },
  { FR30_INSN_BVD, FR30BF_INSN_BVD, FR30BF_SFMT_BVD },
  { FR30_INSN_BV, FR30BF_INSN_BV, FR30BF_SFMT_BVD },
  { FR30_INSN_BNVD, FR30BF_INSN_BNVD, FR30BF_SFMT_BVD },
  { FR30_INSN_BNV, FR30BF_INSN_BNV, FR30BF_SFMT_BVD },
  { FR30_INSN_BLTD, FR30BF_INSN_BLTD, FR30BF_SFMT_BLTD },
  { FR30_INSN_BLT, FR30BF_INSN_BLT, FR30BF_SFMT_BLTD },
  { FR30_INSN_BGED, FR30BF_INSN_BGED, FR30BF_SFMT_BLTD },
  { FR30_INSN_BGE, FR30BF_INSN_BGE, FR30BF_SFMT_BLTD },
  { FR30_INSN_BLED, FR30BF_INSN_BLED, FR30BF_SFMT_BLED },
  { FR30_INSN_BLE, FR30BF_INSN_BLE, FR30BF_SFMT_BLED },
  { FR30_INSN_BGTD, FR30BF_INSN_BGTD, FR30BF_SFMT_BLED },
  { FR30_INSN_BGT, FR30BF_INSN_BGT, FR30BF_SFMT_BLED },
  { FR30_INSN_BLSD, FR30BF_INSN_BLSD, FR30BF_SFMT_BLSD },
  { FR30_INSN_BLS, FR30BF_INSN_BLS, FR30BF_SFMT_BLSD },
  { FR30_INSN_BHID, FR30BF_INSN_BHID, FR30BF_SFMT_BLSD },
  { FR30_INSN_BHI, FR30BF_INSN_BHI, FR30BF_SFMT_BLSD },
  { FR30_INSN_DMOVR13, FR30BF_INSN_DMOVR13, FR30BF_SFMT_DMOVR13 },
  { FR30_INSN_DMOVR13H, FR30BF_INSN_DMOVR13H, FR30BF_SFMT_DMOVR13H },
  { FR30_INSN_DMOVR13B, FR30BF_INSN_DMOVR13B, FR30BF_SFMT_DMOVR13B },
  { FR30_INSN_DMOVR13PI, FR30BF_INSN_DMOVR13PI, FR30BF_SFMT_DMOVR13PI },
  { FR30_INSN_DMOVR13PIH, FR30BF_INSN_DMOVR13PIH, FR30BF_SFMT_DMOVR13PIH },
  { FR30_INSN_DMOVR13PIB, FR30BF_INSN_DMOVR13PIB, FR30BF_SFMT_DMOVR13PIB },
  { FR30_INSN_DMOVR15PI, FR30BF_INSN_DMOVR15PI, FR30BF_SFMT_DMOVR15PI },
  { FR30_INSN_DMOV2R13, FR30BF_INSN_DMOV2R13, FR30BF_SFMT_DMOV2R13 },
  { FR30_INSN_DMOV2R13H, FR30BF_INSN_DMOV2R13H, FR30BF_SFMT_DMOV2R13H },
  { FR30_INSN_DMOV2R13B, FR30BF_INSN_DMOV2R13B, FR30BF_SFMT_DMOV2R13B },
  { FR30_INSN_DMOV2R13PI, FR30BF_INSN_DMOV2R13PI, FR30BF_SFMT_DMOV2R13PI },
  { FR30_INSN_DMOV2R13PIH, FR30BF_INSN_DMOV2R13PIH, FR30BF_SFMT_DMOV2R13PIH },
  { FR30_INSN_DMOV2R13PIB, FR30BF_INSN_DMOV2R13PIB, FR30BF_SFMT_DMOV2R13PIB },
  { FR30_INSN_DMOV2R15PD, FR30BF_INSN_DMOV2R15PD, FR30BF_SFMT_DMOV2R15PD },
  { FR30_INSN_LDRES, FR30BF_INSN_LDRES, FR30BF_SFMT_LDRES },
  { FR30_INSN_STRES, FR30BF_INSN_STRES, FR30BF_SFMT_LDRES },
  { FR30_INSN_COPOP, FR30BF_INSN_COPOP, FR30BF_SFMT_COPOP },
  { FR30_INSN_COPLD, FR30BF_INSN_COPLD, FR30BF_SFMT_COPOP },
  { FR30_INSN_COPST, FR30BF_INSN_COPST, FR30BF_SFMT_COPOP },
  { FR30_INSN_COPSV, FR30BF_INSN_COPSV, FR30BF_SFMT_COPOP },
  { FR30_INSN_NOP, FR30BF_INSN_NOP, FR30BF_SFMT_BNOD },
  { FR30_INSN_ANDCCR, FR30BF_INSN_ANDCCR, FR30BF_SFMT_ANDCCR },
  { FR30_INSN_ORCCR, FR30BF_INSN_ORCCR, FR30BF_SFMT_ANDCCR },
  { FR30_INSN_STILM, FR30BF_INSN_STILM, FR30BF_SFMT_STILM },
  { FR30_INSN_ADDSP, FR30BF_INSN_ADDSP, FR30BF_SFMT_ADDSP },
  { FR30_INSN_EXTSB, FR30BF_INSN_EXTSB, FR30BF_SFMT_EXTSB },
  { FR30_INSN_EXTUB, FR30BF_INSN_EXTUB, FR30BF_SFMT_EXTUB },
  { FR30_INSN_EXTSH, FR30BF_INSN_EXTSH, FR30BF_SFMT_EXTSH },
  { FR30_INSN_EXTUH, FR30BF_INSN_EXTUH, FR30BF_SFMT_EXTUH },
  { FR30_INSN_LDM0, FR30BF_INSN_LDM0, FR30BF_SFMT_LDM0 },
  { FR30_INSN_LDM1, FR30BF_INSN_LDM1, FR30BF_SFMT_LDM1 },
  { FR30_INSN_STM0, FR30BF_INSN_STM0, FR30BF_SFMT_STM0 },
  { FR30_INSN_STM1, FR30BF_INSN_STM1, FR30BF_SFMT_STM1 },
  { FR30_INSN_ENTER, FR30BF_INSN_ENTER, FR30BF_SFMT_ENTER },
  { FR30_INSN_LEAVE, FR30BF_INSN_LEAVE, FR30BF_SFMT_LEAVE },
  { FR30_INSN_XCHB, FR30BF_INSN_XCHB, FR30BF_SFMT_XCHB },
};

static const struct insn_sem fr30bf_insn_sem_invalid = {
  VIRTUAL_INSN_X_INVALID, FR30BF_INSN_X_INVALID, FR30BF_SFMT_EMPTY
};

/* Initialize an IDESC from the compile-time computable parts.  */

static INLINE void
init_idesc (SIM_CPU *cpu, IDESC *id, const struct insn_sem *t)
{
  const CGEN_INSN *insn_table = CGEN_CPU_INSN_TABLE (CPU_CPU_DESC (cpu))->init_entries;

  id->num = t->index;
  id->sfmt = t->sfmt;
  if ((int) t->type <= 0)
    id->idata = & cgen_virtual_insn_table[- (int) t->type];
  else
    id->idata = & insn_table[t->type];
  id->attrs = CGEN_INSN_ATTRS (id->idata);
  /* Oh my god, a magic number.  */
  id->length = CGEN_INSN_BITSIZE (id->idata) / 8;

#if WITH_PROFILE_MODEL_P
  id->timing = & MODEL_TIMING (CPU_MODEL (cpu)) [t->index];
  {
    SIM_DESC sd = CPU_STATE (cpu);
    SIM_ASSERT (t->index == id->timing->num);
  }
#endif

  /* Semantic pointers are initialized elsewhere.  */
}

/* Initialize the instruction descriptor table.  */

void
fr30bf_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = FR30BF_INSN_MAX;
  IDESC *table = fr30bf_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & fr30bf_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = fr30bf_insn_sem, tend = t + sizeof (fr30bf_insn_sem) / sizeof (*t);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
fr30bf_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_INT base_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  FR30BF_INSN_TYPE itype;

  {
    CGEN_INSN_INT insn = base_insn;

    {
      unsigned int val = (((insn >> 8) & (255 << 0)));
      switch (val)
      {
      case 0 : itype = FR30BF_INSN_LDR13; goto extract_sfmt_ldr13;
      case 1 : itype = FR30BF_INSN_LDR13UH; goto extract_sfmt_ldr13;
      case 2 : itype = FR30BF_INSN_LDR13UB; goto extract_sfmt_ldr13;
      case 3 : itype = FR30BF_INSN_LDR15; goto extract_sfmt_ldr15;
      case 4 : itype = FR30BF_INSN_LD; goto extract_sfmt_ld;
      case 5 : itype = FR30BF_INSN_LDUH; goto extract_sfmt_ld;
      case 6 : itype = FR30BF_INSN_LDUB; goto extract_sfmt_ld;
      case 7 :
        {
          unsigned int val = (((insn >> 4) & (15 << 0)));
          switch (val)
          {
          case 0 : itype = FR30BF_INSN_LDR15GR; goto extract_sfmt_ldr15gr;
          case 1 : itype = FR30BF_INSN_MOV2PS; goto extract_sfmt_mov2ps;
          case 8 : itype = FR30BF_INSN_LDR15DR; goto extract_sfmt_ldr15dr;
          case 9 : itype = FR30BF_INSN_LDR15PS; goto extract_sfmt_ldr15ps;
          default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 8 : itype = FR30BF_INSN_DMOV2R13; goto extract_sfmt_dmov2r13;
      case 9 : itype = FR30BF_INSN_DMOV2R13H; goto extract_sfmt_dmov2r13h;
      case 10 : itype = FR30BF_INSN_DMOV2R13B; goto extract_sfmt_dmov2r13b;
      case 11 : itype = FR30BF_INSN_DMOV2R15PD; goto extract_sfmt_dmov2r15pd;
      case 12 : itype = FR30BF_INSN_DMOV2R13PI; goto extract_sfmt_dmov2r13pi;
      case 13 : itype = FR30BF_INSN_DMOV2R13PIH; goto extract_sfmt_dmov2r13pih;
      case 14 : itype = FR30BF_INSN_DMOV2R13PIB; goto extract_sfmt_dmov2r13pib;
      case 15 : itype = FR30BF_INSN_ENTER; goto extract_sfmt_enter;
      case 16 : itype = FR30BF_INSN_STR13; goto extract_sfmt_str13;
      case 17 : itype = FR30BF_INSN_STR13H; goto extract_sfmt_str13;
      case 18 : itype = FR30BF_INSN_STR13B; goto extract_sfmt_str13;
      case 19 : itype = FR30BF_INSN_STR15; goto extract_sfmt_str15;
      case 20 : itype = FR30BF_INSN_ST; goto extract_sfmt_st;
      case 21 : itype = FR30BF_INSN_STH; goto extract_sfmt_st;
      case 22 : itype = FR30BF_INSN_STB; goto extract_sfmt_st;
      case 23 :
        {
          unsigned int val = (((insn >> 4) & (15 << 0)));
          switch (val)
          {
          case 0 : itype = FR30BF_INSN_STR15GR; goto extract_sfmt_str15gr;
          case 1 : itype = FR30BF_INSN_MOVPS; goto extract_sfmt_movps;
          case 8 : itype = FR30BF_INSN_STR15DR; goto extract_sfmt_str15dr;
          case 9 : itype = FR30BF_INSN_STR15PS; goto extract_sfmt_str15ps;
          default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 24 : itype = FR30BF_INSN_DMOVR13; goto extract_sfmt_dmovr13;
      case 25 : itype = FR30BF_INSN_DMOVR13H; goto extract_sfmt_dmovr13h;
      case 26 : itype = FR30BF_INSN_DMOVR13B; goto extract_sfmt_dmovr13b;
      case 27 : itype = FR30BF_INSN_DMOVR15PI; goto extract_sfmt_dmovr15pi;
      case 28 : itype = FR30BF_INSN_DMOVR13PI; goto extract_sfmt_dmovr13pi;
      case 29 : itype = FR30BF_INSN_DMOVR13PIH; goto extract_sfmt_dmovr13pih;
      case 30 : itype = FR30BF_INSN_DMOVR13PIB; goto extract_sfmt_dmovr13pib;
      case 31 : itype = FR30BF_INSN_INT; goto extract_sfmt_int;
      case 32 : /* fall through */
      case 33 : /* fall through */
      case 34 : /* fall through */
      case 35 : /* fall through */
      case 36 : /* fall through */
      case 37 : /* fall through */
      case 38 : /* fall through */
      case 39 : /* fall through */
      case 40 : /* fall through */
      case 41 : /* fall through */
      case 42 : /* fall through */
      case 43 : /* fall through */
      case 44 : /* fall through */
      case 45 : /* fall through */
      case 46 : /* fall through */
      case 47 : itype = FR30BF_INSN_LDR14; goto extract_sfmt_ldr14;
      case 48 : /* fall through */
      case 49 : /* fall through */
      case 50 : /* fall through */
      case 51 : /* fall through */
      case 52 : /* fall through */
      case 53 : /* fall through */
      case 54 : /* fall through */
      case 55 : /* fall through */
      case 56 : /* fall through */
      case 57 : /* fall through */
      case 58 : /* fall through */
      case 59 : /* fall through */
      case 60 : /* fall through */
      case 61 : /* fall through */
      case 62 : /* fall through */
      case 63 : itype = FR30BF_INSN_STR14; goto extract_sfmt_str14;
      case 64 : /* fall through */
      case 65 : /* fall through */
      case 66 : /* fall through */
      case 67 : /* fall through */
      case 68 : /* fall through */
      case 69 : /* fall through */
      case 70 : /* fall through */
      case 71 : /* fall through */
      case 72 : /* fall through */
      case 73 : /* fall through */
      case 74 : /* fall through */
      case 75 : /* fall through */
      case 76 : /* fall through */
      case 77 : /* fall through */
      case 78 : /* fall through */
      case 79 : itype = FR30BF_INSN_LDR14UH; goto extract_sfmt_ldr14uh;
      case 80 : /* fall through */
      case 81 : /* fall through */
      case 82 : /* fall through */
      case 83 : /* fall through */
      case 84 : /* fall through */
      case 85 : /* fall through */
      case 86 : /* fall through */
      case 87 : /* fall through */
      case 88 : /* fall through */
      case 89 : /* fall through */
      case 90 : /* fall through */
      case 91 : /* fall through */
      case 92 : /* fall through */
      case 93 : /* fall through */
      case 94 : /* fall through */
      case 95 : itype = FR30BF_INSN_STR14H; goto extract_sfmt_str14h;
      case 96 : /* fall through */
      case 97 : /* fall through */
      case 98 : /* fall through */
      case 99 : /* fall through */
      case 100 : /* fall through */
      case 101 : /* fall through */
      case 102 : /* fall through */
      case 103 : /* fall through */
      case 104 : /* fall through */
      case 105 : /* fall through */
      case 106 : /* fall through */
      case 107 : /* fall through */
      case 108 : /* fall through */
      case 109 : /* fall through */
      case 110 : /* fall through */
      case 111 : itype = FR30BF_INSN_LDR14UB; goto extract_sfmt_ldr14ub;
      case 112 : /* fall through */
      case 113 : /* fall through */
      case 114 : /* fall through */
      case 115 : /* fall through */
      case 116 : /* fall through */
      case 117 : /* fall through */
      case 118 : /* fall through */
      case 119 : /* fall through */
      case 120 : /* fall through */
      case 121 : /* fall through */
      case 122 : /* fall through */
      case 123 : /* fall through */
      case 124 : /* fall through */
      case 125 : /* fall through */
      case 126 : /* fall through */
      case 127 : itype = FR30BF_INSN_STR14B; goto extract_sfmt_str14b;
      case 128 : itype = FR30BF_INSN_BANDL; goto extract_sfmt_bandl;
      case 129 : itype = FR30BF_INSN_BANDH; goto extract_sfmt_bandl;
      case 130 : itype = FR30BF_INSN_AND; goto extract_sfmt_and;
      case 131 : itype = FR30BF_INSN_ANDCCR; goto extract_sfmt_andccr;
      case 132 : itype = FR30BF_INSN_ANDM; goto extract_sfmt_andm;
      case 133 : itype = FR30BF_INSN_ANDH; goto extract_sfmt_andh;
      case 134 : itype = FR30BF_INSN_ANDB; goto extract_sfmt_andb;
      case 135 : itype = FR30BF_INSN_STILM; goto extract_sfmt_stilm;
      case 136 : itype = FR30BF_INSN_BTSTL; goto extract_sfmt_btstl;
      case 137 : itype = FR30BF_INSN_BTSTH; goto extract_sfmt_btstl;
      case 138 : itype = FR30BF_INSN_XCHB; goto extract_sfmt_xchb;
      case 139 : itype = FR30BF_INSN_MOV; goto extract_sfmt_mov;
      case 140 : itype = FR30BF_INSN_LDM0; goto extract_sfmt_ldm0;
      case 141 : itype = FR30BF_INSN_LDM1; goto extract_sfmt_ldm1;
      case 142 : itype = FR30BF_INSN_STM0; goto extract_sfmt_stm0;
      case 143 : itype = FR30BF_INSN_STM1; goto extract_sfmt_stm1;
      case 144 : itype = FR30BF_INSN_BORL; goto extract_sfmt_bandl;
      case 145 : itype = FR30BF_INSN_BORH; goto extract_sfmt_bandl;
      case 146 : itype = FR30BF_INSN_OR; goto extract_sfmt_and;
      case 147 : itype = FR30BF_INSN_ORCCR; goto extract_sfmt_andccr;
      case 148 : itype = FR30BF_INSN_ORM; goto extract_sfmt_andm;
      case 149 : itype = FR30BF_INSN_ORH; goto extract_sfmt_andh;
      case 150 : itype = FR30BF_INSN_ORB; goto extract_sfmt_andb;
      case 151 :
        {
          unsigned int val = (((insn >> 4) & (15 << 0)));
          switch (val)
          {
          case 0 : itype = FR30BF_INSN_JMP; goto extract_sfmt_jmp;
          case 1 : itype = FR30BF_INSN_CALLR; goto extract_sfmt_callr;
          case 2 : itype = FR30BF_INSN_RET; goto extract_sfmt_ret;
          case 3 : itype = FR30BF_INSN_RETI; goto extract_sfmt_reti;
          case 4 : itype = FR30BF_INSN_DIV0S; goto extract_sfmt_div0s;
          case 5 : itype = FR30BF_INSN_DIV0U; goto extract_sfmt_div0u;
          case 6 : itype = FR30BF_INSN_DIV1; goto extract_sfmt_div1;
          case 7 : itype = FR30BF_INSN_DIV2; goto extract_sfmt_div2;
          case 8 : itype = FR30BF_INSN_EXTSB; goto extract_sfmt_extsb;
          case 9 : itype = FR30BF_INSN_EXTUB; goto extract_sfmt_extub;
          case 10 : itype = FR30BF_INSN_EXTSH; goto extract_sfmt_extsh;
          case 11 : itype = FR30BF_INSN_EXTUH; goto extract_sfmt_extuh;
          default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 152 : itype = FR30BF_INSN_BEORL; goto extract_sfmt_bandl;
      case 153 : itype = FR30BF_INSN_BEORH; goto extract_sfmt_bandl;
      case 154 : itype = FR30BF_INSN_EOR; goto extract_sfmt_and;
      case 155 : itype = FR30BF_INSN_LDI20; goto extract_sfmt_ldi20;
      case 156 : itype = FR30BF_INSN_EORM; goto extract_sfmt_andm;
      case 157 : itype = FR30BF_INSN_EORH; goto extract_sfmt_andh;
      case 158 : itype = FR30BF_INSN_EORB; goto extract_sfmt_andb;
      case 159 :
        {
          unsigned int val = (((insn >> 4) & (15 << 0)));
          switch (val)
          {
          case 0 : itype = FR30BF_INSN_JMPD; goto extract_sfmt_jmp;
          case 1 : itype = FR30BF_INSN_CALLRD; goto extract_sfmt_callr;
          case 2 : itype = FR30BF_INSN_RET_D; goto extract_sfmt_ret;
          case 3 : itype = FR30BF_INSN_INTE; goto extract_sfmt_inte;
          case 6 : itype = FR30BF_INSN_DIV3; goto extract_sfmt_div3;
          case 7 : itype = FR30BF_INSN_DIV4S; goto extract_sfmt_div4s;
          case 8 : itype = FR30BF_INSN_LDI32; goto extract_sfmt_ldi32;
          case 9 : itype = FR30BF_INSN_LEAVE; goto extract_sfmt_leave;
          case 10 : itype = FR30BF_INSN_NOP; goto extract_sfmt_bnod;
          case 12 : itype = FR30BF_INSN_COPOP; goto extract_sfmt_copop;
          case 13 : itype = FR30BF_INSN_COPLD; goto extract_sfmt_copop;
          case 14 : itype = FR30BF_INSN_COPST; goto extract_sfmt_copop;
          case 15 : itype = FR30BF_INSN_COPSV; goto extract_sfmt_copop;
          default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 160 : itype = FR30BF_INSN_ADDNI; goto extract_sfmt_addni;
      case 161 : itype = FR30BF_INSN_ADDN2; goto extract_sfmt_addn2;
      case 162 : itype = FR30BF_INSN_ADDN; goto extract_sfmt_addn;
      case 163 : itype = FR30BF_INSN_ADDSP; goto extract_sfmt_addsp;
      case 164 : itype = FR30BF_INSN_ADDI; goto extract_sfmt_addi;
      case 165 : itype = FR30BF_INSN_ADD2; goto extract_sfmt_add2;
      case 166 : itype = FR30BF_INSN_ADD; goto extract_sfmt_add;
      case 167 : itype = FR30BF_INSN_ADDC; goto extract_sfmt_addc;
      case 168 : itype = FR30BF_INSN_CMPI; goto extract_sfmt_cmpi;
      case 169 : itype = FR30BF_INSN_CMP2; goto extract_sfmt_cmp2;
      case 170 : itype = FR30BF_INSN_CMP; goto extract_sfmt_cmp;
      case 171 : itype = FR30BF_INSN_MULU; goto extract_sfmt_mulu;
      case 172 : itype = FR30BF_INSN_SUB; goto extract_sfmt_add;
      case 173 : itype = FR30BF_INSN_SUBC; goto extract_sfmt_addc;
      case 174 : itype = FR30BF_INSN_SUBN; goto extract_sfmt_addn;
      case 175 : itype = FR30BF_INSN_MUL; goto extract_sfmt_mul;
      case 176 : itype = FR30BF_INSN_LSRI; goto extract_sfmt_lsli;
      case 177 : itype = FR30BF_INSN_LSR2; goto extract_sfmt_lsli;
      case 178 : itype = FR30BF_INSN_LSR; goto extract_sfmt_lsl;
      case 179 : itype = FR30BF_INSN_MOV2DR; goto extract_sfmt_mov2dr;
      case 180 : itype = FR30BF_INSN_LSLI; goto extract_sfmt_lsli;
      case 181 : itype = FR30BF_INSN_LSL2; goto extract_sfmt_lsli;
      case 182 : itype = FR30BF_INSN_LSL; goto extract_sfmt_lsl;
      case 183 : itype = FR30BF_INSN_MOVDR; goto extract_sfmt_movdr;
      case 184 : itype = FR30BF_INSN_ASRI; goto extract_sfmt_lsli;
      case 185 : itype = FR30BF_INSN_ASR2; goto extract_sfmt_lsli;
      case 186 : itype = FR30BF_INSN_ASR; goto extract_sfmt_lsl;
      case 187 : itype = FR30BF_INSN_MULUH; goto extract_sfmt_mulh;
      case 188 : itype = FR30BF_INSN_LDRES; goto extract_sfmt_ldres;
      case 189 : itype = FR30BF_INSN_STRES; goto extract_sfmt_ldres;
      case 191 : itype = FR30BF_INSN_MULH; goto extract_sfmt_mulh;
      case 192 : /* fall through */
      case 193 : /* fall through */
      case 194 : /* fall through */
      case 195 : /* fall through */
      case 196 : /* fall through */
      case 197 : /* fall through */
      case 198 : /* fall through */
      case 199 : /* fall through */
      case 200 : /* fall through */
      case 201 : /* fall through */
      case 202 : /* fall through */
      case 203 : /* fall through */
      case 204 : /* fall through */
      case 205 : /* fall through */
      case 206 : /* fall through */
      case 207 : itype = FR30BF_INSN_LDI8; goto extract_sfmt_ldi8;
      case 208 : /* fall through */
      case 209 : /* fall through */
      case 210 : /* fall through */
      case 211 : /* fall through */
      case 212 : /* fall through */
      case 213 : /* fall through */
      case 214 : /* fall through */
      case 215 : itype = FR30BF_INSN_CALL; goto extract_sfmt_call;
      case 216 : /* fall through */
      case 217 : /* fall through */
      case 218 : /* fall through */
      case 219 : /* fall through */
      case 220 : /* fall through */
      case 221 : /* fall through */
      case 222 : /* fall through */
      case 223 : itype = FR30BF_INSN_CALLD; goto extract_sfmt_call;
      case 224 : itype = FR30BF_INSN_BRA; goto extract_sfmt_brad;
      case 225 : itype = FR30BF_INSN_BNO; goto extract_sfmt_bnod;
      case 226 : itype = FR30BF_INSN_BEQ; goto extract_sfmt_beqd;
      case 227 : itype = FR30BF_INSN_BNE; goto extract_sfmt_beqd;
      case 228 : itype = FR30BF_INSN_BC; goto extract_sfmt_bcd;
      case 229 : itype = FR30BF_INSN_BNC; goto extract_sfmt_bcd;
      case 230 : itype = FR30BF_INSN_BN; goto extract_sfmt_bnd;
      case 231 : itype = FR30BF_INSN_BP; goto extract_sfmt_bnd;
      case 232 : itype = FR30BF_INSN_BV; goto extract_sfmt_bvd;
      case 233 : itype = FR30BF_INSN_BNV; goto extract_sfmt_bvd;
      case 234 : itype = FR30BF_INSN_BLT; goto extract_sfmt_bltd;
      case 235 : itype = FR30BF_INSN_BGE; goto extract_sfmt_bltd;
      case 236 : itype = FR30BF_INSN_BLE; goto extract_sfmt_bled;
      case 237 : itype = FR30BF_INSN_BGT; goto extract_sfmt_bled;
      case 238 : itype = FR30BF_INSN_BLS; goto extract_sfmt_blsd;
      case 239 : itype = FR30BF_INSN_BHI; goto extract_sfmt_blsd;
      case 240 : itype = FR30BF_INSN_BRAD; goto extract_sfmt_brad;
      case 241 : itype = FR30BF_INSN_BNOD; goto extract_sfmt_bnod;
      case 242 : itype = FR30BF_INSN_BEQD; goto extract_sfmt_beqd;
      case 243 : itype = FR30BF_INSN_BNED; goto extract_sfmt_beqd;
      case 244 : itype = FR30BF_INSN_BCD; goto extract_sfmt_bcd;
      case 245 : itype = FR30BF_INSN_BNCD; goto extract_sfmt_bcd;
      case 246 : itype = FR30BF_INSN_BND; goto extract_sfmt_bnd;
      case 247 : itype = FR30BF_INSN_BPD; goto extract_sfmt_bnd;
      case 248 : itype = FR30BF_INSN_BVD; goto extract_sfmt_bvd;
      case 249 : itype = FR30BF_INSN_BNVD; goto extract_sfmt_bvd;
      case 250 : itype = FR30BF_INSN_BLTD; goto extract_sfmt_bltd;
      case 251 : itype = FR30BF_INSN_BGED; goto extract_sfmt_bltd;
      case 252 : itype = FR30BF_INSN_BLED; goto extract_sfmt_bled;
      case 253 : itype = FR30BF_INSN_BGTD; goto extract_sfmt_bled;
      case 254 : itype = FR30BF_INSN_BLSD; goto extract_sfmt_blsd;
      case 255 : itype = FR30BF_INSN_BHID; goto extract_sfmt_blsd;
      default : itype = FR30BF_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_add:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addi:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_u4;
    UINT f_Ri;

    f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addi", "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add2:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    SI f_m4;
    UINT f_Ri;

    f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4))));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_m4) = f_m4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add2", "f_m4 0x%x", 'x', f_m4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addc:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addc", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addn:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addn", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addni:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_u4;
    UINT f_Ri;

    f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addni", "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addn2:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    SI f_m4;
    UINT f_Ri;

    f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4))));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_m4) = f_m4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addn2", "f_m4 0x%x", 'x', f_m4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpi:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_u4;
    UINT f_Ri;

    f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpi", "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp2:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    SI f_m4;
    UINT f_Ri;

    f_m4 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) | (((-1) << (4))));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_m4) = f_m4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp2", "f_m4 0x%x", 'x', f_m4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andm:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andm", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andh:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andh", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andb:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andb", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bandl:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_u4;
    UINT f_Ri;

    f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bandl", "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_btstl:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_u4;
    UINT f_Ri;

    f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_btstl", "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mul:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mul", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mulu:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mulu", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mulh:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mulh", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_div0s:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div0s", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_div0u:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div0u", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_div1:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div1", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_div2:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div2", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_div3:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div3", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_div4s:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div4s", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lsl:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lsl", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_lsli:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_u4;
    UINT f_Ri;

    f_u4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lsli", "f_u4 0x%x", 'x', f_u4, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldi8:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldi8.f
    UINT f_i8;
    UINT f_Ri;

    f_i8 = EXTRACT_MSB0_UINT (insn, 16, 4, 8);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_i8) = f_i8;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi8", "f_i8 0x%x", 'x', f_i8, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldi20:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldi20.f
    UINT f_i20_16;
    UINT f_i20_4;
    UINT f_Ri;
    UINT f_i20;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUHI (current_cpu, pc + 2);
    f_i20_16 = (0|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 0));
    f_i20_4 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);
{
  f_i20 = ((((f_i20_4) << (16))) | (f_i20_16));
}

  /* Record the fields for the semantic handler.  */
  FLD (f_i20) = f_i20;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi20", "f_i20 0x%x", 'x', f_i20, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldi32:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldi32.f
    UINT f_i32;
    UINT f_Ri;
    /* Contents of trailing part of insn.  */
    UINT word_1;
    UINT word_2;

  word_1 = GETIMEMUHI (current_cpu, pc + 2);
  word_2 = GETIMEMUHI (current_cpu, pc + 4);
    f_i32 = (0|(EXTRACT_MSB0_UINT (word_2, 16, 0, 16) << 0)|(EXTRACT_MSB0_UINT (word_1, 16, 0, 16) << 16));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_i32) = f_i32;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi32", "f_i32 0x%x", 'x', f_i32, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ld:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ld", "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr13:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr13", "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rj) = f_Rj;
      FLD (in_h_gr_13) = 13;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr14:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr14.f
    SI f_disp10;
    UINT f_Ri;

    f_disp10 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (2));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_disp10) = f_disp10;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr14", "f_disp10 0x%x", 'x', f_disp10, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_14) = 14;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr14uh:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr14uh.f
    SI f_disp9;
    UINT f_Ri;

    f_disp9 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (1));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_disp9) = f_disp9;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr14uh", "f_disp9 0x%x", 'x', f_disp9, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_14) = 14;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr14ub:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr14ub.f
    INT f_disp8;
    UINT f_Ri;

    f_disp8 = EXTRACT_MSB0_INT (insn, 16, 4, 8);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_disp8) = f_disp8;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr14ub", "f_disp8 0x%x", 'x', f_disp8, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_14) = 14;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr15:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr15.f
    USI f_udisp6;
    UINT f_Ri;

    f_udisp6 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) << (2));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_udisp6) = f_udisp6;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15", "f_udisp6 0x%x", 'x', f_udisp6, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr15gr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr15gr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_Ri) = f_Ri;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15gr", "f_Ri 0x%x", 'x', f_Ri, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_Ri) = f_Ri;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr15dr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr15dr.f
    UINT f_Rs2;

    f_Rs2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_Rs2) = f_Rs2;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15dr", "f_Rs2 0x%x", 'x', f_Rs2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldr15ps:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addsp.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldr15ps", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_st:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_st", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str13:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str13", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (in_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str14:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str14.f
    SI f_disp10;
    UINT f_Ri;

    f_disp10 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (2));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_disp10) = f_disp10;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str14", "f_disp10 0x%x", 'x', f_disp10, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_h_gr_14) = 14;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str14h:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str14h.f
    SI f_disp9;
    UINT f_Ri;

    f_disp9 = ((EXTRACT_MSB0_INT (insn, 16, 4, 8)) << (1));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_disp9) = f_disp9;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str14h", "f_disp9 0x%x", 'x', f_disp9, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_h_gr_14) = 14;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str14b:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str14b.f
    INT f_disp8;
    UINT f_Ri;

    f_disp8 = EXTRACT_MSB0_INT (insn, 16, 4, 8);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_disp8) = f_disp8;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str14b", "f_disp8 0x%x", 'x', f_disp8, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_h_gr_14) = 14;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str15:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str15.f
    USI f_udisp6;
    UINT f_Ri;

    f_udisp6 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 4)) << (2));
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_udisp6) = f_udisp6;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15", "f_udisp6 0x%x", 'x', f_udisp6, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str15gr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_str15gr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15gr", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str15dr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr15dr.f
    UINT f_Rs2;

    f_Rs2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_Rs2) = f_Rs2;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15dr", "f_Rs2 0x%x", 'x', f_Rs2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_str15ps:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addsp.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_str15ps", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mov:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldr13.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mov", "Rj 0x%x", 'x', f_Rj, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movdr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_movdr.f
    UINT f_Rs1;
    UINT f_Ri;

    f_Rs1 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_Rs1) = f_Rs1;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movdr", "f_Rs1 0x%x", 'x', f_Rs1, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movps:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_movdr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movps", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mov2dr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Rs1;
    UINT f_Ri;

    f_Rs1 = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_Rs1) = f_Rs1;
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mov2dr", "f_Rs1 0x%x", 'x', f_Rs1, "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mov2ps:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mov2ps", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jmp:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jmp", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_callr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_mov2dr.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_callr", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_call:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_call.f
    SI f_rel12;

    f_rel12 = ((((EXTRACT_MSB0_INT (insn, 16, 5, 11)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label12) = f_rel12;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_call", "label12 0x%x", 'x', f_rel12, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ret:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ret", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_int:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_int.f
    UINT f_u8;

    f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_u8) = f_u8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_int", "f_u8 0x%x", 'x', f_u8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_inte:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_inte", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_reti:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_reti", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_brad:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_brad", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bnod:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bnod", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_beqd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_beqd", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bcd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bcd", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bnd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bnd", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bvd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bvd", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bltd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bltd", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bled:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bled", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_blsd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_brad.f
    SI f_rel9;

    f_rel9 = ((((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (1))) + (((pc) + (2))));

  /* Record the fields for the semantic handler.  */
  FLD (i_label9) = f_rel9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_blsd", "label9 0x%x", 'x', f_rel9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr13:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
    USI f_dir10;

    f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir10) = f_dir10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr13h:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
    USI f_dir9;

    f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir9) = f_dir9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13h", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr13b:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
    UINT f_dir8;

    f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_dir8) = f_dir8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13b", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr13pi:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
    USI f_dir10;

    f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir10) = f_dir10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13pi", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr13pih:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
    USI f_dir9;

    f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir9) = f_dir9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13pih", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr13pib:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
    UINT f_dir8;

    f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_dir8) = f_dir8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr13pib", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmovr15pi:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr15pi.f
    USI f_dir10;

    f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir10) = f_dir10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmovr15pi", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r13:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
    USI f_dir10;

    f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir10) = f_dir10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r13h:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
    USI f_dir9;

    f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir9) = f_dir9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13h", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r13b:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
    UINT f_dir8;

    f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_dir8) = f_dir8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13b", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r13pi:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
    USI f_dir10;

    f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir10) = f_dir10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13pi", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r13pih:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
    USI f_dir9;

    f_dir9 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (1));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir9) = f_dir9;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13pih", "f_dir9 0x%x", 'x', f_dir9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r13pib:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
    UINT f_dir8;

    f_dir8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_dir8) = f_dir8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r13pib", "f_dir8 0x%x", 'x', f_dir8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_13) = 13;
      FLD (out_h_gr_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dmov2r15pd:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_dmovr15pi.f
    USI f_dir10;

    f_dir10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_dir10) = f_dir10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dmov2r15pd", "f_dir10 0x%x", 'x', f_dir10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldres:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldres", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_copop:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.fmt_empty.f
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUHI (current_cpu, pc + 2);

  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_copop", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_andccr:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_int.f
    UINT f_u8;

    f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_u8) = f_u8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andccr", "f_u8 0x%x", 'x', f_u8, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_stilm:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_int.f
    UINT f_u8;

    f_u8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_u8) = f_u8;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stilm", "f_u8 0x%x", 'x', f_u8, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_addsp:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_addsp.f
    SI f_s10;

    f_s10 = ((EXTRACT_MSB0_INT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_s10) = f_s10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addsp", "f_s10 0x%x", 'x', f_s10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_extsb:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extsb", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_extub:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extub", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_extsh:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extsh", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_extuh:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add2.f
    UINT f_Ri;

    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_extuh", "Ri 0x%x", 'x', f_Ri, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldm0:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldm0.f
    UINT f_reglist_low_ld;

    f_reglist_low_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_reglist_low_ld) = f_reglist_low_ld;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldm0", "f_reglist_low_ld 0x%x", 'x', f_reglist_low_ld, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_0) = 0;
      FLD (out_h_gr_1) = 1;
      FLD (out_h_gr_15) = 15;
      FLD (out_h_gr_2) = 2;
      FLD (out_h_gr_3) = 3;
      FLD (out_h_gr_4) = 4;
      FLD (out_h_gr_5) = 5;
      FLD (out_h_gr_6) = 6;
      FLD (out_h_gr_7) = 7;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldm1:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_ldm1.f
    UINT f_reglist_hi_ld;

    f_reglist_hi_ld = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_reglist_hi_ld) = f_reglist_hi_ld;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldm1", "f_reglist_hi_ld 0x%x", 'x', f_reglist_hi_ld, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_10) = 10;
      FLD (out_h_gr_11) = 11;
      FLD (out_h_gr_12) = 12;
      FLD (out_h_gr_13) = 13;
      FLD (out_h_gr_14) = 14;
      FLD (out_h_gr_15) = 15;
      FLD (out_h_gr_8) = 8;
      FLD (out_h_gr_9) = 9;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_stm0:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stm0.f
    UINT f_reglist_low_st;

    f_reglist_low_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_reglist_low_st) = f_reglist_low_st;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stm0", "f_reglist_low_st 0x%x", 'x', f_reglist_low_st, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_0) = 0;
      FLD (in_h_gr_1) = 1;
      FLD (in_h_gr_15) = 15;
      FLD (in_h_gr_2) = 2;
      FLD (in_h_gr_3) = 3;
      FLD (in_h_gr_4) = 4;
      FLD (in_h_gr_5) = 5;
      FLD (in_h_gr_6) = 6;
      FLD (in_h_gr_7) = 7;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_stm1:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_stm1.f
    UINT f_reglist_hi_st;

    f_reglist_hi_st = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_reglist_hi_st) = f_reglist_hi_st;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stm1", "f_reglist_hi_st 0x%x", 'x', f_reglist_hi_st, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_10) = 10;
      FLD (in_h_gr_11) = 11;
      FLD (in_h_gr_12) = 12;
      FLD (in_h_gr_13) = 13;
      FLD (in_h_gr_14) = 14;
      FLD (in_h_gr_15) = 15;
      FLD (in_h_gr_8) = 8;
      FLD (in_h_gr_9) = 9;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_enter:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_enter.f
    USI f_u10;

    f_u10 = ((EXTRACT_MSB0_UINT (insn, 16, 8, 8)) << (2));

  /* Record the fields for the semantic handler.  */
  FLD (f_u10) = f_u10;
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_enter", "f_u10 0x%x", 'x', f_u10, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_14) = 14;
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_14) = 14;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_leave:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_enter.f


  /* Record the fields for the semantic handler.  */
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_leave", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_gr_14) = 14;
      FLD (in_h_gr_15) = 15;
      FLD (out_h_gr_14) = 14;
      FLD (out_h_gr_15) = 15;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_xchb:
  {
    const IDESC *idesc = &fr30bf_insn_data[itype];
    CGEN_INSN_INT insn = base_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_Rj;
    UINT f_Ri;

    f_Rj = EXTRACT_MSB0_UINT (insn, 16, 8, 4);
    f_Ri = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (i_Ri) = & CPU (h_gr)[f_Ri];
  FLD (i_Rj) = & CPU (h_gr)[f_Rj];
  TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_xchb", "Ri 0x%x", 'x', f_Ri, "Rj 0x%x", 'x', f_Rj, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ri) = f_Ri;
      FLD (in_Rj) = f_Rj;
      FLD (out_Ri) = f_Ri;
    }
#endif
#undef FLD
    return idesc;
  }

}
