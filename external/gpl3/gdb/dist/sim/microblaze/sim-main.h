/* Copyright 2009-2017 Free Software Foundation, Inc.

   This file is part of the Xilinx MicroBlaze simulator.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#ifndef MICROBLAZE_SIM_MAIN
#define MICROBLAZE_SIM_MAIN

#include "microblaze.h"
#include "sim-basics.h"
#include "sim-base.h"

/* The machine state.
   This state is maintained in host byte order.  The
   fetch/store register functions must translate between host
   byte order and the target processor byte order.
   Keeping this data in target byte order simplifies the register
   read/write functions.  Keeping this data in native order improves
   the performance of the simulator.  Simulation speed is deemed more
   important.  */

/* The ordering of the microblaze_regset structure is matched in the
   gdb/config/microblaze/tm-microblaze.h file in the REGISTER_NAMES macro.  */
 struct microblaze_regset
{
  word	          regs[32];		/* primary registers */
  word	          spregs[2];		/* pc + msr */
  int		  cycles;
  int		  insts;
  ubyte           imm_enable;
  half            imm_high;
};

struct _sim_cpu {
  struct microblaze_regset microblaze_cpu;
  sim_cpu_base base;
};

struct sim_state {

  sim_cpu *cpu[MAX_NR_PROCESSORS];

  sim_state_base base;
};

#endif /* MICROBLAZE_SIM_MAIN */
