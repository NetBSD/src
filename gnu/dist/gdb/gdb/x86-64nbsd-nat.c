/* Native-dependent code for NetBSD/amd64
   Derived from the Linux version.

   Copyright 2001, 2002, 2003 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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
#include "regcache.h"
#include "gdb_assert.h"
#include "gdb_string.h"
#include "x86-64-tdep.h"

#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register array layout.  */

static int x86_64_regmap[] = {
  _REG_RAX, _REG_RBX, _REG_RCX, _REG_RDX,
  _REG_RSI, _REG_RDI, _REG_RBP, _REG_URSP,
  _REG_R8, _REG_R9, _REG_R10, _REG_R11,
  _REG_R12, _REG_R13, _REG_R14, _REG_R15,
  _REG_RIP, _REG_RFL, _REG_CS, _REG_SS, 
  _REG_DS, _REG_ES, _REG_FS, _REG_GS
};

/* The register sets used in GNU/Linux ELF core-dumps are identical to
   the register sets used by `ptrace'.  */

#define GETREGS_SUPPLIES(regno) \
  (0 <= (regno) && (regno) < x86_64_num_gregs)
#define GETFPREGS_SUPPLIES(regno) \
  (FP0_REGNUM <= (regno) && (regno) <= MXCSR_REGNUM)


/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct reg * regsetp)
{
  long *regp = (long *) regsetp;
  int i;

  for (i = 0; i < x86_64_num_gregs; i++)
    supply_register (i, (char *) (regp + x86_64_regmap[i]));
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_gregset (struct reg *gregsetp, int regno)
{
  long *regp = (long *) gregsetp;
  int i;

  for (i = 0; i < x86_64_num_gregs; i++)
    if ((regno == -1 || regno == i))
      regcache_collect (i, (char *) (regp + x86_64_regmap[i]));
}

/* Fetch all general-purpose registers from process/thread TID and
   store their values in GDB's register array.  */

static void
fetch_regs (int tid)
{
  struct reg regs;

  if (ptrace (PT_GETREGS, tid, (caddr_t)&regs, 0) < 0)
    perror_with_name ("Couldn't get registers");

  supply_gregset (&regs);
}

/* Store all valid general-purpose registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_regs (int tid, int regno)
{
  struct reg regs;

  if (ptrace (PT_GETREGS, tid, (caddr_t) &regs, 0) < 0)
    perror_with_name ("Couldn't get registers");

  fill_gregset (&regs, regno);

  if (ptrace (PT_SETREGS, tid, (caddr_t) &regs, 0) < 0)
    perror_with_name ("Couldn't write registers");
}


/* Transfering floating-point registers between GDB, inferiors and cores.  */

static void *
x86_64_fxsave_offset (struct fpreg * fp, int regnum)
{
  const char *reg_name;
  int reg_index;

  gdb_assert (x86_64_num_gregs - 1 < regnum && regnum < x86_64_num_regs);

  if (regnum >= FP0_REGNUM && regnum < (FP0_REGNUM + 8))
    return &fp->fp_st[regnum - FP0_REGNUM];

  if (regnum >= XMM0_REGNUM && regnum < (XMM0_REGNUM + 8))
    return &fp->fp_xmm[regnum - XMM0_REGNUM];

  if (regnum == MXCSR_REGNUM)
    return &fp->fp_mxcsr;

  return NULL;
}

/* Fill GDB's register array with the floating-point and SSE register
   values in *FXSAVE.  This function masks off any of the reserved
   bits in *FXSAVE.  */

void
supply_fpregset (struct fpreg * fxsave)
{
  int i;

  for (i = FP0_REGNUM; i <= MXCSR_REGNUM; i++)
    supply_register (i, x86_64_fxsave_offset (fxsave, i));
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FXSAVE with the value in GDB's register array.  If REGNUM is -1, do
   this for all registers.  This function doesn't touch any of the
   reserved bits in *FXSAVE.  */

void
fill_fpregset (struct fpreg * fxsave, int regnum)
{
  int i;
  void *ptr;

  for (i = FP0_REGNUM; i <= MXCSR_REGNUM; i++)
    if (regnum == -1 || regnum == i)
      {
	ptr = x86_64_fxsave_offset (fxsave, i);
	if (ptr)
	  regcache_collect (i, ptr);
      }
}

/* Fetch all floating-point registers from process/thread TID and store
   thier values in GDB's register array.  */

static void
fetch_fpregs (int tid)
{
  struct fpreg fpregs;

  if (ptrace (PT_GETFPREGS, tid, (caddr_t) &fpregs, 0) < 0)
    perror_with_name ("Couldn't get floating point status");

  supply_fpregset (&fpregs);
}

/* Store all valid floating-point registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_fpregs (int tid, int regno)
{
  struct fpreg fpregs;

  if (ptrace (PT_GETFPREGS, tid, (caddr_t) &fpregs, 0) < 0)
    perror_with_name ("Couldn't get floating point status");

  fill_fpregset (&fpregs, regno);

  if (ptrace (PT_SETFPREGS, tid, (caddr_t) &fpregs, 0) < 0)
    perror_with_name ("Couldn't write floating point status");
}


/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNO from the child process.  If REGNO is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

void
fetch_inferior_registers (int regno)
{
  int pid;

  pid = PIDGET (inferior_ptid);

  if (regno == -1)
    {
      fetch_regs (pid);
      fetch_fpregs (pid);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      fetch_regs (pid);
      return;
    }

  if (GETFPREGS_SUPPLIES (regno))
    {
      fetch_fpregs (pid);
      return;
    }

  internal_error (__FILE__, __LINE__,
		  "Got request for bad register number %d.", regno);
}

/* Store register REGNO back into the child process.  If REGNO is -1,
   do this for all registers (including the floating point and SSE
   registers).  */
void
store_inferior_registers (int regno)
{
  int pid;

  pid = PIDGET (inferior_ptid);

  if (regno == -1)
    {
      store_regs (pid, regno);
      store_fpregs (pid, regno);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      store_regs (pid, regno);
      return;
    }

  if (GETFPREGS_SUPPLIES (regno))
    {
      store_fpregs (pid, regno);
      return;
    }

  internal_error (__FILE__, __LINE__,
		  "Got request to store bad register number %d.", regno);
}


static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size,
		      int which, CORE_ADDR reg_addr)
{
  struct reg gregset;
  struct fpreg fpregset;
  switch (which)
    {
    case 0:
      if (core_reg_size != sizeof (gregset))
	warning ("Wrong size gregset in core file.");
      else
	{
	  memcpy (&gregset, core_reg_sect, sizeof (gregset));
	  supply_gregset (&gregset);
	}
      break;

    case 2:
      if (core_reg_size != sizeof (fpregset))
	warning ("Wrong size fpregset in core file.");
      else
	{
	  memcpy (&fpregset, core_reg_sect, sizeof (fpregset));
	  supply_fpregset (&fpregset);
	}
      break;

    default:
      /* We've covered all the kinds of registers we know about here,
         so this must be something we wouldn't know what to do with
         anyway.  Just ignore it.  */
      break;
    }
}

void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{
  int regno;
  uint64_t zero;
  struct switchframe sf;

  /*
   * get the register values out of the sys pcb and
   * store them where `read_register' will find them.
   */
  if (target_read_memory(pcb->pcb_rsp, (char *)&sf, sizeof(sf)))
    error("Cannot switch frame.");

  zero = 0;

  supply_register(0, (char *)&zero);
  supply_register(1, (char *)&sf.sf_rbx);
  supply_register(2, (char *)&zero);
  supply_register(3, (char *)&zero);
  supply_register(4, (char *)&zero);
  supply_register(5, (char *)&zero);
  supply_register(6, (char *)&sf.sf_rbp);
  supply_register(7, (char *)&pcb->pcb_rsp);
  supply_register(8, (char *)&zero);
  supply_register(9, (char *)&zero);
  supply_register(10, (char *)&zero);
  supply_register(11, (char *)&zero);
  supply_register(12, (char *)&sf.sf_r12);
  supply_register(13, (char *)&sf.sf_r13);
  supply_register(14, (char *)&sf.sf_r14);
  supply_register(15, (char *)&sf.sf_r15);
  supply_register(16, (char *)&sf.sf_rip);
  supply_register(17, (char *)&zero);
  supply_register(18, (char *)&zero);
  supply_register(19, (char *)&zero);
  supply_register(20, (char *)&zero);
  supply_register(21, (char *)&zero);
}

void
nbsd_reg_to_internal(regs)
     char *regs;
{
  supply_gregset((struct reg *)regs);
}
void
nbsd_fpreg_to_internal(fregs)
     char *fregs;
{
  supply_fpregset((struct fpreg *)fregs);
}

void
nbsd_internal_to_reg(regs)
     char *regs;
{
  fill_gregset ((struct reg *)regs, -1);
}

void
nbsd_internal_to_fpreg(regs)
     char *regs;
{
  fill_fpregset ((struct fpreg *)regs, -1);
}

/* Register that we are able to handle GNU/Linux ELF core file formats.  */

static struct core_fns nbsd_elf_core_fns = {
  bfd_target_elf_flavour,	/* core_flavour */
  default_check_format,		/* check_format */
  default_core_sniffer,		/* core_sniffer */
  fetch_core_registers,		/* core_read_registers */
  NULL				/* next */
};


void
_initialize_x86_64nbsd_nat (void)
{
  add_core_fns (&nbsd_elf_core_fns);
}
