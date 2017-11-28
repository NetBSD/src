/*  This file is part of the program psim.

    Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au>
    Copyright (C) 1997-2016 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
    */


#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#define SIM_ENGINE_HALT_HOOK(SD,LAST_CPU,CIA) 0 /* disable this hook */

#include "sim-basics.h"
#include "sim-signal.h"

#include <signal.h> /* For kill() in insns:do_trap */

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* These are generated files.  */
#include "itable.h"
#include "idecode.h"

#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR)  \
mn10300_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), (TRANSFER), (ERROR))


#include "sim-base.h"

#include "mn10300_sim.h"

/* Bring data in from the cold */

#define IMEM8(EA) \
(sim_core_read_aligned_1(STATE_CPU(sd, 0), EA, exec_map, (EA)))

#define IMEM8_IMMED(EA, N) \
(sim_core_read_aligned_1(STATE_CPU(sd, 0), EA, exec_map, (EA) + (N)))


/* FIXME: For moment, save/restore PC value found in struct State.
   Struct State will one day go away, being placed in the sim_cpu
   state. */

struct _sim_cpu {
  sim_event *pending_nmi;
  sim_cia cia;
  sim_cpu_base base;
};


struct sim_state {

  /* the processors proper */
  sim_cpu *cpu[MAX_NR_PROCESSORS];

  /* The base class.  */
  sim_state_base base;

};

/* For compatibility, until all functions converted to passing
   SIM_DESC as an argument */
extern SIM_DESC simulator;

/* (re) initialize the simulator */

extern void engine_init(SIM_DESC sd);
extern SIM_CORE_SIGNAL_FN mn10300_core_signal;

#endif
