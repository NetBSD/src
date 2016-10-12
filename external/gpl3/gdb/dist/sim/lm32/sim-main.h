/* Lattice Mico32 simulator support code
   Contributed by Jon Beniston <jon@beniston.com>

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

   This file is part of GDB.

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

/* Main header for the LM32 simulator.  */

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#define WITH_SCACHE_PBB 1

#include "symcat.h"
#include "sim-basics.h"
#include "cgen-types.h"
#include "lm32-desc.h"
#include "lm32-opc.h"
#include "arch.h"
#include "sim-base.h"
#include "cgen-sim.h"
#include "lm32-sim.h"
#include "opcode/cgen.h"

/* The _sim_cpu struct.  */

struct _sim_cpu
{
  /* sim/common cpu base.  */
  sim_cpu_base base;

  /* Static parts of cgen.  */
  CGEN_CPU cgen_cpu;

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
#if defined (WANT_CPU_LM32BF)
  LM32BF_CPU_DATA cpu_data;
#endif

};

/* The sim_state struct.  */

struct sim_state
{
  sim_cpu *cpu[MAX_NR_PROCESSORS];

  CGEN_STATE cgen_state;

  sim_state_base base;
};

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN lm32_core_signal;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
lm32_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

#endif /* SIM_MAIN_H */
