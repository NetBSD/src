/* Native-dependent code for BSD Unix running on ARM's, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 1999 Free Software Foundation, Inc.

   This file is part of GDB.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"

#ifdef FETCH_INFERIOR_REGISTERS
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include "inferior.h"

#define R15_PSR		0xfc000003
#define R15_PC		0x03fffffc

#define PSR_MODE_32	0x00000010

/* From arm-tdep.c */
extern int arm_apcs_32;

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  ptrace (PT_GETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  if (inferior_registers.r_cpsr == 0)
    {
      /* 26-bit target: split PC and PSR out of R15.  */
      inferior_registers.r_cpsr = inferior_registers.r_pc & R15_PSR;
      inferior_registers.r_pc = inferior_registers.r_pc & R15_PC;
    }
  arm_apcs_32 = (inferior_registers.r_cpsr & PSR_MODE_32) != 0;
  memcpy (&registers[REGISTER_BYTE (0)], &inferior_registers,
	  16 * sizeof (unsigned int));
  memcpy (&registers[REGISTER_BYTE (PS_REGNUM)], &inferior_registers.r_cpsr,
	  sizeof (unsigned int));
  ptrace (PT_GETFPREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_fpregisters,
	  0);
  memcpy (&registers[REGISTER_BYTE (F0_REGNUM)], &inferior_fpregisters.fpr[0],
	  8 * sizeof (fp_reg_t));
  memcpy (&registers[REGISTER_BYTE (FPS_REGNUM)],
	  &inferior_fpregisters.fpr_fpsr, sizeof (unsigned int));
  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  memcpy (&inferior_registers, &registers[REGISTER_BYTE (0)],
	  16 * sizeof (unsigned int));
  memcpy (&inferior_registers.r_cpsr, &registers[REGISTER_BYTE (PS_REGNUM)],
	  sizeof (unsigned int));
  if ((inferior_registers.r_cpsr & PSR_MODE_32) == 0)
    {
      /* Target is in 26-bit mode.  Merge PSR into R15.  */
      inferior_registers.r_pc &= R15_PC;
      inferior_registers.r_pc |= inferior_registers.r_cpsr & R15_PSR;
    }
  ptrace (PT_SETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  /* XXX Set FP regs. */
}

struct md_core
{
  struct reg intreg;
  struct fpreg freg;
};

void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct md_core *core_reg = (struct md_core *) core_reg_sect;

  if (core_reg->intreg.r_cpsr == 0)
    {
      /* 26-bit target: split PC and PSR out of R15.  */
      core_reg->intreg.r_cpsr = core_reg->intreg.r_pc & R15_PSR;
      core_reg->intreg.r_pc = core_reg->intreg.r_pc & R15_PC;
    }
  arm_apcs_32 = (core_reg->intreg.r_cpsr & PSR_MODE_32) != 0;
  /* integer registers */
  memcpy (&registers[REGISTER_BYTE (0)], &core_reg->intreg,
	  sizeof (struct reg));
  /* floating point registers */
  /* XXX */
}

#else
#error Not FETCH_INFERIOR_REGISTERS 
#endif /* !FETCH_INFERIOR_REGISTERS */

int 
get_longjmp_target (CORE_ADDR *addr)
{
  return 0;
}
