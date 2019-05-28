/* Native-dependent code for NetBSD/sh.

   Copyright (C) 2002-2019 Free Software Foundation, Inc.

   Contributed by Wasabi Systems, Inc.

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
#include "inferior.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "sh-tdep.h"
#include "inf-ptrace.h"
#include "regcache.h"
#include "inf-ptrace.h"

struct sh_nbsd_nat_target final : public inf_ptrace_target
{
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static sh_nbsd_nat_target the_sh_nbsd_nat_target;

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
struct fpreg { };
typedef struct fpreg fpregset_t;
#endif
#include "gregset.h"
 
/* Determine if PT_GETREGS fetches this register.  */
#define GETREGS_SUPPLIES(gdbarch, regno) \
  (((regno) >= R0_REGNUM && (regno) <= (R0_REGNUM + 15)) \
|| (regno) == gdbarch_pc_regnum (gdbarch) || (regno) == PR_REGNUM \
|| (regno) == MACH_REGNUM || (regno) == MACL_REGNUM \
|| (regno) == SR_REGNUM || (regno) == GBR_REGNUM)

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define SHNBSD_SIZEOF_GREGS	(22 * 4)

void
sh_nbsd_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  pid_t pid = regcache->ptid ().pid ();

  if (regno == -1 || GETREGS_SUPPLIES (regcache->arch (), regno))
    {
      struct reg inferior_registers;

      if (ptrace (PT_GETREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't get registers"));

      sh_corefile_supply_regset (&sh_corefile_gregset, regcache, regno,
				 (char *) &inferior_registers,
				 SHNBSD_SIZEOF_GREGS);

      if (regno != -1)
	return;
    }
}

void
sh_nbsd_nat_target::store_registers (struct regcache *regcache, int regno)
{
  pid_t pid = regcache->ptid ().pid ();

  if (regno == -1 || GETREGS_SUPPLIES (regcache->arch (), regno))
    {
      struct reg inferior_registers;

      if (ptrace (PT_GETREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't get registers"));

      sh_corefile_collect_regset (&sh_corefile_gregset, regcache, regno,
				  (char *) &inferior_registers,
				  SHNBSD_SIZEOF_GREGS);

      if (ptrace (PT_SETREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, inferior_ptid.lwp ()) == -1)
	perror_with_name (_("Couldn't set registers"));

      if (regno != -1)
	return;
    }
}

void
_initialize_shnbsd_nat (void)
{
  add_inf_child_target (&the_sh_nbsd_nat_target);
}
