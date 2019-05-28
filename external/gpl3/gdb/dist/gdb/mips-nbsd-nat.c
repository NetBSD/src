/* Native-dependent code for MIPS systems running NetBSD.

   Copyright (C) 2000-2019 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif

#include "gregset.h"

#include "mips-tdep.h"
#include "nbsd-nat.h"
#include "mips-nbsd-tdep.h"
#include "inf-ptrace.h"
#include "bsd-kvm.h"

#include "machine/pcb.h"

class mips_nbsd_nat_target final : public inf_ptrace_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static mips_nbsd_nat_target the_mips_nbsd_nat_target;

/* Determine if PT_GETREGS fetches this register.  */
static int
getregs_supplies (struct gdbarch *gdbarch, int regno)
{
  return ((regno) >= MIPS_ZERO_REGNUM
	  && (regno) <= gdbarch_pc_regnum (gdbarch));
}

void
mips_nbsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  pid_t pid = regcache->ptid ().pid ();

  struct gdbarch *gdbarch = regcache->arch ();
  if (regno == -1 || getregs_supplies (gdbarch, regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't get registers"));
      
      mipsnbsd_supply_reg (regcache, (char *) &regs, regno);
      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (regcache->arch ()))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mipsnbsd_supply_fpreg (regcache, (char *) &fpregs, regno);
    }
}

void
mips_nbsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  pid_t pid = regcache->ptid ().pid ();

  struct gdbarch *gdbarch = regcache->arch ();
  if (regno == -1 || getregs_supplies (gdbarch, regno))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't get registers"));

      mipsnbsd_fill_reg (regcache, (char *) &regs, regno);

      if (ptrace (PT_SETREGS, ptid_get_pid (inferior_ptid), 
		  (PTRACE_TYPE_ARG3) &regs, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't write registers"));

      if (regno != -1)
	return;
    }

  if (regno == -1
      || regno >= gdbarch_fp0_regnum (regcache->arch ()))
    {
      struct fpreg fpregs; 

      if (ptrace (PT_GETFPREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      mipsnbsd_fill_fpreg (regcache, (char *) &fpregs, regno);

      if (ptrace (PT_SETFPREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

void
_initialize_mipsnbsd_nat (void)
{
  add_inf_child_target (&the_mips_nbsd_nat_target);
}
