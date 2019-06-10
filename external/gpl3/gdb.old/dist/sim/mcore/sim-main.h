/* Simulator for Motorola's MCore processor
   Copyright (C) 2009-2017 Free Software Foundation, Inc.

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

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#include "sim-basics.h"

typedef long int           word;
typedef unsigned long int  uword;

#include "sim-base.h"
#include "bfd.h"

/* The machine state.
   This state is maintained in host byte order.  The
   fetch/store register functions must translate between host
   byte order and the target processor byte order.
   Keeping this data in target byte order simplifies the register
   read/write functions.  Keeping this data in native order improves
   the performance of the simulator.  Simulation speed is deemed more
   important.  */

/* The ordering of the mcore_regset structure is matched in the
   gdb/config/mcore/tm-mcore.h file in the REGISTER_NAMES macro.  */
struct mcore_regset
{
  word gregs[16];	/* primary registers */
  word alt_gregs[16];	/* alt register file */
  word cregs[32];	/* control registers */
  word pc;
};
#define LAST_VALID_CREG	32		/* only 0..12 implemented */
#define NUM_MCORE_REGS	(16 + 16 + LAST_VALID_CREG + 1)

struct _sim_cpu {

  union
  {
    struct mcore_regset regs;
    /* Used by the fetch/store reg helpers to access registers linearly.  */
    word asints[NUM_MCORE_REGS];
  };

  /* Used to switch between gregs/alt_gregs based on the control state.  */
  word *active_gregs;

  int ticks;
  int stalls;
  int cycles;
  int insts;

  sim_cpu_base base;
};

struct sim_state {

  sim_cpu *cpu[MAX_NR_PROCESSORS];

  sim_state_base base;
};

#endif

