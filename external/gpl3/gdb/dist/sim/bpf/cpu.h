/* CPU family header for bpfbf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2023 Free Software Foundation, Inc.

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

#ifndef CPU_BPFBF_H
#define CPU_BPFBF_H

/* Maximum number of instructions that are fetched at a time.
   This is for LIW type instructions sets (e.g. m32r).  */
#define MAX_LIW_INSNS 1

/* Maximum number of instructions that can be executed in parallel.  */
#define MAX_PARALLEL_INSNS 1

/* The size of an "int" needed to hold an instruction word.
   This is usually 32 bits, but some architectures needs 64 bits.  */
typedef CGEN_INSN_LGUINT CGEN_INSN_WORD;

#include "cgen-engine.h"

/* CPU state information.  */
typedef struct {
  /* Hardware elements.  */
  struct {
  /* General Purpose Registers */
  DI h_gpr[16];
#define GET_H_GPR(a1) CPU (h_gpr)[a1]
#define SET_H_GPR(a1, x) (CPU (h_gpr)[a1] = (x))
  /* program counter */
  UDI h_pc;
#define GET_H_PC() CPU (h_pc)
#define SET_H_PC(x) \
do { \
CPU (h_pc) = (x);\
;} while (0)
  } hardware;
#define CPU_CGEN_HW(cpu) (& (cpu)->cpu_data.hardware)
} BPFBF_CPU_DATA;

/* Cover fns for register access.  */
DI bpfbf_h_gpr_get (SIM_CPU *, UINT);
void bpfbf_h_gpr_set (SIM_CPU *, UINT, DI);
UDI bpfbf_h_pc_get (SIM_CPU *);
void bpfbf_h_pc_set (SIM_CPU *, UDI);

/* These must be hand-written.  */
extern CPUREG_FETCH_FN bpfbf_fetch_register;
extern CPUREG_STORE_FN bpfbf_store_register;

typedef struct {
  int empty;
} MODEL_BPF_DEF_DATA;

/* Collection of various things for the trace handler to use.  */

typedef struct trace_record {
  IADDR pc;
  /* FIXME:wip */
} TRACE_RECORD;

#endif /* CPU_BPFBF_H */
