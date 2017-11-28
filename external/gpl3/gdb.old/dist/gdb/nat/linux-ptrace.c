/* Linux-specific ptrace manipulation routines.
   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "linux-ptrace.h"
#include "linux-procfs.h"
#include "linux-waitpid.h"
#include "buffer.h"
#include "gdb_wait.h"
#include "gdb_ptrace.h"
#include <sys/procfs.h>

/* Stores the ptrace options supported by the running kernel.
   A value of -1 means we did not check for features yet.  A value
   of 0 means there are no supported features.  */
static int supported_ptrace_options = -1;

/* Find all possible reasons we could fail to attach PID and append
   these as strings to the already initialized BUFFER.  '\0'
   termination of BUFFER must be done by the caller.  */

void
linux_ptrace_attach_fail_reason (pid_t pid, struct buffer *buffer)
{
  pid_t tracerpid;

  tracerpid = linux_proc_get_tracerpid_nowarn (pid);
  if (tracerpid > 0)
    buffer_xml_printf (buffer, _("process %d is already traced "
				 "by process %d"),
		       (int) pid, (int) tracerpid);

  if (linux_proc_pid_is_zombie_nowarn (pid))
    buffer_xml_printf (buffer, _("process %d is a zombie "
				 "- the process has already terminated"),
		       (int) pid);
}

/* See linux-ptrace.h.  */

char *
linux_ptrace_attach_fail_reason_string (ptid_t ptid, int err)
{
  static char *reason_string;
  struct buffer buffer;
  char *warnings;
  long lwpid = ptid_get_lwp (ptid);

  xfree (reason_string);

  buffer_init (&buffer);
  linux_ptrace_attach_fail_reason (lwpid, &buffer);
  buffer_grow_str0 (&buffer, "");
  warnings = buffer_finish (&buffer);
  if (warnings[0] != '\0')
    reason_string = xstrprintf ("%s (%d), %s",
				safe_strerror (err), err, warnings);
  else
    reason_string = xstrprintf ("%s (%d)",
				safe_strerror (err), err);
  xfree (warnings);
  return reason_string;
}

#if defined __i386__ || defined __x86_64__

/* Address of the 'ret' instruction in asm code block below.  */
EXTERN_C void linux_ptrace_test_ret_to_nx_instr (void);

#include <sys/reg.h>
#include <sys/mman.h>
#include <signal.h>

#endif /* defined __i386__ || defined __x86_64__ */

/* Test broken off-trunk Linux kernel patchset for NX support on i386.  It was
   removed in Fedora kernel 88fa1f0332d188795ed73d7ac2b1564e11a0b4cd.

   Test also x86_64 arch for PaX support.  */

static void
linux_ptrace_test_ret_to_nx (void)
{
#if defined __i386__ || defined __x86_64__
  pid_t child, got_pid;
  gdb_byte *return_address, *pc;
  long l;
  int status, kill_status;
  elf_gregset_t regs;

  return_address
    = (gdb_byte *) mmap (NULL, 2, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (return_address == MAP_FAILED)
    {
      warning (_("linux_ptrace_test_ret_to_nx: Cannot mmap: %s"),
	       safe_strerror (errno));
      return;
    }

  /* Put there 'int3'.  */
  *return_address = 0xcc;

  child = fork ();
  switch (child)
    {
    case -1:
      warning (_("linux_ptrace_test_ret_to_nx: Cannot fork: %s"),
	       safe_strerror (errno));
      return;

    case 0:
      l = ptrace (PTRACE_TRACEME, 0, (PTRACE_TYPE_ARG3) NULL,
		  (PTRACE_TYPE_ARG4) NULL);
      if (l != 0)
	warning (_("linux_ptrace_test_ret_to_nx: Cannot PTRACE_TRACEME: %s"),
		 safe_strerror (errno));
      else
	{
#if defined __i386__
	  asm volatile ("pushl %0;"
			".globl linux_ptrace_test_ret_to_nx_instr;"
			"linux_ptrace_test_ret_to_nx_instr:"
			"ret"
			: : "r" (return_address) : "%esp", "memory");
#elif defined __x86_64__
	  asm volatile ("pushq %0;"
			".globl linux_ptrace_test_ret_to_nx_instr;"
			"linux_ptrace_test_ret_to_nx_instr:"
			"ret"
			: : "r" ((uint64_t) (uintptr_t) return_address)
			: "%rsp", "memory");
#else
# error "!__i386__ && !__x86_64__"
#endif
	  gdb_assert_not_reached ("asm block did not terminate");
	}

      _exit (1);
    }

  errno = 0;
  got_pid = waitpid (child, &status, 0);
  if (got_pid != child)
    {
      warning (_("linux_ptrace_test_ret_to_nx: waitpid returned %ld: %s"),
	       (long) got_pid, safe_strerror (errno));
      return;
    }

  if (WIFSIGNALED (status))
    {
      if (WTERMSIG (status) != SIGKILL)
	warning (_("linux_ptrace_test_ret_to_nx: WTERMSIG %d is not SIGKILL!"),
		 (int) WTERMSIG (status));
      else
	warning (_("Cannot call inferior functions, Linux kernel PaX "
		   "protection forbids return to non-executable pages!"));
      return;
    }

  if (!WIFSTOPPED (status))
    {
      warning (_("linux_ptrace_test_ret_to_nx: status %d is not WIFSTOPPED!"),
	       status);
      return;
    }

  /* We may get SIGSEGV due to missing PROT_EXEC of the return_address.  */
  if (WSTOPSIG (status) != SIGTRAP && WSTOPSIG (status) != SIGSEGV)
    {
      warning (_("linux_ptrace_test_ret_to_nx: "
		 "WSTOPSIG %d is neither SIGTRAP nor SIGSEGV!"),
	       (int) WSTOPSIG (status));
      return;
    }

  if (ptrace (PTRACE_GETREGS, child, (PTRACE_TYPE_ARG3) 0,
	      (PTRACE_TYPE_ARG4) &regs) < 0)
    {
      warning (_("linux_ptrace_test_ret_to_nx: Cannot PTRACE_GETREGS: %s"),
	       safe_strerror (errno));
    }
#if defined __i386__
  pc = (gdb_byte *) (uintptr_t) regs[EIP];
#elif defined __x86_64__
  pc = (gdb_byte *) (uintptr_t) regs[RIP];
#else
# error "!__i386__ && !__x86_64__"
#endif

  kill (child, SIGKILL);
  ptrace (PTRACE_KILL, child, (PTRACE_TYPE_ARG3) NULL,
	  (PTRACE_TYPE_ARG4) NULL);

  errno = 0;
  got_pid = waitpid (child, &kill_status, 0);
  if (got_pid != child)
    {
      warning (_("linux_ptrace_test_ret_to_nx: "
		 "PTRACE_KILL waitpid returned %ld: %s"),
	       (long) got_pid, safe_strerror (errno));
      return;
    }
  if (!WIFSIGNALED (kill_status))
    {
      warning (_("linux_ptrace_test_ret_to_nx: "
		 "PTRACE_KILL status %d is not WIFSIGNALED!"),
	       status);
      return;
    }

  /* + 1 is there as x86* stops after the 'int3' instruction.  */
  if (WSTOPSIG (status) == SIGTRAP && pc == return_address + 1)
    {
      /* PASS */
      return;
    }

  /* We may get SIGSEGV due to missing PROT_EXEC of the RETURN_ADDRESS page.  */
  if (WSTOPSIG (status) == SIGSEGV && pc == return_address)
    {
      /* PASS */
      return;
    }

  if ((void (*) (void)) pc != &linux_ptrace_test_ret_to_nx_instr)
    warning (_("linux_ptrace_test_ret_to_nx: PC %p is neither near return "
	       "address %p nor is the return instruction %p!"),
	     pc, return_address, &linux_ptrace_test_ret_to_nx_instr);
  else
    warning (_("Cannot call inferior functions on this system - "
	       "Linux kernel with broken i386 NX (non-executable pages) "
	       "support detected!"));
#endif /* defined __i386__ || defined __x86_64__ */
}

/* Helper function to fork a process and make the child process call
   the function FUNCTION, passing CHILD_STACK as parameter.

   For MMU-less targets, clone is used instead of fork, and
   CHILD_STACK is used as stack space for the cloned child.  If NULL,
   stack space is allocated via malloc (and subsequently passed to
   FUNCTION).  For MMU targets, CHILD_STACK is ignored.  */

static int
linux_fork_to_function (gdb_byte *child_stack, int (*function) (void *))
{
  int child_pid;

  /* Sanity check the function pointer.  */
  gdb_assert (function != NULL);

#if defined(__UCLIBC__) && defined(HAS_NOMMU)
#define STACK_SIZE 4096

    if (child_stack == NULL)
      child_stack = xmalloc (STACK_SIZE * 4);

    /* Use CLONE_VM instead of fork, to support uClinux (no MMU).  */
#ifdef __ia64__
      child_pid = __clone2 (function, child_stack, STACK_SIZE,
			    CLONE_VM | SIGCHLD, child_stack + STACK_SIZE * 2);
#else /* !__ia64__ */
      child_pid = clone (function, child_stack + STACK_SIZE,
			 CLONE_VM | SIGCHLD, child_stack + STACK_SIZE * 2);
#endif /* !__ia64__ */
#else /* !defined(__UCLIBC) && defined(HAS_NOMMU) */
  child_pid = fork ();

  if (child_pid == 0)
    function (NULL);
#endif /* defined(__UCLIBC) && defined(HAS_NOMMU) */

  if (child_pid == -1)
    perror_with_name (("fork"));

  return child_pid;
}

/* A helper function for linux_check_ptrace_features, called after
   the child forks a grandchild.  */

static int
linux_grandchild_function (void *child_stack)
{
  /* Free any allocated stack.  */
  xfree (child_stack);

  /* This code is only reacheable by the grandchild (child's child)
     process.  */
  _exit (0);
}

/* A helper function for linux_check_ptrace_features, called after
   the parent process forks a child.  The child allows itself to
   be traced by its parent.  */

static int
linux_child_function (void *child_stack)
{
  ptrace (PTRACE_TRACEME, 0, (PTRACE_TYPE_ARG3) 0, (PTRACE_TYPE_ARG4) 0);
  kill (getpid (), SIGSTOP);

  /* Fork a grandchild.  */
  linux_fork_to_function ((gdb_byte *) child_stack, linux_grandchild_function);

  /* This code is only reacheable by the child (grandchild's parent)
     process.  */
  _exit (0);
}

static void linux_test_for_tracesysgood (int child_pid);
static void linux_test_for_tracefork (int child_pid);
static void linux_test_for_exitkill (int child_pid);

/* Determine ptrace features available on this target.  */

void
linux_check_ptrace_features (void)
{
  int child_pid, ret, status;

  /* Initialize the options.  */
  supported_ptrace_options = 0;

  /* Fork a child so we can do some testing.  The child will call
     linux_child_function and will get traced.  The child will
     eventually fork a grandchild so we can test fork event
     reporting.  */
  child_pid = linux_fork_to_function (NULL, linux_child_function);

  ret = my_waitpid (child_pid, &status, 0);
  if (ret == -1)
    perror_with_name (("waitpid"));
  else if (ret != child_pid)
    error (_("linux_check_ptrace_features: waitpid: unexpected result %d."),
	   ret);
  if (! WIFSTOPPED (status))
    error (_("linux_check_ptrace_features: waitpid: unexpected status %d."),
	   status);

  linux_test_for_tracesysgood (child_pid);

  linux_test_for_tracefork (child_pid);

  linux_test_for_exitkill (child_pid);

  /* Clean things up and kill any pending children.  */
  do
    {
      ret = ptrace (PTRACE_KILL, child_pid, (PTRACE_TYPE_ARG3) 0,
		    (PTRACE_TYPE_ARG4) 0);
      if (ret != 0)
	warning (_("linux_check_ptrace_features: failed to kill child"));
      my_waitpid (child_pid, &status, 0);
    }
  while (WIFSTOPPED (status));
}

/* Determine if PTRACE_O_TRACESYSGOOD can be used to catch
   syscalls.  */

static void
linux_test_for_tracesysgood (int child_pid)
{
  int ret;

  ret = ptrace (PTRACE_SETOPTIONS, child_pid, (PTRACE_TYPE_ARG3) 0,
		(PTRACE_TYPE_ARG4) PTRACE_O_TRACESYSGOOD);

  if (ret == 0)
    supported_ptrace_options |= PTRACE_O_TRACESYSGOOD;
}

/* Determine if PTRACE_O_TRACEFORK can be used to follow fork
   events.  */

static void
linux_test_for_tracefork (int child_pid)
{
  int ret, status;
  long second_pid;

  /* First, set the PTRACE_O_TRACEFORK option.  If this fails, we
     know for sure that it is not supported.  */
  ret = ptrace (PTRACE_SETOPTIONS, child_pid, (PTRACE_TYPE_ARG3) 0,
		(PTRACE_TYPE_ARG4) PTRACE_O_TRACEFORK);

  if (ret != 0)
    return;

  /* Check if the target supports PTRACE_O_TRACEVFORKDONE.  */
  ret = ptrace (PTRACE_SETOPTIONS, child_pid, (PTRACE_TYPE_ARG3) 0,
		(PTRACE_TYPE_ARG4) (PTRACE_O_TRACEFORK
				    | PTRACE_O_TRACEVFORKDONE));
  if (ret == 0)
    supported_ptrace_options |= PTRACE_O_TRACEVFORKDONE;

  /* Setting PTRACE_O_TRACEFORK did not cause an error, however we
     don't know for sure that the feature is available; old
     versions of PTRACE_SETOPTIONS ignored unknown options.
     Therefore, we attach to the child process, use PTRACE_SETOPTIONS
     to enable fork tracing, and let it fork.  If the process exits,
     we assume that we can't use PTRACE_O_TRACEFORK; if we get the
     fork notification, and we can extract the new child's PID, then
     we assume that we can.

     We do not explicitly check for vfork tracing here.  It is
     assumed that vfork tracing is available whenever fork tracing
     is available.  */
  ret = ptrace (PTRACE_CONT, child_pid, (PTRACE_TYPE_ARG3) 0,
		(PTRACE_TYPE_ARG4) 0);
  if (ret != 0)
    warning (_("linux_test_for_tracefork: failed to resume child"));

  ret = my_waitpid (child_pid, &status, 0);

  /* Check if we received a fork event notification.  */
  if (ret == child_pid && WIFSTOPPED (status)
      && linux_ptrace_get_extended_event (status) == PTRACE_EVENT_FORK)
    {
      /* We did receive a fork event notification.  Make sure its PID
	 is reported.  */
      second_pid = 0;
      ret = ptrace (PTRACE_GETEVENTMSG, child_pid, (PTRACE_TYPE_ARG3) 0,
		    (PTRACE_TYPE_ARG4) &second_pid);
      if (ret == 0 && second_pid != 0)
	{
	  int second_status;

	  /* We got the PID from the grandchild, which means fork
	     tracing is supported.  */
	  supported_ptrace_options |= PTRACE_O_TRACECLONE;
	  supported_ptrace_options |= (PTRACE_O_TRACEFORK
				       | PTRACE_O_TRACEVFORK
				       | PTRACE_O_TRACEEXEC);

	  /* Do some cleanup and kill the grandchild.  */
	  my_waitpid (second_pid, &second_status, 0);
	  ret = ptrace (PTRACE_KILL, second_pid, (PTRACE_TYPE_ARG3) 0,
			(PTRACE_TYPE_ARG4) 0);
	  if (ret != 0)
	    warning (_("linux_test_for_tracefork: "
		       "failed to kill second child"));
	  my_waitpid (second_pid, &status, 0);
	}
    }
  else
    warning (_("linux_test_for_tracefork: unexpected result from waitpid "
	     "(%d, status 0x%x)"), ret, status);
}

/* Determine if PTRACE_O_EXITKILL can be used.  */

static void
linux_test_for_exitkill (int child_pid)
{
  int ret;

  ret = ptrace (PTRACE_SETOPTIONS, child_pid, (PTRACE_TYPE_ARG3) 0,
		(PTRACE_TYPE_ARG4) PTRACE_O_EXITKILL);

  if (ret == 0)
    supported_ptrace_options |= PTRACE_O_EXITKILL;
}

/* Enable reporting of all currently supported ptrace events.
   OPTIONS is a bit mask of extended features we want enabled,
   if supported by the kernel.  PTRACE_O_TRACECLONE is always
   enabled, if supported.  */

void
linux_enable_event_reporting (pid_t pid, int options)
{
  /* Check if we have initialized the ptrace features for this
     target.  If not, do it now.  */
  if (supported_ptrace_options == -1)
    linux_check_ptrace_features ();

  /* We always want clone events.  */
  options |= PTRACE_O_TRACECLONE;

  /* Filter out unsupported options.  */
  options &= supported_ptrace_options;

  /* Set the options.  */
  ptrace (PTRACE_SETOPTIONS, pid, (PTRACE_TYPE_ARG3) 0,
	  (PTRACE_TYPE_ARG4) (uintptr_t) options);
}

/* Disable reporting of all currently supported ptrace events.  */

void
linux_disable_event_reporting (pid_t pid)
{
  /* Set the options.  */
  ptrace (PTRACE_SETOPTIONS, pid, (PTRACE_TYPE_ARG3) 0, 0);
}

/* Returns non-zero if PTRACE_OPTIONS is contained within
   SUPPORTED_PTRACE_OPTIONS, therefore supported.  Returns 0
   otherwise.  */

static int
ptrace_supports_feature (int ptrace_options)
{
  if (supported_ptrace_options == -1)
    linux_check_ptrace_features ();

  return ((supported_ptrace_options & ptrace_options) == ptrace_options);
}

/* Returns non-zero if PTRACE_EVENT_FORK is supported by ptrace,
   0 otherwise.  Note that if PTRACE_EVENT_FORK is supported so is
   PTRACE_EVENT_CLONE, PTRACE_EVENT_EXEC and PTRACE_EVENT_VFORK,
   since they were all added to the kernel at the same time.  */

int
linux_supports_tracefork (void)
{
  return ptrace_supports_feature (PTRACE_O_TRACEFORK);
}

/* Returns non-zero if PTRACE_EVENT_EXEC is supported by ptrace,
   0 otherwise.  Note that if PTRACE_EVENT_FORK is supported so is
   PTRACE_EVENT_CLONE, PTRACE_EVENT_FORK and PTRACE_EVENT_VFORK,
   since they were all added to the kernel at the same time.  */

int
linux_supports_traceexec (void)
{
  return ptrace_supports_feature (PTRACE_O_TRACEEXEC);
}

/* Returns non-zero if PTRACE_EVENT_CLONE is supported by ptrace,
   0 otherwise.  Note that if PTRACE_EVENT_CLONE is supported so is
   PTRACE_EVENT_FORK, PTRACE_EVENT_EXEC and PTRACE_EVENT_VFORK,
   since they were all added to the kernel at the same time.  */

int
linux_supports_traceclone (void)
{
  return ptrace_supports_feature (PTRACE_O_TRACECLONE);
}

/* Returns non-zero if PTRACE_O_TRACEVFORKDONE is supported by
   ptrace, 0 otherwise.  */

int
linux_supports_tracevforkdone (void)
{
  return ptrace_supports_feature (PTRACE_O_TRACEVFORKDONE);
}

/* Returns non-zero if PTRACE_O_TRACESYSGOOD is supported by ptrace,
   0 otherwise.  */

int
linux_supports_tracesysgood (void)
{
  return ptrace_supports_feature (PTRACE_O_TRACESYSGOOD);
}

/* Display possible problems on this system.  Display them only once per GDB
   execution.  */

void
linux_ptrace_init_warnings (void)
{
  static int warned = 0;

  if (warned)
    return;
  warned = 1;

  linux_ptrace_test_ret_to_nx ();
}

/* Extract extended ptrace event from wait status.  */

int
linux_ptrace_get_extended_event (int wstat)
{
  return (wstat >> 16);
}

/* Determine whether wait status denotes an extended event.  */

int
linux_is_extended_waitstatus (int wstat)
{
  return (linux_ptrace_get_extended_event (wstat) != 0);
}

/* Return true if the event in LP may be caused by breakpoint.  */

int
linux_wstatus_maybe_breakpoint (int wstat)
{
  return (WIFSTOPPED (wstat)
	  && (WSTOPSIG (wstat) == SIGTRAP
	      /* SIGILL and SIGSEGV are also treated as traps in case a
		 breakpoint is inserted at the current PC.  */
	      || WSTOPSIG (wstat) == SIGILL
	      || WSTOPSIG (wstat) == SIGSEGV));
}
