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
#include "gdbcore.h"

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

static void
unsupply_struct_reg (struct reg *reg)
{
  int i;

  for (i = 0; i < 13; i++)
    read_register_gen (A1_REGNUM + i, (char *)&reg->r[i]);
  read_register_gen (SP_REGNUM, (char *)&reg->r_sp);
  read_register_gen (LR_REGNUM, (char *)&reg->r_lr);
  read_register_gen (PS_REGNUM, (char *)&reg->r_cpsr);
  if ((reg->r_cpsr & PSR_MODE_32))
    read_register_gen (PC_REGNUM, (char *)&reg->r_pc);
  else
    arm_read_26bit_r15 ((char *)&reg->r_pc);
}

static void
supply_struct_fpreg (struct fpreg *freg)
{
  int i;

  for (i = 0; i < 8; i++)
    supply_register (F0_REGNUM + i, (char *)&freg->fpr[i]);
  supply_register (FPS_REGNUM, (char *)&freg->fpr_fpsr);
}

static void
unsupply_struct_fpreg (struct fpreg *freg)
{
  memcpy (&freg->fpr[0], &registers[REGISTER_BYTE (F0_REGNUM)], sizeof (freg->fpr));
  memcpy (&freg->fpr_fpsr, &registers[REGISTER_BYTE (FPS_REGNUM)], sizeof (freg->fpr_fpsr));
}

void
nbsd_reg_to_internal (regs)
     char *regs;
{
  supply_struct_reg ((struct reg *)regs);
}

void
nbsd_fpreg_to_internal (fregs)
     char *fregs;
{
  supply_struct_fpreg ((struct fpreg *)fregs);
}

void
nbsd_internal_to_reg (regs)
     char *regs;
{
  unsupply_struct_reg ((struct reg *)regs);
}

void
nbsd_internal_to_fpreg (fregs)
     char *fregs;
{
  unsupply_struct_fpreg ((struct fpreg *)fregs);
}

void
fetch_inferior_registers (int regno)
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  /* Integer registers */
  ptrace (PT_GETREGS, GET_PROCESS(inferior_pid),
	  (PTRACE_ARG3_TYPE) &inferior_registers, GET_LWP(inferior_pid));
  supply_struct_reg (&inferior_registers);

  /* FPA registers */
  ptrace (PT_GETFPREGS, GET_PROCESS(inferior_pid),
	  (PTRACE_ARG3_TYPE) &inferior_fpregisters, GET_LWP(inferior_pid));
  supply_struct_fpreg(&inferior_fpregisters);
}

void
store_inferior_registers (int regno)
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  unsupply_struct_reg (&inferior_registers);
  ptrace (PT_SETREGS, GET_PROCESS(inferior_pid),
	  (PTRACE_ARG3_TYPE) &inferior_registers, GET_LWP(inferior_pid));

  unsupply_struct_fpreg (&inferior_fpregisters);
  ptrace (PT_SETFPREGS, GET_PROCESS(inferior_pid),
	  (PTRACE_ARG3_TYPE) &inferior_registers, GET_LWP(inferior_pid));
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

static void
fetch_elfcore_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct reg intreg;
  struct fpreg freg;

  switch (which)
    {
    case 0:  /* Integer registers */
      if (core_reg_size != sizeof (intreg))
        warning ("Wrong size register set in core file.");
      else
        {
          memcpy (&intreg, core_reg_sect, sizeof (intreg));
          supply_struct_reg (&intreg);
        }
      break;

    case 2:  /* Floating point registers */
      if (core_reg_size != sizeof (freg))
        warning ("Wrong size FP register set in core file.");
      else
        {
          memcpy (&freg, core_reg_sect, sizeof (freg));
          /* XXX */
        }
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

#else
#error Not FETCH_INFERIOR_REGISTERS 
#endif /* !FETCH_INFERIOR_REGISTERS */

int 
get_longjmp_target (CORE_ADDR *addr)
{
  return 0;
}

/* Register that we are able to handle armnbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns armnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns armnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

void
_initialize_armnbsd_nat ()
{
  add_core_fns (&armnbsd_core_fns);
  add_core_fns (&armnbsd_elfcore_fns);
}
