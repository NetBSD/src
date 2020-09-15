/* OpenRISC simulator main header
   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#define WITH_SCACHE_PBB 1

#include "ansidecl.h"
#include "or1k-desc.h"
#include "sim-basics.h"
#include "cgen-types.h"
#include "arch.h"
#include "sim-base.h"
#include "sim-fpu.h"

#include "or1k-opc.h"
#include "cgen-sim.h"
#include "or1k-sim.h"

#define OR1K_DEFAULT_MEM_SIZE 0x800000	/* 8M */

/* The _sim_cpu struct.  */
struct _sim_cpu
{
  /* sim/common cpu base.  */
  sim_cpu_base base;

  /* Static parts of cgen.  */
  CGEN_CPU cgen_cpu;

  OR1K_MISC_PROFILE or1k_misc_profile;
#define CPU_OR1K_MISC_PROFILE(cpu) (& (cpu)->or1k_misc_profile)

  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
  UWI spr[NUM_SPR];

  /* Next instruction will be in delay slot.  */
  BI next_delay_slot;
  /* Currently in delay slot.  */
  BI delay_slot;

#ifdef WANT_CPU_OR1K32BF
  OR1K32BF_CPU_DATA cpu_data;
#endif
};



/* The sim_state struct.  */
struct sim_state
{
  sim_cpu *cpu[MAX_NR_PROCESSORS];

  CGEN_STATE cgen_state;

  sim_state_base base;
};

#endif /* SIM_MAIN_H */
