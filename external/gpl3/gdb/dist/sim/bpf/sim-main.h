/* eBPF simulator main header
   Copyright (C) 2020 Free Software Foundation, Inc.

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
#include "cgen-types.h"
#include "bpf-desc.h"
#include "bpf-opc.h"
#include "arch.h"
#include "sim-base.h"
#include "cgen-sim.h"
#include "bpf-sim.h"


struct _sim_cpu
{
  sim_cpu_base base;
  CGEN_CPU cgen_cpu;

#if defined (WANT_CPU_BPFBF)
  BPFBF_CPU_DATA cpu_data;
#endif
};



struct sim_state
{
  sim_cpu *cpu[MAX_NR_PROCESSORS];
  CGEN_STATE cgen_state;
  sim_state_base base;
};

#endif /* ! SIM_MAIN_H */
