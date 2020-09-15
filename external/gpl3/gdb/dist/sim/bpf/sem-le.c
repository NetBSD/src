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
SEM_FN_NAME (bpfbf_ebpfle,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
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
SEM_FN_NAME (bpfbf_ebpfle,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFLE
    bpfbf_ebpfle_pbb_after (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-before: --before-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFLE
    bpfbf_ebpfle_pbb_before (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-cti-chain: --cti-chain-- */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFLE
#ifdef DEFINE_SWITCH
    vpc = bpfbf_ebpfle_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = bpfbf_ebpfle_pbb_cti_chain (current_cpu, sem_arg,
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
SEM_FN_NAME (bpfbf_ebpfle,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFLE
    vpc = bpfbf_ebpfle_pbb_chain (current_cpu, sem_arg);
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
SEM_FN_NAME (bpfbf_ebpfle,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_BPFBF_EBPFLE
#if defined DEFINE_SWITCH || defined FAST_P
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = bpfbf_ebpfle_pbb_begin (current_cpu, FAST_P);
#else
#if 0 /* cgen engine can't handle dynamic fast/full switching yet.  */
    vpc = bpfbf_ebpfle_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#else
    vpc = bpfbf_ebpfle_pbb_begin (current_cpu, 0);
#endif
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* addile: add $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,addile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* addrle: add $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,addrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ADDDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* add32ile: add32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,add32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ADDSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* add32rle: add32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,add32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ADDSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* subile: sub $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,subile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SUBDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* subrle: sub $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,subrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SUBDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* sub32ile: sub32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,sub32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SUBSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sub32rle: sub32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,sub32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SUBSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mulile: mul $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mulile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = MULDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mulrle: mul $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mulrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = MULDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mul32ile: mul32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mul32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = MULSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mul32rle: mul32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mul32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = MULSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divile: div $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,divile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UDIVDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* divrle: div $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,divrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UDIVDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* div32ile: div32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,div32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UDIVSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* div32rle: div32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,div32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UDIVSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* orile: or $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,orile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ORDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* orrle: or $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,orrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ORDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* or32ile: or32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,or32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ORSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or32rle: or32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,or32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ORSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andile: and $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,andile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ANDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* andrle: and $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,andrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = ANDDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* and32ile: and32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,and32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ANDSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and32rle: and32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,and32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = ANDSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lshile: lsh $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,lshile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SLLDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lshrle: lsh $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,lshrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SLLDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lsh32ile: lsh32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,lsh32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SLLSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lsh32rle: lsh32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,lsh32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SLLSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* rshile: rsh $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,rshile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRLDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* rshrle: rsh $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,rshrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRLDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* rsh32ile: rsh32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,rsh32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRLSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* rsh32rle: rsh32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,rsh32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRLSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* modile: mod $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,modile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UMODDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* modrle: mod $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,modrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = UMODDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mod32ile: mod32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mod32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UMODSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mod32rle: mod32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mod32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = UMODSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xorile: xor $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,xorile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = XORDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* xorrle: xor $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,xorrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = XORDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* xor32ile: xor32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,xor32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = XORSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xor32rle: xor32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,xor32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = XORSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* arshile: arsh $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,arshile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRADI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* arshrle: arsh $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,arshrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = SRADI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* arsh32ile: arsh32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,arsh32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRASI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* arsh32rle: arsh32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,arsh32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = SRASI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* negle: neg $dstle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,negle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lddwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = NEGDI (CPU (h_gpr[FLD (f_dstle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* neg32le: neg32 $dstle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,neg32le) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lddwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = NEGSI (CPU (h_gpr[FLD (f_dstle)]));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* movile: mov $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,movile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = FLD (f_imm32);
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* movrle: mov $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,movrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = CPU (h_gpr[FLD (f_srcle)]);
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* mov32ile: mov32 $dstle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mov32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = FLD (f_imm32);
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mov32rle: mov32 $dstle,$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,mov32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    USI opval = CPU (h_gpr[FLD (f_srcle)]);
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* endlele: endle $dstle,$endsize */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,endlele) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = bpfbf_endle (current_cpu, CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* endbele: endbe $dstle,$endsize */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,endbele) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = bpfbf_endbe (current_cpu, CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lddwle: lddw $dstle,$imm64 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,lddwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lddwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 16);

  {
    DI opval = FLD (f_imm64);
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* ldabsw: ldabsw $imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldabsw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
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
SEM_FN_NAME (bpfbf_ebpfle,ldabsh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
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
SEM_FN_NAME (bpfbf_ebpfle,ldabsb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
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
SEM_FN_NAME (bpfbf_ebpfle,ldabsdw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
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

/* ldindwle: ldindw $srcle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldindwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldindhle: ldindh $srcle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldindhle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = GETMEMHI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldindble: ldindb $srcle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldindble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = GETMEMQI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldinddwle: ldinddw $srcle,$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldinddwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = GETMEMDI (current_cpu, pc, ADDDI (GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[((UINT) 6)]), bpfbf_skb_data_offset (current_cpu))), ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_imm32))));
    CPU (h_gpr[((UINT) 0)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* ldxwle: ldxw $dstle,[$srcle+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldxwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldxhle: ldxh $dstle,[$srcle+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldxhle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = GETMEMHI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldxble: ldxb $dstle,[$srcle+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldxble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = GETMEMQI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldxdwle: ldxdw $dstle,[$srcle+$offset16] */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ldxdwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_srcle)]), FLD (f_offset16)));
    CPU (h_gpr[FLD (f_dstle)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* stxwle: stxw [$dstle+$offset16],$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stxwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = CPU (h_gpr[FLD (f_srcle)]);
    SETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stxhle: stxh [$dstle+$offset16],$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stxhle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = CPU (h_gpr[FLD (f_srcle)]);
    SETMEMHI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stxble: stxb [$dstle+$offset16],$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stxble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = CPU (h_gpr[FLD (f_srcle)]);
    SETMEMQI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stxdwle: stxdw [$dstle+$offset16],$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stxdwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = CPU (h_gpr[FLD (f_srcle)]);
    SETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* stble: stb [$dstle+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    QI opval = FLD (f_imm32);
    SETMEMQI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sthle: sth [$dstle+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,sthle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    HI opval = FLD (f_imm32);
    SETMEMHI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stwle: stw [$dstle+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    SI opval = FLD (f_imm32);
    SETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stdwle: stdw [$dstle+$offset16],$imm32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,stdwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

  {
    DI opval = FLD (f_imm32);
    SETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* jeqile: jeq $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jeqile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jeqrle: jeq $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jeqrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jeq32ile: jeq32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jeq32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jeq32rle: jeq32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jeq32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (EQSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jgtile: jgt $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jgtile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jgtrle: jgt $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jgtrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jgt32ile: jgt32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jgt32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jgt32rle: jgt32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jgt32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTUSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jgeile: jge $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jgeile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jgerle: jge $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jgerle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jge32ile: jge32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jge32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jge32rle: jge32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jge32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEUSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jltile: jlt $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jltile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jltrle: jlt $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jltrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jlt32ile: jlt32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jlt32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jlt32rle: jlt32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jlt32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTUSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jleile: jle $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jleile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jlerle: jle $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jlerle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jle32ile: jle32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jle32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jle32rle: jle32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jle32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEUSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsetile: jset $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsetile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsetrle: jset $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsetrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jset32ile: jset32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jset32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jset32rle: jset32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jset32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (ANDSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jneile: jne $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jneile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NEDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jnerle: jne $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jnerle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NEDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jne32ile: jne32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jne32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NESI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jne32rle: jne32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jne32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (NESI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsgtile: jsgt $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsgtile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsgtrle: jsgt $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsgtrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsgt32ile: jsgt32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsgt32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsgt32rle: jsgt32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsgt32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GTSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsgeile: jsge $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsgeile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsgerle: jsge $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsgerle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GEDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsge32ile: jsge32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsge32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GESI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsge32rle: jsge32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsge32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (GESI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsltile: jslt $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsltile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsltrle: jslt $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsltrle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jslt32ile: jslt32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jslt32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTSI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jslt32rle: jslt32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jslt32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LTSI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsleile: jsle $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsleile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jslerle: jsle $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jslerle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LEDI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* jsle32ile: jsle32 $dstle,$imm32,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsle32ile) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LESI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_imm32))) {
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

/* jsle32rle: jsle32 $dstle,$srcle,$disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,jsle32rle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

if (LESI (CPU (h_gpr[FLD (f_dstle)]), CPU (h_gpr[FLD (f_srcle)]))) {
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

/* callle: call $disp32 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,callle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldindwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

bpfbf_call (current_cpu, FLD (f_imm32), FLD (f_srcle));

  return vpc;
#undef FLD
}

/* ja: ja $disp16 */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,ja) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stble.f
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
SEM_FN_NAME (bpfbf_ebpfle,exit) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
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

/* xadddwle: xadddw [$dstle+$offset16],$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,xadddwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  DI tmp_tmp;
  tmp_tmp = GETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)));
  {
    DI opval = ADDDI (tmp_tmp, CPU (h_gpr[FLD (f_srcle)]));
    SETMEMDI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'D', opval);
  }
}

  return vpc;
#undef FLD
}

/* xaddwle: xaddw [$dstle+$offset16],$srcle */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,xaddwle) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldxwle.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 8);

{
  SI tmp_tmp;
  tmp_tmp = GETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)));
  {
    SI opval = ADDSI (tmp_tmp, CPU (h_gpr[FLD (f_srcle)]));
    SETMEMSI (current_cpu, pc, ADDDI (CPU (h_gpr[FLD (f_dstle)]), FLD (f_offset16)), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}

  return vpc;
#undef FLD
}

/* brkpt: brkpt */

static SEM_PC
SEM_FN_NAME (bpfbf_ebpfle,brkpt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
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
  { BPFBF_EBPFLE_INSN_X_INVALID, SEM_FN_NAME (bpfbf_ebpfle,x_invalid) },
  { BPFBF_EBPFLE_INSN_X_AFTER, SEM_FN_NAME (bpfbf_ebpfle,x_after) },
  { BPFBF_EBPFLE_INSN_X_BEFORE, SEM_FN_NAME (bpfbf_ebpfle,x_before) },
  { BPFBF_EBPFLE_INSN_X_CTI_CHAIN, SEM_FN_NAME (bpfbf_ebpfle,x_cti_chain) },
  { BPFBF_EBPFLE_INSN_X_CHAIN, SEM_FN_NAME (bpfbf_ebpfle,x_chain) },
  { BPFBF_EBPFLE_INSN_X_BEGIN, SEM_FN_NAME (bpfbf_ebpfle,x_begin) },
  { BPFBF_EBPFLE_INSN_ADDILE, SEM_FN_NAME (bpfbf_ebpfle,addile) },
  { BPFBF_EBPFLE_INSN_ADDRLE, SEM_FN_NAME (bpfbf_ebpfle,addrle) },
  { BPFBF_EBPFLE_INSN_ADD32ILE, SEM_FN_NAME (bpfbf_ebpfle,add32ile) },
  { BPFBF_EBPFLE_INSN_ADD32RLE, SEM_FN_NAME (bpfbf_ebpfle,add32rle) },
  { BPFBF_EBPFLE_INSN_SUBILE, SEM_FN_NAME (bpfbf_ebpfle,subile) },
  { BPFBF_EBPFLE_INSN_SUBRLE, SEM_FN_NAME (bpfbf_ebpfle,subrle) },
  { BPFBF_EBPFLE_INSN_SUB32ILE, SEM_FN_NAME (bpfbf_ebpfle,sub32ile) },
  { BPFBF_EBPFLE_INSN_SUB32RLE, SEM_FN_NAME (bpfbf_ebpfle,sub32rle) },
  { BPFBF_EBPFLE_INSN_MULILE, SEM_FN_NAME (bpfbf_ebpfle,mulile) },
  { BPFBF_EBPFLE_INSN_MULRLE, SEM_FN_NAME (bpfbf_ebpfle,mulrle) },
  { BPFBF_EBPFLE_INSN_MUL32ILE, SEM_FN_NAME (bpfbf_ebpfle,mul32ile) },
  { BPFBF_EBPFLE_INSN_MUL32RLE, SEM_FN_NAME (bpfbf_ebpfle,mul32rle) },
  { BPFBF_EBPFLE_INSN_DIVILE, SEM_FN_NAME (bpfbf_ebpfle,divile) },
  { BPFBF_EBPFLE_INSN_DIVRLE, SEM_FN_NAME (bpfbf_ebpfle,divrle) },
  { BPFBF_EBPFLE_INSN_DIV32ILE, SEM_FN_NAME (bpfbf_ebpfle,div32ile) },
  { BPFBF_EBPFLE_INSN_DIV32RLE, SEM_FN_NAME (bpfbf_ebpfle,div32rle) },
  { BPFBF_EBPFLE_INSN_ORILE, SEM_FN_NAME (bpfbf_ebpfle,orile) },
  { BPFBF_EBPFLE_INSN_ORRLE, SEM_FN_NAME (bpfbf_ebpfle,orrle) },
  { BPFBF_EBPFLE_INSN_OR32ILE, SEM_FN_NAME (bpfbf_ebpfle,or32ile) },
  { BPFBF_EBPFLE_INSN_OR32RLE, SEM_FN_NAME (bpfbf_ebpfle,or32rle) },
  { BPFBF_EBPFLE_INSN_ANDILE, SEM_FN_NAME (bpfbf_ebpfle,andile) },
  { BPFBF_EBPFLE_INSN_ANDRLE, SEM_FN_NAME (bpfbf_ebpfle,andrle) },
  { BPFBF_EBPFLE_INSN_AND32ILE, SEM_FN_NAME (bpfbf_ebpfle,and32ile) },
  { BPFBF_EBPFLE_INSN_AND32RLE, SEM_FN_NAME (bpfbf_ebpfle,and32rle) },
  { BPFBF_EBPFLE_INSN_LSHILE, SEM_FN_NAME (bpfbf_ebpfle,lshile) },
  { BPFBF_EBPFLE_INSN_LSHRLE, SEM_FN_NAME (bpfbf_ebpfle,lshrle) },
  { BPFBF_EBPFLE_INSN_LSH32ILE, SEM_FN_NAME (bpfbf_ebpfle,lsh32ile) },
  { BPFBF_EBPFLE_INSN_LSH32RLE, SEM_FN_NAME (bpfbf_ebpfle,lsh32rle) },
  { BPFBF_EBPFLE_INSN_RSHILE, SEM_FN_NAME (bpfbf_ebpfle,rshile) },
  { BPFBF_EBPFLE_INSN_RSHRLE, SEM_FN_NAME (bpfbf_ebpfle,rshrle) },
  { BPFBF_EBPFLE_INSN_RSH32ILE, SEM_FN_NAME (bpfbf_ebpfle,rsh32ile) },
  { BPFBF_EBPFLE_INSN_RSH32RLE, SEM_FN_NAME (bpfbf_ebpfle,rsh32rle) },
  { BPFBF_EBPFLE_INSN_MODILE, SEM_FN_NAME (bpfbf_ebpfle,modile) },
  { BPFBF_EBPFLE_INSN_MODRLE, SEM_FN_NAME (bpfbf_ebpfle,modrle) },
  { BPFBF_EBPFLE_INSN_MOD32ILE, SEM_FN_NAME (bpfbf_ebpfle,mod32ile) },
  { BPFBF_EBPFLE_INSN_MOD32RLE, SEM_FN_NAME (bpfbf_ebpfle,mod32rle) },
  { BPFBF_EBPFLE_INSN_XORILE, SEM_FN_NAME (bpfbf_ebpfle,xorile) },
  { BPFBF_EBPFLE_INSN_XORRLE, SEM_FN_NAME (bpfbf_ebpfle,xorrle) },
  { BPFBF_EBPFLE_INSN_XOR32ILE, SEM_FN_NAME (bpfbf_ebpfle,xor32ile) },
  { BPFBF_EBPFLE_INSN_XOR32RLE, SEM_FN_NAME (bpfbf_ebpfle,xor32rle) },
  { BPFBF_EBPFLE_INSN_ARSHILE, SEM_FN_NAME (bpfbf_ebpfle,arshile) },
  { BPFBF_EBPFLE_INSN_ARSHRLE, SEM_FN_NAME (bpfbf_ebpfle,arshrle) },
  { BPFBF_EBPFLE_INSN_ARSH32ILE, SEM_FN_NAME (bpfbf_ebpfle,arsh32ile) },
  { BPFBF_EBPFLE_INSN_ARSH32RLE, SEM_FN_NAME (bpfbf_ebpfle,arsh32rle) },
  { BPFBF_EBPFLE_INSN_NEGLE, SEM_FN_NAME (bpfbf_ebpfle,negle) },
  { BPFBF_EBPFLE_INSN_NEG32LE, SEM_FN_NAME (bpfbf_ebpfle,neg32le) },
  { BPFBF_EBPFLE_INSN_MOVILE, SEM_FN_NAME (bpfbf_ebpfle,movile) },
  { BPFBF_EBPFLE_INSN_MOVRLE, SEM_FN_NAME (bpfbf_ebpfle,movrle) },
  { BPFBF_EBPFLE_INSN_MOV32ILE, SEM_FN_NAME (bpfbf_ebpfle,mov32ile) },
  { BPFBF_EBPFLE_INSN_MOV32RLE, SEM_FN_NAME (bpfbf_ebpfle,mov32rle) },
  { BPFBF_EBPFLE_INSN_ENDLELE, SEM_FN_NAME (bpfbf_ebpfle,endlele) },
  { BPFBF_EBPFLE_INSN_ENDBELE, SEM_FN_NAME (bpfbf_ebpfle,endbele) },
  { BPFBF_EBPFLE_INSN_LDDWLE, SEM_FN_NAME (bpfbf_ebpfle,lddwle) },
  { BPFBF_EBPFLE_INSN_LDABSW, SEM_FN_NAME (bpfbf_ebpfle,ldabsw) },
  { BPFBF_EBPFLE_INSN_LDABSH, SEM_FN_NAME (bpfbf_ebpfle,ldabsh) },
  { BPFBF_EBPFLE_INSN_LDABSB, SEM_FN_NAME (bpfbf_ebpfle,ldabsb) },
  { BPFBF_EBPFLE_INSN_LDABSDW, SEM_FN_NAME (bpfbf_ebpfle,ldabsdw) },
  { BPFBF_EBPFLE_INSN_LDINDWLE, SEM_FN_NAME (bpfbf_ebpfle,ldindwle) },
  { BPFBF_EBPFLE_INSN_LDINDHLE, SEM_FN_NAME (bpfbf_ebpfle,ldindhle) },
  { BPFBF_EBPFLE_INSN_LDINDBLE, SEM_FN_NAME (bpfbf_ebpfle,ldindble) },
  { BPFBF_EBPFLE_INSN_LDINDDWLE, SEM_FN_NAME (bpfbf_ebpfle,ldinddwle) },
  { BPFBF_EBPFLE_INSN_LDXWLE, SEM_FN_NAME (bpfbf_ebpfle,ldxwle) },
  { BPFBF_EBPFLE_INSN_LDXHLE, SEM_FN_NAME (bpfbf_ebpfle,ldxhle) },
  { BPFBF_EBPFLE_INSN_LDXBLE, SEM_FN_NAME (bpfbf_ebpfle,ldxble) },
  { BPFBF_EBPFLE_INSN_LDXDWLE, SEM_FN_NAME (bpfbf_ebpfle,ldxdwle) },
  { BPFBF_EBPFLE_INSN_STXWLE, SEM_FN_NAME (bpfbf_ebpfle,stxwle) },
  { BPFBF_EBPFLE_INSN_STXHLE, SEM_FN_NAME (bpfbf_ebpfle,stxhle) },
  { BPFBF_EBPFLE_INSN_STXBLE, SEM_FN_NAME (bpfbf_ebpfle,stxble) },
  { BPFBF_EBPFLE_INSN_STXDWLE, SEM_FN_NAME (bpfbf_ebpfle,stxdwle) },
  { BPFBF_EBPFLE_INSN_STBLE, SEM_FN_NAME (bpfbf_ebpfle,stble) },
  { BPFBF_EBPFLE_INSN_STHLE, SEM_FN_NAME (bpfbf_ebpfle,sthle) },
  { BPFBF_EBPFLE_INSN_STWLE, SEM_FN_NAME (bpfbf_ebpfle,stwle) },
  { BPFBF_EBPFLE_INSN_STDWLE, SEM_FN_NAME (bpfbf_ebpfle,stdwle) },
  { BPFBF_EBPFLE_INSN_JEQILE, SEM_FN_NAME (bpfbf_ebpfle,jeqile) },
  { BPFBF_EBPFLE_INSN_JEQRLE, SEM_FN_NAME (bpfbf_ebpfle,jeqrle) },
  { BPFBF_EBPFLE_INSN_JEQ32ILE, SEM_FN_NAME (bpfbf_ebpfle,jeq32ile) },
  { BPFBF_EBPFLE_INSN_JEQ32RLE, SEM_FN_NAME (bpfbf_ebpfle,jeq32rle) },
  { BPFBF_EBPFLE_INSN_JGTILE, SEM_FN_NAME (bpfbf_ebpfle,jgtile) },
  { BPFBF_EBPFLE_INSN_JGTRLE, SEM_FN_NAME (bpfbf_ebpfle,jgtrle) },
  { BPFBF_EBPFLE_INSN_JGT32ILE, SEM_FN_NAME (bpfbf_ebpfle,jgt32ile) },
  { BPFBF_EBPFLE_INSN_JGT32RLE, SEM_FN_NAME (bpfbf_ebpfle,jgt32rle) },
  { BPFBF_EBPFLE_INSN_JGEILE, SEM_FN_NAME (bpfbf_ebpfle,jgeile) },
  { BPFBF_EBPFLE_INSN_JGERLE, SEM_FN_NAME (bpfbf_ebpfle,jgerle) },
  { BPFBF_EBPFLE_INSN_JGE32ILE, SEM_FN_NAME (bpfbf_ebpfle,jge32ile) },
  { BPFBF_EBPFLE_INSN_JGE32RLE, SEM_FN_NAME (bpfbf_ebpfle,jge32rle) },
  { BPFBF_EBPFLE_INSN_JLTILE, SEM_FN_NAME (bpfbf_ebpfle,jltile) },
  { BPFBF_EBPFLE_INSN_JLTRLE, SEM_FN_NAME (bpfbf_ebpfle,jltrle) },
  { BPFBF_EBPFLE_INSN_JLT32ILE, SEM_FN_NAME (bpfbf_ebpfle,jlt32ile) },
  { BPFBF_EBPFLE_INSN_JLT32RLE, SEM_FN_NAME (bpfbf_ebpfle,jlt32rle) },
  { BPFBF_EBPFLE_INSN_JLEILE, SEM_FN_NAME (bpfbf_ebpfle,jleile) },
  { BPFBF_EBPFLE_INSN_JLERLE, SEM_FN_NAME (bpfbf_ebpfle,jlerle) },
  { BPFBF_EBPFLE_INSN_JLE32ILE, SEM_FN_NAME (bpfbf_ebpfle,jle32ile) },
  { BPFBF_EBPFLE_INSN_JLE32RLE, SEM_FN_NAME (bpfbf_ebpfle,jle32rle) },
  { BPFBF_EBPFLE_INSN_JSETILE, SEM_FN_NAME (bpfbf_ebpfle,jsetile) },
  { BPFBF_EBPFLE_INSN_JSETRLE, SEM_FN_NAME (bpfbf_ebpfle,jsetrle) },
  { BPFBF_EBPFLE_INSN_JSET32ILE, SEM_FN_NAME (bpfbf_ebpfle,jset32ile) },
  { BPFBF_EBPFLE_INSN_JSET32RLE, SEM_FN_NAME (bpfbf_ebpfle,jset32rle) },
  { BPFBF_EBPFLE_INSN_JNEILE, SEM_FN_NAME (bpfbf_ebpfle,jneile) },
  { BPFBF_EBPFLE_INSN_JNERLE, SEM_FN_NAME (bpfbf_ebpfle,jnerle) },
  { BPFBF_EBPFLE_INSN_JNE32ILE, SEM_FN_NAME (bpfbf_ebpfle,jne32ile) },
  { BPFBF_EBPFLE_INSN_JNE32RLE, SEM_FN_NAME (bpfbf_ebpfle,jne32rle) },
  { BPFBF_EBPFLE_INSN_JSGTILE, SEM_FN_NAME (bpfbf_ebpfle,jsgtile) },
  { BPFBF_EBPFLE_INSN_JSGTRLE, SEM_FN_NAME (bpfbf_ebpfle,jsgtrle) },
  { BPFBF_EBPFLE_INSN_JSGT32ILE, SEM_FN_NAME (bpfbf_ebpfle,jsgt32ile) },
  { BPFBF_EBPFLE_INSN_JSGT32RLE, SEM_FN_NAME (bpfbf_ebpfle,jsgt32rle) },
  { BPFBF_EBPFLE_INSN_JSGEILE, SEM_FN_NAME (bpfbf_ebpfle,jsgeile) },
  { BPFBF_EBPFLE_INSN_JSGERLE, SEM_FN_NAME (bpfbf_ebpfle,jsgerle) },
  { BPFBF_EBPFLE_INSN_JSGE32ILE, SEM_FN_NAME (bpfbf_ebpfle,jsge32ile) },
  { BPFBF_EBPFLE_INSN_JSGE32RLE, SEM_FN_NAME (bpfbf_ebpfle,jsge32rle) },
  { BPFBF_EBPFLE_INSN_JSLTILE, SEM_FN_NAME (bpfbf_ebpfle,jsltile) },
  { BPFBF_EBPFLE_INSN_JSLTRLE, SEM_FN_NAME (bpfbf_ebpfle,jsltrle) },
  { BPFBF_EBPFLE_INSN_JSLT32ILE, SEM_FN_NAME (bpfbf_ebpfle,jslt32ile) },
  { BPFBF_EBPFLE_INSN_JSLT32RLE, SEM_FN_NAME (bpfbf_ebpfle,jslt32rle) },
  { BPFBF_EBPFLE_INSN_JSLEILE, SEM_FN_NAME (bpfbf_ebpfle,jsleile) },
  { BPFBF_EBPFLE_INSN_JSLERLE, SEM_FN_NAME (bpfbf_ebpfle,jslerle) },
  { BPFBF_EBPFLE_INSN_JSLE32ILE, SEM_FN_NAME (bpfbf_ebpfle,jsle32ile) },
  { BPFBF_EBPFLE_INSN_JSLE32RLE, SEM_FN_NAME (bpfbf_ebpfle,jsle32rle) },
  { BPFBF_EBPFLE_INSN_CALLLE, SEM_FN_NAME (bpfbf_ebpfle,callle) },
  { BPFBF_EBPFLE_INSN_JA, SEM_FN_NAME (bpfbf_ebpfle,ja) },
  { BPFBF_EBPFLE_INSN_EXIT, SEM_FN_NAME (bpfbf_ebpfle,exit) },
  { BPFBF_EBPFLE_INSN_XADDDWLE, SEM_FN_NAME (bpfbf_ebpfle,xadddwle) },
  { BPFBF_EBPFLE_INSN_XADDWLE, SEM_FN_NAME (bpfbf_ebpfle,xaddwle) },
  { BPFBF_EBPFLE_INSN_BRKPT, SEM_FN_NAME (bpfbf_ebpfle,brkpt) },
  { 0, 0 }
};

/* Add the semantic fns to IDESC_TABLE.  */

void
SEM_FN_NAME (bpfbf_ebpfle,init_idesc_table) (SIM_CPU *current_cpu)
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
	idesc_table[sf->index].sem_fast = SEM_FN_NAME (bpfbf_ebpfle,x_invalid);
#else
      if (valid_p)
	idesc_table[sf->index].sem_full = sf->fn;
      else
	idesc_table[sf->index].sem_full = SEM_FN_NAME (bpfbf_ebpfle,x_invalid);
#endif
    }
}

