/*
   Copyright (C) 1988-2019 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _KERNTYPES
#define _KERNTYPES
#endif
#include "defs.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include "nbsd-nat.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <arm/arm32/frame.h>

/* Support for debugging kernel virtual memory images.  */
#include <machine/pcb.h>

#include "arm-tdep.h"
#include "inf-ptrace.h"
#include "bsd-kvm.h"

class arm_nbsd_nat_target final : public nbsd_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static arm_nbsd_nat_target the_arm_nbsd_nat_target;

/* Determine if PT_GETREGS fetches REGNUM.  */

static bool
getregs_supplies (int regnum)
{
  return ((regnum >= ARM_A1_REGNUM && regnum <= ARM_PC_REGNUM)
	  || regnum == ARM_PS_REGNUM);
}

/* Determine if PT_GETFPREGS fetches REGNUM.  */

static bool
getfpregs_supplies (int regnum)
{
  return ((regnum >= ARM_D0_REGNUM && regnum <= ARM_D31_REGNUM)
	  || regnum == ARM_FPSCR_REGNUM);
}

extern int arm_apcs_32;

#define FPSCR(r) ((char *) &(r)->fpr_vfp.vfp_fpscr)
#define FPREG(r, regno) \
    ((char *) (r)->fpr_vfp.vfp_regs + 8 * ((regno) - ARM_D0_REGNUM))

static int
armnbsd_supply_pcb (struct regcache *regcache, struct pcb *pcb)
{
  struct switchframe sf;

  /* The following is true for NetBSD/arm32 in 5.0 and after:

     The pcb contains r8-r13 (sp) at the point of context switch in
     cpu_switchto() or call of dumpsys(). At that point we have a
     stack frame as described by `struct switchframe', which for
     NetBSD/arm32 has the following layout:

	r4   ascending.
	r5        |
	r6        |
	r7       \|/
	old sp
	pc

     we reconstruct the register state as it would look when we just
     returned from cpu_switchto() or dumpsys().  */

  if (!arm_apcs_32)
    return 0;

  /* The stack pointer shouldn't be zero.  */
  if (pcb->pcb_sp == 0)
    return 0;

  read_memory (pcb->pcb_sp, (gdb_byte *) &sf, sizeof sf);

  regcache->raw_supply (ARM_PC_REGNUM, &sf.sf_pc);
  regcache->raw_supply (ARM_SP_REGNUM, &pcb->pcb_sp);
  regcache->raw_supply (12, &pcb->pcb_r12);
  regcache->raw_supply (11, &pcb->pcb_r11);
  regcache->raw_supply (10, &pcb->pcb_r10);
  regcache->raw_supply (9, &pcb->pcb_r9);
  regcache->raw_supply (8, &pcb->pcb_r8);
  regcache->raw_supply (7, &sf.sf_r7);
  regcache->raw_supply (6, &sf.sf_r6);
  regcache->raw_supply (5, &sf.sf_r5);
  regcache->raw_supply (4, &sf.sf_r4);

  return 1;
}

static void
arm_supply_gregset (struct regcache *regcache, struct reg *gregset)
{
  int regno;
  CORE_ADDR r_pc;

  /* Integer registers.  */
  for (regno = ARM_A1_REGNUM; regno < ARM_SP_REGNUM; regno++)
    regcache->raw_supply (regno, (char *) &gregset->r[regno]);

  regcache->raw_supply (ARM_SP_REGNUM, (char *) &gregset->r_sp);
  regcache->raw_supply (ARM_LR_REGNUM, (char *) &gregset->r_lr);
  /* This is ok: we're running native...  */
  r_pc = gdbarch_addr_bits_remove (regcache->arch (), gregset->r_pc);
  regcache->raw_supply (ARM_PC_REGNUM, (char *) &r_pc);

  if (arm_apcs_32)
    regcache->raw_supply (ARM_PS_REGNUM, (char *) &gregset->r_cpsr);
  else
    regcache->raw_supply (ARM_PS_REGNUM, (char *) &gregset->r_pc);
}

static void
arm_supply_vfpregset (struct regcache *regcache, struct fpreg *vfpregset)
{
  int regno;

  for (regno = ARM_D0_REGNUM; regno < 16 + ARM_D0_REGNUM; regno++)
    regcache->raw_supply (regno, FPREG(vfpregset, regno));

  regcache->raw_supply (ARM_FPSCR_REGNUM, FPSCR(vfpregset));
}

static void
fetch_register (struct regcache *regcache, int regno)
{
  struct reg regs;
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  int ret;

  ret = ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp);

  if (ret < 0)
    {
      warning (_("unable to fetch general register"));
      return;
    }

  switch (regno)
    {
    case ARM_SP_REGNUM:
      regcache->raw_supply (ARM_SP_REGNUM, (char *) &regs.r_sp);
      break;

    case ARM_LR_REGNUM:
      regcache->raw_supply (ARM_LR_REGNUM, (char *) &regs.r_lr);
      break;

    case ARM_PC_REGNUM:
      /* This is ok: we're running native...  */
      regs.r_pc = gdbarch_addr_bits_remove (regcache->arch (), regs.r_pc);
      regcache->raw_supply (ARM_PC_REGNUM, (char *) &regs.r_pc);
      break;

    case ARM_PS_REGNUM:
      if (arm_apcs_32)
	regcache->raw_supply (ARM_PS_REGNUM, (char *) &regs.r_cpsr);
      else
	regcache->raw_supply (ARM_PS_REGNUM, (char *) &regs.r_pc);
      break;

    default:
      regcache->raw_supply (regno, (char *) &regs.r[regno]);
      break;
    }
}

static void
fetch_regs (struct regcache *regcache)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  struct reg regs;
  int ret;
  int regno;

  ret = ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp);

  if (ret < 0)
    {
      warning (_("unable to fetch general registers"));
      return;
    }

  arm_supply_gregset (regcache, &regs);
}

static void
fetch_fp_register (struct regcache *regcache, int regno)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  struct fpreg fpregs;
  int ret;

  ret = ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp); 

  if (ret < 0)
    {
      warning (_("unable to fetch floating-point register"));
      return;
    }

  switch (regno)
    {
    case ARM_FPSCR_REGNUM:
      regcache->raw_supply (ARM_FPSCR_REGNUM, FPSCR(&fpregs));
      break;

    default:
      regno += ARM_D0_REGNUM;
      regcache->raw_supply (regno, FPREG(&fpregs, regno));
      break;
    }
}

static void
fetch_fp_regs (struct regcache *regcache)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  struct fpreg fpregs;
  int ret;

  ret = ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp);

  if (ret < 0)
    {
      warning (_("unable to fetch floating-point registers"));
      return;
    }

  arm_supply_vfpregset (regcache, &fpregs);
}

void
arm_nbsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  if (regno >= 0)
    {
      if (getregs_supplies (regno))
	fetch_register (regcache, regno);
      else if (getfpregs_supplies (regno))
	fetch_fp_register (regcache, regno);
      else
        warning (_("unable to fetch register %d"), regno);
    }
  else
    {
      fetch_regs (regcache);
      fetch_fp_regs (regcache);
    }
}


static void
store_register (const struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct reg regs;
  int ret;
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();

  ret = ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp); 

  if (ret < 0)
    {
      warning (_("unable to fetch general registers"));
      return;
    }

  switch (regno)
    {
    case ARM_SP_REGNUM:
      regcache->raw_collect (ARM_SP_REGNUM, (char *) &regs.r_sp);
      break;

    case ARM_LR_REGNUM:
      regcache->raw_collect (ARM_LR_REGNUM, (char *) &regs.r_lr);
      break;

    case ARM_PC_REGNUM:
      if (arm_apcs_32)
	regcache->raw_collect (ARM_PC_REGNUM, (char *) &regs.r_pc);
      else
	{
	  unsigned pc_val;

	  regcache->raw_collect (ARM_PC_REGNUM, (char *) &pc_val);
	  
	  pc_val = gdbarch_addr_bits_remove (gdbarch, pc_val);
	  regs.r_pc ^= gdbarch_addr_bits_remove (gdbarch, regs.r_pc);
	  regs.r_pc |= pc_val;
	}
      break;

    case ARM_PS_REGNUM:
      if (arm_apcs_32)
	regcache->raw_collect (ARM_PS_REGNUM, (char *) &regs.r_cpsr);
      else
	{
	  unsigned psr_val;

	  regcache->raw_collect (ARM_PS_REGNUM, (char *) &psr_val);

	  psr_val ^= gdbarch_addr_bits_remove (gdbarch, psr_val);
	  regs.r_pc = gdbarch_addr_bits_remove (gdbarch, regs.r_pc);
	  regs.r_pc |= psr_val;
	}
      break;

    default:
      regcache->raw_collect (regno, (char *) &regs.r[regno]);
      break;
    }

  ret = ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp);

  if (ret < 0)
    warning (_("unable to write register %d to inferior"), regno);
}

static void
store_regs (const struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  struct reg regs;
  int ret;
  int regno;


  for (regno = ARM_A1_REGNUM; regno < ARM_SP_REGNUM; regno++)
    regcache->raw_collect (regno, (char *) &regs.r[regno]);

  regcache->raw_collect (ARM_SP_REGNUM, (char *) &regs.r_sp);
  regcache->raw_collect (ARM_LR_REGNUM, (char *) &regs.r_lr);

  if (arm_apcs_32)
    {
      regcache->raw_collect (ARM_PC_REGNUM, (char *) &regs.r_pc);
      regcache->raw_collect (ARM_PS_REGNUM, (char *) &regs.r_cpsr);
    }
  else
    {
      unsigned pc_val;
      unsigned psr_val;

      regcache->raw_collect (ARM_PC_REGNUM, (char *) &pc_val);
      regcache->raw_collect (ARM_PS_REGNUM, (char *) &psr_val);
	  
      pc_val = gdbarch_addr_bits_remove (gdbarch, pc_val);
      psr_val ^= gdbarch_addr_bits_remove (gdbarch, psr_val);

      regs.r_pc = pc_val | psr_val;
    }

  ret = ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, lwp); 

  if (ret < 0)
    warning (_("unable to store general registers"));
}

static void
store_fp_register (const struct regcache *regcache, int regno)
{
  struct fpreg fpregs;
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  int ret;

  ret = ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp); 

  if (ret < 0)
    {
      warning (_("unable to fetch floating-point registers"));
      return;
    }

  switch (regno)
    {
    case ARM_FPS_REGNUM:
      regcache->raw_collect (ARM_FPS_REGNUM, FPSCR(&fpregs));
      break;

    default:
      regcache->raw_collect (regno, FPREG(&fpregs, regno));
      break;
    }

  ret = ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp); 

  if (ret < 0)
    warning (_("unable to write register %d to inferior"), regno);
}

static void
store_fp_regs (const struct regcache *regcache)
{
  ptid_t ptid = regcache->ptid ();
  pid_t pid = ptid.pid ();
  int lwp = ptid.lwp ();
  struct fpreg fpregs;
  int ret;
  int regno;


  for (regno = ARM_D0_REGNUM; regno < 16 + ARM_D0_REGNUM; regno++)
    regcache->raw_collect (regno, FPREG(&fpregs, regno));

  regcache->raw_collect (ARM_FPSCR_REGNUM, FPSCR(&fpregs));

  ret = ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, lwp);

  if (ret < 0)
    warning (_("unable to store floating-point registers"));
}

void
arm_nbsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  if (regno >= 0)
    {
      if (getregs_supplies (regno))
	store_register (regcache, regno);
      else if (getfpregs_supplies (regno))
	store_fp_register (regcache, regno);
      else
        warning (_("unable to store register %d"), regno);
    }
  else
    {
      store_regs (regcache);
      store_fp_regs (regcache);
    }
}

struct md_core
{
  struct reg intreg;
  struct fpreg freg;
};

static void
fetch_core_registers (struct regcache *regcache,
		      char *core_reg_sect, unsigned core_reg_size,
		      int which, CORE_ADDR ignore)
{
  struct md_core *core_reg = (struct md_core *) core_reg_sect;
  int regno;
  CORE_ADDR r_pc;

  arm_supply_gregset (regcache, &core_reg->intreg);
  arm_supply_vfpregset (regcache, &core_reg->freg);
}

static void
fetch_elfcore_registers (struct regcache *regcache,
			 char *core_reg_sect, unsigned core_reg_size,
			 int which, CORE_ADDR ignore)
{
  struct reg gregset;
  struct fpreg vfpregset;

  switch (which)
    {
    case 0:	/* Integer registers.  */
      if (core_reg_size != sizeof (struct reg))
	warning (_("wrong size of register set in core file"));
      else
	{
	  /* The memcpy may be unnecessary, but we can't really be sure
	     of the alignment of the data in the core file.  */
	  memcpy (&gregset, core_reg_sect, sizeof (gregset));
	  arm_supply_gregset (regcache, &gregset);
	}
      break;

    case 2:
      if (core_reg_size != sizeof (struct fpreg))
	warning (_("wrong size of FPA register set in core file"));
      else
	{
	  /* The memcpy may be unnecessary, but we can't really be sure
	     of the alignment of the data in the core file.  */
	  memcpy (&vfpregset, core_reg_sect, sizeof (vfpregset));
	  arm_supply_vfpregset (regcache, &vfpregset);
	}
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

static struct core_fns arm_netbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour.  */
  default_check_format,			/* check_format.  */
  default_core_sniffer,			/* core_sniffer.  */
  fetch_core_registers,			/* core_read_registers.  */
  NULL
};

static struct core_fns arm_netbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour.  */
  default_check_format,			/* check_format.  */
  default_core_sniffer,			/* core_sniffer.  */
  fetch_elfcore_registers,		/* core_read_registers.  */
  NULL
};

void
_initialize_arm_netbsd_nat (void)
{
  add_inf_child_target (&the_arm_nbsd_nat_target);

  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (armnbsd_supply_pcb);

  deprecated_add_core_fns (&arm_netbsd_core_fns);
  deprecated_add_core_fns (&arm_netbsd_elfcore_fns);
}
