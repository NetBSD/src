/* Misc. support for CPU family fr30bf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996, 1997, 1998, 1999 Free Software Foundation, Inc.

This file is part of the GNU Simulators.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#define WANT_CPU fr30bf
#define WANT_CPU_FR30BF

#include "sim-main.h"
#include "cgen-ops.h"

/* Get the value of h-pc.  */

USI
fr30bf_h_pc_get (SIM_CPU *current_cpu)
{
  return CPU (h_pc);
}

/* Set a value for h-pc.  */

void
fr30bf_h_pc_set (SIM_CPU *current_cpu, USI newval)
{
  CPU (h_pc) = newval;
}

/* Get the value of h-gr.  */

SI
fr30bf_h_gr_get (SIM_CPU *current_cpu, UINT regno)
{
  return CPU (h_gr[regno]);
}

/* Set a value for h-gr.  */

void
fr30bf_h_gr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  CPU (h_gr[regno]) = newval;
}

/* Get the value of h-cr.  */

SI
fr30bf_h_cr_get (SIM_CPU *current_cpu, UINT regno)
{
  return CPU (h_cr[regno]);
}

/* Set a value for h-cr.  */

void
fr30bf_h_cr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  CPU (h_cr[regno]) = newval;
}

/* Get the value of h-dr.  */

SI
fr30bf_h_dr_get (SIM_CPU *current_cpu, UINT regno)
{
  return GET_H_DR (regno);
}

/* Set a value for h-dr.  */

void
fr30bf_h_dr_set (SIM_CPU *current_cpu, UINT regno, SI newval)
{
  SET_H_DR (regno, newval);
}

/* Get the value of h-ps.  */

USI
fr30bf_h_ps_get (SIM_CPU *current_cpu)
{
  return GET_H_PS ();
}

/* Set a value for h-ps.  */

void
fr30bf_h_ps_set (SIM_CPU *current_cpu, USI newval)
{
  SET_H_PS (newval);
}

/* Get the value of h-r13.  */

SI
fr30bf_h_r13_get (SIM_CPU *current_cpu)
{
  return CPU (h_r13);
}

/* Set a value for h-r13.  */

void
fr30bf_h_r13_set (SIM_CPU *current_cpu, SI newval)
{
  CPU (h_r13) = newval;
}

/* Get the value of h-r14.  */

SI
fr30bf_h_r14_get (SIM_CPU *current_cpu)
{
  return CPU (h_r14);
}

/* Set a value for h-r14.  */

void
fr30bf_h_r14_set (SIM_CPU *current_cpu, SI newval)
{
  CPU (h_r14) = newval;
}

/* Get the value of h-r15.  */

SI
fr30bf_h_r15_get (SIM_CPU *current_cpu)
{
  return CPU (h_r15);
}

/* Set a value for h-r15.  */

void
fr30bf_h_r15_set (SIM_CPU *current_cpu, SI newval)
{
  CPU (h_r15) = newval;
}

/* Get the value of h-nbit.  */

BI
fr30bf_h_nbit_get (SIM_CPU *current_cpu)
{
  return CPU (h_nbit);
}

/* Set a value for h-nbit.  */

void
fr30bf_h_nbit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_nbit) = newval;
}

/* Get the value of h-zbit.  */

BI
fr30bf_h_zbit_get (SIM_CPU *current_cpu)
{
  return CPU (h_zbit);
}

/* Set a value for h-zbit.  */

void
fr30bf_h_zbit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_zbit) = newval;
}

/* Get the value of h-vbit.  */

BI
fr30bf_h_vbit_get (SIM_CPU *current_cpu)
{
  return CPU (h_vbit);
}

/* Set a value for h-vbit.  */

void
fr30bf_h_vbit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_vbit) = newval;
}

/* Get the value of h-cbit.  */

BI
fr30bf_h_cbit_get (SIM_CPU *current_cpu)
{
  return CPU (h_cbit);
}

/* Set a value for h-cbit.  */

void
fr30bf_h_cbit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_cbit) = newval;
}

/* Get the value of h-ibit.  */

BI
fr30bf_h_ibit_get (SIM_CPU *current_cpu)
{
  return CPU (h_ibit);
}

/* Set a value for h-ibit.  */

void
fr30bf_h_ibit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_ibit) = newval;
}

/* Get the value of h-sbit.  */

BI
fr30bf_h_sbit_get (SIM_CPU *current_cpu)
{
  return GET_H_SBIT ();
}

/* Set a value for h-sbit.  */

void
fr30bf_h_sbit_set (SIM_CPU *current_cpu, BI newval)
{
  SET_H_SBIT (newval);
}

/* Get the value of h-tbit.  */

BI
fr30bf_h_tbit_get (SIM_CPU *current_cpu)
{
  return CPU (h_tbit);
}

/* Set a value for h-tbit.  */

void
fr30bf_h_tbit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_tbit) = newval;
}

/* Get the value of h-d0bit.  */

BI
fr30bf_h_d0bit_get (SIM_CPU *current_cpu)
{
  return CPU (h_d0bit);
}

/* Set a value for h-d0bit.  */

void
fr30bf_h_d0bit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_d0bit) = newval;
}

/* Get the value of h-d1bit.  */

BI
fr30bf_h_d1bit_get (SIM_CPU *current_cpu)
{
  return CPU (h_d1bit);
}

/* Set a value for h-d1bit.  */

void
fr30bf_h_d1bit_set (SIM_CPU *current_cpu, BI newval)
{
  CPU (h_d1bit) = newval;
}

/* Get the value of h-ccr.  */

UQI
fr30bf_h_ccr_get (SIM_CPU *current_cpu)
{
  return GET_H_CCR ();
}

/* Set a value for h-ccr.  */

void
fr30bf_h_ccr_set (SIM_CPU *current_cpu, UQI newval)
{
  SET_H_CCR (newval);
}

/* Get the value of h-scr.  */

UQI
fr30bf_h_scr_get (SIM_CPU *current_cpu)
{
  return GET_H_SCR ();
}

/* Set a value for h-scr.  */

void
fr30bf_h_scr_set (SIM_CPU *current_cpu, UQI newval)
{
  SET_H_SCR (newval);
}

/* Get the value of h-ilm.  */

UQI
fr30bf_h_ilm_get (SIM_CPU *current_cpu)
{
  return GET_H_ILM ();
}

/* Set a value for h-ilm.  */

void
fr30bf_h_ilm_set (SIM_CPU *current_cpu, UQI newval)
{
  SET_H_ILM (newval);
}

/* Record trace results for INSN.  */

void
fr30bf_record_trace_results (SIM_CPU *current_cpu, CGEN_INSN *insn,
			    int *indices, TRACE_RECORD *tr)
{
}
