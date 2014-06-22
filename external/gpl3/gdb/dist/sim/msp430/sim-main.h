/* Simulator for TI MSP430 and MSP430X processors.

   Copyright (C) 2012-2014 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

   This file is part of simulators.

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

#ifndef _MSP430_MAIN_SIM_H_
#define _MSP430_MAIN_SIM_H_

#include "sim-basics.h"
#include "sim-signal.h"

typedef unsigned32 sim_cia;

typedef struct _sim_cpu SIM_CPU;

#include "msp430-sim.h"
#include "sim-base.h"

struct _sim_cpu
{
  /* Simulator specific members.  */
  struct msp430_cpu_state state;
  sim_cpu_base base;
};

struct sim_state
{
  sim_cpu *cpu[MAX_NR_PROCESSORS];

#if (WITH_SMP)
#error WITH_SMP not supported by MSP430 sim
#else
#define STATE_CPU(sd,n) ((sd)->cpu[0])
#endif

  asymbol **symbol_table;
  long number_of_symbols;
#define STATE_SYMBOL_TABLE(sd)   ((sd)->symbol_table)
#define STATE_NUM_SYMBOLS(sd) ((sd)->number_of_symbols)
  
  /* Simulator specific members.  */
  sim_state_base base;
};

#define MSP430_CPU(sd)       (STATE_CPU ((sd), 0))
#define MSP430_CPU_STATE(sd) (MSP430_CPU ((sd)->state))

#define CIA_GET(CPU)     ((CPU)->state.regs[0] + 0)
#define CIA_SET(CPU,VAL) ((CPU)->state.regs[0] = (VAL))

#include "sim-config.h"
#include "sim-types.h"
#include "sim-engine.h"
#include "sim-options.h"
#include "run-sim.h"

#define MAYBE_TRACE(type, cpu, fmt, ...)				\
  do									\
    {									\
      if (TRACE_##type##_P (cpu))					\
	trace_generic (CPU_STATE (cpu), cpu, TRACE_##type##_IDX,	\
		       fmt, ## __VA_ARGS__);				\
    }									\
  while (0)

#define TRACE_INSN(cpu, fmt, ...)    MAYBE_TRACE (INSN, cpu, fmt, ## __VA_ARGS__)
#define TRACE_DECODE(cpu, fmt, ...)  MAYBE_TRACE (DECODE, cpu, fmt, ## __VA_ARGS__)
#define TRACE_EXTRACT(cpu, fmt, ...) MAYBE_TRACE (EXTRACT, cpu, fmt, ## __VA_ARGS__)
#define TRACE_SYSCALL(cpu, fmt, ...) MAYBE_TRACE (SYSCALL, cpu, fmt, ## __VA_ARGS__)

#define TRACE_CORE(cpu, addr, size, map, val)				\
  do									\
    {									\
      MAYBE_TRACE (CORE, cpu, "%cBUS %s %i bytes @ 0x%08x: 0x%0*x",	\
		   map == exec_map ? 'I' : 'D',				\
		   map == write_map ? "STORE" : "FETCH",		\
		   size, addr, size * 2, val);				\
      PROFILE_COUNT_CORE (cpu, addr, size, map);			\
    }									\
  while (0)

#define TRACE_EVENTS(cpu, fmt, ...) MAYBE_TRACE (EVENTS, cpu, fmt, ## __VA_ARGS__)

#define TRACE_BRANCH(cpu, oldpc, newpc, fmt, ...)			\
  do									\
    {									\
      MAYBE_TRACE (BRANCH, cpu, fmt " to %#x", ## __VA_ARGS__, newpc);	\
    }									\
  while (0)

extern void trace_register (SIM_DESC, sim_cpu *, const char *, ...)
     __attribute__((format (printf, 3, 4)));

#define TRACE_REGISTER(cpu, fmt, ...)					\
  do									\
    {									\
      if (TRACE_CORE_P (cpu))						\
	trace_register (CPU_STATE (cpu), cpu, fmt, ## __VA_ARGS__);	\
    }									\
  while (0)

#define TRACE_REG(cpu, reg, val) \
  TRACE_REGISTER (cpu, "wrote R%d = %#x", reg, val)

#endif /* _MSP430_MAIN_SIM_H_ */
