/* Native-dependent code for FreeBSD/arm.

   Copyright (C) 2017-2020 Free Software Foundation, Inc.

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

#include "defs.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "fbsd-nat.h"
#include "arm-tdep.h"
#include "arm-fbsd-tdep.h"
#include "inf-ptrace.h"

struct arm_fbsd_nat_target : public fbsd_nat_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
  const struct target_desc *read_description () override;
};

static arm_fbsd_nat_target the_arm_fbsd_nat_target;

/* Determine if PT_GETREGS fetches REGNUM.  */

static bool
getregs_supplies (int regnum)
{
  return ((regnum >= ARM_A1_REGNUM && regnum <= ARM_PC_REGNUM)
	  || regnum == ARM_PS_REGNUM);
}

#ifdef PT_GETVFPREGS
/* Determine if PT_GETVFPREGS fetches REGNUM.  */

static bool
getvfpregs_supplies (int regnum)
{
  return ((regnum >= ARM_D0_REGNUM && regnum <= ARM_D31_REGNUM)
	  || regnum == ARM_FPSCR_REGNUM);
}
#endif

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
arm_fbsd_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1 || getregs_supplies (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache->supply_regset (&arm_fbsd_gregset, regnum, &regs,
			       sizeof (regs));
    }

#ifdef PT_GETVFPREGS
  if (regnum == -1 || getvfpregs_supplies (regnum))
    {
      struct vfpreg vfpregs;

      if (ptrace (PT_GETVFPREGS, pid, (PTRACE_TYPE_ARG3) &vfpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache->supply_regset (&arm_fbsd_vfpregset, regnum, &vfpregs,
			       sizeof (vfpregs));
    }
#endif
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
arm_fbsd_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1 || getregs_supplies (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      regcache->collect_regset (&arm_fbsd_gregset, regnum, &regs,
			       sizeof (regs));

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't write registers"));
    }

#ifdef PT_GETVFPREGS
  if (regnum == -1 || getvfpregs_supplies (regnum))
    {
      struct vfpreg vfpregs;

      if (ptrace (PT_GETVFPREGS, pid, (PTRACE_TYPE_ARG3) &vfpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      regcache->collect_regset (&arm_fbsd_vfpregset, regnum, &vfpregs,
				sizeof (vfpregs));

      if (ptrace (PT_SETVFPREGS, pid, (PTRACE_TYPE_ARG3) &vfpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
#endif
}

/* Implement the to_read_description method.  */

const struct target_desc *
arm_fbsd_nat_target::read_description ()
{
  const struct target_desc *desc;

  desc = arm_fbsd_read_description_auxv (this);
  if (desc == NULL)
    desc = this->beneath ()->read_description ();
  return desc;
}

void _initialize_arm_fbsd_nat ();
void
_initialize_arm_fbsd_nat ()
{
  add_inf_child_target (&the_arm_fbsd_nat_target);
}
