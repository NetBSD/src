/* Native-dependent code for modern i386 BSD's.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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

#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>

#include "i386-tdep.h"
#include "i387-tdep.h"
#include "x86-bsd-nat.h"
#include "i386-bsd-nat.h"
#include "inf-ptrace.h"


/* In older BSD versions we cannot get at some of the segment
   registers.  FreeBSD for example didn't support the %fs and %gs
   registers until the 3.0 release.  We have autoconf checks for their
   presence, and deal gracefully with their absence.  */

/* Offset in `struct reg' where MEMBER is stored.  */
#define REG_OFFSET(member) offsetof (struct reg, member)

/* At i386bsd_reg_offset[REGNUM] you'll find the offset in `struct
   reg' where the GDB register REGNUM is stored.  Unsupported
   registers are marked with `-1'.  */
static int i386bsd_r_reg_offset[] =
{
  REG_OFFSET (r_eax),
  REG_OFFSET (r_ecx),
  REG_OFFSET (r_edx),
  REG_OFFSET (r_ebx),
  REG_OFFSET (r_esp),
  REG_OFFSET (r_ebp),
  REG_OFFSET (r_esi),
  REG_OFFSET (r_edi),
  REG_OFFSET (r_eip),
  REG_OFFSET (r_eflags),
  REG_OFFSET (r_cs),
  REG_OFFSET (r_ss),
  REG_OFFSET (r_ds),
  REG_OFFSET (r_es),
#ifdef HAVE_STRUCT_REG_R_FS
  REG_OFFSET (r_fs),
#else
  -1,
#endif
#ifdef HAVE_STRUCT_REG_R_GS
  REG_OFFSET (r_gs)
#else
  -1
#endif
};

/* Macro to determine if a register is fetched with PT_GETREGS.  */
#define GETREGS_SUPPLIES(regnum) \
  ((0 <= (regnum) && (regnum) <= 15))

#ifdef HAVE_PT_GETXMMREGS
/* Set to 1 if the kernel supports PT_GETXMMREGS.  Initialized to -1
   so that we try PT_GETXMMREGS the first time around.  */
static int have_ptrace_xmmregs = -1;
#endif


/* Supply the general-purpose registers in GREGS, to REGCACHE.  */

static void
i386bsd_supply_gregset (struct regcache *regcache, const void *gregs)
{
  const char *regs = (const char *) gregs;
  int regnum;

  for (regnum = 0; regnum < ARRAY_SIZE (i386bsd_r_reg_offset); regnum++)
    {
      int offset = i386bsd_r_reg_offset[regnum];

      if (offset != -1)
	regcache_raw_supply (regcache, regnum, regs + offset);
    }
}

/* Collect register REGNUM from REGCACHE and store its contents in
   GREGS.  If REGNUM is -1, collect and store all appropriate
   registers.  */

static void
i386bsd_collect_gregset (const struct regcache *regcache,
			 void *gregs, int regnum)
{
  char *regs = (char *) gregs;
  int i;

  for (i = 0; i < ARRAY_SIZE (i386bsd_r_reg_offset); i++)
    {
      if (regnum == -1 || regnum == i)
	{
	  int offset = i386bsd_r_reg_offset[i];

	  if (offset != -1)
	    regcache_raw_collect (regcache, i, regs + offset);
	}
    }
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers (including the floating point registers).  */

static void
i386bsd_fetch_inferior_registers (struct target_ops *ops,
				  struct regcache *regcache, int regnum)
{
  pid_t pid = get_ptrace_pid (regcache_get_ptid (regcache));

  if (regnum == -1 || GETREGS_SUPPLIES (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      i386bsd_supply_gregset (regcache, &regs);
      if (regnum != -1)
	return;
    }

  if (regnum == -1 || regnum >= I386_ST0_REGNUM)
    {
      struct fpreg fpregs;
#ifdef HAVE_PT_GETXMMREGS
      char xmmregs[512];
#endif

#ifdef PT_GETXSTATE_INFO
      if (x86bsd_xsave_len != 0)
	{
	  void *xstateregs;

	  xstateregs = alloca (x86bsd_xsave_len);
	  if (ptrace (PT_GETXSTATE, pid,
		      (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	    perror_with_name (_("Couldn't get extended state status"));

	  i387_supply_xsave (regcache, -1, xstateregs);
	  return;
	}
#endif
      
#ifdef HAVE_PT_GETXMMREGS
      if (have_ptrace_xmmregs != 0
	  && ptrace(PT_GETXMMREGS, pid, (PTRACE_TYPE_ARG3) xmmregs, 0) == 0)
	{
	  have_ptrace_xmmregs = 1;
	  i387_supply_fxsave (regcache, -1, xmmregs);
	}
      else
	{
	  have_ptrace_xmmregs = 0;
#endif
          if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	    perror_with_name (_("Couldn't get floating point status"));

	  i387_supply_fsave (regcache, -1, &fpregs);
#ifdef HAVE_PT_GETXMMREGS
	}
#endif
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating point registers).  */

static void
i386bsd_store_inferior_registers (struct target_ops *ops,
				  struct regcache *regcache, int regnum)
{
  pid_t pid = get_ptrace_pid (regcache_get_ptid (regcache));

  if (regnum == -1 || GETREGS_SUPPLIES (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't get registers"));

      i386bsd_collect_gregset (regcache, &regs, regnum);

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

  if (regnum == -1 || regnum >= I386_ST0_REGNUM)
    {
      struct fpreg fpregs;
#ifdef HAVE_PT_GETXMMREGS
      char xmmregs[512];
#endif

#ifdef PT_GETXSTATE_INFO
      if (x86bsd_xsave_len != 0)
	{
	  void *xstateregs;

	  xstateregs = alloca (x86bsd_xsave_len);
	  if (ptrace (PT_GETXSTATE, pid,
		      (PTRACE_TYPE_ARG3) xstateregs, 0) == -1)
	    perror_with_name (_("Couldn't get extended state status"));

	  i387_collect_xsave (regcache, -1, xstateregs, 0);

	  if (ptrace (PT_SETXSTATE, pid,
		      (PTRACE_TYPE_ARG3) xstateregs, x86bsd_xsave_len) == -1)
	    perror_with_name (_("Couldn't write extended state status"));
	  return;
	}
#endif

#ifdef HAVE_PT_GETXMMREGS
      if (have_ptrace_xmmregs != 0
	  && ptrace(PT_GETXMMREGS, pid, (PTRACE_TYPE_ARG3) xmmregs, 0) == 0)
	{
	  have_ptrace_xmmregs = 1;

	  i387_collect_fxsave (regcache, regnum, xmmregs);

	  if (ptrace (PT_SETXMMREGS, pid, (PTRACE_TYPE_ARG3) xmmregs, 0) == -1)
            perror_with_name (_("Couldn't write XMM registers"));
	}
      else
	{
	  have_ptrace_xmmregs = 0;
#endif
          if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	    perror_with_name (_("Couldn't get floating point status"));

          i387_collect_fsave (regcache, regnum, &fpregs);

          if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	    perror_with_name (_("Couldn't write floating point status"));
#ifdef HAVE_PT_GETXMMREGS
        }
#endif
    }
}

/* Create a prototype *BSD/i386 target.  The client can override it
   with local methods.  */

struct target_ops *
i386bsd_target (void)
{
  struct target_ops *t;

  t = x86bsd_target ();
  t->to_fetch_registers = i386bsd_fetch_inferior_registers;
  t->to_store_registers = i386bsd_store_inferior_registers;
  return t;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_i386bsd_nat (void);

void
_initialize_i386bsd_nat (void)
{
  int offset;

  /* To support the recognition of signal handlers, i386-bsd-tdep.c
     hardcodes some constants.  Inclusion of this file means that we
     are compiling a native debugger, which means that we can use the
     system header files and sysctl(3) to get at the relevant
     information.  */

#if defined (__FreeBSD_version) && __FreeBSD_version >= 400011
#define SC_REG_OFFSET i386fbsd4_sc_reg_offset
#elif defined (__FreeBSD_version) && __FreeBSD_version >= 300005
#define SC_REG_OFFSET i386fbsd_sc_reg_offset
#elif defined (NetBSD) || defined (__NetBSD_Version__)
#define SC_REG_OFFSET i386nbsd_sc_reg_offset
#elif defined (OpenBSD)
#define SC_REG_OFFSET i386obsd_sc_reg_offset
#endif

#ifdef SC_REG_OFFSET

  /* We only check the program counter, stack pointer and frame
     pointer since these members of `struct sigcontext' are essential
     for providing backtraces.  More checks could be added, but would
     involve adding configure checks for the appropriate structure
     members, since older BSD's don't provide all of them.  */

#define SC_PC_OFFSET SC_REG_OFFSET[I386_EIP_REGNUM]
#define SC_SP_OFFSET SC_REG_OFFSET[I386_ESP_REGNUM]
#define SC_FP_OFFSET SC_REG_OFFSET[I386_EBP_REGNUM]

  /* Override the default value for the offset of the program counter
     in the sigcontext structure.  */
  offset = offsetof (struct sigcontext, sc_pc);

  if (SC_PC_OFFSET != offset)
    {
      warning (_("\
offsetof (struct sigcontext, sc_pc) yields %d instead of %d.\n\
Please report this to <bug-gdb@gnu.org>."), 
	       offset, SC_PC_OFFSET);
    }

  SC_PC_OFFSET = offset;

  /* Likewise for the stack pointer.  */
  offset = offsetof (struct sigcontext, sc_sp);

  if (SC_SP_OFFSET != offset)
    {
      warning (_("\
offsetof (struct sigcontext, sc_sp) yields %d instead of %d.\n\
Please report this to <bug-gdb@gnu.org>."),
	       offset, SC_SP_OFFSET);
    }

  SC_SP_OFFSET = offset;

  /* And the frame pointer.  */
  offset = offsetof (struct sigcontext, sc_fp);

  if (SC_FP_OFFSET != offset)
    {
      warning (_("\
offsetof (struct sigcontext, sc_fp) yields %d instead of %d.\n\
Please report this to <bug-gdb@gnu.org>."),
	       offset, SC_FP_OFFSET);
    }

  SC_FP_OFFSET = offset;

#endif /* SC_REG_OFFSET */
}
