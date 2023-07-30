/* eBPF simulator support code
   Copyright (C) 2020-2023 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#define WANT_CPU_BPFBF
#define WANT_CPU bpfbf

#include "sim-main.h"
#include "sim-fpu.h"
#include "sim-signal.h"
#include "cgen-mem.h"
#include "cgen-ops.h"
#include "cpuall.h"
#include "decode.h"

#include "decode-be.h"
#include "decode-le.h"

#include "defs-le.h"  /* For SCACHE */
#include "bpf-helpers.h"

uint64_t skb_data_offset;

IDESC *bpf_idesc_le;
IDESC *bpf_idesc_be;


int
bpfbf_fetch_register (SIM_CPU *current_cpu,
                      int rn,
                      void *buf,
                      int len)
{
  if (rn == 11)
    SETTDI (buf, CPU_PC_GET (current_cpu));
  else if (0 <= rn && rn < 10)
    SETTDI (buf, GET_H_GPR (rn));
  else
    return 0;

  return len;
}

int
bpfbf_store_register (SIM_CPU *current_cpu,
                      int rn,
                      const void *buf,
                      int len)
{
  if (rn == 11)
    CPU_PC_SET (current_cpu, GETTDI (buf));
  else if (0 <= rn && rn < 10)
    SET_H_GPR (rn, GETTDI (buf));
  else
    return 0;

  return len;
}

void
bpfbf_model_insn_before (SIM_CPU *current_cpu, int first_p)
{
  /* XXX */
}

void
bpfbf_model_insn_after (SIM_CPU *current_cpu, int first_p, int cycles)
{
  /* XXX */
}


/***** Instruction helpers.  *****/

/* The semantic routines for most instructions are expressed in RTL in
   the cpu/bpf.cpu file, and automatically translated to C in the
   sem-*.c files in this directory.

   However, some of the semantic routines make use of helper C
   functions.  This happens when the semantics of the instructions
   can't be expressed in RTL alone in a satisfactory way, or not at
   all.

   The following functions implement these C helpers. */

DI
bpfbf_endle (SIM_CPU *current_cpu, DI value, UINT bitsize)
{
  switch (bitsize)
    {
      case 16: return endian_h2le_2(endian_t2h_2(value));
      case 32: return endian_h2le_4(endian_t2h_4(value));
      case 64: return endian_h2le_8(endian_t2h_8(value));
      default: assert(0);
    }
  return value;
}

DI
bpfbf_endbe (SIM_CPU *current_cpu, DI value, UINT bitsize)
{
  switch (bitsize)
    {
      case 16: return endian_h2be_2(endian_t2h_2(value));
      case 32: return endian_h2be_4(endian_t2h_4(value));
      case 64: return endian_h2be_8(endian_t2h_8(value));
      default: assert(0);
    }
  return value;
}

DI
bpfbf_skb_data_offset (SIM_CPU *current_cpu)
{
  /* Simply return the user-configured value.
     This will be 0 if it has not been set. */
  return skb_data_offset;
}


VOID
bpfbf_call (SIM_CPU *current_cpu, INT disp32, UINT src)
{
  /* eBPF supports two kind of CALL instructions: the so called pseudo
     calls ("bpf to bpf") and external calls ("bpf to helper").

     Both kind of calls use the same instruction (CALL).  However,
     external calls are constructed by passing a constant argument to
     the instruction, that identifies the helper, whereas pseudo calls
     result from expressions involving symbols.

     We distinguish calls from pseudo-calls with the later having a 1
     stored in the SRC field of the instruction.  */

  if (src == 1)
    {
      /* This is a pseudo-call.  */

      /* XXX allocate a new stack frame and transfer control.  For
         that we need to analyze the target function, like the kernel
         verifier does.  We better populate a cache
         (function_start_address -> frame_size) so we avoid
         calculating this more than once.  */
      /* XXX note that disp32 is PC-relative in number of 64-bit
         words, _minus one_.  */
    }
  else
    {
      /* This is a call to a helper.

         DISP32 contains the helper number.  Dispatch to the
         corresponding helper emulator in bpf-helpers.c.  */

      switch (disp32) {
        /* case TRACE_PRINTK: */
        case 7:
          bpf_trace_printk (current_cpu);
          break;
        default:;
      }
    }
}

VOID
bpfbf_exit (SIM_CPU *current_cpu)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  /*  r0 holds "return code" */
  DI r0 = GET_H_GPR (0);

  printf ("exit %" PRId64 " (0x%" PRIx64 ")\n", r0, r0);

  sim_engine_halt (sd, current_cpu, NULL, CPU_PC_GET (current_cpu),
                   sim_exited, 0 /* sigrc */);
}

VOID
bpfbf_breakpoint (SIM_CPU *current_cpu)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  sim_engine_halt (sd, current_cpu, NULL, CPU_PC_GET (current_cpu),
                   sim_stopped, SIM_SIGTRAP);
}

/* We use the definitions below instead of the cgen-generated model.c,
   because the later is not really able to work with cpus featuring
   several ISAs.  This should be fixed in CGEN.  */

static void
bpf_def_model_init (SIM_CPU *cpu)
{
  /* Do nothing.  */
}

static void
bpfbf_prepare_run (SIM_CPU *cpu)
{
  /* Nothing.  */
}

static void
bpf_engine_run_full (SIM_CPU *cpu)
{
  if (CURRENT_TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
    {
      if (!bpf_idesc_le)
        {
          bpfbf_ebpfle_init_idesc_table (cpu);
          bpf_idesc_le = CPU_IDESC (cpu);
        }
      else
        CPU_IDESC (cpu) = bpf_idesc_le;

      bpfbf_ebpfle_engine_run_full (cpu);
    }
  else
    {
      if (!bpf_idesc_be)
        {
          bpfbf_ebpfbe_init_idesc_table (cpu);
          bpf_idesc_be = CPU_IDESC (cpu);
        }
      else
        CPU_IDESC (cpu) = bpf_idesc_be;

      bpfbf_ebpfbe_engine_run_full (cpu);
    }
}

#if WITH_FAST

void
bpf_engine_run_fast (SIM_CPU *cpu)
{
  if (CURRENT_TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
    {
      if (!bpf_idesc_le)
        {
          bpfbf_ebpfle_init_idesc_table (cpu);
          bpf_idesc_le = CPU_IDESC (cpu);
        }
      else
        CPU_IDESC (cpu) = bpf_idesc_le;

      bpfbf_ebpfle_engine_run_fast (cpu);
    }
  else
    {
      if (!bpf_idesc_be)
        {
          bpfbf_ebpfbe_init_idesc_table (cpu);
          bpf_idesc_be = CPU_IDESC (cpu);
        }
      else
        CPU_IDESC (cpu) = bpf_idesc_be;

      bpfbf_ebpfbe_engine_run_fast (cpu);
    }
}

#endif /* WITH_FAST */

static const CGEN_INSN *
bpfbf_get_idata (SIM_CPU *cpu, int inum)
{
  return CPU_IDESC (cpu) [inum].idata;
}

static void
bpf_init_cpu (SIM_CPU *cpu)
{
  CPU_REG_FETCH (cpu) = bpfbf_fetch_register;
  CPU_REG_STORE (cpu) = bpfbf_store_register;
  CPU_PC_FETCH (cpu) = bpfbf_h_pc_get;
  CPU_PC_STORE (cpu) = bpfbf_h_pc_set;
  CPU_GET_IDATA (cpu) = bpfbf_get_idata;
  /* Only used by profiling.  0 disables it. */
  CPU_MAX_INSNS (cpu) = 0;
  CPU_INSN_NAME (cpu) = cgen_insn_name;
  CPU_FULL_ENGINE_FN (cpu) = bpf_engine_run_full;
#if WITH_FAST
  CPU_FAST_ENGINE_FN (cpu) = bpf_engine_run_fast;
#else
  CPU_FAST_ENGINE_FN (cpu) = bpf_engine_run_full;
#endif
}

static const SIM_MODEL bpf_models[] =
{
 { "bpf-def", & bpf_mach, MODEL_BPF_DEF, NULL, bpf_def_model_init },
 { 0 }
};

static const SIM_MACH_IMP_PROPERTIES bpfbf_imp_properties =
{
  sizeof (SIM_CPU),
#if WITH_SCACHE
  sizeof (SCACHE)
#else
  0
#endif
};

const SIM_MACH bpf_mach =
{
  "bpf", "bpf", MACH_BPF,
  32, 32, & bpf_models[0], & bpfbf_imp_properties,
  bpf_init_cpu,
  bpfbf_prepare_run
};
