/* Native-dependent code for AMD64 BSD's.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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

#include "gdb_assert.h"
/* We include <signal.h> to make sure `struct fxsave64' is defined on
   NetBSD, since NetBSD's <machine/reg.h> needs it.  */
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "amd64-tdep.h"
#include "amd64-nat.h"
#include "x86bsd-nat.h"
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

      if (ptrace (PT_GETREGS, get_ptrace_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      amd64_supply_native_gregset (regcache, &regs, -1);
      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct fpreg fpregs;
#ifdef PT_GETXSTATE_INFO
      void *xstateregs;

      if (x86bsd_xsave_len != 0)
	{
	  xstateregs = alloca (x86bsd_xsave_len);
	  if (ptrace (PT_GETXSTATE, get_ptrace_pid (inferior_ptid),
		      (PTRACE_TYPE_ARG3) xstateregs, ptid_get_lwp (inferior_ptid)) == -1)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_supply_xsave (regcache, -1, xstateregs);
	  return;
	}
#endif

      if (ptrace (PT_GETFPREGS, get_ptrace_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, ptid_get_lwp (inferior_ptid)) == -1)
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

      if (ptrace (PT_GETREGS, get_ptrace_pid (inferior_ptid),
                  (PTRACE_TYPE_ARG3) &regs, ptid_get_lwp (inferior_ptid)) == -1)
        perror_with_name (_("Couldn't get registers"));

      amd64_collect_native_gregset (regcache, &regs, regnum);

      if (ptrace (PT_SETREGS, get_ptrace_pid (inferior_ptid),
	          (PTRACE_TYPE_ARG3) &regs, ptid_get_lwp (inferior_ptid)) == -1)
        perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct fpreg fpregs;
#ifdef PT_GETXSTATE_INFO
      void *xstateregs;

      if (x86bsd_xsave_len != 0)
	{
	  xstateregs = alloca (x86bsd_xsave_len);
	  if (ptrace (PT_GETXSTATE, get_ptrace_pid (inferior_ptid),
		      (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_collect_xsave (regcache, regnum, xstateregs, 0);

	  if (ptrace (PT_SETXSTATE, get_ptrace_pid (inferior_ptid),
		      (PTRACE_TYPE_ARG3) xstateregs, x86bsd_xsave_len) == -1)
	    perror_with_name (_("Couldn't write extended state status"));
	  return;
	}
#endif

      if (ptrace (PT_GETFPREGS, get_ptrace_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      amd64_collect_fxsave (regcache, regnum, &fpregs);

      if (ptrace (PT_SETFPREGS, get_ptrace_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

/* Create a prototype *BSD/amd64 target.  The client can override it
   with local methods.  */

struct target_ops *
amd64bsd_target (void)
{
  struct target_ops *t;

  t = x86bsd_target ();
  t->to_fetch_registers = amd64bsd_fetch_inferior_registers;
  t->to_store_registers = amd64bsd_store_inferior_registers;
  return t;
}
