// OBSOLETE /* Low level interface to I386 running mach 3.0.
// OBSOLETE    Copyright 1992, 1993, 1994, 1996, 2000, 2001
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "floatformat.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE #include <stdio.h>
// OBSOLETE 
// OBSOLETE #include <mach.h>
// OBSOLETE #include <mach/message.h>
// OBSOLETE #include <mach/exception.h>
// OBSOLETE #include <mach_error.h>
// OBSOLETE 
// OBSOLETE /* Hmmm... Should this not be here?
// OBSOLETE  * Now for i386_float_info() target_has_execution
// OBSOLETE  */
// OBSOLETE #include <target.h>
// OBSOLETE 
// OBSOLETE /* This mess is duplicated in bfd/i386mach3.h
// OBSOLETE 
// OBSOLETE  * This is an ugly way to hack around the incorrect
// OBSOLETE  * definition of UPAGES in i386/machparam.h.
// OBSOLETE  *
// OBSOLETE  * The definition should specify the size reserved
// OBSOLETE  * for "struct user" in core files in PAGES,
// OBSOLETE  * but instead it gives it in 512-byte core-clicks
// OBSOLETE  * for i386 and i860.
// OBSOLETE  */
// OBSOLETE #include <sys/param.h>
// OBSOLETE #if UPAGES == 16
// OBSOLETE #define UAREA_SIZE ctob(UPAGES)
// OBSOLETE #elif UPAGES == 2
// OBSOLETE #define UAREA_SIZE (NBPG*UPAGES)
// OBSOLETE #else
// OBSOLETE FIXME ! !UPAGES is neither 2 nor 16
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /* @@@ Should move print_387_status() to i387-tdep.c */
// OBSOLETE extern void print_387_control_word ();		/* i387-tdep.h */
// OBSOLETE extern void print_387_status_word ();
// OBSOLETE 
// OBSOLETE #define private static
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Find offsets to thread states at compile time.
// OBSOLETE  * If your compiler does not grok this, calculate offsets
// OBSOLETE  * offsets yourself and use them (or get a compatible compiler :-)
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #define  REG_OFFSET(reg) (int)(&((struct i386_thread_state *)0)->reg)
// OBSOLETE 
// OBSOLETE /* at reg_offset[i] is the offset to the i386_thread_state
// OBSOLETE  * location where the gdb registers[i] is stored.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static int reg_offset[] =
// OBSOLETE {
// OBSOLETE   REG_OFFSET (eax), REG_OFFSET (ecx), REG_OFFSET (edx), REG_OFFSET (ebx),
// OBSOLETE   REG_OFFSET (uesp), REG_OFFSET (ebp), REG_OFFSET (esi), REG_OFFSET (edi),
// OBSOLETE   REG_OFFSET (eip), REG_OFFSET (efl), REG_OFFSET (cs), REG_OFFSET (ss),
// OBSOLETE   REG_OFFSET (ds), REG_OFFSET (es), REG_OFFSET (fs), REG_OFFSET (gs)
// OBSOLETE };
// OBSOLETE 
// OBSOLETE #define REG_ADDRESS(state,regnum) ((char *)(state)+reg_offset[regnum])
// OBSOLETE 
// OBSOLETE /* Fetch COUNT contiguous registers from thread STATE starting from REGNUM
// OBSOLETE  * Caller knows that the regs handled in one transaction are of same size.
// OBSOLETE  */
// OBSOLETE #define FETCH_REGS(state, regnum, count) \
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (regnum)], \
// OBSOLETE 	  REG_ADDRESS (state, regnum), \
// OBSOLETE 	  count*REGISTER_SIZE)
// OBSOLETE 
// OBSOLETE /* Store COUNT contiguous registers to thread STATE starting from REGNUM */
// OBSOLETE #define STORE_REGS(state, regnum, count) \
// OBSOLETE   memcpy (REG_ADDRESS (state, regnum), \
// OBSOLETE 	  &registers[REGISTER_BYTE (regnum)], \
// OBSOLETE 	  count*REGISTER_SIZE)
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * Fetch inferiors registers for gdb.
// OBSOLETE  * REGNO specifies which (as gdb views it) register, -1 for all.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fetch_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   kern_return_t ret;
// OBSOLETE   thread_state_data_t state;
// OBSOLETE   unsigned int stateCnt = i386_THREAD_STATE_COUNT;
// OBSOLETE   int index;
// OBSOLETE 
// OBSOLETE   if (!MACH_PORT_VALID (current_thread))
// OBSOLETE     error ("fetch inferior registers: Invalid thread");
// OBSOLETE 
// OBSOLETE   if (must_suspend_thread)
// OBSOLETE     setup_thread (current_thread, 1);
// OBSOLETE 
// OBSOLETE   ret = thread_get_state (current_thread,
// OBSOLETE 			  i386_THREAD_STATE,
// OBSOLETE 			  state,
// OBSOLETE 			  &stateCnt);
// OBSOLETE 
// OBSOLETE   if (ret != KERN_SUCCESS)
// OBSOLETE     warning ("fetch_inferior_registers: %s ",
// OBSOLETE 	     mach_error_string (ret));
// OBSOLETE #if 0
// OBSOLETE   /* It may be more effective to store validate all of them,
// OBSOLETE    * since we fetched them all anyway
// OBSOLETE    */
// OBSOLETE   else if (regno != -1)
// OBSOLETE     supply_register (regno, (char *) state + reg_offset[regno]);
// OBSOLETE #endif
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       for (index = 0; index < NUM_REGS; index++)
// OBSOLETE 	supply_register (index, (char *) state + reg_offset[index]);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (must_suspend_thread)
// OBSOLETE     setup_thread (current_thread, 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store our register values back into the inferior.
// OBSOLETE  * If REGNO is -1, do this for all registers.
// OBSOLETE  * Otherwise, REGNO specifies which register
// OBSOLETE  *
// OBSOLETE  * On mach3 all registers are always saved in one call.
// OBSOLETE  */
// OBSOLETE void
// OBSOLETE store_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   kern_return_t ret;
// OBSOLETE   thread_state_data_t state;
// OBSOLETE   unsigned int stateCnt = i386_THREAD_STATE_COUNT;
// OBSOLETE   register int index;
// OBSOLETE 
// OBSOLETE   if (!MACH_PORT_VALID (current_thread))
// OBSOLETE     error ("store inferior registers: Invalid thread");
// OBSOLETE 
// OBSOLETE   if (must_suspend_thread)
// OBSOLETE     setup_thread (current_thread, 1);
// OBSOLETE 
// OBSOLETE   /* Fetch the state of the current thread */
// OBSOLETE   ret = thread_get_state (current_thread,
// OBSOLETE 			  i386_THREAD_STATE,
// OBSOLETE 			  state,
// OBSOLETE 			  &stateCnt);
// OBSOLETE 
// OBSOLETE   if (ret != KERN_SUCCESS)
// OBSOLETE     {
// OBSOLETE       warning ("store_inferior_registers (get): %s",
// OBSOLETE 	       mach_error_string (ret));
// OBSOLETE       if (must_suspend_thread)
// OBSOLETE 	setup_thread (current_thread, 0);
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* move gdb's registers to thread's state
// OBSOLETE 
// OBSOLETE    * Since we save all registers anyway, save the ones
// OBSOLETE    * that gdb thinks are valid (e.g. ignore the regno
// OBSOLETE    * parameter)
// OBSOLETE    */
// OBSOLETE #if 0
// OBSOLETE   if (regno != -1)
// OBSOLETE     STORE_REGS (state, regno, 1);
// OBSOLETE   else
// OBSOLETE #endif
// OBSOLETE     {
// OBSOLETE       for (index = 0; index < NUM_REGS; index++)
// OBSOLETE 	STORE_REGS (state, index, 1);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Write gdb's current view of register to the thread
// OBSOLETE    */
// OBSOLETE   ret = thread_set_state (current_thread,
// OBSOLETE 			  i386_THREAD_STATE,
// OBSOLETE 			  state,
// OBSOLETE 			  i386_THREAD_STATE_COUNT);
// OBSOLETE 
// OBSOLETE   if (ret != KERN_SUCCESS)
// OBSOLETE     warning ("store_inferior_registers (set): %s",
// OBSOLETE 	     mach_error_string (ret));
// OBSOLETE 
// OBSOLETE   if (must_suspend_thread)
// OBSOLETE     setup_thread (current_thread, 0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Return the address in the core dump or inferior of register REGNO.
// OBSOLETE  * BLOCKEND should be the address of the end of the UPAGES area read
// OBSOLETE  * in memory, but it's not?
// OBSOLETE  *
// OBSOLETE  * Currently our UX server dumps the whole thread state to the
// OBSOLETE  * core file. If your UX does something else, adapt the routine
// OBSOLETE  * below to return the offset to the given register.
// OBSOLETE  * 
// OBSOLETE  * Called by core-aout.c(fetch_core_registers)
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE CORE_ADDR
// OBSOLETE register_addr (int regno, CORE_ADDR blockend)
// OBSOLETE {
// OBSOLETE   CORE_ADDR addr;
// OBSOLETE 
// OBSOLETE   if (regno < 0 || regno >= NUM_REGS)
// OBSOLETE     error ("Invalid register number %d.", regno);
// OBSOLETE 
// OBSOLETE   /* UAREA_SIZE == 8 kB in i386 */
// OBSOLETE   addr = (unsigned int) REG_ADDRESS (UAREA_SIZE - sizeof (struct i386_thread_state), regno);
// OBSOLETE 
// OBSOLETE   return addr;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* jtv@hut.fi: I copied and modified this 387 code from
// OBSOLETE  * gdb/i386-xdep.c. Modifications for Mach 3.0.
// OBSOLETE  *
// OBSOLETE  * i387 status dumper. See also i387-tdep.c
// OBSOLETE  */
// OBSOLETE struct env387
// OBSOLETE {
// OBSOLETE   unsigned short control;
// OBSOLETE   unsigned short r0;
// OBSOLETE   unsigned short status;
// OBSOLETE   unsigned short r1;
// OBSOLETE   unsigned short tag;
// OBSOLETE   unsigned short r2;
// OBSOLETE   unsigned long eip;
// OBSOLETE   unsigned short code_seg;
// OBSOLETE   unsigned short opcode;
// OBSOLETE   unsigned long operand;
// OBSOLETE   unsigned short operand_seg;
// OBSOLETE   unsigned short r3;
// OBSOLETE   unsigned char regs[8][10];
// OBSOLETE };
// OBSOLETE /* This routine is machine independent?
// OBSOLETE  * Should move it to i387-tdep.c but you need to export struct env387
// OBSOLETE  */
// OBSOLETE private
// OBSOLETE print_387_status (unsigned short status, struct env387 *ep)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   int bothstatus;
// OBSOLETE   int top;
// OBSOLETE   int fpreg;
// OBSOLETE   unsigned char *p;
// OBSOLETE 
// OBSOLETE   bothstatus = ((status != 0) && (ep->status != 0));
// OBSOLETE   if (status != 0)
// OBSOLETE     {
// OBSOLETE       if (bothstatus)
// OBSOLETE 	printf_unfiltered ("u: ");
// OBSOLETE       print_387_status_word (status);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (ep->status != 0)
// OBSOLETE     {
// OBSOLETE       if (bothstatus)
// OBSOLETE 	printf_unfiltered ("e: ");
// OBSOLETE       print_387_status_word (ep->status);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   print_387_control_word (ep->control);
// OBSOLETE   printf_unfiltered ("last exception: ");
// OBSOLETE   printf_unfiltered ("opcode %s; ", local_hex_string (ep->opcode));
// OBSOLETE   printf_unfiltered ("pc %s:", local_hex_string (ep->code_seg));
// OBSOLETE   printf_unfiltered ("%s; ", local_hex_string (ep->eip));
// OBSOLETE   printf_unfiltered ("operand %s", local_hex_string (ep->operand_seg));
// OBSOLETE   printf_unfiltered (":%s\n", local_hex_string (ep->operand));
// OBSOLETE 
// OBSOLETE   top = (ep->status >> 11) & 7;
// OBSOLETE 
// OBSOLETE   printf_unfiltered ("regno  tag  msb              lsb  value\n");
// OBSOLETE   for (fpreg = 7; fpreg >= 0; fpreg--)
// OBSOLETE     {
// OBSOLETE       double val;
// OBSOLETE 
// OBSOLETE       printf_unfiltered ("%s %d: ", fpreg == top ? "=>" : "  ", fpreg);
// OBSOLETE 
// OBSOLETE       switch ((ep->tag >> (fpreg * 2)) & 3)
// OBSOLETE 	{
// OBSOLETE 	case 0:
// OBSOLETE 	  printf_unfiltered ("valid ");
// OBSOLETE 	  break;
// OBSOLETE 	case 1:
// OBSOLETE 	  printf_unfiltered ("zero  ");
// OBSOLETE 	  break;
// OBSOLETE 	case 2:
// OBSOLETE 	  printf_unfiltered ("trap  ");
// OBSOLETE 	  break;
// OBSOLETE 	case 3:
// OBSOLETE 	  printf_unfiltered ("empty ");
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       for (i = 9; i >= 0; i--)
// OBSOLETE 	printf_unfiltered ("%02x", ep->regs[fpreg][i]);
// OBSOLETE 
// OBSOLETE       floatformat_to_double (&floatformat_i387_ext, (char *) ep->regs[fpreg],
// OBSOLETE 			     &val);
// OBSOLETE       printf_unfiltered ("  %g\n", val);
// OBSOLETE     }
// OBSOLETE   if (ep->r0)
// OBSOLETE     printf_unfiltered ("warning: reserved0 is %s\n", local_hex_string (ep->r0));
// OBSOLETE   if (ep->r1)
// OBSOLETE     printf_unfiltered ("warning: reserved1 is %s\n", local_hex_string (ep->r1));
// OBSOLETE   if (ep->r2)
// OBSOLETE     printf_unfiltered ("warning: reserved2 is %s\n", local_hex_string (ep->r2));
// OBSOLETE   if (ep->r3)
// OBSOLETE     printf_unfiltered ("warning: reserved3 is %s\n", local_hex_string (ep->r3));
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * values that go into fp_kind (from <i386/fpreg.h>)
// OBSOLETE  */
// OBSOLETE #define FP_NO   0		/* no fp chip, no emulator (no fp support)      */
// OBSOLETE #define FP_SW   1		/* no fp chip, using software emulator          */
// OBSOLETE #define FP_HW   2		/* chip present bit                             */
// OBSOLETE #define FP_287  2		/* 80287 chip present                           */
// OBSOLETE #define FP_387  3		/* 80387 chip present                           */
// OBSOLETE 
// OBSOLETE typedef struct fpstate
// OBSOLETE {
// OBSOLETE #if 1
// OBSOLETE   unsigned char state[FP_STATE_BYTES];	/* "hardware" state */
// OBSOLETE #else
// OBSOLETE   struct env387 state;		/* Actually this */
// OBSOLETE #endif
// OBSOLETE   int status;			/* Duplicate status */
// OBSOLETE }
// OBSOLETE  *fpstate_t;
// OBSOLETE 
// OBSOLETE /* Mach 3 specific routines.
// OBSOLETE  */
// OBSOLETE private boolean_t
// OBSOLETE get_i387_state (struct fpstate *fstate)
// OBSOLETE {
// OBSOLETE   kern_return_t ret;
// OBSOLETE   thread_state_data_t state;
// OBSOLETE   unsigned int fsCnt = i386_FLOAT_STATE_COUNT;
// OBSOLETE   struct i386_float_state *fsp;
// OBSOLETE 
// OBSOLETE   ret = thread_get_state (current_thread,
// OBSOLETE 			  i386_FLOAT_STATE,
// OBSOLETE 			  state,
// OBSOLETE 			  &fsCnt);
// OBSOLETE 
// OBSOLETE   if (ret != KERN_SUCCESS)
// OBSOLETE     {
// OBSOLETE       warning ("Can not get live floating point state: %s",
// OBSOLETE 	       mach_error_string (ret));
// OBSOLETE       return FALSE;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   fsp = (struct i386_float_state *) state;
// OBSOLETE   /* The 387 chip (also 486 counts) or a software emulator? */
// OBSOLETE   if (!fsp->initialized || (fsp->fpkind != FP_387 && fsp->fpkind != FP_SW))
// OBSOLETE     return FALSE;
// OBSOLETE 
// OBSOLETE   /* Clear the target then copy thread's float state there.
// OBSOLETE      Make a copy of the status word, for some reason?
// OBSOLETE    */
// OBSOLETE   memset (fstate, 0, sizeof (struct fpstate));
// OBSOLETE 
// OBSOLETE   fstate->status = fsp->exc_status;
// OBSOLETE 
// OBSOLETE   memcpy (fstate->state, (char *) &fsp->hw_state, FP_STATE_BYTES);
// OBSOLETE 
// OBSOLETE   return TRUE;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE private boolean_t
// OBSOLETE get_i387_core_state (struct fpstate *fstate)
// OBSOLETE {
// OBSOLETE   /* Not implemented yet. Core files do not contain float state. */
// OBSOLETE   return FALSE;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*
// OBSOLETE  * This is called by "info float" command
// OBSOLETE  */
// OBSOLETE void
// OBSOLETE i386_mach3_float_info (void)
// OBSOLETE {
// OBSOLETE   char buf[sizeof (struct fpstate) + 2 * sizeof (int)];
// OBSOLETE   boolean_t valid = FALSE;
// OBSOLETE   fpstate_t fps;
// OBSOLETE 
// OBSOLETE   if (target_has_execution)
// OBSOLETE     valid = get_i387_state (buf);
// OBSOLETE #if 0
// OBSOLETE   else if (WE HAVE CORE FILE)	/* @@@@ Core files not supported */
// OBSOLETE     valid = get_i387_core_state (buf);
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE   if (!valid)
// OBSOLETE     {
// OBSOLETE       warning ("no floating point status saved");
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   fps = (fpstate_t) buf;
// OBSOLETE 
// OBSOLETE   print_387_status (fps->status, (struct env387 *) fps->state);
// OBSOLETE }
