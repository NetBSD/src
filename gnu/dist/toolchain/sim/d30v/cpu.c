/* Mitsubishi Electric Corp. D30V Simulator.
   Copyright (C) 1997, Free Software Foundation, Inc.
   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

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
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifndef _CPU_C_
#define _CPU_C_

#include "sim-main.h"


int
is_wrong_slot (SIM_DESC sd,
	       address_word cia,
	       itable_index index)
{
  switch (STATE_CPU (sd, 0)->unit)
    {
    case memory_unit:
      return !itable[index].option[itable_option_mu];
    case integer_unit:
      return !itable[index].option[itable_option_iu];
    case any_unit:
      return 0;
    default:
      sim_engine_abort (sd, STATE_CPU (sd, 0), cia,
			"internal error - is_wrong_slot - bad switch");
      return -1;
    }
}

int
is_condition_ok (SIM_DESC sd,
		 address_word cia,
		 int cond)
{
  switch (cond)
    {
    case 0x0:
      return 1;
    case 0x1:
      return PSW_VAL(PSW_F0);
    case 0x2:
      return !PSW_VAL(PSW_F0);
    case 0x3:
      return PSW_VAL(PSW_F1);
    case 0x4:
      return !PSW_VAL(PSW_F1);
    case 0x5:
      return PSW_VAL(PSW_F0) && PSW_VAL(PSW_F1);
    case 0x6:
      return PSW_VAL(PSW_F0) && !PSW_VAL(PSW_F1);
    case 0x7:
      sim_engine_abort (sd, STATE_CPU (sd, 0), cia,
			"is_condition_ok - bad instruction condition bits");
      return 0;
    default:
      sim_engine_abort (sd, STATE_CPU (sd, 0), cia,
			"internal error - is_condition_ok - bad switch");
      return -1;
    }
}

/* If --trace-call, trace calls, remembering the current state of
   registers.  */

typedef struct _call_stack {
  struct _call_stack *prev;
  registers regs;
} call_stack;

static call_stack *call_stack_head = (call_stack *)0;
static int call_depth = 0;

void call_occurred (SIM_DESC sd,
		    sim_cpu *cpu,
		    address_word cia,
		    address_word nia)
{
  call_stack *ptr = ZALLOC (call_stack);
  ptr->regs = cpu->regs;
  ptr->prev = call_stack_head;
  call_stack_head = ptr;

  trace_one_insn (sd, cpu, nia, 1, "", 0, "call",
		  "Depth %3d, Return 0x%.8lx, Args 0x%.8lx 0x%.8lx",
		  ++call_depth, (unsigned long)cia+8, (unsigned long)GPR[2],
		  (unsigned long)GPR[3]);
}

/* If --trace-call, trace returns, checking if any saved register was changed.  */

void return_occurred (SIM_DESC sd,
		      sim_cpu *cpu,
		      address_word cia,
		      address_word nia)
{
  char buffer[1024];
  char *buf_ptr = buffer;
  call_stack *ptr = call_stack_head;
  int regno;
  char *prefix = ", Registers that differ: ";

  *buf_ptr = '\0';
  for (regno = 34; regno <= 63; regno++) {
    if (cpu->regs.general_purpose[regno] != ptr->regs.general_purpose[regno]) {
      sprintf (buf_ptr, "%sr%d", prefix, regno);
      buf_ptr += strlen (buf_ptr);
      prefix = " ";
    }
  }

  if (cpu->regs.accumulator[1] != ptr->regs.accumulator[1]) {
    sprintf (buf_ptr, "%sa1", prefix);
    buf_ptr += strlen (buf_ptr);
    prefix = " ";
  }

  trace_one_insn (sd, cpu, cia, 1, "", 0, "return",
		  "Depth %3d, Return 0x%.8lx, Ret. 0x%.8lx 0x%.8lx%s",
		  call_depth--, (unsigned long)nia, (unsigned long)GPR[2],
		  (unsigned long)GPR[3], buffer);

  call_stack_head = ptr->prev;
  zfree (ptr);
}


/* Read/write functions for system call interface.  */
int
d30v_read_mem (host_callback *cb,
	       struct cb_syscall *sc,
	       unsigned long taddr,
	       char *buf,
	       int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  sim_cpu *cpu = STATE_CPU (sd, 0);

  return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes);
}

int
d30v_write_mem (host_callback *cb,
		struct cb_syscall *sc,
		unsigned long taddr,
		const char *buf,
		int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  sim_cpu *cpu = STATE_CPU (sd, 0);

  return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
}

#endif /* _CPU_C_ */
