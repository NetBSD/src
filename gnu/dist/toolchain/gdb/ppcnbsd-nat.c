/* NetBSD/powerpc native-dependent code for GDB, the GNU debugger.
   Copyright 1986, 1987, 1989, 1991, 1992, 1994, 1995, 1996, 1997
	     Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"

#include <sys/param.h>
#include <sys/ptrace.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/pcb.h>

static void
supply_regs (regs)
     char *regs;
{
  int i;

  for (i = 0; i < 32; i++)
    supply_register (GP0_REGNUM + i, regs + (i * 4));

  supply_register (LR_REGNUM,  regs + (32 * 4));
  supply_register (CR_REGNUM,  regs + (33 * 4));
  supply_register (XER_REGNUM, regs + (34 * 4));
  supply_register (CTR_REGNUM, regs + (35 * 4));
  supply_register (PC_REGNUM,  regs + (36 * 4));
  i = 0;
  supply_register (PS_REGNUM,  (char *) &i);
}

static void
unsupply_regs (regs)
     struct reg *regs;
{
  memcpy (regs->fixreg, &registers[REGISTER_BYTE (GP0_REGNUM)],
	  sizeof (regs->fixreg));
  memcpy (&regs->pc, &registers[REGISTER_BYTE (PC_REGNUM)],
	  sizeof (regs->pc));
  memcpy (&regs->cr, &registers[REGISTER_BYTE (CR_REGNUM)],
	  sizeof (regs->cr));
  memcpy (&regs->lr, &registers[REGISTER_BYTE (LR_REGNUM)],
	  sizeof (regs->lr));
  memcpy (&regs->ctr, &registers[REGISTER_BYTE (CTR_REGNUM)],
	  sizeof (regs->ctr));
  memcpy (&regs->xer, &registers[REGISTER_BYTE (XER_REGNUM)],
	  sizeof (regs->xer));

}

static void
supply_fpregs (fregs)
     char *fregs;
{
  int i;

  for (i = 0; i < 32; i++)
    supply_register (FP0_REGNUM + i, fregs + (i * 8));
}

static void
unsupply_fpregs (fregs)
     struct fpreg *fregs;
{
  memset (fregs, 0, sizeof (*fregs));
  memcpy (fregs->fpreg, &registers[REGISTER_BYTE (FP0_REGNUM)], sizeof (fregs->fpreg));
}

void
nbsd_reg_to_internal (regs)
     char *regs;
{
  supply_regs (regs);
}

void
nbsd_fpreg_to_internal (fregs)
     char *fregs;
{
  supply_fpregs (fregs);
}

void
nbsd_internal_to_reg (regs)
     char *regs;
{
  unsupply_regs (regs);
}


void
nbsd_internal_to_fpreg (regs)
     char *regs;
{
  unsupply_fpregs (regs);
}

void
fetch_inferior_registers (regno)
  int regno;
{
  struct reg infreg;
  struct fpreg inffpreg;

  /* Integer registers */
  ptrace (PT_GETREGS, GET_PROCESS (inferior_pid), (PTRACE_ARG3_TYPE) &infreg, 
      GET_LWP (inferior_pid));
  supply_regs ((char *) &infreg);

  /* Floating point registers */
  ptrace (PT_GETFPREGS, GET_PROCESS (inferior_pid), (PTRACE_ARG3_TYPE) &inffpreg,
      GET_LWP (inferior_pid));
  supply_fpregs ((char *) &inffpreg);
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg infreg;
  struct fpreg inffpreg;

  unsupply_regs ((char *)&infreg);
  ptrace(PT_SETREGS, GET_PROCESS (inferior_pid), (PTRACE_ARG3_TYPE) &infreg,
      GET_LWP (inferior_pid));

  /* Floating point registers */
  unsupply_fpregs (&inffpreg);
  ptrace (PT_SETFPREGS, GET_PROCESS (inferior_pid), (PTRACE_ARG3_TYPE) &inffpreg,
      GET_LWP (inferior_pid));
}

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct md_coredump *core_reg = (struct md_coredump *) core_reg_sect;

  /* Integer registers */
  supply_regs ((char *) &core_reg->frame);

  /* Floating point registers */
  supply_fpregs ((char *) &core_reg->fpstate);
}

static void
fetch_elfcore_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  switch (which)
    {
    case 0:  /* Integer registers */
      if (core_reg_size != sizeof (struct reg))
	warning ("Wrong size register set in core file.");
      else
	supply_regs (core_reg_sect);
      break;

    case 2:  /* Floating point registers */
      if (core_reg_size != sizeof (struct fpreg))
	warning ("Wrong size FP register set in core file.");
      else
	supply_fpregs (core_reg_sect);
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}


/* Register that we are able to handle rs6000 core file formats. */

static struct core_fns ppcnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns ppcnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

void
_initialize_core_ppcnbsd ()
{
  add_core_fns (&ppcnbsd_core_fns);
  add_core_fns (&ppcnbsd_elfcore_fns);
}


#ifdef	FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{
  struct switchframe sf;
  int regno;

  /*
   * get the register values out of the sys pcb and
   * store them where `read_register' will find them.
   */
  if (target_read_memory(pcb->pcb_sp, (char *)&sf, sizeof(sf)))
    error("Cannot read switchframe.");
  supply_register(1, (char *)&pcb->pcb_sp);
  supply_register(2, (char *)&sf.fixreg2);
  supply_register(PC_REGNUM, (char *)&sf.lr);
  supply_register(LR_REGNUM, (char *)&sf.lr);
  supply_register(CR_REGNUM, (char *)&sf.cr);
  for (regno = 13; regno < 32; regno++)
    supply_register(regno, (char *)&sf.fixreg[regno - 13]);
}
#endif	/* FETCH_KCORE_REGISTERS */
