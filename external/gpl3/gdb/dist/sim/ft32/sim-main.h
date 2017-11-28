/* Simulator for FTDI FT32 processor.

   Copyright (C) 2009-2017 Free Software Foundation, Inc.
   Contributed by FTDI <support@ftdichip.com>

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
#include "bfd.h"

#include "ft32-sim.h"

struct _sim_cpu {

  /* The following are internal simulator state variables: */

  struct ft32_cpu_state state;

  sim_cpu_base base;
};

struct sim_state {

  sim_cpu *cpu[MAX_NR_PROCESSORS];

  sim_state_base base;
};

#endif
