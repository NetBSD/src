/* Moxie Simulator definition.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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

#include "sim-base.h"

struct _sim_cpu {
  /* The only real register.  */
  uint32_t pc;

  /* We update a cycle counter.  */
  uint32_t cycles;

  sim_cpu_base base;
};

struct sim_state {
  sim_cpu *cpu[MAX_NR_PROCESSORS];

  /* If true, the pc needs more than 2 bytes.  */
  int avr_pc22;

  sim_state_base base;
};

#endif
