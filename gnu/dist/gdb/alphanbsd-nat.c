/* Low level Alpha interface, for GDB when running native.
   Copyright 1993, 1995, 1996 Free Software Foundation, Inc.

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

This file was developed by Gordon W. Ross <gwr@netbsd.org>
as a derivation from alpha-nat.c and m68knbsd-nat.c.  */

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

/* Size of elements in jmpbuf */

#define JB_ELEMENT_SIZE 8

/* The definition for JB_PC in machine/reg.h is wrong.
   And we can't get at the correct definition in setjmp.h as it is
   not always available (eg. if _POSIX_SOURCE is defined which is the
   default). As the defintion is unlikely to change (see comment
   in <setjmp.h>, define the correct value here.  */

#undef JB_PC
#define JB_PC 2

/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure from which
   we extract the pc (JB_PC) that we will land at.  The pc is copied into PC.
   This routine returns true on success. */

int
get_longjmp_target (pc)
     CORE_ADDR *pc;
{
  CORE_ADDR jb_addr;
  char raw_buffer[MAX_REGISTER_RAW_SIZE];

  jb_addr = read_register(A0_REGNUM);

  if (target_read_memory(jb_addr + JB_PC * JB_ELEMENT_SIZE, raw_buffer,
			 sizeof(CORE_ADDR)))
    return 0;

  *pc = extract_address (raw_buffer, sizeof(CORE_ADDR));
  return 1;
}

/* Extract the register values out of the core file and store
   them where `read_register' will find them.

   CORE_REG_SECT points to the register values themselves, read into memory.
   CORE_REG_SIZE is the size of that area.
   WHICH says which set of registers we are handling (0 = int, 2 = float
         on machines where they are discontiguous).
   REG_ADDR is the offset from u.u_ar0 to the register values relative to
            core_reg_sect.  This is used with old-fashioned core files to
	    locate the registers in a large upage-plus-stack ".reg" section.
	    Original upage address X is at location core_reg_sect+x+reg_addr.
 */

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, reg_addr)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     unsigned reg_addr;
{
  struct md_coredump *core_reg;
  struct trapframe *tf;
  struct fpreg *fs;
  register int regnum;

  /* Table to map a gdb regnum to an index in the trapframe regs. */
  static int core_reg_mapping[ZERO_REGNUM] = {
    FRAME_V0,  FRAME_T0,  FRAME_T1,  FRAME_T2,
    FRAME_T3,  FRAME_T4,  FRAME_T5,  FRAME_T6,
    FRAME_T7,  FRAME_S0,  FRAME_S1,  FRAME_S2,
    FRAME_S3,  FRAME_S4,  FRAME_S5,  FRAME_S6,
    FRAME_A0,  FRAME_A1,  FRAME_A2,  FRAME_A3,
    FRAME_A4,  FRAME_A5,  FRAME_T8,  FRAME_T9,
    FRAME_T10, FRAME_T11, FRAME_RA,  FRAME_T12,
    FRAME_AT,  FRAME_GP,  FRAME_SP };

  /* We get everything from the .reg section. */
  if (which != 0)
    return;

  core_reg = (struct md_coredump *)core_reg_sect;
  tf = &core_reg->md_tf;
  fs = &core_reg->md_fpstate;

  if (core_reg_size < sizeof(*core_reg)) {
    fprintf_unfiltered (gdb_stderr, "Couldn't read regs from core file\n");
    return;
  }

  /* Integer registers */
  for (regnum = 0; regnum < ZERO_REGNUM; regnum++)
    *(long *) &registers[REGISTER_BYTE (regnum)] = tf->tf_regs[regnum];
  *(long *) &registers[REGISTER_BYTE (ZERO_REGNUM)] = 0;

  /* Floating point registers */
  memcpy (&registers[REGISTER_BYTE (FP0_REGNUM)],
	  &fs->fpr_regs[0], sizeof(fs->fpr_regs));

  /* Special registers (PC, VFP) */
  *(long *) &registers[REGISTER_BYTE (PC_REGNUM)] = tf->tf_regs[FRAME_PC];
  *(long *) &registers[REGISTER_BYTE (FP_REGNUM)] = 0;

  registers_fetched ();
}

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  /* Integer registers */
  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  /* The PC travels in the R_ZERO slot. */
  *(long *) &registers[REGISTER_BYTE (PC_REGNUM)] =
    inferior_registers.r_regs[R_ZERO];
  inferior_registers.r_regs[R_ZERO] = 0;
  memcpy (&registers[REGISTER_BYTE (0)],
	  &inferior_registers.r_regs[0],
	  sizeof(inferior_registers.r_regs));

  /* Floating point registers */
  ptrace (PT_GETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0);
  memcpy (&registers[REGISTER_BYTE (FP0_REGNUM)],
	  &inferior_fp_registers.fpr_regs[0],
	  sizeof(inferior_fp_registers.fpr_regs));

  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  /* Integer registers */
  memcpy (&inferior_registers.r_regs[0],
	  &registers[REGISTER_BYTE (0)],
	  sizeof(inferior_registers.r_regs));
  /* The PC travels in the R_ZERO slot. */
  inferior_registers.r_regs[R_ZERO] =
    *(long *) &registers[REGISTER_BYTE (PC_REGNUM)];    
  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  /* Floating point registers */
  memcpy (&inferior_fp_registers.fpr_regs[0],
	  &registers[REGISTER_BYTE (FP0_REGNUM)],
	  sizeof(inferior_fp_registers.fpr_regs));
  inferior_fp_registers.fpr_cr = 0;
  ptrace (PT_SETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0);

  registers_fetched ();
}


/*
 * kernel_u_size() is not helpful on NetBSD because
 * the "u" struct is NOT in the core dump file.
 */

#ifdef  FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
fetch_kcore_registers (pcbp)
  struct pcb *pcbp;
{

  /* First clear out any garbage. */
  memset(registers, '\0', REGISTER_BYTES);

  /* SP */
  *(long *) &registers[REGISTER_BYTE (SP_REGNUM)] =
    pcbp->pcb_hw.apcb_ksp;

  /* S0 through S6 */
  memcpy (&registers[REGISTER_BYTE (S0_REGNUM)],
	  &pcbp->pcb_context[0], 7 * sizeof(long));

  /* PC */
  *(long *) &registers[REGISTER_BYTE (PC_REGNUM)] =
    pcbp->pcb_context[7];

  registers_fetched ();
}
#endif  /* FETCH_KCORE_REGISTERS */

static struct core_fns alphanbsd_core_fns =
{
  bfd_target_ecoff_flavour,
  fetch_core_registers,
  NULL
};

static struct core_fns alphanbsd_elf_core_fns =
{
  bfd_target_elf_flavour,
  fetch_core_registers,
  NULL
};

void
_initialize_core_alphanbsd ()
{
  add_core_fns (&alphanbsd_core_fns);
  add_core_fns (&alphanbsd_elf_core_fns);
}
