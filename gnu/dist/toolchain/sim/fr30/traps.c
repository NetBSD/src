/* fr30 exception, interrupt, and trap (EIT) support
   Copyright (C) 1998, 1999 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

This file is part of the GNU simulators.

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

#include "sim-main.h"
#include "targ-vals.h"
#include "cgen-engine.h"

/* The semantic code invokes this for invalid (unrecognized) instructions.  */

SEM_PC
sim_engine_invalid_insn (SIM_CPU *current_cpu, IADDR cia, SEM_PC vpc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

#if 0
  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    {
      h_bsm_set (current_cpu, h_sm_get (current_cpu));
      h_bie_set (current_cpu, h_ie_get (current_cpu));
      h_bcond_set (current_cpu, h_cond_get (current_cpu));
      /* sm not changed */
      h_ie_set (current_cpu, 0);
      h_cond_set (current_cpu, 0);

      h_bpc_set (current_cpu, cia);

      sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
			  EIT_RSVD_INSN_ADDR);
    }
  else
#endif
    sim_engine_halt (sd, current_cpu, NULL, cia, sim_stopped, SIM_SIGILL);
  return vpc;
}

/* Process an address exception.  */

void
fr30_core_signal (SIM_DESC sd, SIM_CPU *current_cpu, sim_cia cia,
		  unsigned int map, int nr_bytes, address_word addr,
		  transfer_type transfer, sim_core_signals sig)
{
#if 0
  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    {
      h_bsm_set (current_cpu, h_sm_get (current_cpu));
      h_bie_set (current_cpu, h_ie_get (current_cpu));
      h_bcond_set (current_cpu, h_cond_get (current_cpu));
      /* sm not changed */
      h_ie_set (current_cpu, 0);
      h_cond_set (current_cpu, 0);

      h_bpc_set (current_cpu, cia);

      sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
			  EIT_ADDR_EXCP_ADDR);
    }
  else
#endif
    sim_core_signal (sd, current_cpu, cia, map, nr_bytes, addr,
		     transfer, sig);
}

/* Read/write functions for system call interface.  */

static int
syscall_read_mem (host_callback *cb, struct cb_syscall *sc,
		  unsigned long taddr, char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  return sim_core_read_buffer (sd, cpu, read_map, buf, taddr, bytes);
}

static int
syscall_write_mem (host_callback *cb, struct cb_syscall *sc,
		   unsigned long taddr, const char *buf, int bytes)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  return sim_core_write_buffer (sd, cpu, write_map, buf, taddr, bytes);
}

/* Subroutine of fr30_int to save the PS and PC and setup for INT and INTE.  */

static void
setup_int (SIM_CPU *current_cpu, PCADDR pc)
{
  USI ssp = fr30bf_h_dr_get (current_cpu, H_DR_SSP);
  USI ps = fr30bf_h_ps_get (current_cpu);

  ssp -= 4;
  SETMEMSI (current_cpu, pc, ssp, ps);
  ssp -= 4;
  SETMEMSI (current_cpu, pc, ssp, pc + 2);
  fr30bf_h_dr_set (current_cpu, H_DR_SSP, ssp);
  fr30bf_h_sbit_set (current_cpu, 0);
}

/* Trap support.
   The result is the pc address to continue at.
   Preprocessing like saving the various registers has already been done.  */

USI
fr30_int (SIM_CPU *current_cpu, PCADDR pc, int num)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  host_callback *cb = STATE_CALLBACK (sd);

#ifdef SIM_HAVE_BREAKPOINTS
  /* Check for breakpoints "owned" by the simulator first, regardless
     of --environment.  */
  if (num == TRAP_BREAKPOINT)
    {
      /* First try sim-break.c.  If it's a breakpoint the simulator "owns"
	 it doesn't return.  Otherwise it returns and let's us try.  */
      sim_handle_breakpoint (sd, current_cpu, pc);
      /* Fall through.  */
    }
#endif

  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    {
      /* The new pc is the trap vector entry.
	 We assume there's a branch there to some handler.  */
      USI new_pc;
      setup_int (current_cpu, pc);
      fr30bf_h_ibit_set (current_cpu, 0);
      new_pc = GETMEMSI (current_cpu, pc,
			 fr30bf_h_dr_get (current_cpu, H_DR_TBR)
			 + 1024 - ((num + 1) * 4));
      return new_pc;
    }

  switch (num)
    {
    case TRAP_SYSCALL :
      {
	/* TODO: find out what the ABI for this is */
	CB_SYSCALL s;

	CB_SYSCALL_INIT (&s);
	s.func = fr30bf_h_gr_get (current_cpu, 0);
	s.arg1 = fr30bf_h_gr_get (current_cpu, 4);
	s.arg2 = fr30bf_h_gr_get (current_cpu, 5);
	s.arg3 = fr30bf_h_gr_get (current_cpu, 6);

	if (s.func == TARGET_SYS_exit)
	  {
	    sim_engine_halt (sd, current_cpu, NULL, pc, sim_exited, s.arg1);
	  }

	s.p1 = (PTR) sd;
	s.p2 = (PTR) current_cpu;
	s.read_mem = syscall_read_mem;
	s.write_mem = syscall_write_mem;
	cb_syscall (cb, &s);
	fr30bf_h_gr_set (current_cpu, 2, s.errcode); /* TODO: check this one */
	fr30bf_h_gr_set (current_cpu, 4, s.result);
	fr30bf_h_gr_set (current_cpu, 1, s.result2); /* TODO: check this one */
	break;
      }

    case TRAP_BREAKPOINT:
      sim_engine_halt (sd, current_cpu, NULL, pc,
		       sim_stopped, SIM_SIGTRAP);
      break;

    default :
      {
	USI new_pc;
	setup_int (current_cpu, pc);
	fr30bf_h_ibit_set (current_cpu, 0);
	new_pc = GETMEMSI (current_cpu, pc,
			   fr30bf_h_dr_get (current_cpu, H_DR_TBR)
			   + 1024 - ((num + 1) * 4));
	return new_pc;
      }
    }

  /* Fake an "reti" insn.
     Since we didn't push anything to stack, all we need to do is
     update pc.  */
  return pc + 2;
}

USI
fr30_inte (SIM_CPU *current_cpu, PCADDR pc, int num)
{
  /* The new pc is the trap #9 vector entry.
     We assume there's a branch there to some handler.  */
  USI new_pc;
  setup_int (current_cpu, pc);
  fr30bf_h_ilm_set (current_cpu, 4);
  new_pc = GETMEMSI (current_cpu, pc,
		     fr30bf_h_dr_get (current_cpu, H_DR_TBR)
		     + 1024 - ((9 + 1) * 4));
  return new_pc;
}
