/* Linux-specific ptrace manipulation routines.
   Copyright (C) 2012-2013 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#include "gdb_string.h"
#endif

#include "linux-ptrace.h"
#include "linux-procfs.h"
#include "buffer.h"
#include "gdb_assert.h"
#include "gdb_wait.h"

/* Find all possible reasons we could fail to attach PID and append these
   newline terminated reason strings to initialized BUFFER.  '\0' termination
   of BUFFER must be done by the caller.  */

void
linux_ptrace_attach_warnings (pid_t pid, struct buffer *buffer)
{
  pid_t tracerpid;

  tracerpid = linux_proc_get_tracerpid (pid);
  if (tracerpid > 0)
    buffer_xml_printf (buffer, _("warning: process %d is already traced "
				 "by process %d\n"),
		       (int) pid, (int) tracerpid);

  if (linux_proc_pid_is_zombie (pid))
    buffer_xml_printf (buffer, _("warning: process %d is a zombie "
				 "- the process has already terminated\n"),
		       (int) pid);
}

#if defined __i386__ || defined __x86_64__

/* Address of the 'ret' instruction in asm code block below.  */
extern void (linux_ptrace_test_ret_to_nx_instr) (void);

#include <sys/reg.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdint.h>

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

  return_address = mmap (NULL, 2, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (return_address == MAP_FAILED)
    {
      warning (_("linux_ptrace_test_ret_to_nx: Cannot mmap: %s"),
	       strerror (errno));
      return;
    }

  /* Put there 'int3'.  */
  *return_address = 0xcc;

  child = fork ();
  switch (child)
    {
    case -1:
      warning (_("linux_ptrace_test_ret_to_nx: Cannot fork: %s"),
	       strerror (errno));
      return;

    case 0:
      l = ptrace (PTRACE_TRACEME, 0, NULL, NULL);
      if (l != 0)
	warning (_("linux_ptrace_test_ret_to_nx: Cannot PTRACE_TRACEME: %s"),
		 strerror (errno));
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
	       (long) got_pid, strerror (errno));
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

  errno = 0;
#if defined __i386__
  l = ptrace (PTRACE_PEEKUSER, child, (void *) (uintptr_t) (EIP * 4), NULL);
#elif defined __x86_64__
  l = ptrace (PTRACE_PEEKUSER, child, (void *) (uintptr_t) (RIP * 8), NULL);
#else
# error "!__i386__ && !__x86_64__"
#endif
  if (errno != 0)
    {
      warning (_("linux_ptrace_test_ret_to_nx: Cannot PTRACE_PEEKUSER: %s"),
	       strerror (errno));
      return;
    }
  pc = (void *) (uintptr_t) l;

  kill (child, SIGKILL);
  ptrace (PTRACE_KILL, child, NULL, NULL);

  errno = 0;
  got_pid = waitpid (child, &kill_status, 0);
  if (got_pid != child)
    {
      warning (_("linux_ptrace_test_ret_to_nx: "
		 "PTRACE_KILL waitpid returned %ld: %s"),
	       (long) got_pid, strerror (errno));
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
