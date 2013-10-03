/* Simulator for Analog Devices Blackfin processors.

   Copyright (C) 2005-2013 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

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

#ifndef _BFIN_MAIN_SIM_H_
#define _BFIN_MAIN_SIM_H_

#include "sim-basics.h"
#include "sim-signal.h"

typedef unsigned32 sim_cia;

#define CIA_GET(cpu)     CPU_PC_GET (cpu)
#define CIA_SET(cpu,val) CPU_PC_SET ((cpu), (val))

typedef struct _sim_cpu SIM_CPU;

#include "bfin-sim.h"

#include "machs.h"

#include "sim-base.h"

struct _sim_cpu {
  /* ... simulator specific members ... */
  struct bfin_cpu_state state;
  sim_cpu_base base;
};
#define BFIN_CPU_STATE ((cpu)->state)

struct sim_state {
  sim_cpu *cpu[MAX_NR_PROCESSORS];
#if (WITH_SMP)
#define STATE_CPU(sd,n) ((sd)->cpu[n])
#else
#define STATE_CPU(sd,n) ((sd)->cpu[0])
#endif
  /* ... simulator specific members ... */
  struct bfin_board_data board;
#define STATE_BOARD_DATA(sd) (&(sd)->board)
  sim_state_base base;
};

#include "sim-config.h"
#include "sim-types.h"
#include "sim-engine.h"
#include "sim-options.h"
#include "run-sim.h"
#include "dv-bfin_trace.h"

#undef MAX
#undef MIN
#undef CLAMP
#undef ALIGN
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(a, b, c) MIN (MAX (a, b), c)
#define ALIGN(addr, size) (((addr) + ((size)-1)) & ~((size)-1))

#define MAYBE_TRACE(type, cpu, fmt, ...) \
  do { \
    if (TRACE_##type##_P (cpu)) \
      trace_generic (CPU_STATE (cpu), cpu, TRACE_##type##_IDX, \
		     fmt, ## __VA_ARGS__); \
  } while (0)
#define TRACE_INSN(cpu, fmt, ...) MAYBE_TRACE (INSN, cpu, fmt, ## __VA_ARGS__)
#define TRACE_DECODE(cpu, fmt, ...) MAYBE_TRACE (DECODE, cpu, fmt, ## __VA_ARGS__)
#define TRACE_EXTRACT(cpu, fmt, ...) MAYBE_TRACE (EXTRACT, cpu, fmt, ## __VA_ARGS__)
#define TRACE_SYSCALL(cpu, fmt, ...) MAYBE_TRACE (SYSCALL, cpu, fmt, ## __VA_ARGS__)
#define TRACE_CORE(cpu, addr, size, map, val) \
  do { \
    MAYBE_TRACE (CORE, cpu, "%cBUS %s %i bytes @ 0x%08x: 0x%0*x", \
		 map == exec_map ? 'I' : 'D', \
		 map == write_map ? "STORE" : "FETCH", \
		 size, addr, size * 2, val); \
    PROFILE_COUNT_CORE (cpu, addr, size, map); \
  } while (0)
#define TRACE_EVENTS(cpu, fmt, ...) MAYBE_TRACE (EVENTS, cpu, fmt, ## __VA_ARGS__)
#define TRACE_BRANCH(cpu, oldpc, newpc, hwloop, fmt, ...) \
  do { \
    MAYBE_TRACE (BRANCH, cpu, fmt " to %#x", ## __VA_ARGS__, newpc); \
    if (STATE_ENVIRONMENT (CPU_STATE (cpu)) == OPERATING_ENVIRONMENT) \
      bfin_trace_queue (cpu, oldpc, newpc, hwloop); \
  } while (0)

extern void trace_register PARAMS ((SIM_DESC sd,
				    sim_cpu *cpu,
				    const char *fmt,
				    ...))
     __attribute__((format (printf, 3, 4)));
#define TRACE_REGISTER(cpu, fmt, ...) \
  do { \
    if (TRACE_CORE_P (cpu)) \
      trace_register (CPU_STATE (cpu), cpu, fmt, ## __VA_ARGS__); \
  } while (0)
#define TRACE_REG(cpu, reg, val) TRACE_REGISTER (cpu, "wrote "#reg" = %#x", val)

/* Default memory size.  */
#define BFIN_DEFAULT_MEM_SIZE (128 * 1024 * 1024)

#endif
