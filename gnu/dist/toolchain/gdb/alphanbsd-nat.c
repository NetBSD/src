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
Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

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

static char zerobuf[MAX_REGISTER_RAW_SIZE] = {0};

/* Determine if PT_GETREGS fetches this register. */
#define GETREGS_SUPPLIES(regno) \
  (((regno) >= V0_REGNUM && (regno) <= ZERO_REGNUM) || \
   (regno) >= PC_REGNUM)

static void
supply_regs (regs)
     char *regs;
{
  int i;

  /* Conveniently, GDB's register indices map directly to the NetBSD
     "reg" structure.  */
  for (i = V0_REGNUM; i < ZERO_REGNUM; i++)
    supply_register (i, regs + (i * 8));
  supply_register (ZERO_REGNUM, zerobuf);

  /* The PC rides in the R_ZERO slot of the "reg" structure.  */
  supply_register (PC_REGNUM, regs + (31 * 8));
}

static void
unsupply_regs (regs)
     struct reg *regs;
{
  memcpy (&regs->r_regs[0], &registers[REGISTER_BYTE (V0_REGNUM)], 31 * 8);
  memcpy (&regs->r_regs[31], &registers[REGISTER_BYTE (PC_REGNUM)], 8);
}

static void
supply_fpregs (fregs)
     char *fregs;
{
  int i;

  for (i = FP0_REGNUM; i < FPCR_REGNUM; i++)
    supply_register (i, fregs + ((i - FP0_REGNUM) * 8));

  supply_register (FPCR_REGNUM, fregs + (32 * 8));
}

static void
unsupply_fpregs (fregs)
     struct fpreg *fregs;
{
  memcpy (&fregs->fpr_regs[0], &registers[REGISTER_BYTE (FP0_REGNUM)], 31 * 8);
  memcpy (&fregs->fpr_cr, &registers[REGISTER_BYTE (FPCR_REGNUM)], 8);
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
nbsd_internal_to_fpreg (fregs)
     char *fregs;
{
	unsupply_fpregs (fregs);
}

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  if (regno == -1 || GETREGS_SUPPLIES (regno))
    {
      ptrace (PT_GETREGS, GET_PROCESS (inferior_pid),
              (PTRACE_ARG3_TYPE) &inferior_registers, GET_LWP (inferior_pid));
      supply_regs ((char *) &inferior_registers);
    }

  if (regno == -1 || regno >= FP0_REGNUM)
    {
      ptrace (PT_GETFPREGS, GET_PROCESS (inferior_pid),
              (PTRACE_ARG3_TYPE) &inferior_fp_registers, GET_LWP (inferior_pid));
      supply_fpregs ((char *) &inferior_fp_registers);
    }

  /* Reset virtual frame pointer.  */
  supply_register (FP_REGNUM, NULL);
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  if (regno == -1 || GETREGS_SUPPLIES (regno))
    {
      memcpy (&inferior_registers.r_regs[0],
	      &registers[REGISTER_BYTE (0)],
	      sizeof(inferior_registers.r_regs));

      /* The PC travels in the R_ZERO slot. */
      inferior_registers.r_regs[R_ZERO] =
        *(long *) &registers[REGISTER_BYTE (PC_REGNUM)];    

      ptrace (PT_SETREGS, GET_PROCESS (inferior_pid),
	      (PTRACE_ARG3_TYPE) &inferior_registers, GET_LWP (inferior_pid));
    }

  if (regno == -1 || regno >= FP0_REGNUM)
    {
      memcpy (&inferior_fp_registers.fpr_regs[0],
	      &registers[REGISTER_BYTE (FP0_REGNUM)],
	      sizeof (inferior_fp_registers.fpr_regs));
      memcpy (&inferior_fp_registers.fpr_cr,
	      &registers[REGISTER_BYTE (FPCR_REGNUM)],
	      sizeof (inferior_fp_registers.fpr_cr));
      ptrace (PT_SETFPREGS, GET_PROCESS (inferior_pid),
	      (PTRACE_ARG3_TYPE) &inferior_fp_registers, GET_LWP (inferior_pid));
    }
}

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct md_coredump *core_reg;
  char *regs;
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
  regs = (char *) &core_reg->md_tf;

  if (core_reg_size < sizeof(*core_reg)) {
    fprintf_unfiltered (gdb_stderr, "Couldn't read regs from core file\n");
    return;
  }

  /* Integer registers */
  for (regnum = 0; regnum < ZERO_REGNUM; regnum++)
    supply_register (regnum, regs + (core_reg_mapping[regnum] * 8));
  supply_register (ZERO_REGNUM, zerobuf);

  /* Floating point registers */
  supply_fpregs ((char *) &core_reg->md_fpstate);

  /* Special registers (PC, VFP) */
  supply_register (PC_REGNUM, regs + (FRAME_PC * 8));
  supply_register (FP_REGNUM, zerobuf);
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
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

static struct core_fns alphanbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_elfcore_registers,		/* core_read_registers */
  NULL					/* next */
};

void
_initialize_alphanbsd_nat ()
{
  add_core_fns (&alphanbsd_core_fns);
  add_core_fns (&alphanbsd_elfcore_fns);
}
