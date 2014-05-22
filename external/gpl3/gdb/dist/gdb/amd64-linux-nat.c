/* Native-dependent code for GNU/Linux x86-64.

   Copyright (C) 2001-2013 Free Software Foundation, Inc.
   Contributed by Jiri Smid, SuSE Labs.

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
#include "gdbcore.h"
#include "regcache.h"
#include "regset.h"
#include "linux-nat.h"
#include "amd64-linux-tdep.h"
#include "linux-btrace.h"
#include "btrace.h"

#include "gdb_assert.h"
#include "gdb_string.h"
#include "elf/common.h"
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <sys/debugreg.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
#include <sys/user.h>
#include <asm/prctl.h>
/* FIXME ezannoni-2003-07-09: we need <sys/reg.h> to be included after
   <asm/ptrace.h> because the latter redefines FS and GS for no apparent
   reason, and those definitions don't match the ones that libpthread_db
   uses, which come from <sys/reg.h>.  */
/* ezannoni-2003-07-09: I think this is fixed.  The extraneous defs have
   been removed from ptrace.h in the kernel.  However, better safe than
   sorry.  */
#include <asm/ptrace.h>
#include <sys/reg.h>
#include "gdb_proc_service.h"

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

#include "amd64-tdep.h"
#include "i386-linux-tdep.h"
#include "amd64-nat.h"
#include "i386-nat.h"
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

/* Mapping between the general-purpose registers in GNU/Linux x86-64
   `struct user' format and GDB's register cache layout for GNU/Linux
   i386.

   Note that most GNU/Linux x86-64 registers are 64-bit, while the
   GNU/Linux i386 registers are all 32-bit, but since we're
   little-endian we get away with that.  */

/* From <sys/reg.h> on GNU/Linux i386.  */
static int amd64_linux_gregset32_reg_offset[] =
{
  RAX * 8, RCX * 8,		/* %eax, %ecx */
  RDX * 8, RBX * 8,		/* %edx, %ebx */
  RSP * 8, RBP * 8,		/* %esp, %ebp */
  RSI * 8, RDI * 8,		/* %esi, %edi */
  RIP * 8, EFLAGS * 8,		/* %eip, %eflags */
  CS * 8, SS * 8,		/* %cs, %ss */
  DS * 8, ES * 8,		/* %ds, %es */
  FS * 8, GS * 8,		/* %fs, %gs */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  ORIG_RAX * 8			/* "orig_eax" */
};


/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* Fill GDB's register cache with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const elf_gregset_t *gregsetp)
{
  amd64_supply_native_gregset (regcache, gregsetp, -1);
}

/* Fill register REGNUM (if it is a general-purpose register) in
   *GREGSETP with the value in GDB's register cache.  If REGNUM is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      elf_gregset_t *gregsetp, int regnum)
{
  amd64_collect_native_gregset (regcache, gregsetp, regnum);
}

/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* Fill GDB's register cache with the floating-point and SSE register
   values in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const elf_fpregset_t *fpregsetp)
{
  amd64_supply_fxsave (regcache, -1, fpregsetp);
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FPREGSETP with the value in GDB's register cache.  If REGNUM is
   -1, do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       elf_fpregset_t *fpregsetp, int regnum)
{
  amd64_collect_fxsave (regcache, regnum, fpregsetp);
}


/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNUM from the child process.  If REGNUM is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

static void
amd64_linux_fetch_inferior_registers (struct target_ops *ops,
				      struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int tid;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid); /* Not a threaded program.  */

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_gregset_t regs;

      if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
	perror_with_name (_("Couldn't get registers"));

      amd64_supply_native_gregset (regcache, &regs, -1);
      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_fpregset_t fpregs;

      if (have_ptrace_getregset)
	{
	  char xstateregs[I386_XSTATE_MAX_SIZE];
	  struct iovec iov;

	  iov.iov_base = xstateregs;
	  iov.iov_len = sizeof (xstateregs);
	  if (ptrace (PTRACE_GETREGSET, tid,
		      (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_supply_xsave (regcache, -1, xstateregs);
	}
      else
	{
	  if (ptrace (PTRACE_GETFPREGS, tid, 0, (long) &fpregs) < 0)
	    perror_with_name (_("Couldn't get floating point status"));

	  amd64_supply_fxsave (regcache, -1, &fpregs);
	}
    }
}

/* Store register REGNUM back into the child process.  If REGNUM is
   -1, do this for all registers (including the floating-point and SSE
   registers).  */

static void
amd64_linux_store_inferior_registers (struct target_ops *ops,
				      struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  int tid;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid); /* Not a threaded program.  */

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_gregset_t regs;

      if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
	perror_with_name (_("Couldn't get registers"));

      amd64_collect_native_gregset (regcache, &regs, regnum);

      if (ptrace (PTRACE_SETREGS, tid, 0, (long) &regs) < 0)
	perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_fpregset_t fpregs;

      if (have_ptrace_getregset)
	{
	  char xstateregs[I386_XSTATE_MAX_SIZE];
	  struct iovec iov;

	  iov.iov_base = xstateregs;
	  iov.iov_len = sizeof (xstateregs);
	  if (ptrace (PTRACE_GETREGSET, tid,
		      (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_collect_xsave (regcache, regnum, xstateregs, 0);

	  if (ptrace (PTRACE_SETREGSET, tid,
		      (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	    perror_with_name (_("Couldn't write extended state status"));
	}
      else
	{
	  if (ptrace (PTRACE_GETFPREGS, tid, 0, (long) &fpregs) < 0)
	    perror_with_name (_("Couldn't get floating point status"));

	  amd64_collect_fxsave (regcache, regnum, &fpregs);

	  if (ptrace (PTRACE_SETFPREGS, tid, 0, (long) &fpregs) < 0)
	    perror_with_name (_("Couldn't write floating point status"));
	}
    }
}

/* Support for debug registers.  */

static unsigned long
amd64_linux_dr_get (ptid_t ptid, int regnum)
{
  int tid;
  unsigned long value;

  tid = TIDGET (ptid);
  if (tid == 0)
    tid = PIDGET (ptid);

  errno = 0;
  value = ptrace (PTRACE_PEEKUSER, tid,
		  offsetof (struct user, u_debugreg[regnum]), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't read debug register"));

  return value;
}

/* Set debug register REGNUM to VALUE in only the one LWP of PTID.  */

static void
amd64_linux_dr_set (ptid_t ptid, int regnum, unsigned long value)
{
  int tid;

  tid = TIDGET (ptid);
  if (tid == 0)
    tid = PIDGET (ptid);

  errno = 0;
  ptrace (PTRACE_POKEUSER, tid,
	  offsetof (struct user, u_debugreg[regnum]), value);
  if (errno != 0)
    perror_with_name (_("Couldn't write debug register"));
}

/* Return the inferior's debug register REGNUM.  */

static CORE_ADDR
amd64_linux_dr_get_addr (int regnum)
{
  /* DR6 and DR7 are retrieved with some other way.  */
  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  return amd64_linux_dr_get (inferior_ptid, regnum);
}

/* Return the inferior's DR7 debug control register.  */

static unsigned long
amd64_linux_dr_get_control (void)
{
  return amd64_linux_dr_get (inferior_ptid, DR_CONTROL);
}

/* Get DR_STATUS from only the one LWP of INFERIOR_PTID.  */

static unsigned long
amd64_linux_dr_get_status (void)
{
  return amd64_linux_dr_get (inferior_ptid, DR_STATUS);
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

/* Set DR_CONTROL to CONTROL in all LWPs of the current inferior.  */

static void
amd64_linux_dr_set_control (unsigned long control)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  iterate_over_lwps (pid_ptid, update_debug_registers_callback, NULL);
}

/* Set address REGNUM (zero based) to ADDR in all LWPs of the current
   inferior.  */

static void
amd64_linux_dr_set_addr (int regnum, CORE_ADDR addr)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);

  iterate_over_lwps (pid_ptid, update_debug_registers_callback, NULL);
}

/* Called when resuming a thread.
   If the debug regs have changed, update the thread's copies.  */

static void
amd64_linux_prepare_to_resume (struct lwp_info *lwp)
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

      /* On Linux kernel before 2.6.33 commit
	 72f674d203cd230426437cdcf7dd6f681dad8b0d
	 if you enable a breakpoint by the DR_CONTROL bits you need to have
	 already written the corresponding DR_FIRSTADDR...DR_LASTADDR registers.

	 Ensure DR_CONTROL gets written as the very last register here.  */

      for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	if (state->dr_ref_count[i] > 0)
	  {
	    amd64_linux_dr_set (lwp->ptid, i, state->dr_mirror[i]);

	    /* If we're setting a watchpoint, any change the inferior
	       had done itself to the debug registers needs to be
	       discarded, otherwise, i386_stopped_data_address can get
	       confused.  */
	    clear_status = 1;
	  }

      amd64_linux_dr_set (lwp->ptid, DR_CONTROL, state->dr_control_mirror);

      lwp->arch_private->debug_registers_changed = 0;
    }

  if (clear_status || lwp->stopped_by_watchpoint)
    amd64_linux_dr_set (lwp->ptid, DR_STATUS, 0);
}

static void
amd64_linux_new_thread (struct lwp_info *lp)
{
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  info->debug_registers_changed = 1;

  lp->arch_private = info;
}

/* linux_nat_new_fork hook.   */

static void
amd64_linux_new_fork (struct lwp_info *parent, pid_t child_pid)
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



/* This function is called by libthread_db as part of its handling of
   a request for a thread's local storage address.  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
                    lwpid_t lwpid, int idx, void **base)
{
  if (gdbarch_bfd_arch_info (target_gdbarch ())->bits_per_word == 32)
    {
      /* The full structure is found in <asm-i386/ldt.h>.  The second
	 integer is the LDT's base_address and that is used to locate
	 the thread's local storage.  See i386-linux-nat.c more
	 info.  */
      unsigned int desc[4];

      /* This code assumes that "int" is 32 bits and that
	 GET_THREAD_AREA returns no more than 4 int values.  */
      gdb_assert (sizeof (int) == 4);	
#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif
      if  (ptrace (PTRACE_GET_THREAD_AREA, 
		   lwpid, (void *) (long) idx, (unsigned long) &desc) < 0)
	return PS_ERR;
      
      /* Extend the value to 64 bits.  Here it's assumed that a "long"
	 and a "void *" are the same.  */
      (*base) = (void *) (long) desc[1];
      return PS_OK;
    }
  else
    {
      /* This definition comes from prctl.h, but some kernels may not
         have it.  */
#ifndef PTRACE_ARCH_PRCTL
#define PTRACE_ARCH_PRCTL      30
#endif
      /* FIXME: ezannoni-2003-07-09 see comment above about include
	 file order.  We could be getting bogus values for these two.  */
      gdb_assert (FS < ELF_NGREG);
      gdb_assert (GS < ELF_NGREG);
      switch (idx)
	{
	case FS:
#ifdef HAVE_STRUCT_USER_REGS_STRUCT_FS_BASE
	    {
	      /* PTRACE_ARCH_PRCTL is obsolete since 2.6.25, where the
		 fs_base and gs_base fields of user_regs_struct can be
		 used directly.  */
	      unsigned long fs;
	      errno = 0;
	      fs = ptrace (PTRACE_PEEKUSER, lwpid,
			   offsetof (struct user_regs_struct, fs_base), 0);
	      if (errno == 0)
		{
		  *base = (void *) fs;
		  return PS_OK;
		}
	    }
#endif
	  if (ptrace (PTRACE_ARCH_PRCTL, lwpid, base, ARCH_GET_FS) == 0)
	    return PS_OK;
	  break;
	case GS:
#ifdef HAVE_STRUCT_USER_REGS_STRUCT_GS_BASE
	    {
	      unsigned long gs;
	      errno = 0;
	      gs = ptrace (PTRACE_PEEKUSER, lwpid,
			   offsetof (struct user_regs_struct, gs_base), 0);
	      if (errno == 0)
		{
		  *base = (void *) gs;
		  return PS_OK;
		}
	    }
#endif
	  if (ptrace (PTRACE_ARCH_PRCTL, lwpid, base, ARCH_GET_GS) == 0)
	    return PS_OK;
	  break;
	default:                   /* Should not happen.  */
	  return PS_BADADDR;
	}
    }
  return PS_ERR;               /* ptrace failed.  */
}


static void (*super_post_startup_inferior) (ptid_t ptid);

static void
amd64_linux_child_post_startup_inferior (ptid_t ptid)
{
  i386_cleanup_dregs ();
  super_post_startup_inferior (ptid);
}


/* When GDB is built as a 64-bit application on linux, the
   PTRACE_GETSIGINFO data is always presented in 64-bit layout.  Since
   debugging a 32-bit inferior with a 64-bit GDB should look the same
   as debugging it with a 32-bit GDB, we do the 32-bit <-> 64-bit
   conversion in-place ourselves.  */

/* These types below (compat_*) define a siginfo type that is layout
   compatible with the siginfo type exported by the 32-bit userspace
   support.  */

typedef int compat_int_t;
typedef unsigned int compat_uptr_t;

typedef int compat_time_t;
typedef int compat_timer_t;
typedef int compat_clock_t;

struct compat_timeval
{
  compat_time_t tv_sec;
  int tv_usec;
};

typedef union compat_sigval
{
  compat_int_t sival_int;
  compat_uptr_t sival_ptr;
} compat_sigval_t;

typedef struct compat_siginfo
{
  int si_signo;
  int si_errno;
  int si_code;

  union
  {
    int _pad[((128 / sizeof (int)) - 3)];

    /* kill() */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
    } _kill;

    /* POSIX.1b timers */
    struct
    {
      compat_timer_t _tid;
      int _overrun;
      compat_sigval_t _sigval;
    } _timer;

    /* POSIX.1b signals */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      compat_sigval_t _sigval;
    } _rt;

    /* SIGCHLD */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      int _status;
      compat_clock_t _utime;
      compat_clock_t _stime;
    } _sigchld;

    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
    struct
    {
      unsigned int _addr;
    } _sigfault;

    /* SIGPOLL */
    struct
    {
      int _band;
      int _fd;
    } _sigpoll;
  } _sifields;
} compat_siginfo_t;

/* For x32, clock_t in _sigchld is 64bit aligned at 4 bytes.  */
typedef struct compat_x32_clock
{
  int lower;
  int upper;
} compat_x32_clock_t;

typedef struct compat_x32_siginfo
{
  int si_signo;
  int si_errno;
  int si_code;

  union
  {
    int _pad[((128 / sizeof (int)) - 3)];

    /* kill() */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
    } _kill;

    /* POSIX.1b timers */
    struct
    {
      compat_timer_t _tid;
      int _overrun;
      compat_sigval_t _sigval;
    } _timer;

    /* POSIX.1b signals */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      compat_sigval_t _sigval;
    } _rt;

    /* SIGCHLD */
    struct
    {
      unsigned int _pid;
      unsigned int _uid;
      int _status;
      compat_x32_clock_t _utime;
      compat_x32_clock_t _stime;
    } _sigchld;

    /* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
    struct
    {
      unsigned int _addr;
    } _sigfault;

    /* SIGPOLL */
    struct
    {
      int _band;
      int _fd;
    } _sigpoll;
  } _sifields;
} compat_x32_siginfo_t;

#define cpt_si_pid _sifields._kill._pid
#define cpt_si_uid _sifields._kill._uid
#define cpt_si_timerid _sifields._timer._tid
#define cpt_si_overrun _sifields._timer._overrun
#define cpt_si_status _sifields._sigchld._status
#define cpt_si_utime _sifields._sigchld._utime
#define cpt_si_stime _sifields._sigchld._stime
#define cpt_si_ptr _sifields._rt._sigval.sival_ptr
#define cpt_si_addr _sifields._sigfault._addr
#define cpt_si_band _sifields._sigpoll._band
#define cpt_si_fd _sifields._sigpoll._fd

/* glibc at least up to 2.3.2 doesn't have si_timerid, si_overrun.
   In their place is si_timer1,si_timer2.  */
#ifndef si_timerid
#define si_timerid si_timer1
#endif
#ifndef si_overrun
#define si_overrun si_timer2
#endif

static void
compat_siginfo_from_siginfo (compat_siginfo_t *to, siginfo_t *from)
{
  memset (to, 0, sizeof (*to));

  to->si_signo = from->si_signo;
  to->si_errno = from->si_errno;
  to->si_code = from->si_code;

  if (to->si_code == SI_TIMER)
    {
      to->cpt_si_timerid = from->si_timerid;
      to->cpt_si_overrun = from->si_overrun;
      to->cpt_si_ptr = (intptr_t) from->si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->cpt_si_pid = from->si_pid;
      to->cpt_si_uid = from->si_uid;
    }
  else if (to->si_code < 0)
    {
      to->cpt_si_pid = from->si_pid;
      to->cpt_si_uid = from->si_uid;
      to->cpt_si_ptr = (intptr_t) from->si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->cpt_si_pid = from->si_pid;
	  to->cpt_si_uid = from->si_uid;
	  to->cpt_si_status = from->si_status;
	  to->cpt_si_utime = from->si_utime;
	  to->cpt_si_stime = from->si_stime;
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->cpt_si_addr = (intptr_t) from->si_addr;
	  break;
	case SIGPOLL:
	  to->cpt_si_band = from->si_band;
	  to->cpt_si_fd = from->si_fd;
	  break;
	default:
	  to->cpt_si_pid = from->si_pid;
	  to->cpt_si_uid = from->si_uid;
	  to->cpt_si_ptr = (intptr_t) from->si_ptr;
	  break;
	}
    }
}

static void
siginfo_from_compat_siginfo (siginfo_t *to, compat_siginfo_t *from)
{
  memset (to, 0, sizeof (*to));

  to->si_signo = from->si_signo;
  to->si_errno = from->si_errno;
  to->si_code = from->si_code;

  if (to->si_code == SI_TIMER)
    {
      to->si_timerid = from->cpt_si_timerid;
      to->si_overrun = from->cpt_si_overrun;
      to->si_ptr = (void *) (intptr_t) from->cpt_si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->si_pid = from->cpt_si_pid;
      to->si_uid = from->cpt_si_uid;
    }
  if (to->si_code < 0)
    {
      to->si_pid = from->cpt_si_pid;
      to->si_uid = from->cpt_si_uid;
      to->si_ptr = (void *) (intptr_t) from->cpt_si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->si_pid = from->cpt_si_pid;
	  to->si_uid = from->cpt_si_uid;
	  to->si_status = from->cpt_si_status;
	  to->si_utime = from->cpt_si_utime;
	  to->si_stime = from->cpt_si_stime;
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->si_addr = (void *) (intptr_t) from->cpt_si_addr;
	  break;
	case SIGPOLL:
	  to->si_band = from->cpt_si_band;
	  to->si_fd = from->cpt_si_fd;
	  break;
	default:
	  to->si_pid = from->cpt_si_pid;
	  to->si_uid = from->cpt_si_uid;
	  to->si_ptr = (void* ) (intptr_t) from->cpt_si_ptr;
	  break;
	}
    }
}

static void
compat_x32_siginfo_from_siginfo (compat_x32_siginfo_t *to,
				 siginfo_t *from)
{
  memset (to, 0, sizeof (*to));

  to->si_signo = from->si_signo;
  to->si_errno = from->si_errno;
  to->si_code = from->si_code;

  if (to->si_code == SI_TIMER)
    {
      to->cpt_si_timerid = from->si_timerid;
      to->cpt_si_overrun = from->si_overrun;
      to->cpt_si_ptr = (intptr_t) from->si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->cpt_si_pid = from->si_pid;
      to->cpt_si_uid = from->si_uid;
    }
  else if (to->si_code < 0)
    {
      to->cpt_si_pid = from->si_pid;
      to->cpt_si_uid = from->si_uid;
      to->cpt_si_ptr = (intptr_t) from->si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->cpt_si_pid = from->si_pid;
	  to->cpt_si_uid = from->si_uid;
	  to->cpt_si_status = from->si_status;
	  memcpy (&to->cpt_si_utime, &from->si_utime,
		  sizeof (to->cpt_si_utime));
	  memcpy (&to->cpt_si_stime, &from->si_stime,
		  sizeof (to->cpt_si_stime));
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->cpt_si_addr = (intptr_t) from->si_addr;
	  break;
	case SIGPOLL:
	  to->cpt_si_band = from->si_band;
	  to->cpt_si_fd = from->si_fd;
	  break;
	default:
	  to->cpt_si_pid = from->si_pid;
	  to->cpt_si_uid = from->si_uid;
	  to->cpt_si_ptr = (intptr_t) from->si_ptr;
	  break;
	}
    }
}

static void
siginfo_from_compat_x32_siginfo (siginfo_t *to,
				 compat_x32_siginfo_t *from)
{
  memset (to, 0, sizeof (*to));

  to->si_signo = from->si_signo;
  to->si_errno = from->si_errno;
  to->si_code = from->si_code;

  if (to->si_code == SI_TIMER)
    {
      to->si_timerid = from->cpt_si_timerid;
      to->si_overrun = from->cpt_si_overrun;
      to->si_ptr = (void *) (intptr_t) from->cpt_si_ptr;
    }
  else if (to->si_code == SI_USER)
    {
      to->si_pid = from->cpt_si_pid;
      to->si_uid = from->cpt_si_uid;
    }
  if (to->si_code < 0)
    {
      to->si_pid = from->cpt_si_pid;
      to->si_uid = from->cpt_si_uid;
      to->si_ptr = (void *) (intptr_t) from->cpt_si_ptr;
    }
  else
    {
      switch (to->si_signo)
	{
	case SIGCHLD:
	  to->si_pid = from->cpt_si_pid;
	  to->si_uid = from->cpt_si_uid;
	  to->si_status = from->cpt_si_status;
	  memcpy (&to->si_utime, &from->cpt_si_utime,
		  sizeof (to->si_utime));
	  memcpy (&to->si_stime, &from->cpt_si_stime,
		  sizeof (to->si_stime));
	  break;
	case SIGILL:
	case SIGFPE:
	case SIGSEGV:
	case SIGBUS:
	  to->si_addr = (void *) (intptr_t) from->cpt_si_addr;
	  break;
	case SIGPOLL:
	  to->si_band = from->cpt_si_band;
	  to->si_fd = from->cpt_si_fd;
	  break;
	default:
	  to->si_pid = from->cpt_si_pid;
	  to->si_uid = from->cpt_si_uid;
	  to->si_ptr = (void* ) (intptr_t) from->cpt_si_ptr;
	  break;
	}
    }
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to NATIVE.  If DIRECTION is 0, copy from NATIVE to
   INF.  */

static int
amd64_linux_siginfo_fixup (siginfo_t *native, gdb_byte *inf, int direction)
{
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If so, then do fixup the siginfo
     object.  */
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      gdb_assert (sizeof (siginfo_t) == sizeof (compat_siginfo_t));

      if (direction == 0)
	compat_siginfo_from_siginfo ((struct compat_siginfo *) inf, native);
      else
	siginfo_from_compat_siginfo (native, (struct compat_siginfo *) inf);

      return 1;
    }
  /* No fixup for native x32 GDB.  */
  else if (gdbarch_addr_bit (gdbarch) == 32 && sizeof (void *) == 8)
    {
      gdb_assert (sizeof (siginfo_t) == sizeof (compat_x32_siginfo_t));

      if (direction == 0)
	compat_x32_siginfo_from_siginfo ((struct compat_x32_siginfo *) inf,
					 native);
      else
	siginfo_from_compat_x32_siginfo (native,
					 (struct compat_x32_siginfo *) inf);

      return 1;
    }
  else
    return 0;
}

/* Get Linux/x86 target description from running target.

   Value of CS segment register:
     1. 64bit process: 0x33.
     2. 32bit process: 0x23.

   Value of DS segment register:
     1. LP64 process: 0x0.
     2. X32 process: 0x2b.
 */

#define AMD64_LINUX_USER64_CS	0x33
#define AMD64_LINUX_X32_DS	0x2b

static const struct target_desc *
amd64_linux_read_description (struct target_ops *ops)
{
  unsigned long cs;
  unsigned long ds;
  int tid;
  int is_64bit;
  int is_x32;
  static uint64_t xcr0;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid); /* Not a threaded program.  */

  /* Get CS register.  */
  errno = 0;
  cs = ptrace (PTRACE_PEEKUSER, tid,
	       offsetof (struct user_regs_struct, cs), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't get CS register"));

  is_64bit = cs == AMD64_LINUX_USER64_CS;

  /* Get DS register.  */
  errno = 0;
  ds = ptrace (PTRACE_PEEKUSER, tid,
	       offsetof (struct user_regs_struct, ds), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't get DS register"));

  is_x32 = ds == AMD64_LINUX_X32_DS;

  if (sizeof (void *) == 4 && is_64bit && !is_x32)
    error (_("Can't debug 64-bit process with 32-bit GDB"));

  if (have_ptrace_getregset == -1)
    {
      uint64_t xstateregs[(I386_XSTATE_SSE_SIZE / sizeof (uint64_t))];
      struct iovec iov;

      iov.iov_base = xstateregs;
      iov.iov_len = sizeof (xstateregs);

      /* Check if PTRACE_GETREGSET works.  */
      if (ptrace (PTRACE_GETREGSET, tid,
		  (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	have_ptrace_getregset = 0;
      else
	{
	  have_ptrace_getregset = 1;

	  /* Get XCR0 from XSAVE extended state.  */
	  xcr0 = xstateregs[(I386_LINUX_XSAVE_XCR0_OFFSET
			     / sizeof (uint64_t))];
	}
    }

  /* Check the native XCR0 only if PTRACE_GETREGSET is available.  */
  if (have_ptrace_getregset
      && (xcr0 & I386_XSTATE_AVX_MASK) == I386_XSTATE_AVX_MASK)
    {
      if (is_64bit)
	{
	  if (is_x32)
	    return tdesc_x32_avx_linux;
	  else
	    return tdesc_amd64_avx_linux;
	}
      else
	return tdesc_i386_avx_linux;
    }
  else
    {
      if (is_64bit)
	{
	  if (is_x32)
	    return tdesc_x32_linux;
	  else
	    return tdesc_amd64_linux;
	}
      else
	return tdesc_i386_linux;
    }
}

/* Enable branch tracing.  */

static struct btrace_target_info *
amd64_linux_enable_btrace (ptid_t ptid)
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
amd64_linux_disable_btrace (struct btrace_target_info *tinfo)
{
  int errcode = linux_disable_btrace (tinfo);

  if (errcode != 0)
    error (_("Could not disable branch tracing: %s."), safe_strerror (errcode));
}

/* Teardown branch tracing.  */

static void
amd64_linux_teardown_btrace (struct btrace_target_info *tinfo)
{
  /* Ignore errors.  */
  linux_disable_btrace (tinfo);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_amd64_linux_nat (void);

void
_initialize_amd64_linux_nat (void)
{
  struct target_ops *t;

  amd64_native_gregset32_reg_offset = amd64_linux_gregset32_reg_offset;
  amd64_native_gregset32_num_regs = I386_LINUX_NUM_REGS;
  amd64_native_gregset64_reg_offset = amd64_linux_gregset_reg_offset;
  amd64_native_gregset64_num_regs = AMD64_LINUX_NUM_REGS;

  gdb_assert (ARRAY_SIZE (amd64_linux_gregset32_reg_offset)
	      == amd64_native_gregset32_num_regs);

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  i386_use_watchpoints (t);

  i386_dr_low.set_control = amd64_linux_dr_set_control;
  i386_dr_low.set_addr = amd64_linux_dr_set_addr;
  i386_dr_low.get_addr = amd64_linux_dr_get_addr;
  i386_dr_low.get_status = amd64_linux_dr_get_status;
  i386_dr_low.get_control = amd64_linux_dr_get_control;
  i386_set_debug_register_length (8);

  /* Override the GNU/Linux inferior startup hook.  */
  super_post_startup_inferior = t->to_post_startup_inferior;
  t->to_post_startup_inferior = amd64_linux_child_post_startup_inferior;

  /* Add our register access methods.  */
  t->to_fetch_registers = amd64_linux_fetch_inferior_registers;
  t->to_store_registers = amd64_linux_store_inferior_registers;

  t->to_read_description = amd64_linux_read_description;

  /* Add btrace methods.  */
  t->to_supports_btrace = linux_supports_btrace;
  t->to_enable_btrace = amd64_linux_enable_btrace;
  t->to_disable_btrace = amd64_linux_disable_btrace;
  t->to_teardown_btrace = amd64_linux_teardown_btrace;
  t->to_read_btrace = linux_read_btrace;

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, amd64_linux_new_thread);
  linux_nat_set_new_fork (t, amd64_linux_new_fork);
  linux_nat_set_forget_process (t, i386_forget_process);
  linux_nat_set_siginfo_fixup (t, amd64_linux_siginfo_fixup);
  linux_nat_set_prepare_to_resume (t, amd64_linux_prepare_to_resume);
}
