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

/* Common routine to supply registers from a struct reg.  Used for registers
   from ptrace and from core dumps. */
static void
supply_struct_reg (struct reg *reg)
{
  int i;

  for (i = 0; i < 13; i++)
    supply_register (A1_REGNUM + i, (char *)&reg->r[i]);
  supply_register (SP_REGNUM, (char *)&reg->r_sp);
  supply_register (LR_REGNUM, (char *)&reg->r_lr);
  if (reg->r_cpsr != 0)
    {
      supply_register (PC_REGNUM, (char *)&reg->r_pc);
      supply_register (PS_REGNUM, (char *)&reg->r_cpsr);
    }
  else
    arm_supply_26bit_r15 ((char *)&reg->r_pc);

  arm_apcs_32 = (read_register (PS_REGNUM) & PSR_MODE_32) != 0;
}


void
fetch_inferior_registers (int regno)
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;
  int i;

  /* Integer registers */
  ptrace (PT_GETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  supply_struct_reg (&inferior_registers);

  /* FPA registers */
  ptrace (PT_GETFPREGS, inferior_pid, (PTRACE_ARG3_TYPE) &inferior_fpregisters,
	  0);
  for (i = 0; i < 8; i++)
    supply_register (F0_REGNUM + i, (char *)&inferior_fpregisters.fpr[i]);
  supply_register (FPS_REGNUM, (char *)&inferior_fpregisters.fpr_fpsr);
}

void
store_inferior_registers (int regno)
{
  struct reg inferior_registers;
  int i;

  for (i = 0; i < 13; i++)
    read_register_gen (A1_REGNUM + i, (char *)&inferior_registers.r[i]);
  read_register_gen (SP_REGNUM, (char *)&inferior_registers.r_sp);
  read_register_gen (LR_REGNUM, (char *)&inferior_registers.r_lr);
  if ((inferior_registers.r_cpsr & PSR_MODE_32))
    read_register_gen (PC_REGNUM, (char *)&inferior_registers.r_pc);
  else
    arm_read_26bit_r15 ((char *)&inferior_registers.r_pc);
  read_register_gen (PS_REGNUM, (char *)&inferior_registers.r_cpsr);
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

  supply_struct_reg (&core_reg->intreg);
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
