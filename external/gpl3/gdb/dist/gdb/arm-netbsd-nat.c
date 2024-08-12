/* Native-dependent code for BSD Unix running on ARM's, for GDB.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

/* We define this to get types like register_t.  */
#define _KERNTYPES
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "target.h"
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/sysctl.h>
#include <machine/reg.h>
#include <machine/pcb.h>
#include <machine/frame.h>
#include <arm/arm32/frame.h>

#include "arm-tdep.h"
#include "arm-netbsd-tdep.h"
#include "aarch32-tdep.h"
#include "inf-ptrace.h"
#include "netbsd-nat.h"
#include "bsd-kvm.h"

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

class arm_netbsd_nat_target final : public nbsd_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
  const struct target_desc *read_description () override;
};

static arm_netbsd_nat_target the_arm_netbsd_nat_target;

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
arm_supply_vfpregset (struct regcache *regcache, struct fpreg *fpregset)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (regcache->arch ());
  if (tdep->vfp_register_count == 0)
    return;

  struct vfpreg &vfp = fpregset->fpr_vfp;
  for (int regno = 0; regno <= tdep->vfp_register_count; regno++)
    regcache->raw_supply (regno + ARM_D0_REGNUM, (char *) &vfp.vfp_regs[regno]);

  regcache->raw_supply (ARM_FPSCR_REGNUM, (char *) &vfp.vfp_fpscr);
}

static void
fetch_register (struct regcache *regcache, int regno)
{
  struct reg inferior_registers;
  int ret;
  int lwp = regcache->ptid ().lwp ();

  ret = ptrace (PT_GETREGS, regcache->ptid ().pid (),
		(PTRACE_TYPE_ARG3) &inferior_registers, lwp);

  if (ret < 0)
    {
      warning (_("unable to fetch general register"));
      return;
    }
  arm_nbsd_supply_gregset (nullptr, regcache, regno, &inferior_registers,
			   sizeof (inferior_registers));
}

static void
fetch_fp_register (struct regcache *regcache, int regno)
{
  struct fpreg inferior_fp_registers;
  int lwp = regcache->ptid ().lwp ();

  int ret = ptrace (PT_GETFPREGS, regcache->ptid ().pid (),
		    (PTRACE_TYPE_ARG3) &inferior_fp_registers, lwp);

  struct vfpreg &vfp = inferior_fp_registers.fpr_vfp;

  if (ret < 0)
    {
      warning (_("unable to fetch floating-point register"));
      return;
    }

  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (regcache->arch ());
  if (regno == ARM_FPSCR_REGNUM && tdep->vfp_register_count != 0)
    regcache->raw_supply (ARM_FPSCR_REGNUM, (char *) &vfp.vfp_fpscr);
  else if (regno >= ARM_D0_REGNUM
	   && regno <= ARM_D0_REGNUM + tdep->vfp_register_count)
    {
      regcache->raw_supply (regno,
			    (char *) &vfp.vfp_regs[regno - ARM_D0_REGNUM]);
    }
  else
    warning (_("Invalid register number."));
}

static void
fetch_fp_regs (struct regcache *regcache)
{
  struct fpreg inferior_fp_registers;
  int lwp = regcache->ptid ().lwp ();
  int ret;
  int regno;

  ret = ptrace (PT_GETFPREGS, regcache->ptid ().pid (),
		(PTRACE_TYPE_ARG3) &inferior_fp_registers, lwp);

  if (ret < 0)
    {
      warning (_("unable to fetch general registers"));
      return;
    }

  arm_supply_vfpregset (regcache, &inferior_fp_registers);
}

void
arm_netbsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  if (regno >= 0)
    {
      /* Handle discontinuous range for general purpose registers.
	 For floating-point registers, it happens to be continuous,
	 but use specific static function for clarity.  */
      if (getregs_supplies (regno))
	fetch_register (regcache, regno);
      else if (getfpregs_supplies (regno))
	fetch_fp_register (regcache, regno);
      else
	warning (_("unable to fetch register %d"), regno);
    }
  else
    {
      fetch_register (regcache, -1);
      fetch_fp_regs (regcache);
    }
}


static void
store_register (const struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct reg inferior_registers;
  int lwp = regcache->ptid ().lwp ();
  int ret;

  ret = ptrace (PT_GETREGS, regcache->ptid ().pid (),
		(PTRACE_TYPE_ARG3) &inferior_registers, lwp);

  if (ret < 0)
    {
      warning (_("unable to fetch general registers"));
      return;
    }

  switch (regno)
    {
    case ARM_SP_REGNUM:
      regcache->raw_collect (ARM_SP_REGNUM, (char *) &inferior_registers.r_sp);
      break;

    case ARM_LR_REGNUM:
      regcache->raw_collect (ARM_LR_REGNUM, (char *) &inferior_registers.r_lr);
      break;

    case ARM_PC_REGNUM:
      if (arm_apcs_32)
	regcache->raw_collect (ARM_PC_REGNUM,
			       (char *) &inferior_registers.r_pc);
      else
	{
	  unsigned pc_val;

	  regcache->raw_collect (ARM_PC_REGNUM, (char *) &pc_val);
	  
	  pc_val = gdbarch_addr_bits_remove (gdbarch, pc_val);
	  inferior_registers.r_pc ^= gdbarch_addr_bits_remove
				       (gdbarch, inferior_registers.r_pc);
	  inferior_registers.r_pc |= pc_val;
	}
      break;

    case ARM_PS_REGNUM:
      if (arm_apcs_32)
	regcache->raw_collect (ARM_PS_REGNUM,
			       (char *) &inferior_registers.r_cpsr);
      else
	{
	  unsigned psr_val;

	  regcache->raw_collect (ARM_PS_REGNUM, (char *) &psr_val);

	  psr_val ^= gdbarch_addr_bits_remove (gdbarch, psr_val);
	  inferior_registers.r_pc = gdbarch_addr_bits_remove
				      (gdbarch, inferior_registers.r_pc);
	  inferior_registers.r_pc |= psr_val;
	}
      break;

    default:
      regcache->raw_collect (regno, (char *) &inferior_registers.r[regno]);
      break;
    }

  ret = ptrace (PT_SETREGS, regcache->ptid ().pid (),
		(PTRACE_TYPE_ARG3) &inferior_registers, lwp);

  if (ret < 0)
    warning (_("unable to write register %d to inferior"), regno);
}

static void
store_regs (const struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct reg inferior_registers;
  int lwp = regcache->ptid ().lwp ();
  int ret;
  int regno;


  for (regno = ARM_A1_REGNUM; regno < ARM_SP_REGNUM; regno++)
    regcache->raw_collect (regno, (char *) &inferior_registers.r[regno]);

  regcache->raw_collect (ARM_SP_REGNUM, (char *) &inferior_registers.r_sp);
  regcache->raw_collect (ARM_LR_REGNUM, (char *) &inferior_registers.r_lr);

  if (arm_apcs_32)
    {
      regcache->raw_collect (ARM_PC_REGNUM, (char *) &inferior_registers.r_pc);
      regcache->raw_collect (ARM_PS_REGNUM,
			     (char *) &inferior_registers.r_cpsr);
    }
  else
    {
      unsigned pc_val;
      unsigned psr_val;

      regcache->raw_collect (ARM_PC_REGNUM, (char *) &pc_val);
      regcache->raw_collect (ARM_PS_REGNUM, (char *) &psr_val);
	  
      pc_val = gdbarch_addr_bits_remove (gdbarch, pc_val);
      psr_val ^= gdbarch_addr_bits_remove (gdbarch, psr_val);

      inferior_registers.r_pc = pc_val | psr_val;
    }

  ret = ptrace (PT_SETREGS, regcache->ptid ().pid (),
		(PTRACE_TYPE_ARG3) &inferior_registers, lwp);

  if (ret < 0)
    warning (_("unable to store general registers"));
}

static void
store_fp_register (const struct regcache *regcache, int regno)
{
  struct fpreg inferior_fp_registers;
  int lwp = regcache->ptid ().lwp ();
  int ret = ptrace (PT_GETFPREGS, regcache->ptid ().pid (),
		    (PTRACE_TYPE_ARG3) &inferior_fp_registers, lwp);
  struct vfpreg &vfp = inferior_fp_registers.fpr_vfp;

  if (ret < 0)
    {
      warning (_("unable to fetch floating-point registers"));
      return;
    }

  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (regcache->arch ());
  if (regno == ARM_FPSCR_REGNUM && tdep->vfp_register_count != 0)
    regcache->raw_collect (ARM_FPSCR_REGNUM, (char *) &vfp.vfp_fpscr);
  else if (regno >= ARM_D0_REGNUM
	   && regno <= ARM_D0_REGNUM + tdep->vfp_register_count)
    {
      regcache->raw_collect (regno,
			     (char *) &vfp.vfp_regs[regno - ARM_D0_REGNUM]);
    }
  else
    warning (_("Invalid register number."));

  ret = ptrace (PT_SETFPREGS, regcache->ptid ().pid (),
		(PTRACE_TYPE_ARG3) &inferior_fp_registers, lwp);

  if (ret < 0)
    warning (_("unable to write register %d to inferior"), regno);
}

static void
store_fp_regs (const struct regcache *regcache)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (regcache->arch ());
  int lwp = regcache->ptid ().lwp ();
  if (tdep->vfp_register_count == 0)
    return;

  struct fpreg fpregs;
  for (int regno = 0; regno <= tdep->vfp_register_count; regno++)
    regcache->raw_collect
      (regno + ARM_D0_REGNUM, (char *) &fpregs.fpr_vfp.vfp_regs[regno]);

  regcache->raw_collect (ARM_FPSCR_REGNUM,
			 (char *) &fpregs.fpr_vfp.vfp_fpscr);

  int ret = ptrace (PT_SETFPREGS, regcache->ptid ().pid (),
		    (PTRACE_TYPE_ARG3) &fpregs, lwp);

  if (ret < 0)
    warning (_("unable to store floating-point registers"));
}

void
arm_netbsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  if (regno >= 0)
    {
      if (regno < ARM_F0_REGNUM || regno > ARM_FPS_REGNUM)
	store_register (regcache, regno);
      else
	store_fp_register (regcache, regno);
    }
  else
    {
      store_regs (regcache);
      store_fp_regs (regcache);
    }
}

const struct target_desc *
arm_netbsd_nat_target::read_description ()
{
  int flag;
  size_t len = sizeof (flag);

  if (sysctlbyname("machdep.fpu_present", &flag, &len, NULL, 0) != 0
      || !flag)
    return arm_read_description (ARM_FP_TYPE_NONE, false);

  len = sizeof(flag);
  if (sysctlbyname("machdep.neon_present", &flag, &len, NULL, 0) == 0 && flag)
    return aarch32_read_description (false);

  return arm_read_description (ARM_FP_TYPE_VFPV3, false);
}

void _initialize_arm_netbsd_nat ();
void
_initialize_arm_netbsd_nat ()
{
  /* Support debugging kernel virtual memory images.  */
  bsd_kvm_add_target (armnbsd_supply_pcb);

  add_inf_child_target (&the_arm_netbsd_nat_target);
}
