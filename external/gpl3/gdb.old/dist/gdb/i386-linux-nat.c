/* Native-dependent code for GNU/Linux i386.

   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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
#include "i386-nat.h"
#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "regset.h"
#include "target.h"
#include "linux-nat.h"
#include "linux-btrace.h"
#include "btrace.h"

#include "gdb_assert.h"
#include <string.h>
#include "elf/common.h"
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/procfs.h>

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#ifndef ORIG_EAX
#define ORIG_EAX -1
#endif

#ifdef HAVE_SYS_DEBUGREG_H
#include <sys/debugreg.h>
#endif

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

#include "i387-tdep.h"
#include "i386-tdep.h"
#include "i386-linux-tdep.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

#include "i386-xstate.h"

#ifndef PTRACE_GETREGSET
#define PTRACE_GETREGSET	0x4204
#endif

#ifndef PTRACE_SETREGSET
#define PTRACE_SETREGSET	0x4205
#endif

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the thread.  */
  int debug_registers_changed;
};

/* Does the current host support PTRACE_GETREGSET?  */
static int have_ptrace_getregset = -1;


/* The register sets used in GNU/Linux ELF core-dumps are identical to
   the register sets in `struct user' that is used for a.out
   core-dumps, and is also used by `ptrace'.  The corresponding types
   are `elf_gregset_t' for the general-purpose registers (with
   `elf_greg_t' the type of a single GP register) and `elf_fpregset_t'
   for the floating-point registers.

   Those types used to be available under the names `gregset_t' and
   `fpregset_t' too, and this file used those names in the past.  But
   those names are now used for the register sets used in the
   `mcontext_t' type, and have a different size and layout.  */

/* Which ptrace request retrieves which registers?
   These apply to the corresponding SET requests as well.  */

#define GETREGS_SUPPLIES(regno) \
  ((0 <= (regno) && (regno) <= 15) || (regno) == I386_LINUX_ORIG_EAX_REGNUM)

#define GETFPXREGS_SUPPLIES(regno) \
  (I386_ST0_REGNUM <= (regno) && (regno) < I386_SSE_NUM_REGS)

#define GETXSTATEREGS_SUPPLIES(regno) \
  (I386_ST0_REGNUM <= (regno) && (regno) < I386_MPX_NUM_REGS)

/* Does the current host support the GETREGS request?  */
int have_ptrace_getregs =
#ifdef HAVE_PTRACE_GETREGS
  1
#else
  0
#endif
;

/* Does the current host support the GETFPXREGS request?  The header
   file may or may not define it, and even if it is defined, the
   kernel will return EIO if it's running on a pre-SSE processor.

   My instinct is to attach this to some architecture- or
   target-specific data structure, but really, a particular GDB
   process can only run on top of one kernel at a time.  So it's okay
   for this to be a simple variable.  */
int have_ptrace_getfpxregs =
#ifdef HAVE_PTRACE_GETFPXREGS
  -1
#else
  0
#endif
;


/* Accessing registers through the U area, one at a time.  */

/* Fetch one register.  */

static void
fetch_register (struct regcache *regcache, int regno)
{
  int tid;
  int val;

  gdb_assert (!have_ptrace_getregs);
  if (i386_linux_gregset_reg_offset[regno] == -1)
    {
      regcache_raw_supply (regcache, regno, NULL);
      return;
    }

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

  errno = 0;
  val = ptrace (PTRACE_PEEKUSER, tid,
		i386_linux_gregset_reg_offset[regno], 0);
  if (errno != 0)
    error (_("Couldn't read register %s (#%d): %s."), 
	   gdbarch_register_name (get_regcache_arch (regcache), regno),
	   regno, safe_strerror (errno));

  regcache_raw_supply (regcache, regno, &val);
}

/* Store one register.  */

static void
store_register (const struct regcache *regcache, int regno)
{
  int tid;
  int val;

  gdb_assert (!have_ptrace_getregs);
  if (i386_linux_gregset_reg_offset[regno] == -1)
    return;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

  errno = 0;
  regcache_raw_collect (regcache, regno, &val);
  ptrace (PTRACE_POKEUSER, tid,
	  i386_linux_gregset_reg_offset[regno], val);
  if (errno != 0)
    error (_("Couldn't write register %s (#%d): %s."),
	   gdbarch_register_name (get_regcache_arch (regcache), regno),
	   regno, safe_strerror (errno));
}


/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const elf_gregset_t *gregsetp)
{
  const gdb_byte *regp = (const gdb_byte *) gregsetp;
  int i;

  for (i = 0; i < I386_NUM_GREGS; i++)
    regcache_raw_supply (regcache, i,
			 regp + i386_linux_gregset_reg_offset[i]);

  if (I386_LINUX_ORIG_EAX_REGNUM
	< gdbarch_num_regs (get_regcache_arch (regcache)))
    regcache_raw_supply (regcache, I386_LINUX_ORIG_EAX_REGNUM, regp
			 + i386_linux_gregset_reg_offset[I386_LINUX_ORIG_EAX_REGNUM]);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      elf_gregset_t *gregsetp, int regno)
{
  gdb_byte *regp = (gdb_byte *) gregsetp;
  int i;

  for (i = 0; i < I386_NUM_GREGS; i++)
    if (regno == -1 || regno == i)
      regcache_raw_collect (regcache, i,
			    regp + i386_linux_gregset_reg_offset[i]);

  if ((regno == -1 || regno == I386_LINUX_ORIG_EAX_REGNUM)
      && I386_LINUX_ORIG_EAX_REGNUM
	   < gdbarch_num_regs (get_regcache_arch (regcache)))
    regcache_raw_collect (regcache, I386_LINUX_ORIG_EAX_REGNUM, regp
			  + i386_linux_gregset_reg_offset[I386_LINUX_ORIG_EAX_REGNUM]);
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all general-purpose registers from process/thread TID and
   store their values in GDB's register array.  */

static void
fetch_regs (struct regcache *regcache, int tid)
{
  elf_gregset_t regs;
  elf_gregset_t *regs_p = &regs;

  if (ptrace (PTRACE_GETREGS, tid, 0, (int) &regs) < 0)
    {
      if (errno == EIO)
	{
	  /* The kernel we're running on doesn't support the GETREGS
             request.  Reset `have_ptrace_getregs'.  */
	  have_ptrace_getregs = 0;
	  return;
	}

      perror_with_name (_("Couldn't get registers"));
    }

  supply_gregset (regcache, (const elf_gregset_t *) regs_p);
}

/* Store all valid general-purpose registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_regs (const struct regcache *regcache, int tid, int regno)
{
  elf_gregset_t regs;

  if (ptrace (PTRACE_GETREGS, tid, 0, (int) &regs) < 0)
    perror_with_name (_("Couldn't get registers"));

  fill_gregset (regcache, &regs, regno);
  
  if (ptrace (PTRACE_SETREGS, tid, 0, (int) &regs) < 0)
    perror_with_name (_("Couldn't write registers"));
}

#else

static void fetch_regs (struct regcache *regcache, int tid) {}
static void store_regs (const struct regcache *regcache, int tid, int regno) {}

#endif


/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* Fill GDB's register array with the floating-point register values in
   *FPREGSETP.  */

void 
supply_fpregset (struct regcache *regcache, const elf_fpregset_t *fpregsetp)
{
  i387_supply_fsave (regcache, -1, fpregsetp);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       elf_fpregset_t *fpregsetp, int regno)
{
  i387_collect_fsave (regcache, regno, fpregsetp);
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all floating-point registers from process/thread TID and store
   thier values in GDB's register array.  */

static void
fetch_fpregs (struct regcache *regcache, int tid)
{
  elf_fpregset_t fpregs;

  if (ptrace (PTRACE_GETFPREGS, tid, 0, (int) &fpregs) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  supply_fpregset (regcache, (const elf_fpregset_t *) &fpregs);
}

/* Store all valid floating-point registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_fpregs (const struct regcache *regcache, int tid, int regno)
{
  elf_fpregset_t fpregs;

  if (ptrace (PTRACE_GETFPREGS, tid, 0, (int) &fpregs) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  fill_fpregset (regcache, &fpregs, regno);

  if (ptrace (PTRACE_SETFPREGS, tid, 0, (int) &fpregs) < 0)
    perror_with_name (_("Couldn't write floating point status"));
}

#else

static void
fetch_fpregs (struct regcache *regcache, int tid)
{
}

static void
store_fpregs (const struct regcache *regcache, int tid, int regno)
{
}

#endif


/* Transfering floating-point and SSE registers to and from GDB.  */

/* Fetch all registers covered by the PTRACE_GETREGSET request from
   process/thread TID and store their values in GDB's register array.
   Return non-zero if successful, zero otherwise.  */

static int
fetch_xstateregs (struct regcache *regcache, int tid)
{
  char xstateregs[I386_XSTATE_MAX_SIZE];
  struct iovec iov;

  if (!have_ptrace_getregset)
    return 0;

  iov.iov_base = xstateregs;
  iov.iov_len = sizeof(xstateregs);
  if (ptrace (PTRACE_GETREGSET, tid, (unsigned int) NT_X86_XSTATE,
	      &iov) < 0)
    perror_with_name (_("Couldn't read extended state status"));

  i387_supply_xsave (regcache, -1, xstateregs);
  return 1;
}

/* Store all valid registers in GDB's register array covered by the
   PTRACE_SETREGSET request into the process/thread specified by TID.
   Return non-zero if successful, zero otherwise.  */

static int
store_xstateregs (const struct regcache *regcache, int tid, int regno)
{
  char xstateregs[I386_XSTATE_MAX_SIZE];
  struct iovec iov;

  if (!have_ptrace_getregset)
    return 0;
  
  iov.iov_base = xstateregs;
  iov.iov_len = sizeof(xstateregs);
  if (ptrace (PTRACE_GETREGSET, tid, (unsigned int) NT_X86_XSTATE,
	      &iov) < 0)
    perror_with_name (_("Couldn't read extended state status"));

  i387_collect_xsave (regcache, regno, xstateregs, 0);

  if (ptrace (PTRACE_SETREGSET, tid, (unsigned int) NT_X86_XSTATE,
	      (int) &iov) < 0)
    perror_with_name (_("Couldn't write extended state status"));

  return 1;
}

#ifdef HAVE_PTRACE_GETFPXREGS

/* Fetch all registers covered by the PTRACE_GETFPXREGS request from
   process/thread TID and store their values in GDB's register array.
   Return non-zero if successful, zero otherwise.  */

static int
fetch_fpxregs (struct regcache *regcache, int tid)
{
  elf_fpxregset_t fpxregs;

  if (! have_ptrace_getfpxregs)
    return 0;

  if (ptrace (PTRACE_GETFPXREGS, tid, 0, (int) &fpxregs) < 0)
    {
      if (errno == EIO)
	{
	  have_ptrace_getfpxregs = 0;
	  return 0;
	}

      perror_with_name (_("Couldn't read floating-point and SSE registers"));
    }

  i387_supply_fxsave (regcache, -1, (const elf_fpxregset_t *) &fpxregs);
  return 1;
}

/* Store all valid registers in GDB's register array covered by the
   PTRACE_SETFPXREGS request into the process/thread specified by TID.
   Return non-zero if successful, zero otherwise.  */

static int
store_fpxregs (const struct regcache *regcache, int tid, int regno)
{
  elf_fpxregset_t fpxregs;

  if (! have_ptrace_getfpxregs)
    return 0;
  
  if (ptrace (PTRACE_GETFPXREGS, tid, 0, &fpxregs) == -1)
    {
      if (errno == EIO)
	{
	  have_ptrace_getfpxregs = 0;
	  return 0;
	}

      perror_with_name (_("Couldn't read floating-point and SSE registers"));
    }

  i387_collect_fxsave (regcache, regno, &fpxregs);

  if (ptrace (PTRACE_SETFPXREGS, tid, 0, &fpxregs) == -1)
    perror_with_name (_("Couldn't write floating-point and SSE registers"));

  return 1;
}

#else

static int
fetch_fpxregs (struct regcache *regcache, int tid)
{
  return 0;
}

static int
store_fpxregs (const struct regcache *regcache, int tid, int regno)
{
  return 0;
}

#endif /* HAVE_PTRACE_GETFPXREGS */


/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNO from the child process.  If REGNO is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

static void
i386_linux_fetch_inferior_registers (struct target_ops *ops,
				     struct regcache *regcache, int regno)
{
  int tid;

  /* Use the old method of peeking around in `struct user' if the
     GETREGS request isn't available.  */
  if (!have_ptrace_getregs)
    {
      int i;

      for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
	if (regno == -1 || regno == i)
	  fetch_register (regcache, i);

      return;
    }

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

  /* Use the PTRACE_GETFPXREGS request whenever possible, since it
     transfers more registers in one system call, and we'll cache the
     results.  But remember that fetch_fpxregs can fail, and return
     zero.  */
  if (regno == -1)
    {
      fetch_regs (regcache, tid);

      /* The call above might reset `have_ptrace_getregs'.  */
      if (!have_ptrace_getregs)
	{
	  i386_linux_fetch_inferior_registers (ops, regcache, regno);
	  return;
	}

      if (fetch_xstateregs (regcache, tid))
	return;
      if (fetch_fpxregs (regcache, tid))
	return;
      fetch_fpregs (regcache, tid);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      fetch_regs (regcache, tid);
      return;
    }

  if (GETXSTATEREGS_SUPPLIES (regno))
    {
      if (fetch_xstateregs (regcache, tid))
	return;
    }

  if (GETFPXREGS_SUPPLIES (regno))
    {
      if (fetch_fpxregs (regcache, tid))
	return;

      /* Either our processor or our kernel doesn't support the SSE
	 registers, so read the FP registers in the traditional way,
	 and fill the SSE registers with dummy values.  It would be
	 more graceful to handle differences in the register set using
	 gdbarch.  Until then, this will at least make things work
	 plausibly.  */
      fetch_fpregs (regcache, tid);
      return;
    }

  internal_error (__FILE__, __LINE__,
		  _("Got request for bad register number %d."), regno);
}

/* Store register REGNO back into the child process.  If REGNO is -1,
   do this for all registers (including the floating point and SSE
   registers).  */
static void
i386_linux_store_inferior_registers (struct target_ops *ops,
				     struct regcache *regcache, int regno)
{
  int tid;

  /* Use the old method of poking around in `struct user' if the
     SETREGS request isn't available.  */
  if (!have_ptrace_getregs)
    {
      int i;

      for (i = 0; i < gdbarch_num_regs (get_regcache_arch (regcache)); i++)
	if (regno == -1 || regno == i)
	  store_register (regcache, i);

      return;
    }

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

  /* Use the PTRACE_SETFPXREGS requests whenever possible, since it
     transfers more registers in one system call.  But remember that
     store_fpxregs can fail, and return zero.  */
  if (regno == -1)
    {
      store_regs (regcache, tid, regno);
      if (store_xstateregs (regcache, tid, regno))
	return;
      if (store_fpxregs (regcache, tid, regno))
	return;
      store_fpregs (regcache, tid, regno);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      store_regs (regcache, tid, regno);
      return;
    }

  if (GETXSTATEREGS_SUPPLIES (regno))
    {
      if (store_xstateregs (regcache, tid, regno))
	return;
    }

  if (GETFPXREGS_SUPPLIES (regno))
    {
      if (store_fpxregs (regcache, tid, regno))
	return;

      /* Either our processor or our kernel doesn't support the SSE
	 registers, so just write the FP registers in the traditional
	 way.  */
      store_fpregs (regcache, tid, regno);
      return;
    }

  internal_error (__FILE__, __LINE__,
		  _("Got request to store bad register number %d."), regno);
}


/* Support for debug registers.  */

/* Get debug register REGNUM value from only the one LWP of PTID.  */

static unsigned long
i386_linux_dr_get (ptid_t ptid, int regnum)
{
  int tid;
  unsigned long value;

  tid = ptid_get_lwp (ptid);
  if (tid == 0)
    tid = ptid_get_pid (ptid);

  errno = 0;
  value = ptrace (PTRACE_PEEKUSER, tid,
		  offsetof (struct user, u_debugreg[regnum]), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't read debug register"));

  return value;
}

/* Set debug register REGNUM to VALUE in only the one LWP of PTID.  */

static void
i386_linux_dr_set (ptid_t ptid, int regnum, unsigned long value)
{
  int tid;

  tid = ptid_get_lwp (ptid);
  if (tid == 0)
    tid = ptid_get_pid (ptid);

  errno = 0;
  ptrace (PTRACE_POKEUSER, tid,
	  offsetof (struct user, u_debugreg[regnum]), value);
  if (errno != 0)
    perror_with_name (_("Couldn't write debug register"));
}

/* Return the inferior's debug register REGNUM.  */

static CORE_ADDR
i386_linux_dr_get_addr (int regnum)
{
  /* DR6 and DR7 are retrieved with some other way.  */
  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  return i386_linux_dr_get (inferior_ptid, regnum);
}

/* Return the inferior's DR7 debug control register.  */

static unsigned long
i386_linux_dr_get_control (void)
{
  return i386_linux_dr_get (inferior_ptid, DR_CONTROL);
}

/* Get DR_STATUS from only the one LWP of INFERIOR_PTID.  */

static unsigned long
i386_linux_dr_get_status (void)
{
  return i386_linux_dr_get (inferior_ptid, DR_STATUS);
}

/* Callback for iterate_over_lwps.  Update the debug registers of
   LWP.  */

static int
update_debug_registers_callback (struct lwp_info *lwp, void *arg)
{
  if (lwp->arch_private == NULL)
    lwp->arch_private = XCNEW (struct arch_lwp_info);

  /* The actual update is done later just before resuming the lwp, we
     just mark that the registers need updating.  */
  lwp->arch_private->debug_registers_changed = 1;

  /* If the lwp isn't stopped, force it to momentarily pause, so we
     can update its debug registers.  */
  if (!lwp->stopped)
    linux_stop_lwp (lwp);

  /* Continue the iteration.  */
  return 0;
}

/* Set DR_CONTROL to ADDR in all LWPs of the current inferior.  */

static void
i386_linux_dr_set_control (unsigned long control)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  iterate_over_lwps (pid_ptid, update_debug_registers_callback, NULL);
}

/* Set address REGNUM (zero based) to ADDR in all LWPs of the current
   inferior.  */

static void
i386_linux_dr_set_addr (int regnum, CORE_ADDR addr)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);

  iterate_over_lwps (pid_ptid, update_debug_registers_callback, NULL);
}

/* Called when resuming a thread.
   If the debug regs have changed, update the thread's copies.  */

static void
i386_linux_prepare_to_resume (struct lwp_info *lwp)
{
  int clear_status = 0;

  /* NULL means this is the main thread still going through the shell,
     or, no watchpoint has been set yet.  In that case, there's
     nothing to do.  */
  if (lwp->arch_private == NULL)
    return;

  if (lwp->arch_private->debug_registers_changed)
    {
      struct i386_debug_reg_state *state
	= i386_debug_reg_state (ptid_get_pid (lwp->ptid));
      int i;

      /* See amd64_linux_prepare_to_resume for Linux kernel note on
	 i386_linux_dr_set calls ordering.  */

      for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	if (state->dr_ref_count[i] > 0)
	  {
	    i386_linux_dr_set (lwp->ptid, i, state->dr_mirror[i]);

	    /* If we're setting a watchpoint, any change the inferior
	       had done itself to the debug registers needs to be
	       discarded, otherwise, i386_stopped_data_address can get
	       confused.  */
	    clear_status = 1;
	  }

      i386_linux_dr_set (lwp->ptid, DR_CONTROL, state->dr_control_mirror);

      lwp->arch_private->debug_registers_changed = 0;
    }

  if (clear_status || lwp->stopped_by_watchpoint)
    i386_linux_dr_set (lwp->ptid, DR_STATUS, 0);
}

static void
i386_linux_new_thread (struct lwp_info *lp)
{
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  info->debug_registers_changed = 1;

  lp->arch_private = info;
}

/* linux_nat_new_fork hook.   */

static void
i386_linux_new_fork (struct lwp_info *parent, pid_t child_pid)
{
  pid_t parent_pid;
  struct i386_debug_reg_state *parent_state;
  struct i386_debug_reg_state *child_state;

  /* NULL means no watchpoint has ever been set in the parent.  In
     that case, there's nothing to do.  */
  if (parent->arch_private == NULL)
    return;

  /* Linux kernel before 2.6.33 commit
     72f674d203cd230426437cdcf7dd6f681dad8b0d
     will inherit hardware debug registers from parent
     on fork/vfork/clone.  Newer Linux kernels create such tasks with
     zeroed debug registers.

     GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  The debug registers mirror will become zeroed
     in the end before detaching the forked off process, thus making
     this compatible with older Linux kernels too.  */

  parent_pid = ptid_get_pid (parent->ptid);
  parent_state = i386_debug_reg_state (parent_pid);
  child_state = i386_debug_reg_state (child_pid);
  *child_state = *parent_state;
}



/* Called by libthread_db.  Returns a pointer to the thread local
   storage (or its descriptor).  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph, 
		    lwpid_t lwpid, int idx, void **base)
{
  /* NOTE: cagney/2003-08-26: The definition of this buffer is found
     in the kernel header <asm-i386/ldt.h>.  It, after padding, is 4 x
     4 byte integers in size: `entry_number', `base_addr', `limit',
     and a bunch of status bits.

     The values returned by this ptrace call should be part of the
     regcache buffer, and ps_get_thread_area should channel its
     request through the regcache.  That way remote targets could
     provide the value using the remote protocol and not this direct
     call.

     Is this function needed?  I'm guessing that the `base' is the
     address of a descriptor that libthread_db uses to find the
     thread local address base that GDB needs.  Perhaps that
     descriptor is defined by the ABI.  Anyway, given that
     libthread_db calls this function without prompting (gdb
     requesting tls base) I guess it needs info in there anyway.  */
  unsigned int desc[4];
  gdb_assert (sizeof (int) == 4);

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid,
	      (void *) idx, (unsigned long) &desc) < 0)
    return PS_ERR;

  *(int *)base = desc[1];
  return PS_OK;
}


/* The instruction for a GNU/Linux system call is:
       int $0x80
   or 0xcd 0x80.  */

static const unsigned char linux_syscall[] = { 0xcd, 0x80 };

#define LINUX_SYSCALL_LEN (sizeof linux_syscall)

/* The system call number is stored in the %eax register.  */
#define LINUX_SYSCALL_REGNUM I386_EAX_REGNUM

/* We are specifically interested in the sigreturn and rt_sigreturn
   system calls.  */

#ifndef SYS_sigreturn
#define SYS_sigreturn		0x77
#endif
#ifndef SYS_rt_sigreturn
#define SYS_rt_sigreturn	0xad
#endif

/* Offset to saved processor flags, from <asm/sigcontext.h>.  */
#define LINUX_SIGCONTEXT_EFLAGS_OFFSET (64)

/* Resume execution of the inferior process.
   If STEP is nonzero, single-step it.
   If SIGNAL is nonzero, give it that signal.  */

static void
i386_linux_resume (struct target_ops *ops,
		   ptid_t ptid, int step, enum gdb_signal signal)
{
  int pid = ptid_get_pid (ptid);

  int request;

  if (catch_syscall_enabled () > 0)
   request = PTRACE_SYSCALL;
  else
    request = PTRACE_CONT;

  if (step)
    {
      struct regcache *regcache = get_thread_regcache (pid_to_ptid (pid));
      struct gdbarch *gdbarch = get_regcache_arch (regcache);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      ULONGEST pc;
      gdb_byte buf[LINUX_SYSCALL_LEN];

      request = PTRACE_SINGLESTEP;

      regcache_cooked_read_unsigned (regcache,
				     gdbarch_pc_regnum (gdbarch), &pc);

      /* Returning from a signal trampoline is done by calling a
         special system call (sigreturn or rt_sigreturn, see
         i386-linux-tdep.c for more information).  This system call
         restores the registers that were saved when the signal was
         raised, including %eflags.  That means that single-stepping
         won't work.  Instead, we'll have to modify the signal context
         that's about to be restored, and set the trace flag there.  */

      /* First check if PC is at a system call.  */
      if (target_read_memory (pc, buf, LINUX_SYSCALL_LEN) == 0
	  && memcmp (buf, linux_syscall, LINUX_SYSCALL_LEN) == 0)
	{
	  ULONGEST syscall;
	  regcache_cooked_read_unsigned (regcache,
					 LINUX_SYSCALL_REGNUM, &syscall);

	  /* Then check the system call number.  */
	  if (syscall == SYS_sigreturn || syscall == SYS_rt_sigreturn)
	    {
	      ULONGEST sp, addr;
	      unsigned long int eflags;

	      regcache_cooked_read_unsigned (regcache, I386_ESP_REGNUM, &sp);
	      if (syscall == SYS_rt_sigreturn)
		addr = read_memory_unsigned_integer (sp + 8, 4, byte_order)
		  + 20;
	      else
		addr = sp;

	      /* Set the trace flag in the context that's about to be
                 restored.  */
	      addr += LINUX_SIGCONTEXT_EFLAGS_OFFSET;
	      read_memory (addr, (gdb_byte *) &eflags, 4);
	      eflags |= 0x0100;
	      write_memory (addr, (gdb_byte *) &eflags, 4);
	    }
	}
    }

  if (ptrace (request, pid, 0, gdb_signal_to_host (signal)) == -1)
    perror_with_name (("ptrace"));
}

static void (*super_post_startup_inferior) (ptid_t ptid);

static void
i386_linux_child_post_startup_inferior (ptid_t ptid)
{
  i386_cleanup_dregs ();
  super_post_startup_inferior (ptid);
}

/* Get Linux/x86 target description from running target.  */

static const struct target_desc *
i386_linux_read_description (struct target_ops *ops)
{
  int tid;
  static uint64_t xcr0;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

#ifdef HAVE_PTRACE_GETFPXREGS
  if (have_ptrace_getfpxregs == -1)
    {
      elf_fpxregset_t fpxregs;

      if (ptrace (PTRACE_GETFPXREGS, tid, 0, (int) &fpxregs) < 0)
	{
	  have_ptrace_getfpxregs = 0;
	  have_ptrace_getregset = 0;
	  return tdesc_i386_mmx_linux;
	}
    }
#endif

  if (have_ptrace_getregset == -1)
    {
      uint64_t xstateregs[(I386_XSTATE_SSE_SIZE / sizeof (uint64_t))];
      struct iovec iov;

      iov.iov_base = xstateregs;
      iov.iov_len = sizeof (xstateregs);

      /* Check if PTRACE_GETREGSET works.  */
      if (ptrace (PTRACE_GETREGSET, tid, (unsigned int) NT_X86_XSTATE,
		  &iov) < 0)
	have_ptrace_getregset = 0;
      else
	{
	  have_ptrace_getregset = 1;

	  /* Get XCR0 from XSAVE extended state.  */
	  xcr0 = xstateregs[(I386_LINUX_XSAVE_XCR0_OFFSET
			     / sizeof (long long))];
	}
    }

  /* Check the native XCR0 only if PTRACE_GETREGSET is available.  */
  if (have_ptrace_getregset)
    {
      switch ((xcr0 & I386_XSTATE_ALL_MASK))
	{
	case I386_XSTATE_MPX_MASK:
	  return tdesc_i386_mpx_linux;
	case I386_XSTATE_AVX_MASK:
	  return tdesc_i386_avx_linux;
	default:
	  return tdesc_i386_linux;
	}
    }
  else
    return tdesc_i386_linux;
}

/* Enable branch tracing.  */

static struct btrace_target_info *
i386_linux_enable_btrace (ptid_t ptid)
{
  struct btrace_target_info *tinfo;
  struct gdbarch *gdbarch;

  errno = 0;
  tinfo = linux_enable_btrace (ptid);

  if (tinfo == NULL)
    error (_("Could not enable branch tracing for %s: %s."),
	   target_pid_to_str (ptid), safe_strerror (errno));

  /* Fill in the size of a pointer in bits.  */
  gdbarch = target_thread_architecture (ptid);
  tinfo->ptr_bits = gdbarch_ptr_bit (gdbarch);

  return tinfo;
}

/* Disable branch tracing.  */

static void
i386_linux_disable_btrace (struct btrace_target_info *tinfo)
{
  int errcode = linux_disable_btrace (tinfo);

  if (errcode != 0)
    error (_("Could not disable branch tracing: %s."), safe_strerror (errcode));
}

/* Teardown branch tracing.  */

static void
i386_linux_teardown_btrace (struct btrace_target_info *tinfo)
{
  /* Ignore errors.  */
  linux_disable_btrace (tinfo);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_i386_linux_nat;

void
_initialize_i386_linux_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  i386_use_watchpoints (t);

  i386_dr_low.set_control = i386_linux_dr_set_control;
  i386_dr_low.set_addr = i386_linux_dr_set_addr;
  i386_dr_low.get_addr = i386_linux_dr_get_addr;
  i386_dr_low.get_status = i386_linux_dr_get_status;
  i386_dr_low.get_control = i386_linux_dr_get_control;
  i386_set_debug_register_length (4);

  /* Override the default ptrace resume method.  */
  t->to_resume = i386_linux_resume;

  /* Override the GNU/Linux inferior startup hook.  */
  super_post_startup_inferior = t->to_post_startup_inferior;
  t->to_post_startup_inferior = i386_linux_child_post_startup_inferior;

  /* Add our register access methods.  */
  t->to_fetch_registers = i386_linux_fetch_inferior_registers;
  t->to_store_registers = i386_linux_store_inferior_registers;

  t->to_read_description = i386_linux_read_description;

  /* Add btrace methods.  */
  t->to_supports_btrace = linux_supports_btrace;
  t->to_enable_btrace = i386_linux_enable_btrace;
  t->to_disable_btrace = i386_linux_disable_btrace;
  t->to_teardown_btrace = i386_linux_teardown_btrace;
  t->to_read_btrace = linux_read_btrace;

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, i386_linux_new_thread);
  linux_nat_set_new_fork (t, i386_linux_new_fork);
  linux_nat_set_forget_process (t, i386_forget_process);
  linux_nat_set_prepare_to_resume (t, i386_linux_prepare_to_resume);
}
