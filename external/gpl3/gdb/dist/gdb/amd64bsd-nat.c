/* Native-dependent code for AMD64 BSD's.

   Copyright (C) 2003-2013 Free Software Foundation, Inc.

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
#include "regcache.h"
#include "target.h"

/* We include <signal.h> to make sure `struct fxsave64' is defined on
   NetBSD, since NetBSD's <machine/reg.h> needs it.  */
#include "gdb_assert.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "amd64-tdep.h"
#include "amd64-nat.h"
#include "amd64bsd-nat.h"
#include "inf-ptrace.h"


/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers (including the floating-point registers).  */

static void
amd64bsd_fetch_inferior_registers (struct target_ops *ops,
				   struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      amd64_supply_native_gregset (regcache, &regs, -1);
      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      amd64_supply_fxsave (regcache, -1, &fpregs);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating-point registers).  */

static void
amd64bsd_store_inferior_registers (struct target_ops *ops,
				   struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, PIDGET (inferior_ptid),
                  (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't get registers"));

      amd64_collect_native_gregset (regcache, &regs, regnum);

      if (ptrace (PT_SETREGS, PIDGET (inferior_ptid),
	          (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      amd64_collect_fxsave (regcache, regnum, &fpregs);

      if (ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

/* Create a prototype *BSD/amd64 target.  The client can override it
   with local methods.  */

struct target_ops *
amd64bsd_target (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = amd64bsd_fetch_inferior_registers;
  t->to_store_registers = amd64bsd_store_inferior_registers;
  return t;
}


/* Support for debug registers.  */

#ifdef HAVE_PT_GETDBREGS

static unsigned long
amd64bsd_dr_get (ptid_t ptid, int regnum)
{
  struct dbreg dbregs;

  if (ptrace (PT_GETDBREGS, PIDGET (inferior_ptid),
	      (PTRACE_TYPE_ARG3) &dbregs, 0) == -1)
    perror_with_name (_("Couldn't read debug registers"));

  return DBREG_DRX ((&dbregs), regnum);
}

static void
amd64bsd_dr_set (int regnum, unsigned long value)
{
  struct dbreg dbregs;

  if (ptrace (PT_GETDBREGS, PIDGET (inferior_ptid),
              (PTRACE_TYPE_ARG3) &dbregs, 0) == -1)
    perror_with_name (_("Couldn't get debug registers"));

  /* For some mysterious reason, some of the reserved bits in the
     debug control register get set.  Mask these off, otherwise the
     ptrace call below will fail.  */
  DBREG_DRX ((&dbregs), 7) &= ~(0xffffffff0000fc00);

  DBREG_DRX ((&dbregs), regnum) = value;

  if (ptrace (PT_SETDBREGS, PIDGET (inferior_ptid),
              (PTRACE_TYPE_ARG3) &dbregs, 0) == -1)
    perror_with_name (_("Couldn't write debug registers"));
}

void
amd64bsd_dr_set_control (unsigned long control)
{
  amd64bsd_dr_set (7, control);
}

void
amd64bsd_dr_set_addr (int regnum, CORE_ADDR addr)
{
  gdb_assert (regnum >= 0 && regnum <= 4);

  amd64bsd_dr_set (regnum, addr);
}

CORE_ADDR
amd64bsd_dr_get_addr (int regnum)
{
  return amd64bsd_dr_get (inferior_ptid, regnum);
}

unsigned long
amd64bsd_dr_get_status (void)
{
  return amd64bsd_dr_get (inferior_ptid, 6);
}

unsigned long
amd64bsd_dr_get_control (void)
{
  return amd64bsd_dr_get (inferior_ptid, 7);
}

#endif /* PT_GETDBREGS */
