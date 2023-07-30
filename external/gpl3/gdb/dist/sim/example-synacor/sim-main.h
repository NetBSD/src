/* Example synacor simulator.

   Copyright (C) 2005-2023 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

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

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#include "sim-basics.h"
#include "sim-base.h"

struct _sim_cpu {
  /* ... simulator specific members ... */
  uint16_t regs[8];
  sim_cia pc;

  /* This isn't a real register, and the stack is not directly addressable,
     so use memory outside of the 16-bit address space.  */
  uint32_t sp;

  sim_cpu_base base;
};

extern void step_once (SIM_CPU *);
extern void initialize_cpu (SIM_DESC, SIM_CPU *);

#endif
