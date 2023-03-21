/* Simulator instruction semantics for bpfbf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2020 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#define WANT_CPU bpfbf
#define WANT_CPU_BPFBF

#include "sim-main.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

#undef GET_ATTR
#define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)

/* This is used so that we can compile two copies of the semantic code,
   one with full feature support and one without that runs fast(er).
   FAST_P, when desired, is defined on the command line, -DFAST_P=1.  */
#if FAST_P
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_semf_,fn)
#undef CGEN_TRACE_RESULT
#define CGEN_TRACE_RESULT(cpu, abuf, name, type, val)
#else
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_sem_,fn)
#endif

/* x-invalid: --invalid-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
    /* Update the recorded pc in the cpu state struct.
       Only necessary for WITH_SCACHE case, but to avoid the
       conditional compilation ....  */
    SET_H_PC (pc);
    /* Virtual insns have zero size.  Overwrite vpc with address of next insn
       using the default-insn-bitsize spec.  When executing insns in parallel
       we may want to queue the fault and continue execution.  */
    vpc = SEM_NEXT_VPC (sem_arg, pc, 8);
    vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
  }

  return vpc;
#undef FLD
}

/* x-after: --after-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFBE
    bpfbf_ebpfbe_pbb_after (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-before: --before-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFBE
    bpfbf_ebpfbe_pbb_before (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-cti-chain: --cti-chain-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFBE
#ifdef DEFINE_SWITCH
    vpc = bpfbf_ebpfbe_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = bpfbf_ebpfbe_pbb_cti_chain (current_cpu, sem_arg,
			       CPU_PBB_BR_TYPE (current_cpu),
			       CPU_PBB_BR_NPC (current_cpu));
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* x-chain: --chain-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFBE
    vpc = bpfbf_ebpfbe_pbb_chain (current_cpu, sem_arg);
#ifdef DEFINE_SWITCH
    BREAK (sem);
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* x-begin: --begin-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFBE
#if defined DEFINE_SWITCH || defined FAST_P
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = bpfbf_ebpfbe_pbb_begin (current_cpu, FAST_P);
#else
#if 0 /* cgen engine can't handle dynamic fast/full switching yet.  */
    vpc = bpfbf_ebpfbe_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#else
    vpc = bpfbf_ebpfbe_pbb_begin (current_cpu, 0);
#endif
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* addibe: add $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,addibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* addrbe: add $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,addrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ADDDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* add32ibe: add32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,add32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ADDSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* add32rbe: add32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,add32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ADDSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* subibe: sub $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,subibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SUBDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* subrbe: sub $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,subrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SUBDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* sub32ibe: sub32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,sub32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SUBSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sub32rbe: sub32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,sub32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SUBSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mulibe: mul $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mulibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = MULDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mulrbe: mul $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mulrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = MULDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mul32ibe: mul32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mul32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = MULSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mul32rbe: mul32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mul32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = MULSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divibe: div $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,divibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UDIVDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* divrbe: div $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,divrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UDIVDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* div32ibe: div32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,div32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UDIVSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* div32rbe: div32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,div32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UDIVSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* oribe: or $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,oribe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ORDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* orrbe: or $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,orrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ORDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* or32ibe: or32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,or32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ORSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or32rbe: or32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,or32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ORSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andibe: and $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,andibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ANDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* andrbe: and $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,andrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ANDDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* and32ibe: and32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,and32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ANDSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and32rbe: and32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,and32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ANDSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lshibe: lsh $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,lshibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SLLDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lshrbe: lsh $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,lshrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SLLDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lsh32ibe: lsh32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,lsh32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SLLSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lsh32rbe: lsh32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,lsh32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SLLSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* rshibe: rsh $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,rshibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRLDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* rshrbe: rsh $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,rshrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRLDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* rsh32ibe: rsh32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,rsh32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRLSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* rsh32rbe: rsh32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,rsh32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRLSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* modibe: mod $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,modibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UMODDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* modrbe: mod $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,modrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UMODDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mod32ibe: mod32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mod32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UMODSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mod32rbe: mod32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mod32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UMODSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xoribe: xor $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,xoribe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = XORDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* xorrbe: xor $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,xorrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = XORDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* xor32ibe: xor32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,xor32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = XORSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xor32rbe: xor32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,xor32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = XORSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* arshibe: arsh $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,arshibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRADI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* arshrbe: arsh $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,arshrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRADI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* arsh32ibe: arsh32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,arsh32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRASI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* arsh32rbe: arsh32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,arsh32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRASI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* negbe: neg $dstbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,negbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lddwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = NEGDI (CPU (h_gpr[FLD (f_dstbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* neg32be: neg32 $dstbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,neg32be) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lddwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = NEGSI (CPU (h_gpr[FLD (f_dstbe)]));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* movibe: mov $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,movibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = FLD (f_imm32);
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* movrbe: mov $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,movrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = CPU (h_gpr[FLD (f_srcbe)]);
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mov32ibe: mov32 $dstbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mov32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = FLD (f_imm32);
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mov32rbe: mov32 $dstbe,$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,mov32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = CPU (h_gpr[FLD (f_srcbe)]);
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* endlebe: endle $dstbe,$endsize */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,endlebe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = bpfbf_endle (current_cpu, CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* endbebe: endbe $dstbe,$endsize */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,endbebe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = bpfbf_endbe (current_cpu, CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lddwbe: lddw $dstbe,$imm64 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,lddwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lddwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 16);

  {
    DI opval = FLD (f_imm64);
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* ldabsw: ldabsw $imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldabsw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), FLD (f_imm32)));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldabsh: ldabsh $imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldabsh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = GETMEMHI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), FLD (f_imm32)));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldabsb: ldabsb $imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldabsb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = GETMEMQI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), FLD (f_imm32)));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldabsdw: ldabsdw $imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldabsdw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = GETMEMDI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), FLD (f_imm32)));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* ldindwbe: ldindw $srcbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldindwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldindhbe: ldindh $srcbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldindhbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = GETMEMHI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldindbbe: ldindb $srcbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldindbbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = GETMEMQI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldinddwbe: ldinddw $srcbe,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldinddwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = GETMEMDI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* ldxwbe: ldxw $dstbe,[$srcbe+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldxwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldxhbe: ldxh $dstbe,[$srcbe+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldxhbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = GETMEMHI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldxbbe: ldxb $dstbe,[$srcbe+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldxbbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = GETMEMQI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldxdwbe: ldxdw $dstbe,[$srcbe+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ldxdwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcbe)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstbe)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* stxwbe: stxw [$dstbe+$offset16],$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stxwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = CPU (h_gpr[FLD (f_srcbe)]);
    SETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stxhbe: stxh [$dstbe+$offset16],$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stxhbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = CPU (h_gpr[FLD (f_srcbe)]);
    SETMEMHI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stxbbe: stxb [$dstbe+$offset16],$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stxbbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = CPU (h_gpr[FLD (f_srcbe)]);
    SETMEMQI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stxdwbe: stxdw [$dstbe+$offset16],$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stxdwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = CPU (h_gpr[FLD (f_srcbe)]);
    SETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* stbbe: stb [$dstbe+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stbbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = FLD (f_imm32);
    SETMEMQI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sthbe: sth [$dstbe+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,sthbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = FLD (f_imm32);
    SETMEMHI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stwbe: stw [$dstbe+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = FLD (f_imm32);
    SETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stdwbe: stdw [$dstbe+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,stdwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = FLD (f_imm32);
    SETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* jeqibe: jeq $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jeqibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jeqrbe: jeq $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jeqrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jeq32ibe: jeq32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jeq32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jeq32rbe: jeq32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jeq32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jgtibe: jgt $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jgtibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jgtrbe: jgt $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jgtrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jgt32ibe: jgt32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jgt32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jgt32rbe: jgt32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jgt32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jgeibe: jge $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jgeibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jgerbe: jge $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jgerbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jge32ibe: jge32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jge32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jge32rbe: jge32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jge32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jltibe: jlt $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jltibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jltrbe: jlt $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jltrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jlt32ibe: jlt32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jlt32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jlt32rbe: jlt32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jlt32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jleibe: jle $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jleibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jlerbe: jle $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jlerbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jle32ibe: jle32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jle32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jle32rbe: jle32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jle32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsetibe: jset $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsetibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsetrbe: jset $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsetrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jset32ibe: jset32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jset32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jset32rbe: jset32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jset32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jneibe: jne $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jneibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NEDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jnerbe: jne $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jnerbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NEDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jne32ibe: jne32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jne32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NESI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jne32rbe: jne32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jne32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NESI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsgtibe: jsgt $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsgtibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsgtrbe: jsgt $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsgtrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsgt32ibe: jsgt32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsgt32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsgt32rbe: jsgt32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsgt32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsgeibe: jsge $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsgeibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsgerbe: jsge $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsgerbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsge32ibe: jsge32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsge32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GESI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsge32rbe: jsge32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsge32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GESI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsltibe: jslt $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsltibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsltrbe: jslt $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsltrbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jslt32ibe: jslt32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jslt32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTSI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jslt32rbe: jslt32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jslt32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTSI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsleibe: jsle $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsleibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jslerbe: jsle $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jslerbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEDI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsle32ibe: jsle32 $dstbe,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsle32ibe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LESI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_imm32))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jsle32rbe: jsle32 $dstbe,$srcbe,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,jsle32rbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LESI (CPU (h_gpr[FLD (f_dstbe)]), CPU (h_gpr[FLD (f_srcbe)]))) {
  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* callbe: call $disp32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,callbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

bpfbf_call (current_cpu, FLD (f_imm32), FLD (f_srcbe));

  return vpc;
#undef FLD
}

/* ja: ja $disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,ja) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stbbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ADDDI (pc, MULDI (ADDHI (FLD (f_offset16), 1), 8));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'D', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* exit: exit */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,exit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

bpfbf_exit (current_cpu);

  return vpc;
#undef FLD
}

/* xadddwbe: xadddw [$dstbe+$offset16],$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,xadddwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  DI tmp_tmp;
  tmp_tmp = GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)));
  {
    DI opval = ADDDI (tmp_tmp, CPU (h_gpr[FLD (f_srcbe)]));
    SETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'D', opval);
  }
}

  return vpc;
#undef FLD
}

/* xaddwbe: xaddw [$dstbe+$offset16],$srcbe */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,xaddwbe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwbe.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_tmp;
  tmp_tmp = GETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)));
  {
    SI opval = ADDSI (tmp_tmp, CPU (h_gpr[FLD (f_srcbe)]));
    SETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstbe)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* brkpt: brkpt */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfbe,brkpt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

bpfbf_breakpoint (current_cpu);

  return vpc;
#undef FLD
}

/* Table of all semantic fns.  */

static const struct sem_fn_desc sem_fns[] = {
  { BPFBF_EBPFBE_INSN_X_INVALID, SEM_FN_NAME (bpfbf_ebpfbe,x_invalid) },
  { BPFBF_EBPFBE_INSN_X_AFTER, SEM_FN_NAME (bpfbf_ebpfbe,x_after) },
  { BPFBF_EBPFBE_INSN_X_BEFORE, SEM_FN_NAME (bpfbf_ebpfbe,x_before) },
  { BPFBF_EBPFBE_INSN_X_CTI_CHAIN, SEM_FN_NAME (bpfbf_ebpfbe,x_cti_chain) },
  { BPFBF_EBPFBE_INSN_X_CHAIN, SEM_FN_NAME (bpfbf_ebpfbe,x_chain) },
  { BPFBF_EBPFBE_INSN_X_BEGIN, SEM_FN_NAME (bpfbf_ebpfbe,x_begin) },
  { BPFBF_EBPFBE_INSN_ADDIBE, SEM_FN_NAME (bpfbf_ebpfbe,addibe) },
  { BPFBF_EBPFBE_INSN_ADDRBE, SEM_FN_NAME (bpfbf_ebpfbe,addrbe) },
  { BPFBF_EBPFBE_INSN_ADD32IBE, SEM_FN_NAME (bpfbf_ebpfbe,add32ibe) },
  { BPFBF_EBPFBE_INSN_ADD32RBE, SEM_FN_NAME (bpfbf_ebpfbe,add32rbe) },
  { BPFBF_EBPFBE_INSN_SUBIBE, SEM_FN_NAME (bpfbf_ebpfbe,subibe) },
  { BPFBF_EBPFBE_INSN_SUBRBE, SEM_FN_NAME (bpfbf_ebpfbe,subrbe) },
  { BPFBF_EBPFBE_INSN_SUB32IBE, SEM_FN_NAME (bpfbf_ebpfbe,sub32ibe) },
  { BPFBF_EBPFBE_INSN_SUB32RBE, SEM_FN_NAME (bpfbf_ebpfbe,sub32rbe) },
  { BPFBF_EBPFBE_INSN_MULIBE, SEM_FN_NAME (bpfbf_ebpfbe,mulibe) },
  { BPFBF_EBPFBE_INSN_MULRBE, SEM_FN_NAME (bpfbf_ebpfbe,mulrbe) },
  { BPFBF_EBPFBE_INSN_MUL32IBE, SEM_FN_NAME (bpfbf_ebpfbe,mul32ibe) },
  { BPFBF_EBPFBE_INSN_MUL32RBE, SEM_FN_NAME (bpfbf_ebpfbe,mul32rbe) },
  { BPFBF_EBPFBE_INSN_DIVIBE, SEM_FN_NAME (bpfbf_ebpfbe,divibe) },
  { BPFBF_EBPFBE_INSN_DIVRBE, SEM_FN_NAME (bpfbf_ebpfbe,divrbe) },
  { BPFBF_EBPFBE_INSN_DIV32IBE, SEM_FN_NAME (bpfbf_ebpfbe,div32ibe) },
  { BPFBF_EBPFBE_INSN_DIV32RBE, SEM_FN_NAME (bpfbf_ebpfbe,div32rbe) },
  { BPFBF_EBPFBE_INSN_ORIBE, SEM_FN_NAME (bpfbf_ebpfbe,oribe) },
  { BPFBF_EBPFBE_INSN_ORRBE, SEM_FN_NAME (bpfbf_ebpfbe,orrbe) },
  { BPFBF_EBPFBE_INSN_OR32IBE, SEM_FN_NAME (bpfbf_ebpfbe,or32ibe) },
  { BPFBF_EBPFBE_INSN_OR32RBE, SEM_FN_NAME (bpfbf_ebpfbe,or32rbe) },
  { BPFBF_EBPFBE_INSN_ANDIBE, SEM_FN_NAME (bpfbf_ebpfbe,andibe) },
  { BPFBF_EBPFBE_INSN_ANDRBE, SEM_FN_NAME (bpfbf_ebpfbe,andrbe) },
  { BPFBF_EBPFBE_INSN_AND32IBE, SEM_FN_NAME (bpfbf_ebpfbe,and32ibe) },
  { BPFBF_EBPFBE_INSN_AND32RBE, SEM_FN_NAME (bpfbf_ebpfbe,and32rbe) },
  { BPFBF_EBPFBE_INSN_LSHIBE, SEM_FN_NAME (bpfbf_ebpfbe,lshibe) },
  { BPFBF_EBPFBE_INSN_LSHRBE, SEM_FN_NAME (bpfbf_ebpfbe,lshrbe) },
  { BPFBF_EBPFBE_INSN_LSH32IBE, SEM_FN_NAME (bpfbf_ebpfbe,lsh32ibe) },
  { BPFBF_EBPFBE_INSN_LSH32RBE, SEM_FN_NAME (bpfbf_ebpfbe,lsh32rbe) },
  { BPFBF_EBPFBE_INSN_RSHIBE, SEM_FN_NAME (bpfbf_ebpfbe,rshibe) },
  { BPFBF_EBPFBE_INSN_RSHRBE, SEM_FN_NAME (bpfbf_ebpfbe,rshrbe) },
  { BPFBF_EBPFBE_INSN_RSH32IBE, SEM_FN_NAME (bpfbf_ebpfbe,rsh32ibe) },
  { BPFBF_EBPFBE_INSN_RSH32RBE, SEM_FN_NAME (bpfbf_ebpfbe,rsh32rbe) },
  { BPFBF_EBPFBE_INSN_MODIBE, SEM_FN_NAME (bpfbf_ebpfbe,modibe) },
  { BPFBF_EBPFBE_INSN_MODRBE, SEM_FN_NAME (bpfbf_ebpfbe,modrbe) },
  { BPFBF_EBPFBE_INSN_MOD32IBE, SEM_FN_NAME (bpfbf_ebpfbe,mod32ibe) },
  { BPFBF_EBPFBE_INSN_MOD32RBE, SEM_FN_NAME (bpfbf_ebpfbe,mod32rbe) },
  { BPFBF_EBPFBE_INSN_XORIBE, SEM_FN_NAME (bpfbf_ebpfbe,xoribe) },
  { BPFBF_EBPFBE_INSN_XORRBE, SEM_FN_NAME (bpfbf_ebpfbe,xorrbe) },
  { BPFBF_EBPFBE_INSN_XOR32IBE, SEM_FN_NAME (bpfbf_ebpfbe,xor32ibe) },
  { BPFBF_EBPFBE_INSN_XOR32RBE, SEM_FN_NAME (bpfbf_ebpfbe,xor32rbe) },
  { BPFBF_EBPFBE_INSN_ARSHIBE, SEM_FN_NAME (bpfbf_ebpfbe,arshibe) },
  { BPFBF_EBPFBE_INSN_ARSHRBE, SEM_FN_NAME (bpfbf_ebpfbe,arshrbe) },
  { BPFBF_EBPFBE_INSN_ARSH32IBE, SEM_FN_NAME (bpfbf_ebpfbe,arsh32ibe) },
  { BPFBF_EBPFBE_INSN_ARSH32RBE, SEM_FN_NAME (bpfbf_ebpfbe,arsh32rbe) },
  { BPFBF_EBPFBE_INSN_NEGBE, SEM_FN_NAME (bpfbf_ebpfbe,negbe) },
  { BPFBF_EBPFBE_INSN_NEG32BE, SEM_FN_NAME (bpfbf_ebpfbe,neg32be) },
  { BPFBF_EBPFBE_INSN_MOVIBE, SEM_FN_NAME (bpfbf_ebpfbe,movibe) },
  { BPFBF_EBPFBE_INSN_MOVRBE, SEM_FN_NAME (bpfbf_ebpfbe,movrbe) },
  { BPFBF_EBPFBE_INSN_MOV32IBE, SEM_FN_NAME (bpfbf_ebpfbe,mov32ibe) },
  { BPFBF_EBPFBE_INSN_MOV32RBE, SEM_FN_NAME (bpfbf_ebpfbe,mov32rbe) },
  { BPFBF_EBPFBE_INSN_ENDLEBE, SEM_FN_NAME (bpfbf_ebpfbe,endlebe) },
  { BPFBF_EBPFBE_INSN_ENDBEBE, SEM_FN_NAME (bpfbf_ebpfbe,endbebe) },
  { BPFBF_EBPFBE_INSN_LDDWBE, SEM_FN_NAME (bpfbf_ebpfbe,lddwbe) },
  { BPFBF_EBPFBE_INSN_LDABSW, SEM_FN_NAME (bpfbf_ebpfbe,ldabsw) },
  { BPFBF_EBPFBE_INSN_LDABSH, SEM_FN_NAME (bpfbf_ebpfbe,ldabsh) },
  { BPFBF_EBPFBE_INSN_LDABSB, SEM_FN_NAME (bpfbf_ebpfbe,ldabsb) },
  { BPFBF_EBPFBE_INSN_LDABSDW, SEM_FN_NAME (bpfbf_ebpfbe,ldabsdw) },
  { BPFBF_EBPFBE_INSN_LDINDWBE, SEM_FN_NAME (bpfbf_ebpfbe,ldindwbe) },
  { BPFBF_EBPFBE_INSN_LDINDHBE, SEM_FN_NAME (bpfbf_ebpfbe,ldindhbe) },
  { BPFBF_EBPFBE_INSN_LDINDBBE, SEM_FN_NAME (bpfbf_ebpfbe,ldindbbe) },
  { BPFBF_EBPFBE_INSN_LDINDDWBE, SEM_FN_NAME (bpfbf_ebpfbe,ldinddwbe) },
  { BPFBF_EBPFBE_INSN_LDXWBE, SEM_FN_NAME (bpfbf_ebpfbe,ldxwbe) },
  { BPFBF_EBPFBE_INSN_LDXHBE, SEM_FN_NAME (bpfbf_ebpfbe,ldxhbe) },
  { BPFBF_EBPFBE_INSN_LDXBBE, SEM_FN_NAME (bpfbf_ebpfbe,ldxbbe) },
  { BPFBF_EBPFBE_INSN_LDXDWBE, SEM_FN_NAME (bpfbf_ebpfbe,ldxdwbe) },
  { BPFBF_EBPFBE_INSN_STXWBE, SEM_FN_NAME (bpfbf_ebpfbe,stxwbe) },
  { BPFBF_EBPFBE_INSN_STXHBE, SEM_FN_NAME (bpfbf_ebpfbe,stxhbe) },
  { BPFBF_EBPFBE_INSN_STXBBE, SEM_FN_NAME (bpfbf_ebpfbe,stxbbe) },
  { BPFBF_EBPFBE_INSN_STXDWBE, SEM_FN_NAME (bpfbf_ebpfbe,stxdwbe) },
  { BPFBF_EBPFBE_INSN_STBBE, SEM_FN_NAME (bpfbf_ebpfbe,stbbe) },
  { BPFBF_EBPFBE_INSN_STHBE, SEM_FN_NAME (bpfbf_ebpfbe,sthbe) },
  { BPFBF_EBPFBE_INSN_STWBE, SEM_FN_NAME (bpfbf_ebpfbe,stwbe) },
  { BPFBF_EBPFBE_INSN_STDWBE, SEM_FN_NAME (bpfbf_ebpfbe,stdwbe) },
  { BPFBF_EBPFBE_INSN_JEQIBE, SEM_FN_NAME (bpfbf_ebpfbe,jeqibe) },
  { BPFBF_EBPFBE_INSN_JEQRBE, SEM_FN_NAME (bpfbf_ebpfbe,jeqrbe) },
  { BPFBF_EBPFBE_INSN_JEQ32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jeq32ibe) },
  { BPFBF_EBPFBE_INSN_JEQ32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jeq32rbe) },
  { BPFBF_EBPFBE_INSN_JGTIBE, SEM_FN_NAME (bpfbf_ebpfbe,jgtibe) },
  { BPFBF_EBPFBE_INSN_JGTRBE, SEM_FN_NAME (bpfbf_ebpfbe,jgtrbe) },
  { BPFBF_EBPFBE_INSN_JGT32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jgt32ibe) },
  { BPFBF_EBPFBE_INSN_JGT32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jgt32rbe) },
  { BPFBF_EBPFBE_INSN_JGEIBE, SEM_FN_NAME (bpfbf_ebpfbe,jgeibe) },
  { BPFBF_EBPFBE_INSN_JGERBE, SEM_FN_NAME (bpfbf_ebpfbe,jgerbe) },
  { BPFBF_EBPFBE_INSN_JGE32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jge32ibe) },
  { BPFBF_EBPFBE_INSN_JGE32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jge32rbe) },
  { BPFBF_EBPFBE_INSN_JLTIBE, SEM_FN_NAME (bpfbf_ebpfbe,jltibe) },
  { BPFBF_EBPFBE_INSN_JLTRBE, SEM_FN_NAME (bpfbf_ebpfbe,jltrbe) },
  { BPFBF_EBPFBE_INSN_JLT32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jlt32ibe) },
  { BPFBF_EBPFBE_INSN_JLT32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jlt32rbe) },
  { BPFBF_EBPFBE_INSN_JLEIBE, SEM_FN_NAME (bpfbf_ebpfbe,jleibe) },
  { BPFBF_EBPFBE_INSN_JLERBE, SEM_FN_NAME (bpfbf_ebpfbe,jlerbe) },
  { BPFBF_EBPFBE_INSN_JLE32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jle32ibe) },
  { BPFBF_EBPFBE_INSN_JLE32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jle32rbe) },
  { BPFBF_EBPFBE_INSN_JSETIBE, SEM_FN_NAME (bpfbf_ebpfbe,jsetibe) },
  { BPFBF_EBPFBE_INSN_JSETRBE, SEM_FN_NAME (bpfbf_ebpfbe,jsetrbe) },
  { BPFBF_EBPFBE_INSN_JSET32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jset32ibe) },
  { BPFBF_EBPFBE_INSN_JSET32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jset32rbe) },
  { BPFBF_EBPFBE_INSN_JNEIBE, SEM_FN_NAME (bpfbf_ebpfbe,jneibe) },
  { BPFBF_EBPFBE_INSN_JNERBE, SEM_FN_NAME (bpfbf_ebpfbe,jnerbe) },
  { BPFBF_EBPFBE_INSN_JNE32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jne32ibe) },
  { BPFBF_EBPFBE_INSN_JNE32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jne32rbe) },
  { BPFBF_EBPFBE_INSN_JSGTIBE, SEM_FN_NAME (bpfbf_ebpfbe,jsgtibe) },
  { BPFBF_EBPFBE_INSN_JSGTRBE, SEM_FN_NAME (bpfbf_ebpfbe,jsgtrbe) },
  { BPFBF_EBPFBE_INSN_JSGT32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jsgt32ibe) },
  { BPFBF_EBPFBE_INSN_JSGT32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jsgt32rbe) },
  { BPFBF_EBPFBE_INSN_JSGEIBE, SEM_FN_NAME (bpfbf_ebpfbe,jsgeibe) },
  { BPFBF_EBPFBE_INSN_JSGERBE, SEM_FN_NAME (bpfbf_ebpfbe,jsgerbe) },
  { BPFBF_EBPFBE_INSN_JSGE32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jsge32ibe) },
  { BPFBF_EBPFBE_INSN_JSGE32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jsge32rbe) },
  { BPFBF_EBPFBE_INSN_JSLTIBE, SEM_FN_NAME (bpfbf_ebpfbe,jsltibe) },
  { BPFBF_EBPFBE_INSN_JSLTRBE, SEM_FN_NAME (bpfbf_ebpfbe,jsltrbe) },
  { BPFBF_EBPFBE_INSN_JSLT32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jslt32ibe) },
  { BPFBF_EBPFBE_INSN_JSLT32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jslt32rbe) },
  { BPFBF_EBPFBE_INSN_JSLEIBE, SEM_FN_NAME (bpfbf_ebpfbe,jsleibe) },
  { BPFBF_EBPFBE_INSN_JSLERBE, SEM_FN_NAME (bpfbf_ebpfbe,jslerbe) },
  { BPFBF_EBPFBE_INSN_JSLE32IBE, SEM_FN_NAME (bpfbf_ebpfbe,jsle32ibe) },
  { BPFBF_EBPFBE_INSN_JSLE32RBE, SEM_FN_NAME (bpfbf_ebpfbe,jsle32rbe) },
  { BPFBF_EBPFBE_INSN_CALLBE, SEM_FN_NAME (bpfbf_ebpfbe,callbe) },
  { BPFBF_EBPFBE_INSN_JA, SEM_FN_NAME (bpfbf_ebpfbe,ja) },
  { BPFBF_EBPFBE_INSN_EXIT, SEM_FN_NAME (bpfbf_ebpfbe,exit) },
  { BPFBF_EBPFBE_INSN_XADDDWBE, SEM_FN_NAME (bpfbf_ebpfbe,xadddwbe) },
  { BPFBF_EBPFBE_INSN_XADDWBE, SEM_FN_NAME (bpfbf_ebpfbe,xaddwbe) },
  { BPFBF_EBPFBE_INSN_BRKPT, SEM_FN_NAME (bpfbf_ebpfbe,brkpt) },
  { 0, 0 }
};

/* Add the semantic fns to IDESC_TABLE.  */

void
SEM_FN_NAME (bpfbf_ebpfbe,init_idesc_table) (SIM_CPU *current_cpu)
{
  IDESC *idesc_table = CPU_IDESC (current_cpu);
  const struct sem_fn_desc *sf;
  int mach_num = MACH_NUM (CPU_MACH (current_cpu));

  for (sf = &sem_fns[0]; sf->fn != 0; ++sf)
    {
      const CGEN_INSN *insn = idesc_table[sf->index].idata;
      int valid_p = (CGEN_INSN_VIRTUAL_P (insn)
		     || CGEN_INSN_MACH_HAS_P (insn, mach_num));
#if FAST_P
      if (valid_p)
	idesc_table[sf->index].sem_fast = sf->fn;
      else
	idesc_table[sf->index].sem_fast = SEM_FN_NAME (bpfbf_ebpfbe,x_invalid);
#else
      if (valid_p)
	idesc_table[sf->index].sem_full = sf->fn;
      else
	idesc_table[sf->index].sem_full = SEM_FN_NAME (bpfbf_ebpfbe,x_invalid);
#endif
    }
}

