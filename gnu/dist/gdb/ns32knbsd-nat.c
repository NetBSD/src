/* Functions specific to running gdb native on an ns32k running NetBSD
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

#define RF(dst, src) \
	memcpy(&registers[REGISTER_BYTE(dst)], &src, sizeof(src))

#define RS(src, dst) \
	memcpy(&dst, &registers[REGISTER_BYTE(src)], sizeof(dst))

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  ptrace (PT_GETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);

  RF(R0_REGNUM + 0, inferior_registers.r_r0);
  RF(R0_REGNUM + 1, inferior_registers.r_r1);
  RF(R0_REGNUM + 2, inferior_registers.r_r2);
  RF(R0_REGNUM + 3, inferior_registers.r_r3);
  RF(R0_REGNUM + 4, inferior_registers.r_r4);
  RF(R0_REGNUM + 5, inferior_registers.r_r5);
  RF(R0_REGNUM + 6, inferior_registers.r_r6);
  RF(R0_REGNUM + 7, inferior_registers.r_r7);

  RF(SP_REGNUM	  , inferior_registers.r_sp);
  RF(FP_REGNUM	  , inferior_registers.r_fp);
  RF(PC_REGNUM	  , inferior_registers.r_pc);
  RF(PS_REGNUM	  , inferior_registers.r_psr);

  RF(FPS_REGNUM   , inferior_fpregisters.r_fsr);
  RF(FP0_REGNUM +0, inferior_fpregisters.r_freg[0]);
  RF(FP0_REGNUM +2, inferior_fpregisters.r_freg[2]);
  RF(FP0_REGNUM +4, inferior_fpregisters.r_freg[4]);
  RF(FP0_REGNUM +6, inferior_fpregisters.r_freg[6]);
  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fpregisters;

  RS(R0_REGNUM + 0, inferior_registers.r_r0);
  RS(R0_REGNUM + 1, inferior_registers.r_r1);
  RS(R0_REGNUM + 2, inferior_registers.r_r2);
  RS(R0_REGNUM + 3, inferior_registers.r_r3);
  RS(R0_REGNUM + 4, inferior_registers.r_r4);
  RS(R0_REGNUM + 5, inferior_registers.r_r5);
  RS(R0_REGNUM + 6, inferior_registers.r_r6);
  RS(R0_REGNUM + 7, inferior_registers.r_r7);

  RS(SP_REGNUM	  , inferior_registers.r_sp);
  RS(FP_REGNUM	  , inferior_registers.r_fp);
  RS(PC_REGNUM	  , inferior_registers.r_pc);
  RS(PS_REGNUM	  , inferior_registers.r_psr);

  RS(FPS_REGNUM   , inferior_fpregisters.r_fsr);
  RS(FP0_REGNUM +0, inferior_fpregisters.r_freg[0]);
  RS(FP0_REGNUM +2, inferior_fpregisters.r_freg[2]);
  RS(FP0_REGNUM +4, inferior_fpregisters.r_freg[4]);
  RS(FP0_REGNUM +6, inferior_fpregisters.r_freg[6]);

  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  ptrace (PT_SETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);
}


/* XXX - Add this to machine/regs.h instead? */
struct coreregs {
  struct reg intreg;
  struct fpreg freg;
};

/* Get registers from a core file. */
static void
fetch_core_registers (core_reg_sect, core_reg_size, which, reg_addr)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     unsigned int reg_addr;	/* Unused in this version */
{
  struct coreregs *core_reg;

  core_reg = (struct coreregs *)core_reg_sect;

  /*
   * We have *all* registers in the first core section.
   */
  if (which != 0)
	  return;

  if (core_reg_size < sizeof(*core_reg)) {
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

  RF(SP_REGNUM	  , core_reg->intreg.r_sp);
  RF(FP_REGNUM	  , core_reg->intreg.r_fp);
  RF(PC_REGNUM	  , core_reg->intreg.r_pc);
  RF(PS_REGNUM	  , core_reg->intreg.r_psr);

  /* Floating point registers */
  RF(FPS_REGNUM   , core_reg->freg.r_fsr);
  RF(FP0_REGNUM +0, core_reg->freg.r_freg[0]);
  RF(FP0_REGNUM +2, core_reg->freg.r_freg[2]);
  RF(FP0_REGNUM +4, core_reg->freg.r_freg[4]);
  RF(FP0_REGNUM +6, core_reg->freg.r_freg[6]);
  registers_fetched ();
}

/* Register that we are able to handle ns32knbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns nat_core_fns =
{
  bfd_target_unknown_flavour,
  fetch_core_registers,
  NULL
};

void
_initialize_ns32knbsd_nat ()
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
  struct switchframe sf;
  struct reg intreg;
  int dummy;

  /* Integer registers */
  if (target_read_memory(pcb->pcb_ksp, &sf, sizeof sf))
     error("Cannot read integer registers.");

  /* We use the psr at kernel entry */
  if (target_read_memory(pcb->pcb_onstack, &intreg, sizeof intreg))
     error("Cannot read processor status register.");

  dummy = 0;
  RF(R0_REGNUM + 0, dummy);
  RF(R0_REGNUM + 1, dummy);
  RF(R0_REGNUM + 2, dummy);
  RF(R0_REGNUM + 3, sf.sf_r3);
  RF(R0_REGNUM + 4, sf.sf_r4);
  RF(R0_REGNUM + 5, sf.sf_r5);
  RF(R0_REGNUM + 6, sf.sf_r6);
  RF(R0_REGNUM + 7, sf.sf_r7);

  dummy = pcb->pcb_kfp + 8;
  RF(SP_REGNUM	  , dummy);
  RF(FP_REGNUM	  , sf.sf_fp);
  RF(PC_REGNUM	  , sf.sf_pc);
  RF(PS_REGNUM	  , intreg.r_psr);

  /* The kernel does not use the FPU, so ignore it. */
  registers_fetched ();
}
#endif	/* FETCH_KCORE_REGISTERS */

