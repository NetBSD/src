/* Functions specific to running gdb native on an sh3 running NetBSD
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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

This file was developed by SAITOH Masanobu <msaitoh@netbsd.org>
as a derivation from i386nbsd-nat.c and ns32knbsd-nat.c.  */

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"
#include "target.h"
#include <sys/ptrace.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <string.h>

#define RF(dst, src) \
	memcpy(&registers[REGISTER_BYTE(dst)], &src, sizeof(src))

#define RS(src, dst) \
	memcpy(&dst, &registers[REGISTER_BYTE(src)], sizeof(dst))

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  /* Integer registers */
  RF(R0_REGNUM + 0, inferior_registers.r_r0);
  RF(R0_REGNUM + 1, inferior_registers.r_r1);
  RF(R0_REGNUM + 2, inferior_registers.r_r2);
  RF(R0_REGNUM + 3, inferior_registers.r_r3);
  RF(R0_REGNUM + 4, inferior_registers.r_r4);
  RF(R0_REGNUM + 5, inferior_registers.r_r5);
  RF(R0_REGNUM + 6, inferior_registers.r_r6);
  RF(R0_REGNUM + 7, inferior_registers.r_r7);
  RF(R0_REGNUM + 8, inferior_registers.r_r8);
  RF(R0_REGNUM + 9, inferior_registers.r_r9);
  RF(R0_REGNUM + 10, inferior_registers.r_r10);
  RF(R0_REGNUM + 11, inferior_registers.r_r11);
  RF(R0_REGNUM + 12, inferior_registers.r_r12);
  RF(R0_REGNUM + 13, inferior_registers.r_r13);
  RF(R0_REGNUM + 14, inferior_registers.r_r14);
  RF(R0_REGNUM + 15, inferior_registers.r_r15);
  RF(PR_REGNUM, inferior_registers.r_pr);
  RF(PC_REGNUM, inferior_registers.r_spc);
  RF(SR_REGNUM, inferior_registers.r_ssr);
  RF(MACH_REGNUM, inferior_registers.r_mach);
  RF(MACL_REGNUM, inferior_registers.r_macl);

  /* FIXME: FP regs? */
  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  /* Integer registers */
  RS(R0_REGNUM + 0, inferior_registers.r_r0);
  RS(R0_REGNUM + 1, inferior_registers.r_r1);
  RS(R0_REGNUM + 2, inferior_registers.r_r2);
  RS(R0_REGNUM + 3, inferior_registers.r_r3);
  RS(R0_REGNUM + 4, inferior_registers.r_r4);
  RS(R0_REGNUM + 5, inferior_registers.r_r5);
  RS(R0_REGNUM + 6, inferior_registers.r_r6);
  RS(R0_REGNUM + 7, inferior_registers.r_r7);
  RS(R0_REGNUM + 8, inferior_registers.r_r8);
  RS(R0_REGNUM + 9, inferior_registers.r_r9);
  RS(R0_REGNUM + 10, inferior_registers.r_r10);
  RS(R0_REGNUM + 11, inferior_registers.r_r11);
  RS(R0_REGNUM + 12, inferior_registers.r_r12);
  RS(R0_REGNUM + 13, inferior_registers.r_r13);
  RS(R0_REGNUM + 14, inferior_registers.r_r14);
  RS(R0_REGNUM + 15, inferior_registers.r_r15);
  RS(PR_REGNUM, inferior_registers.r_pr);
  RS(PC_REGNUM, inferior_registers.r_spc);
  RS(SR_REGNUM, inferior_registers.r_ssr);
  RS(MACH_REGNUM, inferior_registers.r_mach);
  RS(MACL_REGNUM, inferior_registers.r_macl);

  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  /* FIXME: FP regs? */
  registers_fetched ();
}


/* XXX - Add this to machine/regs.h instead? */
struct md_core {
  struct reg intreg;
#if 0
  struct fpreg freg;
#endif
};

#if 0
static struct fpreg i386_fp_registers;
static int i386_fp_read = 0;
#endif

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
  RF(R0_REGNUM + 0, core_reg->intreg.r_r0);
  RF(R0_REGNUM + 1, core_reg->intreg.r_r1);
  RF(R0_REGNUM + 2, core_reg->intreg.r_r2);
  RF(R0_REGNUM + 3, core_reg->intreg.r_r3);
  RF(R0_REGNUM + 4, core_reg->intreg.r_r4);
  RF(R0_REGNUM + 5, core_reg->intreg.r_r5);
  RF(R0_REGNUM + 6, core_reg->intreg.r_r6);
  RF(R0_REGNUM + 7, core_reg->intreg.r_r7);
  RF(R0_REGNUM + 8, core_reg->intreg.r_r8);
  RF(R0_REGNUM + 9, core_reg->intreg.r_r9);
  RF(R0_REGNUM + 10, core_reg->intreg.r_r10);
  RF(R0_REGNUM + 11, core_reg->intreg.r_r11);
  RF(R0_REGNUM + 12, core_reg->intreg.r_r12);
  RF(R0_REGNUM + 13, core_reg->intreg.r_r13);
  RF(R0_REGNUM + 14, core_reg->intreg.r_r14);
  RF(R0_REGNUM + 15, core_reg->intreg.r_r15);
  RF(PR_REGNUM, core_reg->intreg.r_pr);
  RF(PC_REGNUM, core_reg->intreg.r_spc);
  RF(SR_REGNUM, core_reg->intreg.r_ssr);
  RF(MACH_REGNUM, core_reg->intreg.r_mach);
  RF(MACL_REGNUM, core_reg->intreg.r_macl);

#if 0
  /* Floating point registers */
  i386_fp_registers = core_reg->freg;
  i386_fp_read = 1;
#endif

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
#if 0
  int i, regno, regs[4];

  /*
   * get the register values out of the sys pcb and
   * store them where `read_register' will find them.
   */
  if (target_read_memory(pcb->pcb_tss.tss_esp+4,
			 (char *)regs, sizeof(regs)))
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
#endif

  /* The kernel does not use the FPU, so ignore it. */
  registers_fetched ();
}
#endif	/* FETCH_KCORE_REGISTERS */
