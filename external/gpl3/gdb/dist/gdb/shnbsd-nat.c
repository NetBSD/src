/* Native-dependent code for NetBSD/sh.

   Copyright (C) 2002, 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

#include "nbsd-nat.h"

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

static void
shnbsd_fetch_inferior_registers (struct target_ops *ops,
				 struct regcache *regcache, int regno)
{
  if (regno == -1 || GETREGS_SUPPLIES (get_regcache_arch (regcache), regno))
    {
      struct reg inferior_registers;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      sh_corefile_supply_regset (&sh_corefile_gregset, regcache, regno,
				 (char *) &inferior_registers,
				 SHNBSD_SIZEOF_GREGS);

      if (regno != -1)
	return;
    }
}

static void
shnbsd_store_inferior_registers (struct target_ops *ops,
				 struct regcache *regcache, int regno)
{
  if (regno == -1 || GETREGS_SUPPLIES (get_regcache_arch (regcache), regno))
    {
      struct reg inferior_registers;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      sh_corefile_collect_regset (&sh_corefile_gregset, regcache, regno,
				  (char *) &inferior_registers,
				  SHNBSD_SIZEOF_GREGS);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &inferior_registers, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't set registers"));

      if (regno != -1)
	return;
    }
}

void
supply_gregset (struct regcache *regcache, const gregset_t *gregs)
{
      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) gregs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't set registers"));
}

void
fill_gregset (const struct regcache *regcache, gregset_t *gregs, int regnum)
{
   if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
	(PTRACE_TYPE_ARG3) gregs, TIDGET (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_shnbsd_nat (void);

void
_initialize_shnbsd_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = shnbsd_fetch_inferior_registers;
  t->to_store_registers = shnbsd_store_inferior_registers;
  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;
  add_target (t);
}
