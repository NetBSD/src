// OBSOLETE /* Native dependent code for Mach 386's for GDB, the GNU debugger.
// OBSOLETE    Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1995, 1996, 1999, 2000,
// OBSOLETE    2001 Free Software Foundation, Inc.
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
// OBSOLETE #include "frame.h"
// OBSOLETE #include "inferior.h"
// OBSOLETE #include "gdbcore.h"
// OBSOLETE #include "regcache.h"
// OBSOLETE 
// OBSOLETE #include <sys/param.h>
// OBSOLETE #include <sys/dir.h>
// OBSOLETE #include <sys/user.h>
// OBSOLETE #include <signal.h>
// OBSOLETE #include <sys/ioctl.h>
// OBSOLETE #include <fcntl.h>
// OBSOLETE 
// OBSOLETE #include <sys/ptrace.h>
// OBSOLETE #include <machine/reg.h>
// OBSOLETE 
// OBSOLETE #include <sys/file.h>
// OBSOLETE #include "gdb_stat.h"
// OBSOLETE #include <sys/core.h>
// OBSOLETE 
// OBSOLETE static void fetch_core_registers (char *, unsigned, int, CORE_ADDR);
// OBSOLETE 
// OBSOLETE void
// OBSOLETE fetch_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   struct regs inferior_registers;
// OBSOLETE   struct fp_state inferior_fp_registers;
// OBSOLETE 
// OBSOLETE   registers_fetched ();
// OBSOLETE 
// OBSOLETE   ptrace (PTRACE_GETREGS, PIDGET (inferior_ptid),
// OBSOLETE 	  (PTRACE_ARG3_TYPE) & inferior_registers);
// OBSOLETE   ptrace (PTRACE_GETFPREGS, PIDGET (inferior_ptid),
// OBSOLETE 	  (PTRACE_ARG3_TYPE) & inferior_fp_registers);
// OBSOLETE 
// OBSOLETE   memcpy (registers, &inferior_registers, sizeof inferior_registers);
// OBSOLETE 
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (FP0_REGNUM)],
// OBSOLETE 	  inferior_fp_registers.f_st,
// OBSOLETE 	  sizeof inferior_fp_registers.f_st);
// OBSOLETE   memcpy (&registers[REGISTER_BYTE (FPC_REGNUM)],
// OBSOLETE 	  &inferior_fp_registers.f_ctrl,
// OBSOLETE 	  sizeof inferior_fp_registers - sizeof inferior_fp_registers.f_st);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Store our register values back into the inferior.
// OBSOLETE    If REGNO is -1, do this for all registers.
// OBSOLETE    Otherwise, REGNO specifies which register (so we can save time).  */
// OBSOLETE 
// OBSOLETE void
// OBSOLETE store_inferior_registers (int regno)
// OBSOLETE {
// OBSOLETE   struct regs inferior_registers;
// OBSOLETE   struct fp_state inferior_fp_registers;
// OBSOLETE 
// OBSOLETE   memcpy (&inferior_registers, registers, 20 * 4);
// OBSOLETE 
// OBSOLETE   memcpy (inferior_fp_registers.f_st, &registers[REGISTER_BYTE (FP0_REGNUM)],
// OBSOLETE 	  sizeof inferior_fp_registers.f_st);
// OBSOLETE   memcpy (&inferior_fp_registers.f_ctrl,
// OBSOLETE 	  &registers[REGISTER_BYTE (FPC_REGNUM)],
// OBSOLETE 	  sizeof inferior_fp_registers - sizeof inferior_fp_registers.f_st);
// OBSOLETE 
// OBSOLETE #ifdef PTRACE_FP_BUG
// OBSOLETE   if (regno == FP_REGNUM || regno == -1)
// OBSOLETE     /* Storing the frame pointer requires a gross hack, in which an
// OBSOLETE        instruction that moves eax into ebp gets single-stepped.  */
// OBSOLETE     {
// OBSOLETE       int stack = inferior_registers.r_reg[SP_REGNUM];
// OBSOLETE       int stuff = ptrace (PTRACE_PEEKDATA, PIDGET (inferior_ptid),
// OBSOLETE 			  (PTRACE_ARG3_TYPE) stack);
// OBSOLETE       int reg = inferior_registers.r_reg[EAX];
// OBSOLETE       inferior_registers.r_reg[EAX] =
// OBSOLETE 	inferior_registers.r_reg[FP_REGNUM];
// OBSOLETE       ptrace (PTRACE_SETREGS, PIDGET (inferior_ptid),
// OBSOLETE 	      (PTRACE_ARG3_TYPE) & inferior_registers);
// OBSOLETE       ptrace (PTRACE_POKEDATA, PIDGET (inferior_ptid),
// OBSOLETE               (PTRACE_ARG3_TYPE) stack, 0xc589);
// OBSOLETE       ptrace (PTRACE_SINGLESTEP, PIDGET (inferior_ptid),
// OBSOLETE               (PTRACE_ARG3_TYPE) stack, 0);
// OBSOLETE       wait (0);
// OBSOLETE       ptrace (PTRACE_POKEDATA, PIDGET (inferior_ptid),
// OBSOLETE               (PTRACE_ARG3_TYPE) stack, stuff);
// OBSOLETE       inferior_registers.r_reg[EAX] = reg;
// OBSOLETE     }
// OBSOLETE #endif
// OBSOLETE   ptrace (PTRACE_SETREGS, PIDGET (inferior_ptid),
// OBSOLETE 	  (PTRACE_ARG3_TYPE) & inferior_registers);
// OBSOLETE   ptrace (PTRACE_SETFPREGS, PIDGET (inferior_ptid),
// OBSOLETE 	  (PTRACE_ARG3_TYPE) & inferior_fp_registers);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Provide registers to GDB from a core file.
// OBSOLETE 
// OBSOLETE    CORE_REG_SECT points to an array of bytes, which were obtained from
// OBSOLETE    a core file which BFD thinks might contain register contents. 
// OBSOLETE    CORE_REG_SIZE is its size.
// OBSOLETE 
// OBSOLETE    WHICH says which register set corelow suspects this is:
// OBSOLETE      0 --- the general-purpose register set
// OBSOLETE      2 --- the floating-point register set
// OBSOLETE 
// OBSOLETE    REG_ADDR isn't used.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE fetch_core_registers (char *core_reg_sect, unsigned core_reg_size,
// OBSOLETE 		      int which, CORE_ADDR reg_addr)
// OBSOLETE {
// OBSOLETE   int val;
// OBSOLETE 
// OBSOLETE   switch (which)
// OBSOLETE     {
// OBSOLETE     case 0:
// OBSOLETE     case 1:
// OBSOLETE       memcpy (registers, core_reg_sect, core_reg_size);
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case 2:
// OBSOLETE       memcpy (&registers[REGISTER_BYTE (FP0_REGNUM)],
// OBSOLETE 	      core_reg_sect,
// OBSOLETE 	      core_reg_size);	/* FIXME, probably bogus */
// OBSOLETE #ifdef FPC_REGNUM
// OBSOLETE       memcpy (&registers[REGISTER_BYTE (FPC_REGNUM)],
// OBSOLETE 	      &corestr.c_fpu.f_fpstatus.f_ctrl,
// OBSOLETE 	      sizeof corestr.c_fpu.f_fpstatus -
// OBSOLETE 	      sizeof corestr.c_fpu.f_fpstatus.f_st);
// OBSOLETE #endif
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Register that we are able to handle i386mach core file formats.
// OBSOLETE    FIXME: is this really bfd_target_unknown_flavour? */
// OBSOLETE 
// OBSOLETE static struct core_fns i386mach_core_fns =
// OBSOLETE {
// OBSOLETE   bfd_target_unknown_flavour,		/* core_flavour */
// OBSOLETE   default_check_format,			/* check_format */
// OBSOLETE   default_core_sniffer,			/* core_sniffer */
// OBSOLETE   fetch_core_registers,			/* core_read_registers */
// OBSOLETE   NULL					/* next */
// OBSOLETE };
// OBSOLETE 
// OBSOLETE void
// OBSOLETE _initialize_core_i386mach (void)
// OBSOLETE {
// OBSOLETE   add_core_fns (&i386mach_core_fns);
// OBSOLETE }
