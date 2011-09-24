/* Native-dependent code for GNU/Linux x86-64.

   Copyright (C) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
   2011 Free Software Foundation, Inc.
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

#include "gdb_assert.h"
#include "gdb_string.h"
#include "elf/common.h"
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <sys/debugreg.h>
#include <sys/syscall.h>
#include <sys/procfs.h>
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

static unsigned long amd64_linux_dr[DR_CONTROL + 1];

static unsigned long
amd64_linux_dr_get (ptid_t ptid, int regnum)
{
  int tid;
  unsigned long value;

  tid = TIDGET (ptid);
  if (tid == 0)
    tid = PIDGET (ptid);

  /* FIXME: kettenis/2001-03-27: Calling perror_with_name if the
     ptrace call fails breaks debugging remote targets.  The correct
     way to fix this is to add the hardware breakpoint and watchpoint
     stuff to the target vector.  For now, just return zero if the
     ptrace call fails.  */
  errno = 0;
  value = ptrace (PTRACE_PEEKUSER, tid,
		  offsetof (struct user, u_debugreg[regnum]), 0);
  if (errno != 0)
#if 0
    perror_with_name (_("Couldn't read debug register"));
#else
    return 0;
#endif

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

/* Set DR_CONTROL to ADDR in all LWPs of LWP_LIST.  */

static void
amd64_linux_dr_set_control (unsigned long control)
{
  struct lwp_info *lp;
  ptid_t ptid;

  amd64_linux_dr[DR_CONTROL] = control;
  ALL_LWPS (lp, ptid)
    amd64_linux_dr_set (ptid, DR_CONTROL, control);
}

/* Set address REGNUM (zero based) to ADDR in all LWPs of LWP_LIST.  */

static void
amd64_linux_dr_set_addr (int regnum, CORE_ADDR addr)
{
  struct lwp_info *lp;
  ptid_t ptid;

  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);

  amd64_linux_dr[DR_FIRSTADDR + regnum] = addr;
  ALL_LWPS (lp, ptid)
    amd64_linux_dr_set (ptid, DR_FIRSTADDR + regnum, addr);
}

/* Set address REGNUM (zero based) to zero in all LWPs of LWP_LIST.  */

static void
amd64_linux_dr_reset_addr (int regnum)
{
  amd64_linux_dr_set_addr (regnum, 0);
}

/* Get DR_STATUS from only the one LWP of INFERIOR_PTID.  */

static unsigned long
amd64_linux_dr_get_status (void)
{
  return amd64_linux_dr_get (inferior_ptid, DR_STATUS);
}

/* Unset MASK bits in DR_STATUS in all LWPs of LWP_LIST.  */

static void
amd64_linux_dr_unset_status (unsigned long mask)
{
  struct lwp_info *lp;
  ptid_t ptid;

  ALL_LWPS (lp, ptid)
    {
      unsigned long value;
      
      value = amd64_linux_dr_get (ptid, DR_STATUS);
      value &= ~mask;
      amd64_linux_dr_set (ptid, DR_STATUS, value);
    }
}


static void
amd64_linux_new_thread (ptid_t ptid)
{
  int i;

  for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
    amd64_linux_dr_set (ptid, i, amd64_linux_dr[i]);

  amd64_linux_dr_set (ptid, DR_CONTROL, amd64_linux_dr[DR_CONTROL]);
}


/* This function is called by libthread_db as part of its handling of
   a request for a thread's local storage address.  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
                    lwpid_t lwpid, int idx, void **base)
{
  if (gdbarch_ptr_bit (target_gdbarch) == 32)
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
	  if (ptrace (PTRACE_ARCH_PRCTL, lwpid, base, ARCH_GET_FS) == 0)
	    return PS_OK;
	  break;
	case GS:
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

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to NATIVE.  If DIRECTION is 0, copy from NATIVE to
   INF.  */

static int
amd64_linux_siginfo_fixup (struct siginfo *native, gdb_byte *inf, int direction)
{
  /* Is the inferior 32-bit?  If so, then do fixup the siginfo
     object.  */
  if (gdbarch_addr_bit (get_frame_arch (get_current_frame ())) == 32)
    {
      gdb_assert (sizeof (struct siginfo) == sizeof (compat_siginfo_t));

      if (direction == 0)
	compat_siginfo_from_siginfo ((struct compat_siginfo *) inf, native);
      else
	siginfo_from_compat_siginfo (native, (struct compat_siginfo *) inf);

      return 1;
    }
  else
    return 0;
}

/* Get Linux/x86 target description from running target.

   Value of CS segment register:
     1. 64bit process: 0x33.
     2. 32bit process: 0x23.
 */

#define AMD64_LINUX_USER64_CS	0x33

static const struct target_desc *
amd64_linux_read_description (struct target_ops *ops)
{
  unsigned long cs;
  int tid;
  int is_64bit;
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
	return tdesc_amd64_avx_linux;
      else
	return tdesc_i386_avx_linux;
    }
  else
    {
      if (is_64bit)
	return tdesc_amd64_linux;
      else
	return tdesc_i386_linux;
    }
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
  i386_dr_low.reset_addr = amd64_linux_dr_reset_addr;
  i386_dr_low.get_status = amd64_linux_dr_get_status;
  i386_dr_low.unset_status = amd64_linux_dr_unset_status;
  i386_set_debug_register_length (8);

  /* Override the GNU/Linux inferior startup hook.  */
  super_post_startup_inferior = t->to_post_startup_inferior;
  t->to_post_startup_inferior = amd64_linux_child_post_startup_inferior;

  /* Add our register access methods.  */
  t->to_fetch_registers = amd64_linux_fetch_inferior_registers;
  t->to_store_registers = amd64_linux_store_inferior_registers;

  t->to_read_description = amd64_linux_read_description;

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, amd64_linux_new_thread);
  linux_nat_set_siginfo_fixup (t, amd64_linux_siginfo_fixup);
}
