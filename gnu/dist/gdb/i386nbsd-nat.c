/* Functions specific to running gdb native on an i386 running NetBSD
   Copyright 1989, 1992, 1993, 1994, 1996 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  memcpy (&registers[REGISTER_BYTE (0)], &inferior_registers,
	  sizeof(inferior_registers));

  /* FIXME: FP regs? */
  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  memcpy (&inferior_registers, &registers[REGISTER_BYTE (0)],
	  sizeof(inferior_registers));
  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  /* FIXME: FP regs? */
}


/* XXX - Add this to machine/regs.h instead? */
struct md_core {
  struct reg intreg;
  struct fpreg freg;
};

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, reg_addr)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     unsigned int reg_addr;	/* Unused in this version */
{
  struct md_core *core_reg;

  core_reg = (struct md_core *)core_reg_sect;

  /* We get everything from the .reg section. */
  if (which != 0)
    return;

  if (core_reg_size < sizeof(struct reg)) {
    fprintf_unfiltered (gdb_stderr, "Couldn't read regs from core file\n");
    return;
  }

  /* Integer registers */
  memcpy(&registers[REGISTER_BYTE (0)],
	 &core_reg->intreg, sizeof(struct reg));

  /* XXX - floating point registers? */

  registers_fetched ();
}

/* Register that we are able to handle i386nbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns nat_core_fns =
{
  bfd_target_unknown_flavour,
  fetch_core_registers,
  NULL
};

void
_initialize_i386nbsd_nat ()
{
  add_core_fns (&nat_core_fns);
}


/*
 * kernel_u_size() is not helpful on NetBSD because
 * the "u" struct is NOT in the core dump file.
 */

#ifdef	FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{
  int i, regno, regs[4];

  /*
   * get the register values out of the sys pcb and
   * store them where `read_register' will find them.
   */
  if (target_read_memory(pcb->pcb_tss.tss_esp+4, regs, sizeof regs))
    error("Cannot read ebx, esi, and edi.");
  for (i = 0, regno = 0; regno < 3; regno++)
    supply_register(regno, (char *)&i);
  supply_register(3, (char *)&regs[2]);
  supply_register(4, (char *)&pcb->pcb_tss.tss_esp);
  supply_register(5, (char *)&pcb->pcb_tss.tss_ebp);
  supply_register(6, (char *)&regs[1]);
  supply_register(7, (char *)&regs[0]);
  supply_register(8, (char *)&regs[3]);
  for (i = 0, regno = 9; regno < 10; regno++)
    supply_register(regno, (char *)&i);
#if 0
  i = 0x08;
  supply_register(10, (char *)&i);
  i = 0x10;
  supply_register(11, (char *)&i);
#endif

  /* The kernel does not use the FPU, so ignore it. */
  registers_fetched ();
}
#endif	/* FETCH_KCORE_REGISTERS */
