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

#define WANT_CPU fr30bf
#define WANT_CPU_FR30BF

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
#undef TRACE_RESULT
#define TRACE_RESULT(cpu, abuf, name, type, val)
#else
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_sem_,fn)
#endif

/* x-invalid: --invalid-- */

static SEM_PC
SEM_FN_NAME (fr30bf,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
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
    vpc = SEM_NEXT_VPC (sem_arg, pc, 2);
    vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
  }

  return vpc;
#undef FLD
}

/* x-after: --after-- */

static SEM_PC
SEM_FN_NAME (fr30bf,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
    fr30bf_pbb_after (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-before: --before-- */

static SEM_PC
SEM_FN_NAME (fr30bf,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
    fr30bf_pbb_before (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-cti-chain: --cti-chain-- */

static SEM_PC
SEM_FN_NAME (fr30bf,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* x-chain: --chain-- */

static SEM_PC
SEM_FN_NAME (fr30bf,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_FR30BF
    vpc = fr30bf_pbb_chain (current_cpu, sem_arg);
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
SEM_FN_NAME (fr30bf,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* add: add $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,add) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* addi: add $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,addi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* add2: add2 $m4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,add2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* addc: addc $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,addc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* addn: addn $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,addn) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addni: addn $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,addni) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), FLD (f_u4));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addn2: addn2 $m4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,addn2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), FLD (f_m4));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sub: sub $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,sub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* subc: subc $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,subc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* subn: subn $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,subn) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = SUBSI (* FLD (i_Ri), * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmp: cmp $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,cmp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* cmpi: cmp $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,cmpi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* cmp2: cmp2 $m4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,cmp2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* and: and $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,and) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* or: or $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,or) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* eor: eor $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,eor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* andm: and $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,andm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* andh: andh $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,andh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* andb: andb $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,andb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* orm: or $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,orm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* orh: orh $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,orh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* orb: orb $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,orb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* eorm: eor $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,eorm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* eorh: eorh $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,eorh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* eorb: eorb $Rj,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,eorb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* bandl: bandl $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,bandl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ANDQI (ORQI (FLD (f_u4), 240), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* borl: borl $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,borl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* beorl: beorl $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,beorl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = XORQI (FLD (f_u4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* bandh: bandh $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,bandh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ANDQI (ORQI (SLLQI (FLD (f_u4), 4), 15), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* borh: borh $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,borh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = ORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* beorh: beorh $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,beorh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = XORQI (SLLQI (FLD (f_u4), 4), GETMEMQI (current_cpu, pc, * FLD (i_Ri)));
    SETMEMQI (current_cpu, pc, * FLD (i_Ri), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* btstl: btstl $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,btstl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* btsth: btsth $u4,@$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,btsth) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* mul: mul $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,mul) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* mulu: mulu $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,mulu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* mulh: mulh $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,mulh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* muluh: muluh $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,muluh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* div0s: div0s $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,div0s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* div0u: div0u $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,div0u) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* div1: div1 $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,div1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* div2: div2 $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,div2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* div3: div3 */

static SEM_PC
SEM_FN_NAME (fr30bf,div3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (EQBI (CPU (h_zbit), 1)) {
  {
    SI opval = ADDSI (GET_H_DR (((UINT) 5)), 1);
    SET_H_DR (((UINT) 5), opval);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
}

  abuf->written = written;
  return vpc;
#undef FLD
}

/* div4s: div4s */

static SEM_PC
SEM_FN_NAME (fr30bf,div4s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

if (EQBI (CPU (h_d1bit), 1)) {
  {
    SI opval = NEGSI (GET_H_DR (((UINT) 5)));
    SET_H_DR (((UINT) 5), opval);
    written |= (1 << 2);
    TRACE_RESULT (current_cpu, abuf, "dr-5", 'x', opval);
  }
}

  abuf->written = written;
  return vpc;
#undef FLD
}

/* lsl: lsl $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lsl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* lsli: lsl $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lsli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* lsl2: lsl2 $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lsl2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* lsr: lsr $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lsr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* lsri: lsr $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lsri) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* lsr2: lsr2 $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lsr2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* asr: asr $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,asr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* asri: asr $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,asri) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* asr2: asr2 $u4,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,asr2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* ldi8: ldi:8 $i8,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldi8) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldi8.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = FLD (f_i8);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldi20: ldi:20 $i20,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldi20) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldi20.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = FLD (f_i20);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldi32: ldi:32 $i32,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldi32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldi32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 6);

  {
    SI opval = FLD (f_i32);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ld: ld @$Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lduh: lduh @$Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,lduh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUHI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldub: ldub @$Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUQI (current_cpu, pc, * FLD (i_Rj));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr13: ld @($R13,$Rj),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr13uh: lduh @($R13,$Rj),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr13uh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr13ub: ldub @($R13,$Rj),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr13ub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr14: ld @($R14,$disp10),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr14) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr14.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr14uh: lduh @($R14,$disp9),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr14uh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr14uh.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr14ub: ldub @($R14,$disp8),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr14ub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr14ub.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMUQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr15: ld @($R15,$udisp6),$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr15) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr15.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (FLD (f_udisp6), CPU (h_gr[((UINT) 15)])));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldr15gr: ld @$R15+,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr15gr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr15gr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* ldr15dr: ld @$R15+,$Rs2 */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr15dr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr15dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* ldr15ps: ld @$R15+,$ps */

static SEM_PC
SEM_FN_NAME (fr30bf,ldr15ps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addsp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* st: st $Ri,@$Rj */

static SEM_PC
SEM_FN_NAME (fr30bf,st) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sth: sth $Ri,@$Rj */

static SEM_PC
SEM_FN_NAME (fr30bf,sth) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = * FLD (i_Ri);
    SETMEMHI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stb: stb $Ri,@$Rj */

static SEM_PC
SEM_FN_NAME (fr30bf,stb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = * FLD (i_Ri);
    SETMEMQI (current_cpu, pc, * FLD (i_Rj), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str13: st $Ri,@($R13,$Rj) */

static SEM_PC
SEM_FN_NAME (fr30bf,str13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str13h: sth $Ri,@($R13,$Rj) */

static SEM_PC
SEM_FN_NAME (fr30bf,str13h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = * FLD (i_Ri);
    SETMEMHI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str13b: stb $Ri,@($R13,$Rj) */

static SEM_PC
SEM_FN_NAME (fr30bf,str13b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = * FLD (i_Ri);
    SETMEMQI (current_cpu, pc, ADDSI (* FLD (i_Rj), CPU (h_gr[((UINT) 13)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str14: st $Ri,@($R14,$disp10) */

static SEM_PC
SEM_FN_NAME (fr30bf,str14) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str14.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, ADDSI (FLD (f_disp10), CPU (h_gr[((UINT) 14)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str14h: sth $Ri,@($R14,$disp9) */

static SEM_PC
SEM_FN_NAME (fr30bf,str14h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str14h.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = * FLD (i_Ri);
    SETMEMHI (current_cpu, pc, ADDSI (FLD (f_disp9), CPU (h_gr[((UINT) 14)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str14b: stb $Ri,@($R14,$disp8) */

static SEM_PC
SEM_FN_NAME (fr30bf,str14b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str14b.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = * FLD (i_Ri);
    SETMEMQI (current_cpu, pc, ADDSI (FLD (f_disp8), CPU (h_gr[((UINT) 14)])), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str15: st $Ri,@($R15,$udisp6) */

static SEM_PC
SEM_FN_NAME (fr30bf,str15) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str15.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_udisp6)), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* str15gr: st $Ri,@-$R15 */

static SEM_PC
SEM_FN_NAME (fr30bf,str15gr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_str15gr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* str15dr: st $Rs2,@-$R15 */

static SEM_PC
SEM_FN_NAME (fr30bf,str15dr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr15dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* str15ps: st $ps,@-$R15 */

static SEM_PC
SEM_FN_NAME (fr30bf,str15ps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addsp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* mov: mov $Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,mov) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldr13.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Rj);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* movdr: mov $Rs1,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,movdr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movdr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GET_H_DR (FLD (f_Rs1));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* movps: mov $ps,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,movps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_movdr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GET_H_PS ();
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mov2dr: mov $Ri,$Rs1 */

static SEM_PC
SEM_FN_NAME (fr30bf,mov2dr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = * FLD (i_Ri);
    SET_H_DR (FLD (f_Rs1), opval);
    TRACE_RESULT (current_cpu, abuf, "Rs1", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* mov2ps: mov $Ri,$ps */

static SEM_PC
SEM_FN_NAME (fr30bf,mov2ps) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = * FLD (i_Ri);
    SET_H_PS (opval);
    TRACE_RESULT (current_cpu, abuf, "ps", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* jmp: jmp @$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,jmp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = * FLD (i_Ri);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* jmpd: jmp:d @$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,jmpd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = * FLD (i_Ri);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* callr: call @$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,callr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* callrd: call:d @$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,callrd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_mov2dr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* call: call $label12 */

static SEM_PC
SEM_FN_NAME (fr30bf,call) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_call.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* calld: call:d $label12 */

static SEM_PC
SEM_FN_NAME (fr30bf,calld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_call.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* ret: ret */

static SEM_PC
SEM_FN_NAME (fr30bf,ret) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = GET_H_DR (((UINT) 1));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* ret:d: ret:d */

static SEM_PC
SEM_FN_NAME (fr30bf,ret_d) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = GET_H_DR (((UINT) 1));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* int: int $u8 */

static SEM_PC
SEM_FN_NAME (fr30bf,int) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_int.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* inte: inte */

static SEM_PC
SEM_FN_NAME (fr30bf,inte) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* reti: reti */

static SEM_PC
SEM_FN_NAME (fr30bf,reti) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* brad: bra:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,brad) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bra: bra $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bra) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    USI opval = FLD (i_label9);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bnod: bno:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bnod) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

{
((void) 0); /*nop*/
}

  return vpc;
#undef FLD
}

/* bno: bno $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bno) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* beqd: beq:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,beqd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* beq: beq $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,beq) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bned: bne:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bned) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bne: bne $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bne) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bcd: bc:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bcd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bc: bc $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bncd: bnc:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bncd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bnc: bnc $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bnc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bnd: bn:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bnd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bn: bn $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bn) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bpd: bp:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bpd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bp: bp $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bvd: bv:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bvd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bv: bv $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bv) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bnvd: bnv:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bnvd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bnv: bnv $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bnv) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bltd: blt:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bltd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* blt: blt $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,blt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bged: bge:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bged) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bge: bge $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bge) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bled: ble:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bled) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* ble: ble $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,ble) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bgtd: bgt:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bgtd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bgt: bgt $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bgt) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* blsd: bls:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,blsd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bls: bls $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bls) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bhid: bhi:d $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bhid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* bhi: bhi $label9 */

static SEM_PC
SEM_FN_NAME (fr30bf,bhi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_brad.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* dmovr13: dmov $R13,@$dir10 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMSI (current_cpu, pc, FLD (f_dir10), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* dmovr13h: dmovh $R13,@$dir9 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr13h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    HI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMHI (current_cpu, pc, FLD (f_dir9), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* dmovr13b: dmovb $R13,@$dir8 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr13b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    QI opval = CPU (h_gr[((UINT) 13)]);
    SETMEMQI (current_cpu, pc, FLD (f_dir8), opval);
    TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* dmovr13pi: dmov @$R13+,@$dir10 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr13pi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmovr13pih: dmovh @$R13+,@$dir9 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr13pih) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmovr13pib: dmovb @$R13+,@$dir8 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr13pib) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmovr15pi: dmov @$R15+,@$dir10 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmovr15pi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr15pi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmov2r13: dmov @$dir10,$R13 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r13) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMSI (current_cpu, pc, FLD (f_dir10));
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* dmov2r13h: dmovh @$dir9,$R13 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r13h) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMHI (current_cpu, pc, FLD (f_dir9));
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* dmov2r13b: dmovb @$dir8,$R13 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r13b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = GETMEMQI (current_cpu, pc, FLD (f_dir8));
    CPU (h_gr[((UINT) 13)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-13", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* dmov2r13pi: dmov @$dir10,@$R13+ */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r13pi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmov2r13pih: dmovh @$dir9,@$R13+ */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r13pih) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pih.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmov2r13pib: dmovb @$dir8,@$R13+ */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r13pib) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr13pib.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* dmov2r15pd: dmov @$dir10,@-$R15 */

static SEM_PC
SEM_FN_NAME (fr30bf,dmov2r15pd) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_dmovr15pi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* ldres: ldres @$Ri+,$u4 */

static SEM_PC
SEM_FN_NAME (fr30bf,ldres) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), 4);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stres: stres $u4,@$Ri+ */

static SEM_PC
SEM_FN_NAME (fr30bf,stres) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (* FLD (i_Ri), 4);
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* copop: copop $u4c,$ccc,$CRj,$CRi */

static SEM_PC
SEM_FN_NAME (fr30bf,copop) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* copld: copld $u4c,$ccc,$Rjc,$CRi */

static SEM_PC
SEM_FN_NAME (fr30bf,copld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* copst: copst $u4c,$ccc,$CRj,$Ric */

static SEM_PC
SEM_FN_NAME (fr30bf,copst) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* copsv: copsv $u4c,$ccc,$CRj,$Ric */

static SEM_PC
SEM_FN_NAME (fr30bf,copsv) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* nop: nop */

static SEM_PC
SEM_FN_NAME (fr30bf,nop) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.fmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* andccr: andccr $u8 */

static SEM_PC
SEM_FN_NAME (fr30bf,andccr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_int.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    UQI opval = ANDQI (GET_H_CCR (), FLD (f_u8));
    SET_H_CCR (opval);
    TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* orccr: orccr $u8 */

static SEM_PC
SEM_FN_NAME (fr30bf,orccr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_int.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    UQI opval = ORQI (GET_H_CCR (), FLD (f_u8));
    SET_H_CCR (opval);
    TRACE_RESULT (current_cpu, abuf, "ccr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* stilm: stilm $u8 */

static SEM_PC
SEM_FN_NAME (fr30bf,stilm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_int.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    UQI opval = ANDSI (FLD (f_u8), 31);
    SET_H_ILM (opval);
    TRACE_RESULT (current_cpu, abuf, "ilm", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addsp: addsp $s10 */

static SEM_PC
SEM_FN_NAME (fr30bf,addsp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addsp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ADDSI (CPU (h_gr[((UINT) 15)]), FLD (f_s10));
    CPU (h_gr[((UINT) 15)]) = opval;
    TRACE_RESULT (current_cpu, abuf, "gr-15", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* extsb: extsb $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,extsb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = EXTQISI (ANDQI (* FLD (i_Ri), 255));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* extub: extub $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,extub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ZEXTQISI (ANDQI (* FLD (i_Ri), 255));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* extsh: extsh $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,extsh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = EXTHISI (ANDHI (* FLD (i_Ri), 65535));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* extuh: extuh $Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,extuh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add2.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

  {
    SI opval = ZEXTHISI (ANDHI (* FLD (i_Ri), 65535));
    * FLD (i_Ri) = opval;
    TRACE_RESULT (current_cpu, abuf, "Ri", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ldm0: ldm0 ($reglist_low_ld) */

static SEM_PC
SEM_FN_NAME (fr30bf,ldm0) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldm0.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* ldm1: ldm1 ($reglist_hi_ld) */

static SEM_PC
SEM_FN_NAME (fr30bf,ldm1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_ldm1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* stm0: stm0 ($reglist_low_st) */

static SEM_PC
SEM_FN_NAME (fr30bf,stm0) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stm0.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* stm1: stm1 ($reglist_hi_st) */

static SEM_PC
SEM_FN_NAME (fr30bf,stm1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_stm1.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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
  return vpc;
#undef FLD
}

/* enter: enter $u10 */

static SEM_PC
SEM_FN_NAME (fr30bf,enter) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_enter.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* leave: leave */

static SEM_PC
SEM_FN_NAME (fr30bf,leave) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_enter.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* xchb: xchb @$Rj,$Ri */

static SEM_PC
SEM_FN_NAME (fr30bf,xchb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_add.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 2);

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

  return vpc;
#undef FLD
}

/* Table of all semantic fns.  */

static const struct sem_fn_desc sem_fns[] = {
  { FR30BF_INSN_X_INVALID, SEM_FN_NAME (fr30bf,x_invalid) },
  { FR30BF_INSN_X_AFTER, SEM_FN_NAME (fr30bf,x_after) },
  { FR30BF_INSN_X_BEFORE, SEM_FN_NAME (fr30bf,x_before) },
  { FR30BF_INSN_X_CTI_CHAIN, SEM_FN_NAME (fr30bf,x_cti_chain) },
  { FR30BF_INSN_X_CHAIN, SEM_FN_NAME (fr30bf,x_chain) },
  { FR30BF_INSN_X_BEGIN, SEM_FN_NAME (fr30bf,x_begin) },
  { FR30BF_INSN_ADD, SEM_FN_NAME (fr30bf,add) },
  { FR30BF_INSN_ADDI, SEM_FN_NAME (fr30bf,addi) },
  { FR30BF_INSN_ADD2, SEM_FN_NAME (fr30bf,add2) },
  { FR30BF_INSN_ADDC, SEM_FN_NAME (fr30bf,addc) },
  { FR30BF_INSN_ADDN, SEM_FN_NAME (fr30bf,addn) },
  { FR30BF_INSN_ADDNI, SEM_FN_NAME (fr30bf,addni) },
  { FR30BF_INSN_ADDN2, SEM_FN_NAME (fr30bf,addn2) },
  { FR30BF_INSN_SUB, SEM_FN_NAME (fr30bf,sub) },
  { FR30BF_INSN_SUBC, SEM_FN_NAME (fr30bf,subc) },
  { FR30BF_INSN_SUBN, SEM_FN_NAME (fr30bf,subn) },
  { FR30BF_INSN_CMP, SEM_FN_NAME (fr30bf,cmp) },
  { FR30BF_INSN_CMPI, SEM_FN_NAME (fr30bf,cmpi) },
  { FR30BF_INSN_CMP2, SEM_FN_NAME (fr30bf,cmp2) },
  { FR30BF_INSN_AND, SEM_FN_NAME (fr30bf,and) },
  { FR30BF_INSN_OR, SEM_FN_NAME (fr30bf,or) },
  { FR30BF_INSN_EOR, SEM_FN_NAME (fr30bf,eor) },
  { FR30BF_INSN_ANDM, SEM_FN_NAME (fr30bf,andm) },
  { FR30BF_INSN_ANDH, SEM_FN_NAME (fr30bf,andh) },
  { FR30BF_INSN_ANDB, SEM_FN_NAME (fr30bf,andb) },
  { FR30BF_INSN_ORM, SEM_FN_NAME (fr30bf,orm) },
  { FR30BF_INSN_ORH, SEM_FN_NAME (fr30bf,orh) },
  { FR30BF_INSN_ORB, SEM_FN_NAME (fr30bf,orb) },
  { FR30BF_INSN_EORM, SEM_FN_NAME (fr30bf,eorm) },
  { FR30BF_INSN_EORH, SEM_FN_NAME (fr30bf,eorh) },
  { FR30BF_INSN_EORB, SEM_FN_NAME (fr30bf,eorb) },
  { FR30BF_INSN_BANDL, SEM_FN_NAME (fr30bf,bandl) },
  { FR30BF_INSN_BORL, SEM_FN_NAME (fr30bf,borl) },
  { FR30BF_INSN_BEORL, SEM_FN_NAME (fr30bf,beorl) },
  { FR30BF_INSN_BANDH, SEM_FN_NAME (fr30bf,bandh) },
  { FR30BF_INSN_BORH, SEM_FN_NAME (fr30bf,borh) },
  { FR30BF_INSN_BEORH, SEM_FN_NAME (fr30bf,beorh) },
  { FR30BF_INSN_BTSTL, SEM_FN_NAME (fr30bf,btstl) },
  { FR30BF_INSN_BTSTH, SEM_FN_NAME (fr30bf,btsth) },
  { FR30BF_INSN_MUL, SEM_FN_NAME (fr30bf,mul) },
  { FR30BF_INSN_MULU, SEM_FN_NAME (fr30bf,mulu) },
  { FR30BF_INSN_MULH, SEM_FN_NAME (fr30bf,mulh) },
  { FR30BF_INSN_MULUH, SEM_FN_NAME (fr30bf,muluh) },
  { FR30BF_INSN_DIV0S, SEM_FN_NAME (fr30bf,div0s) },
  { FR30BF_INSN_DIV0U, SEM_FN_NAME (fr30bf,div0u) },
  { FR30BF_INSN_DIV1, SEM_FN_NAME (fr30bf,div1) },
  { FR30BF_INSN_DIV2, SEM_FN_NAME (fr30bf,div2) },
  { FR30BF_INSN_DIV3, SEM_FN_NAME (fr30bf,div3) },
  { FR30BF_INSN_DIV4S, SEM_FN_NAME (fr30bf,div4s) },
  { FR30BF_INSN_LSL, SEM_FN_NAME (fr30bf,lsl) },
  { FR30BF_INSN_LSLI, SEM_FN_NAME (fr30bf,lsli) },
  { FR30BF_INSN_LSL2, SEM_FN_NAME (fr30bf,lsl2) },
  { FR30BF_INSN_LSR, SEM_FN_NAME (fr30bf,lsr) },
  { FR30BF_INSN_LSRI, SEM_FN_NAME (fr30bf,lsri) },
  { FR30BF_INSN_LSR2, SEM_FN_NAME (fr30bf,lsr2) },
  { FR30BF_INSN_ASR, SEM_FN_NAME (fr30bf,asr) },
  { FR30BF_INSN_ASRI, SEM_FN_NAME (fr30bf,asri) },
  { FR30BF_INSN_ASR2, SEM_FN_NAME (fr30bf,asr2) },
  { FR30BF_INSN_LDI8, SEM_FN_NAME (fr30bf,ldi8) },
  { FR30BF_INSN_LDI20, SEM_FN_NAME (fr30bf,ldi20) },
  { FR30BF_INSN_LDI32, SEM_FN_NAME (fr30bf,ldi32) },
  { FR30BF_INSN_LD, SEM_FN_NAME (fr30bf,ld) },
  { FR30BF_INSN_LDUH, SEM_FN_NAME (fr30bf,lduh) },
  { FR30BF_INSN_LDUB, SEM_FN_NAME (fr30bf,ldub) },
  { FR30BF_INSN_LDR13, SEM_FN_NAME (fr30bf,ldr13) },
  { FR30BF_INSN_LDR13UH, SEM_FN_NAME (fr30bf,ldr13uh) },
  { FR30BF_INSN_LDR13UB, SEM_FN_NAME (fr30bf,ldr13ub) },
  { FR30BF_INSN_LDR14, SEM_FN_NAME (fr30bf,ldr14) },
  { FR30BF_INSN_LDR14UH, SEM_FN_NAME (fr30bf,ldr14uh) },
  { FR30BF_INSN_LDR14UB, SEM_FN_NAME (fr30bf,ldr14ub) },
  { FR30BF_INSN_LDR15, SEM_FN_NAME (fr30bf,ldr15) },
  { FR30BF_INSN_LDR15GR, SEM_FN_NAME (fr30bf,ldr15gr) },
  { FR30BF_INSN_LDR15DR, SEM_FN_NAME (fr30bf,ldr15dr) },
  { FR30BF_INSN_LDR15PS, SEM_FN_NAME (fr30bf,ldr15ps) },
  { FR30BF_INSN_ST, SEM_FN_NAME (fr30bf,st) },
  { FR30BF_INSN_STH, SEM_FN_NAME (fr30bf,sth) },
  { FR30BF_INSN_STB, SEM_FN_NAME (fr30bf,stb) },
  { FR30BF_INSN_STR13, SEM_FN_NAME (fr30bf,str13) },
  { FR30BF_INSN_STR13H, SEM_FN_NAME (fr30bf,str13h) },
  { FR30BF_INSN_STR13B, SEM_FN_NAME (fr30bf,str13b) },
  { FR30BF_INSN_STR14, SEM_FN_NAME (fr30bf,str14) },
  { FR30BF_INSN_STR14H, SEM_FN_NAME (fr30bf,str14h) },
  { FR30BF_INSN_STR14B, SEM_FN_NAME (fr30bf,str14b) },
  { FR30BF_INSN_STR15, SEM_FN_NAME (fr30bf,str15) },
  { FR30BF_INSN_STR15GR, SEM_FN_NAME (fr30bf,str15gr) },
  { FR30BF_INSN_STR15DR, SEM_FN_NAME (fr30bf,str15dr) },
  { FR30BF_INSN_STR15PS, SEM_FN_NAME (fr30bf,str15ps) },
  { FR30BF_INSN_MOV, SEM_FN_NAME (fr30bf,mov) },
  { FR30BF_INSN_MOVDR, SEM_FN_NAME (fr30bf,movdr) },
  { FR30BF_INSN_MOVPS, SEM_FN_NAME (fr30bf,movps) },
  { FR30BF_INSN_MOV2DR, SEM_FN_NAME (fr30bf,mov2dr) },
  { FR30BF_INSN_MOV2PS, SEM_FN_NAME (fr30bf,mov2ps) },
  { FR30BF_INSN_JMP, SEM_FN_NAME (fr30bf,jmp) },
  { FR30BF_INSN_JMPD, SEM_FN_NAME (fr30bf,jmpd) },
  { FR30BF_INSN_CALLR, SEM_FN_NAME (fr30bf,callr) },
  { FR30BF_INSN_CALLRD, SEM_FN_NAME (fr30bf,callrd) },
  { FR30BF_INSN_CALL, SEM_FN_NAME (fr30bf,call) },
  { FR30BF_INSN_CALLD, SEM_FN_NAME (fr30bf,calld) },
  { FR30BF_INSN_RET, SEM_FN_NAME (fr30bf,ret) },
  { FR30BF_INSN_RET_D, SEM_FN_NAME (fr30bf,ret_d) },
  { FR30BF_INSN_INT, SEM_FN_NAME (fr30bf,int) },
  { FR30BF_INSN_INTE, SEM_FN_NAME (fr30bf,inte) },
  { FR30BF_INSN_RETI, SEM_FN_NAME (fr30bf,reti) },
  { FR30BF_INSN_BRAD, SEM_FN_NAME (fr30bf,brad) },
  { FR30BF_INSN_BRA, SEM_FN_NAME (fr30bf,bra) },
  { FR30BF_INSN_BNOD, SEM_FN_NAME (fr30bf,bnod) },
  { FR30BF_INSN_BNO, SEM_FN_NAME (fr30bf,bno) },
  { FR30BF_INSN_BEQD, SEM_FN_NAME (fr30bf,beqd) },
  { FR30BF_INSN_BEQ, SEM_FN_NAME (fr30bf,beq) },
  { FR30BF_INSN_BNED, SEM_FN_NAME (fr30bf,bned) },
  { FR30BF_INSN_BNE, SEM_FN_NAME (fr30bf,bne) },
  { FR30BF_INSN_BCD, SEM_FN_NAME (fr30bf,bcd) },
  { FR30BF_INSN_BC, SEM_FN_NAME (fr30bf,bc) },
  { FR30BF_INSN_BNCD, SEM_FN_NAME (fr30bf,bncd) },
  { FR30BF_INSN_BNC, SEM_FN_NAME (fr30bf,bnc) },
  { FR30BF_INSN_BND, SEM_FN_NAME (fr30bf,bnd) },
  { FR30BF_INSN_BN, SEM_FN_NAME (fr30bf,bn) },
  { FR30BF_INSN_BPD, SEM_FN_NAME (fr30bf,bpd) },
  { FR30BF_INSN_BP, SEM_FN_NAME (fr30bf,bp) },
  { FR30BF_INSN_BVD, SEM_FN_NAME (fr30bf,bvd) },
  { FR30BF_INSN_BV, SEM_FN_NAME (fr30bf,bv) },
  { FR30BF_INSN_BNVD, SEM_FN_NAME (fr30bf,bnvd) },
  { FR30BF_INSN_BNV, SEM_FN_NAME (fr30bf,bnv) },
  { FR30BF_INSN_BLTD, SEM_FN_NAME (fr30bf,bltd) },
  { FR30BF_INSN_BLT, SEM_FN_NAME (fr30bf,blt) },
  { FR30BF_INSN_BGED, SEM_FN_NAME (fr30bf,bged) },
  { FR30BF_INSN_BGE, SEM_FN_NAME (fr30bf,bge) },
  { FR30BF_INSN_BLED, SEM_FN_NAME (fr30bf,bled) },
  { FR30BF_INSN_BLE, SEM_FN_NAME (fr30bf,ble) },
  { FR30BF_INSN_BGTD, SEM_FN_NAME (fr30bf,bgtd) },
  { FR30BF_INSN_BGT, SEM_FN_NAME (fr30bf,bgt) },
  { FR30BF_INSN_BLSD, SEM_FN_NAME (fr30bf,blsd) },
  { FR30BF_INSN_BLS, SEM_FN_NAME (fr30bf,bls) },
  { FR30BF_INSN_BHID, SEM_FN_NAME (fr30bf,bhid) },
  { FR30BF_INSN_BHI, SEM_FN_NAME (fr30bf,bhi) },
  { FR30BF_INSN_DMOVR13, SEM_FN_NAME (fr30bf,dmovr13) },
  { FR30BF_INSN_DMOVR13H, SEM_FN_NAME (fr30bf,dmovr13h) },
  { FR30BF_INSN_DMOVR13B, SEM_FN_NAME (fr30bf,dmovr13b) },
  { FR30BF_INSN_DMOVR13PI, SEM_FN_NAME (fr30bf,dmovr13pi) },
  { FR30BF_INSN_DMOVR13PIH, SEM_FN_NAME (fr30bf,dmovr13pih) },
  { FR30BF_INSN_DMOVR13PIB, SEM_FN_NAME (fr30bf,dmovr13pib) },
  { FR30BF_INSN_DMOVR15PI, SEM_FN_NAME (fr30bf,dmovr15pi) },
  { FR30BF_INSN_DMOV2R13, SEM_FN_NAME (fr30bf,dmov2r13) },
  { FR30BF_INSN_DMOV2R13H, SEM_FN_NAME (fr30bf,dmov2r13h) },
  { FR30BF_INSN_DMOV2R13B, SEM_FN_NAME (fr30bf,dmov2r13b) },
  { FR30BF_INSN_DMOV2R13PI, SEM_FN_NAME (fr30bf,dmov2r13pi) },
  { FR30BF_INSN_DMOV2R13PIH, SEM_FN_NAME (fr30bf,dmov2r13pih) },
  { FR30BF_INSN_DMOV2R13PIB, SEM_FN_NAME (fr30bf,dmov2r13pib) },
  { FR30BF_INSN_DMOV2R15PD, SEM_FN_NAME (fr30bf,dmov2r15pd) },
  { FR30BF_INSN_LDRES, SEM_FN_NAME (fr30bf,ldres) },
  { FR30BF_INSN_STRES, SEM_FN_NAME (fr30bf,stres) },
  { FR30BF_INSN_COPOP, SEM_FN_NAME (fr30bf,copop) },
  { FR30BF_INSN_COPLD, SEM_FN_NAME (fr30bf,copld) },
  { FR30BF_INSN_COPST, SEM_FN_NAME (fr30bf,copst) },
  { FR30BF_INSN_COPSV, SEM_FN_NAME (fr30bf,copsv) },
  { FR30BF_INSN_NOP, SEM_FN_NAME (fr30bf,nop) },
  { FR30BF_INSN_ANDCCR, SEM_FN_NAME (fr30bf,andccr) },
  { FR30BF_INSN_ORCCR, SEM_FN_NAME (fr30bf,orccr) },
  { FR30BF_INSN_STILM, SEM_FN_NAME (fr30bf,stilm) },
  { FR30BF_INSN_ADDSP, SEM_FN_NAME (fr30bf,addsp) },
  { FR30BF_INSN_EXTSB, SEM_FN_NAME (fr30bf,extsb) },
  { FR30BF_INSN_EXTUB, SEM_FN_NAME (fr30bf,extub) },
  { FR30BF_INSN_EXTSH, SEM_FN_NAME (fr30bf,extsh) },
  { FR30BF_INSN_EXTUH, SEM_FN_NAME (fr30bf,extuh) },
  { FR30BF_INSN_LDM0, SEM_FN_NAME (fr30bf,ldm0) },
  { FR30BF_INSN_LDM1, SEM_FN_NAME (fr30bf,ldm1) },
  { FR30BF_INSN_STM0, SEM_FN_NAME (fr30bf,stm0) },
  { FR30BF_INSN_STM1, SEM_FN_NAME (fr30bf,stm1) },
  { FR30BF_INSN_ENTER, SEM_FN_NAME (fr30bf,enter) },
  { FR30BF_INSN_LEAVE, SEM_FN_NAME (fr30bf,leave) },
  { FR30BF_INSN_XCHB, SEM_FN_NAME (fr30bf,xchb) },
  { 0, 0 }
};

/* Add the semantic fns to IDESC_TABLE.  */

void
SEM_FN_NAME (fr30bf,init_idesc_table) (SIM_CPU *current_cpu)
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
	idesc_table[sf->index].sem_fast = SEM_FN_NAME (fr30bf,x_invalid);
#else
      if (valid_p)
	idesc_table[sf->index].sem_full = sf->fn;
      else
	idesc_table[sf->index].sem_full = SEM_FN_NAME (fr30bf,x_invalid);
#endif
    }
}

