/*  This file is part of the program psim.

    Copyright (C) 1994-1997, Andrew Cagney <cagney@highland.com.au>
    Copyright (C) 1996, 1997, Free Software Foundation

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
    */


#ifndef ENGINE_C
#define ENGINE_C

#include "sim-main.h"

#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

static void
do_stack_swap (SIM_DESC sd)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  unsigned new_sp = (PSW_VAL(PSW_SM) != 0);
  if (cpu->regs.current_sp != new_sp)
    {
      cpu->regs.sp[cpu->regs.current_sp] = SP;
      cpu->regs.current_sp = new_sp;
      SP = cpu->regs.sp[cpu->regs.current_sp];
    }
}

#if WITH_TRACE
/* Implement ALU tracing of 32-bit registers.  */
static void
trace_alu32 (SIM_DESC sd,
	     sim_cpu *cpu,
	     address_word cia,
	     unsigned32 *ptr)
{
  unsigned32 value = *ptr;

  if (ptr >= &GPR[0] && ptr <= &GPR[NR_GENERAL_PURPOSE_REGISTERS])
    trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu",
		    "Set register r%-2d = 0x%.8lx (%ld)",
		    ptr - &GPR[0], (long)value, (long)value);

  else if (ptr == &PSW || ptr == &bPSW || ptr == &DPSW)
    trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu",
		    "Set register %s = 0x%.8lx%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
		    (ptr == &PSW) ? "psw" : ((ptr == &bPSW) ? "bpsw" : "dpsw"),
		    (long)value,
		    (value & (0x80000000 >> PSW_SM)) ? ", sm" : "",
		    (value & (0x80000000 >> PSW_EA)) ? ", ea" : "",
		    (value & (0x80000000 >> PSW_DB)) ? ", db" : "",
		    (value & (0x80000000 >> PSW_DS)) ? ", ds" : "",
		    (value & (0x80000000 >> PSW_IE)) ? ", ie" : "",
		    (value & (0x80000000 >> PSW_RP)) ? ", rp" : "",
		    (value & (0x80000000 >> PSW_MD)) ? ", md" : "",
		    (value & (0x80000000 >> PSW_F0)) ? ", f0" : "",
		    (value & (0x80000000 >> PSW_F1)) ? ", f1" : "",
		    (value & (0x80000000 >> PSW_F2)) ? ", f2" : "",
		    (value & (0x80000000 >> PSW_F3)) ? ", f3" : "",
		    (value & (0x80000000 >> PSW_S))  ? ", s"  : "",
		    (value & (0x80000000 >> PSW_V))  ? ", v"  : "",
		    (value & (0x80000000 >> PSW_VA)) ? ", va" : "",
		    (value & (0x80000000 >> PSW_C))  ? ", c"  : "");

  else if (ptr >= &CREG[0] && ptr <= &CREG[NR_CONTROL_REGISTERS])
    trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu",
		    "Set register cr%d = 0x%.8lx (%ld)",
		    ptr - &CREG[0], (long)value, (long)value);
}

/* Implement ALU tracing of 32-bit registers.  */
static void
trace_alu64 (SIM_DESC sd,
	     sim_cpu *cpu,
	     address_word cia,
	     unsigned64 *ptr)
{
  unsigned64 value = *ptr;

  if (ptr >= &ACC[0] && ptr <= &ACC[NR_ACCUMULATORS])
    trace_one_insn (sd, cpu, cia, 1, "engine.c", __LINE__, "alu",
		    "Set register a%-2d = 0x%.8lx 0x%.8lx",
		    ptr - &ACC[0],
		    (unsigned long)(unsigned32)(value >> 32),
		    (unsigned long)(unsigned32)value);

}
#endif

/* Process all of the queued up writes in order now */
void
unqueue_writes (SIM_DESC sd,
		sim_cpu *cpu,
		address_word cia)
{
  int i, num;
  int did_psw = 0;
  unsigned32 *psw_addr = &PSW;

  num = WRITE32_NUM;
  for (i = 0; i < num; i++)
    {
      unsigned32 mask = WRITE32_MASK (i);
      unsigned32 *ptr = WRITE32_PTR (i);
      unsigned32 value = (*ptr & ~mask) | (WRITE32_VALUE (i) & mask);
      int j;

      if (ptr == psw_addr)
       {
	 /* If MU instruction was not a MVTSYS, resolve PSW
             contention in favour of IU. */
	  if(! STATE_CPU (sd, 0)->mvtsys_left_p)
	    {
	      /* Detect contention in parallel writes to the same PSW flags.
		 The hardware allows the updates from IU to prevail over
		 those from MU. */
	      
	      unsigned32 flag_bits =
		BIT32 (PSW_F0) | BIT32 (PSW_F1) |
		BIT32 (PSW_F2) | BIT32 (PSW_F3) |
		BIT32 (PSW_S) | BIT32 (PSW_V) |
		BIT32 (PSW_VA) | BIT32 (PSW_C);
	      unsigned32 my_flag_bits = mask & flag_bits;
	      
	      for (j = i + 1; j < num; j++)
		if (WRITE32_PTR (j) == psw_addr && /* write to PSW */
		    WRITE32_MASK (j) & my_flag_bits)  /* some of the same flags */
		  {
		    /* Recompute local mask & value, to suppress this
		       earlier write to the same flag bits. */
		    
		    unsigned32 new_mask = mask & ~(WRITE32_MASK (j) & my_flag_bits);
		    
		    /* There is a special case for the VA (accumulated
		       overflow) flag, in that it is only included in the
		       second instruction's mask if the overflow
		       occurred.  Yet the hardware still suppresses the
		       first instruction's update to VA.  So we kludge
		       this by inferring PSW_V -> PSW_VA for the second
		       instruction. */
		    
		    if (WRITE32_MASK (j) & BIT32 (PSW_V))
		      {
			new_mask &= ~BIT32 (PSW_VA);
		      }
		    
		    value = (*ptr & ~new_mask) | (WRITE32_VALUE (i) & new_mask);
		  }
	    }
	  
         did_psw = 1;
       }

      *ptr = value;

#if WITH_TRACE
      if (TRACE_ALU_P (cpu))
	trace_alu32 (sd, cpu, cia, ptr);
#endif
    }

  num = WRITE64_NUM;
  for (i = 0; i < num; i++)
    {
      unsigned64 *ptr = WRITE64_PTR (i);
      *ptr = WRITE64_VALUE (i);

#if WITH_TRACE
      if (TRACE_ALU_P (cpu))
	trace_alu64 (sd, cpu, cia, ptr);
#endif
    }

  WRITE32_NUM = 0;
  WRITE64_NUM = 0;

  if (DID_TRAP == 1) /* ordinary trap */
    {
      bPSW = PSW;
      PSW &= (BIT32 (PSW_DB) | BIT32 (PSW_SM));
      did_psw = 1;
    }
  else if (DID_TRAP == 2) /* debug trap */
    {
      DPSW = PSW;
      PSW &= BIT32 (PSW_DS);
      PSW |= BIT32 (PSW_DS);
      did_psw = 1;
    }
  DID_TRAP = 0;

  if (did_psw)
    do_stack_swap (sd);
}


/* SIMULATE INSTRUCTIONS, various different ways of achieving the same
   thing */

static address_word
do_long (SIM_DESC sd,
	 l_instruction_word instruction,
	 address_word cia)
{
  address_word nia = l_idecode_issue(sd,
				     instruction,
				     cia);

  unqueue_writes (sd, STATE_CPU (sd, 0), cia);
  return nia;
}

static address_word
do_2_short (SIM_DESC sd,
	    s_instruction_word insn1,
	    s_instruction_word insn2,
	    cpu_units unit,
	    address_word cia)
{
  address_word nia;

  /* run the first instruction */
  STATE_CPU (sd, 0)->unit = unit;
  STATE_CPU (sd, 0)->left_kills_right_p = 0;
  STATE_CPU (sd, 0)->mvtsys_left_p = 0;
  nia = s_idecode_issue(sd,
			insn1,
			cia);

  unqueue_writes (sd, STATE_CPU (sd, 0), cia);

  /* Only do the second instruction if the PC has not changed */
  if ((nia == INVALID_INSTRUCTION_ADDRESS) &&
      (! STATE_CPU (sd, 0)->left_kills_right_p)) {
    STATE_CPU (sd, 0)->unit = any_unit;
    nia = s_idecode_issue (sd,
			   insn2,
			   cia);

    unqueue_writes (sd, STATE_CPU (sd, 0), cia);
  }

  STATE_CPU (sd, 0)->left_kills_right_p = 0;
  STATE_CPU (sd, 0)->mvtsys_left_p = 0;
  return nia;
}

static address_word
do_parallel (SIM_DESC sd,
	     s_instruction_word left_insn,
	     s_instruction_word right_insn,
	     address_word cia)
{
  address_word nia_left;
  address_word nia_right;
  address_word nia;

  /* run the first instruction */
  STATE_CPU (sd, 0)->unit = memory_unit;
  STATE_CPU (sd, 0)->left_kills_right_p = 0;
  STATE_CPU (sd, 0)->mvtsys_left_p = 0;
  nia_left = s_idecode_issue(sd,
			     left_insn,
			     cia);

  /* run the second instruction */
  STATE_CPU (sd, 0)->unit = integer_unit;
  nia_right = s_idecode_issue(sd,
			      right_insn,
			      cia);

  /* merge the PC's */
  if (nia_left == INVALID_INSTRUCTION_ADDRESS) {
    if (nia_right == INVALID_INSTRUCTION_ADDRESS)
      nia = INVALID_INSTRUCTION_ADDRESS;
    else
      nia = nia_right;
  }
  else {
    if (nia_right == INVALID_INSTRUCTION_ADDRESS)
      nia = nia_left;
    else {
      sim_engine_abort (sd, STATE_CPU (sd, 0), cia, "parallel jumps");
      nia = INVALID_INSTRUCTION_ADDRESS;
    }
  }

  unqueue_writes (sd, STATE_CPU (sd, 0), cia);
  return nia;
}


typedef enum {
  p_insn = 0,
  long_insn = 3,
  l_r_insn = 1,
  r_l_insn = 2,
} instruction_types;

STATIC_INLINE instruction_types
instruction_type(l_instruction_word insn)
{
  int fm0 = MASKED64(insn, 0, 0) != 0;
  int fm1 = MASKED64(insn, 32, 32) != 0;
  return ((fm0 << 1) | fm1);
}



void
sim_engine_run (SIM_DESC sd,
		int last_cpu_nr,
		int nr_cpus,
		int siggnal)
{
  while (1)
    {
      address_word cia = PC;
      address_word nia;
      l_instruction_word insn = IMEM(cia);
      int rp_was_set;
      int rpt_c_was_nonzero;

      /* Before executing the instruction, we need to test whether or
	 not RPT_C is greater than zero, and save that state for use
	 after executing the instruction.  In particular, we need to
	 not care whether the instruction changes RPT_C itself. */

      rpt_c_was_nonzero = (RPT_C > 0);

      /* Before executing the instruction, we need to check to see if
	 we have to decrement RPT_C, the repeat count register.  Do this
	 if PC == RPT_E, but only if we are in an active repeat block. */

      if (PC == RPT_E &&
	  (RPT_C > 0 || PSW_VAL (PSW_RP) != 0))
	{
	  RPT_C --;
	}
      
      /* Now execute the instruction at PC */

      switch (instruction_type (insn))
	{
	case long_insn:
	  nia = do_long (sd, insn, cia);
	  break;
	case r_l_insn:
	  /* L <- R */
	  nia = do_2_short (sd, insn, insn >> 32, integer_unit, cia);
	  break;
	case l_r_insn:
	  /* L -> R */
	  nia = do_2_short (sd, insn >> 32, insn, memory_unit, cia);
	  break;
	case p_insn:
	  nia = do_parallel (sd, insn >> 32, insn, cia);
	  break;
	default:
	  sim_engine_abort (sd, STATE_CPU (sd, 0), cia,
			    "internal error - engine_run_until_stop - bad switch");
	  nia = -1;
	}

      if (TRACE_ACTION)
	{
	  if (TRACE_ACTION & TRACE_ACTION_CALL)
	    call_occurred (sd, STATE_CPU (sd, 0), cia, nia);

	  if (TRACE_ACTION & TRACE_ACTION_RETURN)
	    return_occurred (sd, STATE_CPU (sd, 0), cia, nia);

	  TRACE_ACTION = 0;
	}

      /* Check now to see if we need to reset the RP bit in the PSW.
	 There are three conditions for this, the RP bit is already
	 set (just a speed optimization), the instruction we just
	 executed is the last instruction in the loop, and the repeat
	 count is currently zero. */

      rp_was_set = PSW_VAL (PSW_RP);
      if (rp_was_set && (PC == RPT_E) && RPT_C == 0)
	{
	  PSW_SET (PSW_RP, 0);
	}

      /* Now update the PC.  If we just executed a jump instruction,
	 that takes precedence over everything else.  Next comes
	 branching back to RPT_S as a result of a loop.  Finally, the
	 default is to simply advance to the next inline
	 instruction. */

      if (nia != INVALID_INSTRUCTION_ADDRESS)
	{
	  PC = nia;
	}
      else if (rp_was_set && rpt_c_was_nonzero && (PC == RPT_E))
	{
	  PC = RPT_S;
	}
      else
	{
	  PC = cia + 8;
	}

      /* Check for DDBT (debugger debug trap) condition.  Do this after
	 the repeat block checks so the excursion to the trap handler does
	 not alter looping state. */

      if (cia == IBA && PSW_VAL (PSW_DB))
	{
	  DPC = PC;
	  PSW_SET (PSW_EA, 1);
	  DPSW = PSW;
	  /* clear all bits in PSW except SM */
	  PSW &= BIT32 (PSW_SM);
	  /* add DS bit */
	  PSW |= BIT32 (PSW_DS);
	  /* dispatch to DDBT handler */
	  PC = 0xfffff128; /* debugger_debug_trap_address */
	}

      /* process any events */
      /* FIXME - should L->R or L<-R insns count as two cycles? */
      if (sim_events_tick (sd))
	{
	  sim_events_process (sd);
	}
    }  
}


/* d30v external interrupt handler.

   Note: This should be replaced by a proper interrupt delivery
   mechanism.  This interrupt mechanism discards later interrupts if
   an earlier interrupt hasn't been delivered.

   Note: This interrupt mechanism does not reset its self when the
   simulator is re-opened. */

void
d30v_interrupt_event (SIM_DESC sd,
		      void *data)
{
  if (PSW_VAL (PSW_IE))
    /* interrupts not masked */
    {
      /* scrub any pending interrupt */
      if (sd->pending_interrupt != NULL)
	sim_events_deschedule (sd, sd->pending_interrupt);
      /* deliver */
      bPSW = PSW;
      bPC = PC;
      PSW = 0;
      PC = 0xfffff138; /* external interrupt */
      do_stack_swap (sd);
    }
  else if (sd->pending_interrupt == NULL)
    /* interrupts masked and no interrupt pending */
    {
      sd->pending_interrupt = sim_events_schedule (sd, 1,
						   d30v_interrupt_event,
						   data);
    }
}

#endif
