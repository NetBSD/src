/* Simulator instruction semantics for fr30bf.

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

#ifdef DEFINE_LABELS

  /* The labels have the case they have because the enum of insn types
     is all uppercase and in the non-stdc case the insn symbol is built
     into the enum name.  */

  static struct {
    int index;
    void *label;
  } labels[] = {
    { FR30BF_INSN_X_INVALID, && case_sem_INSN_X_INVALID },
    { FR30BF_INSN_X_AFTER, && case_sem_INSN_X_AFTER },
    { FR30BF_INSN_X_BEFORE, && case_sem_INSN_X_BEFORE },
    { FR30BF_INSN_X_CTI_CHAIN, && case_sem_INSN_X_CTI_CHAIN },
    { FR30BF_INSN_X_CHAIN, && case_sem_INSN_X_CHAIN },
    { FR30BF_INSN_X_BEGIN, && case_sem_INSN_X_BEGIN },
    { FR30BF_INSN_ADD, && case_sem_INSN_ADD },
    { FR30BF_INSN_ADDI, && case_sem_INSN_ADDI },
    { FR30BF_INSN_ADD2, && case_sem_INSN_ADD2 },
    { FR30BF_INSN_ADDC, && case_sem_INSN_ADDC },
    { FR30BF_INSN_ADDN, && case_sem_INSN_ADDN },
    { FR30BF_INSN_ADDNI, && case_sem_INSN_ADDNI },
    { FR30BF_INSN_ADDN2, && case_sem_INSN_ADDN2 },
    { FR30BF_INSN_SUB, && case_sem_INSN_SUB },
    { FR30BF_INSN_SUBC, && case_sem_INSN_SUBC },
    { FR30BF_INSN_SUBN, && case_sem_INSN_SUBN },
    { FR30BF_INSN_CMP, && case_sem_INSN_CMP },
    { FR30BF_INSN_CMPI, && case_sem_INSN_CMPI },
    { FR30BF_INSN_CMP2, && case_sem_INSN_CMP2 },
    { FR30BF_INSN_AND, && case_sem_INSN_AND },
    { FR30BF_INSN_OR, && case_sem_INSN_OR },
    { FR30BF_INSN_EOR, && case_sem_INSN_EOR },
    { FR30BF_INSN_ANDM, && case_sem_INSN_ANDM },
    { FR30BF_INSN_ANDH, && case_sem_INSN_ANDH },
    { FR30BF_INSN_ANDB, && case_sem_INSN_ANDB },
    { FR30BF_INSN_ORM, && case_sem_INSN_ORM },
    { FR30BF_INSN_ORH, && case_sem_INSN_ORH },
    { FR30BF_INSN_ORB, && case_sem_INSN_ORB },
    { FR30BF_INSN_EORM, && case_sem_INSN_EORM },
    { FR30BF_INSN_EORH, && case_sem_INSN_EORH },
    { FR30BF_INSN_EORB, && case_sem_INSN_EORB },
    { FR30BF_INSN_BANDL, && case_sem_INSN_BANDL },
    { FR30BF_INSN_BORL, && case_sem_INSN_BORL },
    { FR30BF_INSN_BEORL, && case_sem_INSN_BEORL },
    { FR30BF_INSN_BANDH, && case_sem_INSN_BANDH },
    { FR30BF_INSN_BORH, && case_sem_INSN_BORH },
    { FR30BF_INSN_BEORH, && case_sem_INSN_BEORH },
    { FR30BF_INSN_BTSTL, && case_sem_INSN_BTSTL },
    { FR30BF_INSN_BTSTH, && case_sem_INSN_BTSTH },
    { FR30BF_INSN_MUL, && case_sem_INSN_MUL },
    { FR30BF_INSN_MULU, && case_sem_INSN_MULU },
    { FR30BF_INSN_MULH, && case_sem_INSN_MULH },
    { FR30BF_INSN_MULUH, && case_sem_INSN_MULUH },
    { FR30BF_INSN_DIV0S, && case_sem_INSN_DIV0S },
    { FR30BF_INSN_DIV0U, && case_sem_INSN_DIV0U },
    { FR30BF_INSN_DIV1, && case_sem_INSN_DIV1 },
    { FR30BF_INSN_DIV2, && case_sem_INSN_DIV2 },
    { FR30BF_INSN_DIV3, && case_sem_INSN_DIV3 },
    { FR30BF_INSN_DIV4S, && case_sem_INSN_DIV4S },
    { FR30BF_INSN_LSL, && case_sem_INSN_LSL },
    { FR30BF_INSN_LSLI, && case_sem_INSN_LSLI },
    { FR30BF_INSN_LSL2, && case_sem_INSN_LSL2 },
    { FR30BF_INSN_LSR, && case_sem_INSN_LSR },
    { FR30BF_INSN_LSRI, && case_sem_INSN_LSRI },
    { FR30BF_INSN_LSR2, && case_sem_INSN_LSR2 },
    { FR30BF_INSN_ASR, && case_sem_INSN_ASR },
    { FR30BF_INSN_ASRI, && case_sem_INSN_ASRI },
    { FR30BF_INSN_ASR2, && case_sem_INSN_ASR2 },
    { FR30BF_INSN_LDI8, && case_sem_INSN_LDI8 },
    { FR30BF_INSN_LDI20, && case_sem_INSN_LDI20 },
    { FR30BF_INSN_LDI32, && case_sem_INSN_LDI32 },
    { FR30BF_INSN_LD, && case_sem_INSN_LD },
    { FR30BF_INSN_LDUH, && case_sem_INSN_LDUH },
    { FR30BF_INSN_LDUB, && case_sem_INSN_LDUB },
    { FR30BF_INSN_LDR13, && case_sem_INSN_LDR13 },
    { FR30BF_INSN_LDR13UH, && case_sem_INSN_LDR13UH },
    { FR30BF_INSN_LDR13UB, && case_sem_INSN_LDR13UB },
    { FR30BF_INSN_LDR14, && case_sem_INSN_LDR14 },
    { FR30BF_INSN_LDR14UH, && case_sem_INSN_LDR14UH },
    { FR30BF_INSN_LDR14UB, && case_sem_INSN_LDR14UB },
    { FR30BF_INSN_LDR15, && case_sem_INSN_LDR15 },
    { FR30BF_INSN_LDR15GR, && case_sem_INSN_LDR15GR },
    { FR30BF_INSN_LDR15DR, && case_sem_INSN_LDR15DR },
    { FR30BF_INSN_LDR15PS, && case_sem_INSN_LDR15PS },
    { FR30BF_INSN_ST, && case_sem_INSN_ST },
    { FR30BF_INSN_STH, && case_sem_INSN_STH },
    { FR30BF_INSN_STB, && case_sem_INSN_STB },
    { FR30BF_INSN_STR13, && case_sem_INSN_STR13 },
    { FR30BF_INSN_STR13H, && case_sem_INSN_STR13H },
    { FR30BF_INSN_STR13B, && case_sem_INSN_STR13B },
    { FR30BF_INSN_STR14, && case_sem_INSN_STR14 },
    { FR30BF_INSN_STR14H, && case_sem_INSN_STR14H },
    { FR30BF_INSN_STR14B, && case_sem_INSN_STR14B },
    { FR30BF_INSN_STR15, && case_sem_INSN_STR15 },
    { FR30BF_INSN_STR15GR, && case_sem_INSN_STR15GR },
    { FR30BF_INSN_STR15DR, && case_sem_INSN_STR15DR },
    { FR30BF_INSN_STR15PS, && case_sem_INSN_STR15PS },
    { FR30BF_INSN_MOV, && case_sem_INSN_MOV },
    { FR30BF_INSN_MOVDR, && case_sem_INSN_MOVDR },
    { FR30BF_INSN_MOVPS, && case_sem_INSN_MOVPS },
    { FR30BF_INSN_MOV2DR, && case_sem_INSN_MOV2DR },
    { FR30BF_INSN_MOV2PS, && case_sem_INSN_MOV2PS },
    { FR30BF_INSN_JMP, && case_sem_INSN_JMP },
    { FR30BF_INSN_JMPD, && case_sem_INSN_JMPD },
    { FR30BF_INSN_CALLR, && case_sem_INSN_CALLR },
    { FR30BF_INSN_CALLRD, && case_sem_INSN_CALLRD },
    { FR30BF_INSN_CALL, && case_sem_INSN_CALL },
    { FR30BF_INSN_CALLD, && case_sem_INSN_CALLD },
    { FR30BF_INSN_RET, && case_sem_INSN_RET },
    { FR30BF_INSN_RET_D, && case_sem_INSN_RET_D },
    { FR30BF_INSN_INT, && case_sem_INSN_INT },
    { FR30BF_INSN_INTE, && case_sem_INSN_INTE },
    { FR30BF_INSN_RETI, && case_sem_INSN_RETI },
    { FR30BF_INSN_BRAD, && case_sem_INSN_BRAD },
    { FR30BF_INSN_BRA, && case_sem_INSN_BRA },
    { FR30BF_INSN_BNOD, && case_sem_INSN_BNOD },
    { FR30BF_INSN_BNO, && case_sem_INSN_BNO },
    { FR30BF_INSN_BEQD, && case_sem_INSN_BEQD },
    { FR30BF_INSN_BEQ, && case_sem_INSN_BEQ },
    { FR30BF_INSN_BNED, && case_sem_INSN_BNED },
    { FR30BF_INSN_BNE, && case_sem_INSN_BNE },
    { FR30BF_INSN_BCD, && case_sem_INSN_BCD },
    { FR30BF_INSN_BC, && case_sem_INSN_BC },
    { FR30BF_INSN_BNCD, && case_sem_INSN_BNCD },
    { FR30BF_INSN_BNC, && case_sem_INSN_BNC },
    { FR30BF_INSN_BND, && case_sem_INSN_BND },
    { FR30BF_INSN_BN, && case_sem_INSN_BN },
    { FR30BF_INSN_BPD, && case_sem_INSN_BPD },
    { FR30BF_INSN_BP, && case_sem_INSN_BP },
    { FR30BF_INSN_BVD, && case_sem_INSN_BVD },
    { FR30BF_INSN_BV, && case_sem_INSN_BV },
    { FR30BF_INSN_BNVD, && case_sem_INSN_BNVD },
    { FR30BF_INSN_BNV, && case_sem_INSN_BNV },
    { FR30BF_INSN_BLTD, && case_sem_INSN_BLTD },
    { FR30BF_INSN_BLT, && case_sem_INSN_BLT },
    { FR30BF_INSN_BGED, && case_sem_INSN_BGED },
    { FR30BF_INSN_BGE, && case_sem_INSN_BGE },
    { FR30BF_INSN_BLED, && case_sem_INSN_BLED },
    { FR30BF_INSN_BLE, && case_sem_INSN_BLE },
    { FR30BF_INSN_BGTD, && case_sem_INSN_BGTD },
    { FR30BF_INSN_BGT, && case_sem_INSN_BGT },
    { FR30BF_INSN_BLSD, && case_sem_INSN_BLSD },
    { FR30BF_INSN_BLS, && case_sem_INSN_BLS },
    { FR30BF_INSN_BHID, && case_sem_INSN_BHID },
    { FR30BF_INSN_BHI, && case_sem_INSN_BHI },
    { FR30BF_INSN_DMOVR13, && case_sem_INSN_DMOVR13 },
    { FR30BF_INSN_DMOVR13H, && case_sem_INSN_DMOVR13H },
    { FR30BF_INSN_DMOVR13B, && case_sem_INSN_DMOVR13B },
    { FR30BF_INSN_DMOVR13PI, && case_sem_INSN_DMOVR13PI },
    { FR30BF_INSN_DMOVR13PIH, && case_sem_INSN_DMOVR13PIH },
    { FR30BF_INSN_DMOVR13PIB, && case_sem_INSN_DMOVR13PIB },
    { FR30BF_INSN_DMOVR15PI, && case_sem_INSN_DMOVR15PI },
    { FR30BF_INSN_DMOV2R13, && case_sem_INSN_DMOV2R13 },
    { FR30BF_INSN_DMOV2R13H, && case_sem_INSN_DMOV2R13H },
    { FR30BF_INSN_DMOV2R13B, && case_sem_INSN_DMOV2R13B },
    { FR30BF_INSN_DMOV2R13PI, && case_sem_INSN_DMOV2R13PI },
    { FR30BF_INSN_DMOV2R13PIH, && case_sem_INSN_DMOV2R13PIH },
    { FR30BF_INSN_DMOV2R13PIB, && case_sem_INSN_DMOV2R13PIB },
    { FR30BF_INSN_DMOV2R15PD, && case_sem_INSN_DMOV2R15PD },
    { FR30BF_INSN_LDRES, && case_sem_INSN_LDRES },
    { FR30BF_INSN_STRES, && case_sem_INSN_STRES },
    { FR30BF_INSN_COPOP, && case_sem_INSN_COPOP },
    { FR30BF_INSN_COPLD, && case_sem_INSN_COPLD },
    { FR30BF_INSN_COPST, && case_sem_INSN_COPST },
    { FR30BF_INSN_COPSV, && case_sem_INSN_COPSV },
    { FR30BF_INSN_NOP, && case_sem_INSN_NOP },
    { FR30BF_INSN_ANDCCR, && case_sem_INSN_ANDCCR },
    { FR30BF_INSN_ORCCR, && case_sem_INSN_ORCCR },
    { FR30BF_INSN_STILM, && case_sem_INSN_STILM },
    { FR30BF_INSN_ADDSP, && case_sem_INSN_ADDSP },
    { FR30BF_INSN_EXTSB, && case_sem_INSN_EXTSB },
    { FR30BF_INSN_EXTUB, && case_sem_INSN_EXTUB },
    { FR30BF_INSN_EXTSH, && case_sem_INSN_EXTSH },
    { FR30BF_INSN_EXTUH, && case_sem_INSN_EXTUH },
    { FR30BF_INSN_LDM0, && case_sem_INSN_LDM0 },
    { FR30BF_INSN_LDM1, && case_sem_INSN_LDM1 },
    { FR30BF_INSN_STM0, && case_sem_INSN_STM0 },
    { FR30BF_INSN_STM1, && case_sem_INSN_STM1 },
    { FR30BF_INSN_ENTER, && case_sem_INSN_ENTER },
    { FR30BF_INSN_LEAVE, && case_sem_INSN_LEAVE },
    { FR30BF_INSN_XCHB, && case_sem_INSN_XCHB },
    { 0, 0 }
  };
  int i;

  for (i = 0; labels[i].label != 0; ++i)
    {
#if FAST_P
      CPU_IDESC (current_cpu) [labels[i].index].sem_fast_lab = labels[i].label;
#else
      CPU_IDESC (current_cpu) [labels[i].index].sem_full_lab = labels[i].label;
#endif
    }

#undef DEFINE_LABELS
#endif /* DEFINE_LABELS */

#ifdef DEFINE_SWITCH

/* If hyper-fast [well not unnecessarily slow] execution is selected, turn
   off frills like tracing and profiling.  */
/* FIXME: A better way would be to have TRACE_RESULT check for something
   that can cause it to be optimized out.  Another way would be to emit
   special handlers into the instruction "stream".  */

#if FAST_P
#undef TRACE_RESULT
#define TRACE_RESULT(cpu, abuf, name, type, val)
#endif

#undef GET_ATTR
#define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)

{

#if WITH_SCACHE_PBB

/* Branch to next handler without going around main loop.  */
#define NEXT(vpc) goto * SEM_ARGBUF (vpc) -> semantic.sem_case
SWITCH (sem, SEM_ARGBUF (vpc) -> semantic.sem_case)

#else /* ! WITH_SCACHE_PBB */

#define NEXT(vpc) BREAK (sem)
#ifdef __GNUC__
#if FAST_P
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_fast_lab)
#else
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_full_lab)
#endif
#else
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->num)
#endif

#endif /* ! WITH_SCACHE_PBB */

    {

  CASE (sem, INSN_X_INVALID) : /* --invalid-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
    /* Update the recorded pc in the cpu state struct.
       Only necessary for WITH_SCACHE case, but to avoid the
       conditional compilation ....  */
    SET_H_PC (pc);
    /* Virtual insns have zero size.  Overwrite vpc with address of next insn
       using the default-insn-bitsize spec.  When executing insns in parallel
       we may want to queue the fault and continue execution.  */
    vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
    vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_AFTER) : /* --after-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
    fr30bf_pbb_after (current_cpu, sem_arg);
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_BEFORE) : /* --before-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
    fr30bf_pbb_before (current_cpu, sem_arg);
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_CTI_CHAIN) : /* --cti-chain-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
#ifdef DEFINE_SWITCH
    vpc = fr30bf_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = fr30bf_pbb_cti_chain (current_cpu, sem_arg,
			       CPU_PBB_BR_TYPE (current_cpu),
			       CPU_PBB_BR_NPC (current_cpu));
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_CHAIN) : /* --chain-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
    vpc = fr30bf_pbb_chain (current_cpu, sem_arg);
#ifdef DEFINE_SWITCH
    BREAK (sem);
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_BEGIN) : /* --begin-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
#ifdef DEFINE_SWITCH
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = fr30bf_pbb_begin (current_cpu, FAST_P);
#else
    vpc = fr30bf_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADD) : /* add $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    BI opval = ADDOFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = ADDCFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = ADDSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDI) : /* add $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    BI opval = ADDOFSI (* FLD (i_Ri), FLD (f_u4), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = ADDCFSI (* FLD (i_Ri), FLD (f_u4), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = ADDSI (* FLD (i_Ri), FLD (f_u4));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADD2) : /* add2 $m4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    BI opval = ADDOFSI (* FLD (i_Ri), FLD (f_m4), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = ADDCFSI (* FLD (i_Ri), FLD (f_m4), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = ADDSI (* FLD (i_Ri), FLD (f_m4));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDC) : /* addc $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = ADDCSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
  {
    BI opval = ADDOFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = ADDCFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = tmp_tmp;
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDN) : /* addn $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDNI) : /* addn $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), FLD (f_u4));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDN2) : /* addn2 $m4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), FLD (f_m4));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUB) : /* sub $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    BI opval = SUBOFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = SUBCFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUBC) : /* subc $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = SUBCSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
  {
    BI opval = SUBOFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = SUBCFSI (* FLD (i_Ri), * FLD (i_Rj), CPU (h_cbit));
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = tmp_tmp;
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUBN) : /* subn $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMP) : /* cmp $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp1;
  {
    BI opval = SUBOFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = SUBCFSI (* FLD (i_Ri), * FLD (i_Rj), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  tmp_tmp1 = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
{
  {
    BI opval = EQSI (tmp_tmp1, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (tmp_tmp1, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPI) : /* cmp $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp1;
  {
    BI opval = SUBOFSI (* FLD (i_Ri), FLD (f_u4), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = SUBCFSI (* FLD (i_Ri), FLD (f_u4), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  tmp_tmp1 = SUBSI (* FLD (i_Ri), FLD (f_u4));
{
  {
    BI opval = EQSI (tmp_tmp1, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (tmp_tmp1, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMP2) : /* cmp2 $m4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp1;
  {
    BI opval = SUBOFSI (* FLD (i_Ri), FLD (f_m4), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
  {
    BI opval = SUBCFSI (* FLD (i_Ri), FLD (f_m4), 0);
    CPU (h_cbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  tmp_tmp1 = SUBSI (* FLD (i_Ri), FLD (f_m4));
{
  {
    BI opval = EQSI (tmp_tmp1, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (tmp_tmp1, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_AND) : /* and $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = ANDSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_OR) : /* or $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = ORSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EOR) : /* eor $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = XORSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
{
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDM) : /* and $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = ANDSI (GETMEMSI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQSI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    SI opval = tmp_tmp;
    SETMEMSI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDH) : /* andh $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  HI tmp_tmp;
  tmp_tmp = ANDHI (GETMEMHI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQHI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTHI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    HI opval = tmp_tmp;
    SETMEMHI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDB) : /* andb $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  QI tmp_tmp;
  tmp_tmp = ANDQI (GETMEMQI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQQI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTQI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    QI opval = tmp_tmp;
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORM) : /* or $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = ORSI (GETMEMSI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQSI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    SI opval = tmp_tmp;
    SETMEMSI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORH) : /* orh $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  HI tmp_tmp;
  tmp_tmp = ORHI (GETMEMHI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQHI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTHI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    HI opval = tmp_tmp;
    SETMEMHI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORB) : /* orb $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  QI tmp_tmp;
  tmp_tmp = ORQI (GETMEMQI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQQI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTQI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    QI opval = tmp_tmp;
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EORM) : /* eor $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = XORSI (GETMEMSI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQSI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTSI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    SI opval = tmp_tmp;
    SETMEMSI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EORH) : /* eorh $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  HI tmp_tmp;
  tmp_tmp = XORHI (GETMEMHI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQHI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTHI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    HI opval = tmp_tmp;
    SETMEMHI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EORB) : /* eorb $Rj,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  QI tmp_tmp;
  tmp_tmp = XORQI (GETMEMQI (current_cpu, pc, * FLD (i_Ri)), * FLD (i_Rj));
{
  {
    BI opval = EQQI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTQI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}
  {
    QI opval = tmp_tmp;
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BANDL) : /* bandl $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ANDQI (ORQI (FLD (f_u4), 240), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BORL) : /* borl $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BEORL) : /* beorl $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = XORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BANDH) : /* bandh $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ANDQI (ORQI (SLLQI (FLD (f_u4), 4), 15), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BORH) : /* borh $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BEORH) : /* beorh $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = XORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BTSTL) : /* btstl $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  QI tmp_tmp;
  tmp_tmp = ANDQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
  {
    BI opval = EQQI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = 0;
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BTSTH) : /* btsth $u4,@$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  QI tmp_tmp;
  tmp_tmp = ANDQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
  {
    BI opval = EQQI (tmp_tmp, 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = LTQI (tmp_tmp, 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MUL) : /* mul $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  DI tmp_tmp;
  tmp_tmp = MULDI (EXTSIDI (* FLD (i_Rj)), EXTSIDI (* FLD (i_Ri)));
  {
    SI opval = TRUNCDISI (tmp_tmp);
    SET_H_DR (((UINT) 5), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_tmp, 32));
    SET_H_DR (((UINT) 4), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
  {
    BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQDI (tmp_tmp, MAKEDI (0, 0));
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = ORIF (GTDI (tmp_tmp, MAKEDI (0, 2147483647)), LTDI (tmp_tmp, NEGDI (MAKEDI (0, 0x80000000))));
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULU) : /* mulu $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  DI tmp_tmp;
  tmp_tmp = MULDI (ZEXTSIDI (* FLD (i_Rj)), ZEXTSIDI (* FLD (i_Ri)));
  {
    SI opval = TRUNCDISI (tmp_tmp);
    SET_H_DR (((UINT) 5), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
  {
    SI opval = TRUNCDISI (SRLDI (tmp_tmp, 32));
    SET_H_DR (((UINT) 4), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
  {
    BI opval = LTSI (GET_H_DR (((UINT) 4)), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    BI opval = NESI (GET_H_DR (((UINT) 4)), 0);
    CPU (h_vbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "vbit", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULH) : /* mulh $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = MULHI (TRUNCSIHI (* FLD (i_Rj)), TRUNCSIHI (* FLD (i_Ri)));
    SET_H_DR (((UINT) 5), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
  {
    BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = GESI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULUH) : /* muluh $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = MULSI (ANDSI (* FLD (i_Rj), 65535), ANDSI (* FLD (i_Ri), 65535));
    SET_H_DR (((UINT) 5), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
  {
    BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = GESI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIV0S) : /* div0s $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    BI opval = LTSI (GET_H_DR (((UINT) 5)), 0);
    CPU (h_d0bit) = opval;
    TRACE_RESULT (current_cpu, abuf, "d0bit", 'x', opval);
  }
  {
    BI opval = XORBI (CPU (h_d0bit), LTSI (* FLD (i_Ri), 0));
    CPU (h_d1bit) = opval;
    TRACE_RESULT (current_cpu, abuf, "d1bit", 'x', opval);
  }
if (NEBI (CPU (h_d0bit), 0)) {
  {
    SI opval = 0xffffffff;
    SET_H_DR (((UINT) 4), opval);
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
} else {
  {
    SI opval = 0;
    SET_H_DR (((UINT) 4), opval);
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIV0U) : /* div0u $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    BI opval = 0;
    CPU (h_d0bit) = opval;
    TRACE_RESULT (current_cpu, abuf, "d0bit", 'x', opval);
  }
  {
    BI opval = 0;
    CPU (h_d1bit) = opval;
    TRACE_RESULT (current_cpu, abuf, "d1bit", 'x', opval);
  }
  {
    SI opval = 0;
    SET_H_DR (((UINT) 4), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIV1) : /* div1 $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  {
    SI opval = SLLSI (GET_H_DR (((UINT) 4)), 1);
    SET_H_DR (((UINT) 4), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
if (LTSI (GET_H_DR (((UINT) 5)), 0)) {
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 4)), 1);
    SET_H_DR (((UINT) 4), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
}
  {
    SI opval = SLLSI (GET_H_DR (((UINT) 5)), 1);
    SET_H_DR (((UINT) 5), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
if (EQBI (CPU (h_d1bit), 1)) {
{
  tmp_tmp = ADDSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
  {
    BI opval = ADDCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 6);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
} else {
{
  tmp_tmp = SUBSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
  {
    BI opval = SUBCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 6);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
}
if (NOTBI (XORBI (XORBI (CPU (h_d0bit), CPU (h_d1bit)), CPU (h_cbit)))) {
{
  {
    SI opval = tmp_tmp;
    SET_H_DR (((UINT) 4), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
  {
    SI opval = ORSI (GET_H_DR (((UINT) 5)), 1);
    SET_H_DR (((UINT) 5), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
}
}
  {
    BI opval = EQSI (GET_H_DR (((UINT) 4)), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIV2) : /* div2 $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
if (EQBI (CPU (h_d1bit), 1)) {
{
  tmp_tmp = ADDSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
  {
    BI opval = ADDCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
} else {
{
  tmp_tmp = SUBSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri));
  {
    BI opval = SUBCFSI (GET_H_DR (((UINT) 4)), * FLD (i_Ri), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
}
if (EQSI (tmp_tmp, 0)) {
{
  {
    BI opval = 1;
    CPU (h_zbit) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
  {
    SI opval = 0;
    SET_H_DR (((UINT) 4), opval);
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "dr-4", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_zbit) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIV3) : /* div3 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (EQBI (CPU (h_zbit), 1)) {
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 5)), 1);
    SET_H_DR (((UINT) 5), opval);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIV4S) : /* div4s */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (EQBI (CPU (h_d1bit), 1)) {
  {
    SI opval = NEGSI (GET_H_DR (((UINT) 5)));
    SET_H_DR (((UINT) 5), opval);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LSL) : /* lsl $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = ANDSI (* FLD (i_Rj), 31);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (32, tmp_shift))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SLLSI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LSLI) : /* lsl $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = FLD (f_u4);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (32, tmp_shift))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SLLSI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LSL2) : /* lsl2 $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = ADDSI (FLD (f_u4), 16);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (32, tmp_shift))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SLLSI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LSR) : /* lsr $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = ANDSI (* FLD (i_Rj), 31);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SRLSI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LSRI) : /* lsr $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = FLD (f_u4);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SRLSI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LSR2) : /* lsr2 $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = ADDSI (FLD (f_u4), 16);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SRLSI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ASR) : /* asr $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = ANDSI (* FLD (i_Rj), 31);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SRASI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ASRI) : /* asr $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = FLD (f_u4);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SRASI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ASR2) : /* asr2 $u4,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_shift;
  tmp_shift = ADDSI (FLD (f_u4), 16);
if (NESI (tmp_shift, 0)) {
{
  {
    BI opval = NESI (ANDSI (* FLD (i_Ri), SLLSI (1, SUBSI (tmp_shift, 1))), 0);
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
  {
    SI opval = SRASI (* FLD (i_Ri), tmp_shift);
    * FLD (i_Ri) = opval;
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
}
} else {
  {
    BI opval = 0;
    CPU (h_cbit) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "cbit", 'x', opval);
  }
}
  {
    BI opval = LTSI (* FLD (i_Ri), 0);
    CPU (h_nbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "nbit", 'x', opval);
  }
  {
    BI opval = EQSI (* FLD (i_Ri), 0);
    CPU (h_zbit) = opval;
    TRACE_RESULT (current_cpu, abuf, "zbit", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDI8) : /* ldi:8 $i8,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldi8.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = FLD (f_i8);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDI20) : /* ldi:20 $i20,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldi20.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = FLD (f_i20);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDI32) : /* ldi:32 $i32,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldi32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 6);

  {
    SI opval = FLD (f_i32);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LD) : /* ld @$Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDUH) : /* lduh @$Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUHI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDUB) : /* ldub @$Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR13) : /* ld @($R13,$Rj),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR13UH) : /* lduh @($R13,$Rj),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR13UB) : /* ldub @($R13,$Rj),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR14) : /* ld @($R14,$disp10),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr14.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR14UH) : /* lduh @($R14,$disp9),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr14uh.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR14UB) : /* ldub @($R14,$disp8),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr14ub.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR15) : /* ld @($R15,$udisp6),$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr15.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_udisp6), CPU (h_gr[((UINT) 15)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR15GR) : /* ld @$R15+,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr15gr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
if (NESI (FLD (f_Ri), 15)) {
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR15DR) : /* ld @$R15+,$Rs2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr15dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = tmp_tmp;
    SET_H_DR (FLD (f_Rs2), opval);
    TRACE_RESULT (current_cpu, abuf, "Rs2", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDR15PS) : /* ld @$R15+,$ps */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addsp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    SET_H_PS (opval);
    TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ST) : /* st $Ri,@$Rj */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STH) : /* sth $Ri,@$Rj */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = * FLD (i_Ri);
    SETMEMHI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STB) : /* stb $Ri,@$Rj */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = * FLD (i_Ri);
    SETMEMQI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR13) : /* st $Ri,@($R13,$Rj) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR13H) : /* sth $Ri,@($R13,$Rj) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = * FLD (i_Ri);
    SETMEMHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR13B) : /* stb $Ri,@($R13,$Rj) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = * FLD (i_Ri);
    SETMEMQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR14) : /* st $Ri,@($R14,$disp10) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str14.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR14H) : /* sth $Ri,@($R14,$disp9) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str14h.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = * FLD (i_Ri);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR14B) : /* stb $Ri,@($R14,$disp8) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str14b.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = * FLD (i_Ri);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR15) : /* st $Ri,@($R15,$udisp6) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str15.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_udisp6)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR15GR) : /* st $Ri,@-$R15 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_str15gr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = * FLD (i_Ri);
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = tmp_tmp;
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR15DR) : /* st $Rs2,@-$R15 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr15dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = GET_H_DR (FLD (f_Rs2));
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = tmp_tmp;
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STR15PS) : /* st $ps,@-$R15 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addsp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = GET_H_PS ();
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOV) : /* mov $Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldr13.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Rj);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVDR) : /* mov $Rs1,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movdr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GET_H_DR (FLD (f_Rs1));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOVPS) : /* mov $ps,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_movdr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GET_H_PS ();
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOV2DR) : /* mov $Ri,$Rs1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SET_H_DR (FLD (f_Rs1), opval);
    TRACE_RESULT (current_cpu, abuf, "Rs1", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MOV2PS) : /* mov $Ri,$ps */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = * FLD (i_Ri);
    SET_H_PS (opval);
    TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_JMP) : /* jmp @$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = * FLD (i_Ri);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_JMPD) : /* jmp:d @$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = * FLD (i_Ri);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLR) : /* call @$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = ADDSI (pc, 2);
    SET_H_DR (((UINT) 1), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-1", 'x', opval);
  }
  {
    USI opval = * FLD (i_Ri);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLRD) : /* call:d @$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
{
  {
    SI opval = ADDSI (pc, 4);
    SET_H_DR (((UINT) 1), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-1", 'x', opval);
  }
  {
    USI opval = * FLD (i_Ri);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALL) : /* call $label12 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_call.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = ADDSI (pc, 2);
    SET_H_DR (((UINT) 1), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-1", 'x', opval);
  }
  {
    USI opval = FLD (i_label12);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLD) : /* call:d $label12 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_call.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
{
  {
    SI opval = ADDSI (pc, 4);
    SET_H_DR (((UINT) 1), opval);
    TRACE_RESULT (current_cpu, abuf, "dr-1", 'x', opval);
  }
  {
    USI opval = FLD (i_label12);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_RET) : /* ret */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = GET_H_DR (((UINT) 1));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_RET_D) : /* ret:d */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = GET_H_DR (((UINT) 1));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_INT) : /* int $u8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_int.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
; /*clobber*/
; /*clobber*/
; /*clobber*/
  {
    SI opval = fr30_int (current_cpu, pc, FLD (f_u8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_INTE) : /* inte */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
; /*clobber*/
; /*clobber*/
; /*clobber*/
  {
    SI opval = fr30_inte (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_RETI) : /* reti */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (EQBI (GET_H_SBIT (), 0)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 2)));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 7);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 2)), 4);
    SET_H_DR (((UINT) 2), opval);
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "dr-2", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 2)));
    SET_H_PS (opval);
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
  }
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 2)), 4);
    SET_H_DR (((UINT) 2), opval);
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "dr-2", 'x', opval);
  }
}
} else {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 3)));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 7);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 3)), 4);
    SET_H_DR (((UINT) 3), opval);
    written |= (1 << 6);
    TRACE_RESULT (current_cpu, abuf, "dr-3", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, GET_H_DR (((UINT) 3)));
    SET_H_PS (opval);
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
  }
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 3)), 4);
    SET_H_DR (((UINT) 3), opval);
    written |= (1 << 6);
    TRACE_RESULT (current_cpu, abuf, "dr-3", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BRAD) : /* bra:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BRA) : /* bra $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNOD) : /* bno:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
((void) 0); /*nop*/
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNO) : /* bno $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BEQD) : /* beq:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (CPU (h_zbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BEQ) : /* beq $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (CPU (h_zbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNED) : /* bne:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (CPU (h_zbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNE) : /* bne $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (CPU (h_zbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BCD) : /* bc:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (CPU (h_cbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BC) : /* bc $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (CPU (h_cbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNCD) : /* bnc:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (CPU (h_cbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNC) : /* bnc $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (CPU (h_cbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BND) : /* bn:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (CPU (h_nbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BN) : /* bn $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (CPU (h_nbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BPD) : /* bp:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (CPU (h_nbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BP) : /* bp $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (CPU (h_nbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BVD) : /* bv:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (CPU (h_vbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BV) : /* bv $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (CPU (h_vbit)) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNVD) : /* bnv:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (CPU (h_vbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNV) : /* bnv $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (CPU (h_vbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLTD) : /* blt:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (XORBI (CPU (h_vbit), CPU (h_nbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLT) : /* blt $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (XORBI (CPU (h_vbit), CPU (h_nbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGED) : /* bge:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (XORBI (CPU (h_vbit), CPU (h_nbit)))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGE) : /* bge $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (XORBI (CPU (h_vbit), CPU (h_nbit)))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLED) : /* ble:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLE) : /* ble $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGTD) : /* bgt:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit)))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGT) : /* bgt $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (ORBI (XORBI (CPU (h_vbit), CPU (h_nbit)), CPU (h_zbit)))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLSD) : /* bls:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (ORBI (CPU (h_cbit), CPU (h_zbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BLS) : /* bls $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (ORBI (CPU (h_cbit), CPU (h_zbit))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BHID) : /* bhi:d $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (NOTBI (ORBI (CPU (h_cbit), CPU (h_zbit)))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BHI) : /* bhi $label9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_brad.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (NOTBI (ORBI (CPU (h_cbit), CPU (h_zbit)))) {
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR13) : /* dmov $R13,@$dir10 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR13H) : /* dmovh $R13,@$dir9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMHI (current_cpu, pc, FLD (f_dir9), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR13B) : /* dmovb $R13,@$dir8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMQI (current_cpu, pc, FLD (f_dir8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR13PI) : /* dmov @$R13+,@$dir10 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 13)]));
    SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 4);
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR13PIH) : /* dmovh @$R13+,@$dir9 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    HI opval = GETMEMHI (current_cpu, pc, CPU (h_gr[((UINT) 13)]));
    SETMEMHI (current_cpu, pc, FLD (f_dir9), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 2);
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR13PIB) : /* dmovb @$R13+,@$dir8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    QI opval = GETMEMQI (current_cpu, pc, CPU (h_gr[((UINT) 13)]));
    SETMEMQI (current_cpu, pc, FLD (f_dir8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 1);
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOVR15PI) : /* dmov @$R15+,@$dir10 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr15pi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R13) : /* dmov @$dir10,$R13 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R13H) : /* dmovh @$dir9,$R13 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMHI (current_cpu, pc, FLD (f_dir9));
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R13B) : /* dmovb @$dir8,$R13 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMQI (current_cpu, pc, FLD (f_dir8));
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R13PI) : /* dmov @$dir10,@$R13+ */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 13)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 4);
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R13PIH) : /* dmovh @$dir9,@$R13+ */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    HI opval = GETMEMHI (current_cpu, pc, FLD (f_dir9));
    SETMEMHI (current_cpu, pc, CPU (h_gr[((UINT) 13)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 2);
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R13PIB) : /* dmovb @$dir8,@$R13+ */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    QI opval = GETMEMQI (current_cpu, pc, FLD (f_dir8));
    SETMEMQI (current_cpu, pc, CPU (h_gr[((UINT) 13)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 13)]), 1);
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DMOV2R15PD) : /* dmov @$dir10,@-$R15 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_dmovr15pi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDRES) : /* ldres @$Ri+,$u4 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), 4);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STRES) : /* stres $u4,@$Ri+ */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), 4);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_COPOP) : /* copop $u4c,$ccc,$CRj,$CRi */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_COPLD) : /* copld $u4c,$ccc,$Rjc,$CRi */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_COPST) : /* copst $u4c,$ccc,$CRj,$Ric */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_COPSV) : /* copsv $u4c,$ccc,$CRj,$Ric */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOP) : /* nop */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.fmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDCCR) : /* andccr $u8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_int.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    UQI opval = ANDQI (GET_H_CCR (), FLD (f_u8));
    SET_H_CCR (opval);
    TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORCCR) : /* orccr $u8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_int.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    UQI opval = ORQI (GET_H_CCR (), FLD (f_u8));
    SET_H_CCR (opval);
    TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STILM) : /* stilm $u8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_int.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    UQI opval = ANDSI (FLD (f_u8), 31);
    SET_H_ILM (opval);
    TRACE_RESULT (current_cpu, abuf, "ilm", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDSP) : /* addsp $s10 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addsp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_s10));
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EXTSB) : /* extsb $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = EXTQISI (ANDQI (* FLD (i_Ri), 255));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EXTUB) : /* extub $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ZEXTQISI (ANDQI (* FLD (i_Ri), 255));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EXTSH) : /* extsh $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = EXTHISI (ANDHI (* FLD (i_Ri), 65535));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_EXTUH) : /* extuh $Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add2.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ZEXTHISI (ANDHI (* FLD (i_Ri), 65535));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDM0) : /* ldm0 ($reglist_low_ld) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldm0.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (ANDSI (FLD (f_reglist_low_ld), 1)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 0)]) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "gr-0", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 2)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 1)]) = opval;
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "gr-1", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 4)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 2)]) = opval;
    written |= (1 << 6);
    TRACE_RESULT (current_cpu, abuf, "gr-2", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 8)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 3)]) = opval;
    written |= (1 << 7);
    TRACE_RESULT (current_cpu, abuf, "gr-3", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 16)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 4)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-4", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 32)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 5)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-5", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 64)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 6)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-6", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_ld), 128)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 7)]) = opval;
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "gr-7", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LDM1) : /* ldm1 ($reglist_hi_ld) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_ldm1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (ANDSI (FLD (f_reglist_hi_ld), 1)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 8)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-8", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 2)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 9)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-9", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 4)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 10)]) = opval;
    written |= (1 << 3);
    TRACE_RESULT (current_cpu, abuf, "gr-10", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 8)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 11)]) = opval;
    written |= (1 << 4);
    TRACE_RESULT (current_cpu, abuf, "gr-11", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 16)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 12)]) = opval;
    written |= (1 << 5);
    TRACE_RESULT (current_cpu, abuf, "gr-12", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 32)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 13)]) = opval;
    written |= (1 << 6);
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 64)) {
{
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 14)]) = opval;
    written |= (1 << 7);
    TRACE_RESULT (current_cpu, abuf, "gr-14", 'x', opval);
  }
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_ld), 128)) {
  {
    SI opval = GETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]));
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 8);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STM0) : /* stm0 ($reglist_low_st) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stm0.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (ANDSI (FLD (f_reglist_low_st), 1)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 7)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 2)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 6)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 4)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 5)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 8)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 4)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 16)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 3)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 32)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 2)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 64)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 1)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_low_st), 128)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 0)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 11);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_STM1) : /* stm1 ($reglist_hi_st) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_stm1.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
if (ANDSI (FLD (f_reglist_hi_st), 1)) {
{
  SI tmp_save_r15;
  tmp_save_r15 = CPU (h_gr[((UINT) 15)]);
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = tmp_save_r15;
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 2)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 14)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 4)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 8)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 12)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 16)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 11)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 32)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 10)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 64)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 9)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
if (ANDSI (FLD (f_reglist_hi_st), 128)) {
{
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    written |= (1 << 9);
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = CPU (h_gr[((UINT) 8)]);
    SETMEMSI (current_cpu, pc, CPU (h_gr[((UINT) 15)]), opval);
    written |= (1 << 10);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ENTER) : /* enter $u10 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_enter.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = SUBSI (CPU (h_gr[((UINT) 15)]), 4);
  {
    SI opval = CPU (h_gr[((UINT) 14)]);
    SETMEMSI (current_cpu, pc, tmp_tmp, opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
  {
    SI opval = tmp_tmp;
    CPU (h_gr[((UINT) 14)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-14", 'x', opval);
  }
  {
    SI opval = SUBSI (CPU (h_gr[((UINT) 15)]), FLD (f_u10));
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LEAVE) : /* leave */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_enter.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 14)]), 4);
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }
  {
    SI opval = GETMEMSI (current_cpu, pc, SUBSI (CPU (h_gr[((UINT) 15)]), 4));
    CPU (h_gr[((UINT) 14)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-14", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XCHB) : /* xchb @$Rj,$Ri */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_add.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  SI tmp_tmp;
  tmp_tmp = * FLD (i_Ri);
  {
    SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }
  {
    UQI opval = tmp_tmp;
    SETMEMUQI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);


    }
  ENDSWITCH (sem) /* End of semantic switch.  */

  /* At this point `vpc' contains the next insn to execute.  */
}

#undef DEFINE_SWITCH
#endif /* DEFINE_SWITCH */
