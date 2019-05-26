/* Native-dependent code for AMD64 BSD's.

   Copyright (C) 2003-2019 Free Software Foundation, Inc.

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
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "amd64-tdep.h"
#include "amd64-nat.h"
#include "x86-bsd-nat.h"
#include "inf-ptrace.h"


/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers (including the floating-point registers).  */

void
amd64bsd_fetch_inferior_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      amd64_supply_native_gregset (regcache, &regs, -1);
      if (regnum != -1)
	return;
    }

#ifdef PT_GETFSBASE
  if (regnum == -1 || regnum == AMD64_FSBASE_REGNUM)
    {
      register_t base;

      if (ptrace (PT_GETFSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't get segment register fs_base"));

      regcache->raw_supply (AMD64_FSBASE_REGNUM, &base);
      if (regnum != -1)
	return;
    }
#endif
#ifdef PT_GETGSBASE
  if (regnum == -1 || regnum == AMD64_GSBASE_REGNUM)
    {
      register_t base;

      if (ptrace (PT_GETGSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't get segment register gs_base"));

      regcache->raw_supply (AMD64_GSBASE_REGNUM, &base);
      if (regnum != -1)
	return;
    }
#endif

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct fpreg fpregs;
#ifdef PT_GETXSTATE_INFO
      void *xstateregs;

      if (x86bsd_xsave_len != 0)
	{
	  xstateregs = alloca (x86bsd_xsave_len);
	  if (ptrace (PT_GETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs, 0)
	      == -1)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_supply_xsave (regcache, -1, xstateregs);
	  return;
	}
#endif

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      amd64_supply_fxsave (regcache, -1, &fpregs);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating-point registers).  */

void
amd64bsd_store_inferior_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  pid_t pid = get_ptrace_pid (regcache->ptid ());

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't get registers"));

      amd64_collect_native_gregset (regcache, &regs, regnum);

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

#ifdef PT_SETFSBASE
  if (regnum == -1 || regnum == AMD64_FSBASE_REGNUM)
    {
      register_t base;

      regcache->raw_collect (AMD64_FSBASE_REGNUM, &base);

      if (ptrace (PT_SETFSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't write segment register fs_base"));
      if (regnum != -1)
	return;
    }
#endif
#ifdef PT_SETGSBASE
  if (regnum == -1 || regnum == AMD64_GSBASE_REGNUM)
    {
      register_t base;

      regcache->raw_collect (AMD64_GSBASE_REGNUM, &base);

      if (ptrace (PT_SETGSBASE, pid, (PTRACE_TYPE_ARG3) &base, 0) == -1)
	perror_with_name (_("Couldn't write segment register gs_base"));
      if (regnum != -1)
	return;
    }
#endif

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      struct fpreg fpregs;
#ifdef PT_GETXSTATE_INFO
      void *xstateregs;

      if (x86bsd_xsave_len != 0)
	{
	  xstateregs = alloca (x86bsd_xsave_len);
	  if (ptrace (PT_GETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs, 0)
	      == -1)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_collect_xsave (regcache, regnum, xstateregs, 0);

	  if (ptrace (PT_SETXSTATE, pid, (PTRACE_TYPE_ARG3) xstateregs,
		      x86bsd_xsave_len) == -1)
	    perror_with_name (_("Couldn't write extended state status"));
	  return;
	}
#endif

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      amd64_collect_fxsave (regcache, regnum, &fpregs);

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}
