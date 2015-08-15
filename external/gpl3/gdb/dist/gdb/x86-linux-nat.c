/* Native-dependent code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 1999-2015 Free Software Foundation, Inc.

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
#include "elf/common.h"
#include "gdb_proc_service.h"
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/procfs.h>
#include <sys/uio.h>

#include "x86-nat.h"
#include "linux-nat.h"
#ifndef __x86_64__
#include "i386-linux-nat.h"
#endif
#include "x86-linux-nat.h"
#include "i386-linux-tdep.h"
#ifdef __x86_64__
#include "amd64-linux-tdep.h"
#endif
#include "x86-xstate.h"
#include "nat/linux-btrace.h"

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the thread.  */
  int debug_registers_changed;
};

/* Does the current host support PTRACE_GETREGSET?  */
int have_ptrace_getregset = -1;


/* Support for debug registers.  */

/* Get debug register REGNUM value from only the one LWP of PTID.  */

static unsigned long
x86_linux_dr_get (ptid_t ptid, int regnum)
{
  int tid;
  unsigned long value;

  gdb_assert (ptid_lwp_p (ptid));
  tid = ptid_get_lwp (ptid);

  errno = 0;
  value = ptrace (PTRACE_PEEKUSER, tid,
		  offsetof (struct user, u_debugreg[regnum]), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't read debug register"));

  return value;
}

/* Set debug register REGNUM to VALUE in only the one LWP of PTID.  */

static void
x86_linux_dr_set (ptid_t ptid, int regnum, unsigned long value)
{
  int tid;

  gdb_assert (ptid_lwp_p (ptid));
  tid = ptid_get_lwp (ptid);

  errno = 0;
  ptrace (PTRACE_POKEUSER, tid,
	  offsetof (struct user, u_debugreg[regnum]), value);
  if (errno != 0)
    perror_with_name (_("Couldn't write debug register"));
}

/* Return the inferior's debug register REGNUM.  */

static CORE_ADDR
x86_linux_dr_get_addr (int regnum)
{
  /* DR6 and DR7 are retrieved with some other way.  */
  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  return x86_linux_dr_get (inferior_ptid, regnum);
}

/* Return the inferior's DR7 debug control register.  */

static unsigned long
x86_linux_dr_get_control (void)
{
  return x86_linux_dr_get (inferior_ptid, DR_CONTROL);
}

/* Get DR_STATUS from only the one LWP of INFERIOR_PTID.  */

static unsigned long
x86_linux_dr_get_status (void)
{
  return x86_linux_dr_get (inferior_ptid, DR_STATUS);
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
x86_linux_dr_set_control (unsigned long control)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  iterate_over_lwps (pid_ptid, update_debug_registers_callback, NULL);
}

/* Set address REGNUM (zero based) to ADDR in all LWPs of the current
   inferior.  */

static void
x86_linux_dr_set_addr (int regnum, CORE_ADDR addr)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);

  iterate_over_lwps (pid_ptid, update_debug_registers_callback, NULL);
}

/* Called when resuming a thread.
   If the debug regs have changed, update the thread's copies.  */

static void
x86_linux_prepare_to_resume (struct lwp_info *lwp)
{
  int clear_status = 0;

  /* NULL means this is the main thread still going through the shell,
     or, no watchpoint has been set yet.  In that case, there's
     nothing to do.  */
  if (lwp->arch_private == NULL)
    return;

  if (lwp->arch_private->debug_registers_changed)
    {
      struct x86_debug_reg_state *state
	= x86_debug_reg_state (ptid_get_pid (lwp->ptid));
      int i;

      /* On Linux kernel before 2.6.33 commit
	 72f674d203cd230426437cdcf7dd6f681dad8b0d
	 if you enable a breakpoint by the DR_CONTROL bits you need to have
	 already written the corresponding DR_FIRSTADDR...DR_LASTADDR registers.

	 Ensure DR_CONTROL gets written as the very last register here.  */

      /* Clear DR_CONTROL first.  In some cases, setting DR0-3 to a
	 value that doesn't match what is enabled in DR_CONTROL
	 results in EINVAL.  */
      x86_linux_dr_set (lwp->ptid, DR_CONTROL, 0);

      ALL_DEBUG_ADDRESS_REGISTERS (i)
	if (state->dr_ref_count[i] > 0)
	  {
	    x86_linux_dr_set (lwp->ptid, i, state->dr_mirror[i]);

	    /* If we're setting a watchpoint, any change the inferior
	       had done itself to the debug registers needs to be
	       discarded, otherwise, x86_stopped_data_address can get
	       confused.  */
	    clear_status = 1;
	  }

      /* If DR_CONTROL is supposed to be zero, we've already set it
	 above.  */
      if (state->dr_control_mirror != 0)
	x86_linux_dr_set (lwp->ptid, DR_CONTROL, state->dr_control_mirror);

      lwp->arch_private->debug_registers_changed = 0;
    }

  if (clear_status || lwp->stop_reason == LWP_STOPPED_BY_WATCHPOINT)
    x86_linux_dr_set (lwp->ptid, DR_STATUS, 0);
}

static void
x86_linux_new_thread (struct lwp_info *lp)
{
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  info->debug_registers_changed = 1;

  lp->arch_private = info;
}


/* linux_nat_new_fork hook.   */

static void
x86_linux_new_fork (struct lwp_info *parent, pid_t child_pid)
{
  pid_t parent_pid;
  struct x86_debug_reg_state *parent_state;
  struct x86_debug_reg_state *child_state;

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
  parent_state = x86_debug_reg_state (parent_pid);
  child_state = x86_debug_reg_state (child_pid);
  *child_state = *parent_state;
}


static void (*super_post_startup_inferior) (struct target_ops *self,
					    ptid_t ptid);

static void
x86_linux_child_post_startup_inferior (struct target_ops *self, ptid_t ptid)
{
  x86_cleanup_dregs ();
  super_post_startup_inferior (self, ptid);
}

#ifdef __x86_64__
/* Value of CS segment register:
     64bit process: 0x33
     32bit process: 0x23  */
#define AMD64_LINUX_USER64_CS 0x33

/* Value of DS segment register:
     LP64 process: 0x0
     X32 process: 0x2b  */
#define AMD64_LINUX_X32_DS 0x2b
#endif

/* Get Linux/x86 target description from running target.  */

static const struct target_desc *
x86_linux_read_description (struct target_ops *ops)
{
  int tid;
  int is_64bit = 0;
#ifdef __x86_64__
  int is_x32;
#endif
  static uint64_t xcr0;
  uint64_t xcr0_features_bits;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

#ifdef __x86_64__
  {
    unsigned long cs;
    unsigned long ds;

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
  }
#elif HAVE_PTRACE_GETFPXREGS
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
      uint64_t xstateregs[(X86_XSTATE_SSE_SIZE / sizeof (uint64_t))];
      struct iovec iov;

      iov.iov_base = xstateregs;
      iov.iov_len = sizeof (xstateregs);

      /* Check if PTRACE_GETREGSET works.  */
      if (ptrace (PTRACE_GETREGSET, tid,
		  (unsigned int) NT_X86_XSTATE, &iov) < 0)
	have_ptrace_getregset = 0;
      else
	{
	  have_ptrace_getregset = 1;

	  /* Get XCR0 from XSAVE extended state.  */
	  xcr0 = xstateregs[(I386_LINUX_XSAVE_XCR0_OFFSET
			     / sizeof (uint64_t))];
	}
    }

  /* Check the native XCR0 only if PTRACE_GETREGSET is available.  If
     PTRACE_GETREGSET is not available then set xcr0_features_bits to
     zero so that the "no-features" descriptions are returned by the
     switches below.  */
  if (have_ptrace_getregset)
    xcr0_features_bits = xcr0 & X86_XSTATE_ALL_MASK;
  else
    xcr0_features_bits = 0;

  if (is_64bit)
    {
#ifdef __x86_64__
      switch (xcr0_features_bits)
	{
	case X86_XSTATE_MPX_AVX512_MASK:
	case X86_XSTATE_AVX512_MASK:
	  if (is_x32)
	    return tdesc_x32_avx512_linux;
	  else
	    return tdesc_amd64_avx512_linux;
	case X86_XSTATE_MPX_MASK:
	  if (is_x32)
	    return tdesc_x32_avx_linux; /* No MPX on x32 using AVX.  */
	  else
	    return tdesc_amd64_mpx_linux;
	case X86_XSTATE_AVX_MASK:
	  if (is_x32)
	    return tdesc_x32_avx_linux;
	  else
	    return tdesc_amd64_avx_linux;
	default:
	  if (is_x32)
	    return tdesc_x32_linux;
	  else
	    return tdesc_amd64_linux;
	}
#endif
    }
  else
    {
      switch (xcr0_features_bits)
	{
	case X86_XSTATE_MPX_AVX512_MASK:
	case X86_XSTATE_AVX512_MASK:
	  return tdesc_i386_avx512_linux;
	case X86_XSTATE_MPX_MASK:
	  return tdesc_i386_mpx_linux;
	case X86_XSTATE_AVX_MASK:
	  return tdesc_i386_avx_linux;
	default:
	  return tdesc_i386_linux;
	}
    }

  gdb_assert_not_reached ("failed to return tdesc");
}


/* Enable branch tracing.  */

static struct btrace_target_info *
x86_linux_enable_btrace (struct target_ops *self, ptid_t ptid)
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
x86_linux_disable_btrace (struct target_ops *self,
			  struct btrace_target_info *tinfo)
{
  enum btrace_error errcode = linux_disable_btrace (tinfo);

  if (errcode != BTRACE_ERR_NONE)
    error (_("Could not disable branch tracing."));
}

/* Teardown branch tracing.  */

static void
x86_linux_teardown_btrace (struct target_ops *self,
			   struct btrace_target_info *tinfo)
{
  /* Ignore errors.  */
  linux_disable_btrace (tinfo);
}

static enum btrace_error
x86_linux_read_btrace (struct target_ops *self,
		       VEC (btrace_block_s) **data,
		       struct btrace_target_info *btinfo,
		       enum btrace_read_type type)
{
  return linux_read_btrace (data, btinfo, type);
}


/* Helper for ps_get_thread_area.  Sets BASE_ADDR to a pointer to
   the thread local storage (or its descriptor) and returns PS_OK
   on success.  Returns PS_ERR on failure.  */

ps_err_e
x86_linux_get_thread_area (pid_t pid, void *addr, unsigned int *base_addr)
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

  /* This code assumes that "int" is 32 bits and that
     GET_THREAD_AREA returns no more than 4 int values.  */
  gdb_assert (sizeof (int) == 4);

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

  if (ptrace (PTRACE_GET_THREAD_AREA, pid, addr, &desc) < 0)
    return PS_ERR;

  *base_addr = desc[1];
  return PS_OK;
}


/* Create an x86 GNU/Linux target.  */

struct target_ops *
x86_linux_create_target (void)
{
  /* Fill in the generic GNU/Linux methods.  */
  struct target_ops *t = linux_target ();

  /* Initialize the debug register function vectors.  */
  x86_use_watchpoints (t);
  x86_dr_low.set_control = x86_linux_dr_set_control;
  x86_dr_low.set_addr = x86_linux_dr_set_addr;
  x86_dr_low.get_addr = x86_linux_dr_get_addr;
  x86_dr_low.get_status = x86_linux_dr_get_status;
  x86_dr_low.get_control = x86_linux_dr_get_control;
  x86_set_debug_register_length (sizeof (void *));

  /* Override the GNU/Linux inferior startup hook.  */
  super_post_startup_inferior = t->to_post_startup_inferior;
  t->to_post_startup_inferior = x86_linux_child_post_startup_inferior;

  /* Add the description reader.  */
  t->to_read_description = x86_linux_read_description;

  /* Add btrace methods.  */
  t->to_supports_btrace = linux_supports_btrace;
  t->to_enable_btrace = x86_linux_enable_btrace;
  t->to_disable_btrace = x86_linux_disable_btrace;
  t->to_teardown_btrace = x86_linux_teardown_btrace;
  t->to_read_btrace = x86_linux_read_btrace;

  return t;
}

/* Add an x86 GNU/Linux target.  */

void
x86_linux_add_target (struct target_ops *t)
{
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, x86_linux_new_thread);
  linux_nat_set_new_fork (t, x86_linux_new_fork);
  linux_nat_set_forget_process (t, x86_forget_process);
  linux_nat_set_prepare_to_resume (t, x86_linux_prepare_to_resume);
}
