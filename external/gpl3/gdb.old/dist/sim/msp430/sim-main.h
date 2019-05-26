/* Simulator for TI MSP430 and MSP430X processors.

   Copyright (C) 2012-2017 Free Software Foundation, Inc.
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

  /* Simulator specific members.  */
  sim_state_base base;
};

#define MSP430_CPU(sd)       (STATE_CPU ((sd), 0))
#define MSP430_CPU_STATE(sd) (MSP430_CPU ((sd)->state))

#include "sim-config.h"
#include "sim-types.h"
#include "sim-engine.h"
#include "sim-options.h"

#endif /* _MSP430_MAIN_SIM_H_ */
