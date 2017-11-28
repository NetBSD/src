/* Low-level child interface to ptrace.

   Copyright (C) 1988-2017 Free Software Foundation, Inc.

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
#include "command.h"
#include "inferior.h"
#include "inflow.h"
#include "terminal.h"
#include "gdbcore.h"
#include "regcache.h"
#include "nat/gdb_ptrace.h"
#include "gdb_wait.h"
#include <signal.h>

#include "inf-ptrace.h"
#include "inf-child.h"
#include "gdbthread.h"



#ifdef PT_GET_PROCESS_STATE

/* Target hook for follow_fork.  On entry and at return inferior_ptid is
   the ptid of the followed inferior.  */

static int
inf_ptrace_follow_fork (struct target_ops *ops, int follow_child,
			int detach_fork)
{
  if (!follow_child)
    {
      struct thread_info *tp = inferior_thread ();
      pid_t child_pid = ptid_get_pid (tp->pending_follow.value.related_pid);

      /* Breakpoints have already been detached from the child by
	 infrun.c.  */

      if (ptrace (PT_DETACH, child_pid, (PTRACE_TYPE_ARG3)1, 0) == -1)
	perror_with_name (("ptrace"));
    }

  return 0;
}

static int
inf_ptrace_insert_fork_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}

static int
inf_ptrace_remove_fork_catchpoint (struct target_ops *self, int pid)
{
  return 0;
}

#endif /* PT_GET_PROCESS_STATE */


/* Prepare to be traced.  */

static void
inf_ptrace_me (void)
{
  /* "Trace me, Dr. Memory!"  */
  if (ptrace (PT_TRACE_ME, 0, (PTRACE_TYPE_ARG3) 0, 0) < 0)
    trace_start_error_with_name ("ptrace");
}

/* Start a new inferior Unix child process.  EXEC_FILE is the file to
   run, ALLARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  If FROM_TTY is non-zero, be
   chatty about it.  */

static void
inf_ptrace_create_inferior (struct target_ops *ops,
			    const char *exec_file, const std::string &allargs,
			    char **env, int from_tty)
{
  int pid;

  /* Do not change either targets above or the same target if already present.
     The reason is the target stack is shared across multiple inferiors.  */
  int ops_already_pushed = target_is_pushed (ops);
  struct cleanup *back_to = make_cleanup (null_cleanup, NULL);

  if (! ops_already_pushed)
    {
      /* Clear possible core file with its process_stratum.  */
      push_target (ops);
      make_cleanup_unpush_target (ops);
    }

  pid = fork_inferior (exec_file, allargs, env, inf_ptrace_me, NULL,
		       NULL, NULL, NULL);

  discard_cleanups (back_to);

  startup_inferior (START_INFERIOR_TRAPS_EXPECTED);

  /* On some targets, there must be some explicit actions taken after
     the inferior has been started up.  */
  target_post_startup_inferior (pid_to_ptid (pid));
}

#ifdef PT_GET_PROCESS_STATE

static void
inf_ptrace_post_startup_inferior (struct target_ops *self, ptid_t pid)
{
  ptrace_event_t pe;

  /* Set the initial event mask.  */
  memset (&pe, 0, sizeof pe);
  pe.pe_set_event |= PTRACE_FORK;
  if (ptrace (PT_SET_EVENT_MASK, ptid_get_pid (pid),
	      (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
    perror_with_name (("ptrace"));
}

#endif

/* Clean up a rotting corpse of an inferior after it died.  */

static void
inf_ptrace_mourn_inferior (struct target_ops *ops)
{
  int status;

  /* Wait just one more time to collect the inferior's exit status.
     Do not check whether this succeeds though, since we may be
     dealing with a process that we attached to.  Such a process will
     only report its exit status to its original parent.  */
  waitpid (ptid_get_pid (inferior_ptid), &status, 0);

  inf_child_mourn_inferior (ops);
}

/* Attach to the process specified by ARGS.  If FROM_TTY is non-zero,
   be chatty about it.  */

static void
inf_ptrace_attach (struct target_ops *ops, const char *args, int from_tty)
{
  char *exec_file;
  pid_t pid;
  struct inferior *inf;

  /* Do not change either targets above or the same target if already present.
     The reason is the target stack is shared across multiple inferiors.  */
  int ops_already_pushed = target_is_pushed (ops);
  struct cleanup *back_to = make_cleanup (null_cleanup, NULL);

  pid = parse_pid_to_attach (args);

  if (pid == getpid ())		/* Trying to masturbate?  */
    error (_("I refuse to debug myself!"));

  if (! ops_already_pushed)
    {
      /* target_pid_to_str already uses the target.  Also clear possible core
	 file with its process_stratum.  */
      push_target (ops);
      make_cleanup_unpush_target (ops);
    }

  if (from_tty)
    {
      exec_file = get_exec_file (0);

      if (exec_file)
	printf_unfiltered (_("Attaching to program: %s, %s\n"), exec_file,
			   target_pid_to_str (pid_to_ptid (pid)));
      else
	printf_unfiltered (_("Attaching to %s\n"),
			   target_pid_to_str (pid_to_ptid (pid)));

      gdb_flush (gdb_stdout);
    }

#ifdef PT_ATTACH
  errno = 0;
  ptrace (PT_ATTACH, pid, (PTRACE_TYPE_ARG3)0, 0);
  if (errno != 0)
    perror_with_name (("ptrace"));
#else
  error (_("This system does not support attaching to a process"));
#endif

  inf = current_inferior ();
  inferior_appeared (inf, pid);
  inf->attach_flag = 1;
  inferior_ptid = pid_to_ptid (pid);

  /* Always add a main thread.  If some target extends the ptrace
     target, it should decorate the ptid later with more info.  */
  add_thread_silent (inferior_ptid);

  discard_cleanups (back_to);
}

#ifdef PT_GET_PROCESS_STATE

static void
inf_ptrace_post_attach (struct target_ops *self, int pid)
{
  ptrace_event_t pe;

  /* Set the initial event mask.  */
  memset (&pe, 0, sizeof pe);
  pe.pe_set_event |= PTRACE_FORK;
  if (ptrace (PT_SET_EVENT_MASK, pid,
	      (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
    perror_with_name (("ptrace"));
}

#endif

/* Detach from the inferior, optionally passing it the signal
   specified by ARGS.  If FROM_TTY is non-zero, be chatty about it.  */

static void
inf_ptrace_detach (struct target_ops *ops, const char *args, int from_tty)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  int sig = 0;

  target_announce_detach (from_tty);
  if (args)
    sig = atoi (args);

#ifdef PT_DETACH
  /* We'd better not have left any breakpoints in the program or it'll
     die when it hits one.  Also note that this may only work if we
     previously attached to the inferior.  It *might* work if we
     started the process ourselves.  */
  errno = 0;
  ptrace (PT_DETACH, pid, (PTRACE_TYPE_ARG3)1, sig);
  if (errno != 0)
    perror_with_name (("ptrace"));
#else
  error (_("This system does not support detaching from a process"));
#endif

  inf_ptrace_detach_success (ops);
}

/* See inf-ptrace.h.  */

void
inf_ptrace_detach_success (struct target_ops *ops)
{
  pid_t pid = ptid_get_pid (inferior_ptid);

  inferior_ptid = null_ptid;
  detach_inferior (pid);

  inf_child_maybe_unpush_target (ops);
}

/* Kill the inferior.  */

static void
inf_ptrace_kill (struct target_ops *ops)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  int status;

  if (pid == 0)
    return;

  ptrace (PT_KILL, pid, (PTRACE_TYPE_ARG3)0, 0);
  waitpid (pid, &status, 0);

  target_mourn_inferior (inferior_ptid);
}

/* Interrupt the inferior.  */

static void
inf_ptrace_interrupt (struct target_ops *self, ptid_t ptid)
{
  /* Send a SIGINT to the process group.  This acts just like the user
     typed a ^C on the controlling terminal.  Note that using a
     negative process number in kill() is a System V-ism.  The proper
     BSD interface is killpg().  However, all modern BSDs support the
     System V interface too.  */
  kill (-inferior_process_group (), SIGINT);
}

/* Return which PID to pass to ptrace in order to observe/control the
   tracee identified by PTID.  */

pid_t
get_ptrace_pid (ptid_t ptid)
{
  pid_t pid;

#ifndef __NetBSD__
  /* If we have an LWPID to work with, use it.  Otherwise, we're
     dealing with a non-threaded program/target.  */
  pid = ptid_get_lwp (ptid);
  if (pid == 0)
#endif
    pid = ptid_get_pid (ptid);
  return pid;
}

/* Resume execution of thread PTID, or all threads if PTID is -1.  If
   STEP is nonzero, single-step it.  If SIGNAL is nonzero, give it
   that signal.  */

static void
inf_ptrace_resume (struct target_ops *ops,
		   ptid_t ptid, int step, enum gdb_signal signal)
{
  pid_t pid;
  int request, sig;

  if (ptid_equal (minus_one_ptid, ptid))
    /* Resume all threads.  Traditionally ptrace() only supports
       single-threaded processes, so simply resume the inferior.  */
    pid = ptid_get_pid (inferior_ptid);
  else
    pid = get_ptrace_pid (ptid);

  if (catch_syscall_enabled () > 0)
    request = PT_SYSCALL;
  else
    request = PT_CONTINUE;

  if (step)
    {
      /* If this system does not support PT_STEP, a higher level
         function will have called single_step() to transmute the step
         request into a continue request (by setting breakpoints on
         all possible successor instructions), so we don't have to
         worry about that here.  */
      request = PT_STEP;
#ifdef __NetBSD__
      /*
       * On NetBSD the data field of PT_STEP contains the thread
       * to be stepped; all other threads are continued if this value is > 0
       */
      sig = ptid_get_lwp(ptid);
#else
      sig = 0;
#endif
    } else
      sig = gdb_signal_to_host (signal);

  /* An address of (PTRACE_TYPE_ARG3)1 tells ptrace to continue from
     where it was.  If GDB wanted it to start some other way, we have
     already written a new program counter value to the child.  */
  errno = 0;
  ptrace (request, pid, (PTRACE_TYPE_ARG3)1, sig);
  if (errno != 0)
    perror_with_name (("ptrace"));
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

static ptid_t
inf_ptrace_wait (struct target_ops *ops,
		 ptid_t ptid, struct target_waitstatus *ourstatus, int options)
{
  pid_t pid;
  int status, save_errno;

  do
    {
      set_sigint_trap ();

      do
	{
	  pid = waitpid (ptid_get_pid (ptid), &status, 0);
	  save_errno = errno;
	}
      while (pid == -1 && errno == EINTR);

      clear_sigint_trap ();

      if (pid == -1)
	{
	  fprintf_unfiltered (gdb_stderr,
			      _("Child process unexpectedly missing: %s.\n"),
			      safe_strerror (save_errno));

	  /* Claim it exited with unknown signal.  */
	  ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
	  ourstatus->value.sig = GDB_SIGNAL_UNKNOWN;
	  return inferior_ptid;
	}

      /* Ignore terminated detached child processes.  */
      if (!WIFSTOPPED (status) && pid != ptid_get_pid (inferior_ptid))
	pid = -1;
    }
  while (pid == -1);

#ifdef PT_GET_PROCESS_STATE
  if (WIFSTOPPED (status))
    {
      ptrace_state_t pe;
      pid_t fpid;

      if (ptrace (PT_GET_PROCESS_STATE, pid,
		  (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
	perror_with_name (("ptrace"));

      switch (pe.pe_report_event)
	{
	case PTRACE_FORK:
	  ourstatus->kind = TARGET_WAITKIND_FORKED;
	  ourstatus->value.related_pid = pid_to_ptid (pe.pe_other_pid);

	  /* Make sure the other end of the fork is stopped too.  */
	  fpid = waitpid (pe.pe_other_pid, &status, 0);
	  if (fpid == -1)
	    perror_with_name (("waitpid"));

	  if (ptrace (PT_GET_PROCESS_STATE, fpid,
		      (PTRACE_TYPE_ARG3)&pe, sizeof pe) == -1)
	    perror_with_name (("ptrace"));

	  gdb_assert (pe.pe_report_event == PTRACE_FORK);
	  gdb_assert (pe.pe_other_pid == pid);
	  if (fpid == ptid_get_pid (inferior_ptid))
	    {
	      ourstatus->value.related_pid = pid_to_ptid (pe.pe_other_pid);
	      return pid_to_ptid (fpid);
	    }

	  return pid_to_ptid (pid);
	}
    }
#endif

  store_waitstatus (ourstatus, status);
  return pid_to_ptid (pid);
}

/* Transfer data via ptrace into process PID's memory from WRITEBUF, or
   from process PID's memory into READBUF.  Start at target address ADDR
   and transfer up to LEN bytes.  Exactly one of READBUF and WRITEBUF must
   be non-null.  Return the number of transferred bytes.  */

static ULONGEST
inf_ptrace_peek_poke (pid_t pid, gdb_byte *readbuf,
		      const gdb_byte *writebuf,
		      ULONGEST addr, ULONGEST len)
{
  ULONGEST n;
  unsigned int chunk;

  /* We transfer aligned words.  Thus align ADDR down to a word
     boundary and determine how many bytes to skip at the
     beginning.  */
  ULONGEST skip = addr & (sizeof (PTRACE_TYPE_RET) - 1);
  addr -= skip;

  for (n = 0;
       n < len;
       n += chunk, addr += sizeof (PTRACE_TYPE_RET), skip = 0)
    {
      /* Restrict to a chunk that fits in the current word.  */
      chunk = std::min (sizeof (PTRACE_TYPE_RET) - skip, len - n);

      /* Use a union for type punning.  */
      union
      {
	PTRACE_TYPE_RET word;
	gdb_byte byte[sizeof (PTRACE_TYPE_RET)];
      } buf;

      /* Read the word, also when doing a partial word write.  */
      if (readbuf != NULL || chunk < sizeof (PTRACE_TYPE_RET))
	{
	  errno = 0;
	  buf.word = ptrace (PT_READ_I, pid,
			     (PTRACE_TYPE_ARG3)(uintptr_t) addr, 0);
	  if (errno != 0)
	    break;
	  if (readbuf != NULL)
	    memcpy (readbuf + n, buf.byte + skip, chunk);
	}
      if (writebuf != NULL)
	{
	  memcpy (buf.byte + skip, writebuf + n, chunk);
	  errno = 0;
	  ptrace (PT_WRITE_D, pid, (PTRACE_TYPE_ARG3)(uintptr_t) addr,
		  buf.word);
	  if (errno != 0)
	    {
	      /* Using the appropriate one (I or D) is necessary for
		 Gould NP1, at least.  */
	      errno = 0;
	      ptrace (PT_WRITE_I, pid, (PTRACE_TYPE_ARG3)(uintptr_t) addr,
		      buf.word);
	      if (errno != 0)
		break;
	    }
	}
    }

  return n;
}

/* Implement the to_xfer_partial target_ops method.  */

static enum target_xfer_status
inf_ptrace_xfer_partial (struct target_ops *ops, enum target_object object,
			 const char *annex, gdb_byte *readbuf,
			 const gdb_byte *writebuf,
			 ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  pid_t pid = get_ptrace_pid (inferior_ptid);

  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
#ifdef PT_IO
      /* OpenBSD 3.1, NetBSD 1.6 and FreeBSD 5.0 have a new PT_IO
	 request that promises to be much more efficient in reading
	 and writing data in the traced process's address space.  */
      {
	struct ptrace_io_desc piod;

	/* NOTE: We assume that there are no distinct address spaces
	   for instruction and data.  However, on OpenBSD 3.9 and
	   later, PIOD_WRITE_D doesn't allow changing memory that's
	   mapped read-only.  Since most code segments will be
	   read-only, using PIOD_WRITE_D will prevent us from
	   inserting breakpoints, so we use PIOD_WRITE_I instead.  */
	piod.piod_op = writebuf ? PIOD_WRITE_I : PIOD_READ_D;
	piod.piod_addr = writebuf ? (void *) writebuf : readbuf;
	piod.piod_offs = (void *) (long) offset;
	piod.piod_len = len;

	errno = 0;
	if (ptrace (PT_IO, pid, (caddr_t)&piod, 0) == 0)
	  {
	    /* Return the actual number of bytes read or written.  */
	    *xfered_len = piod.piod_len;
	    return (piod.piod_len == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
	/* If the PT_IO request is somehow not supported, fallback on
	   using PT_WRITE_D/PT_READ_D.  Otherwise we will return zero
	   to indicate failure.  */
	if (errno == EACCES)
	  {
	    fprintf_unfiltered (gdb_stderr, "Cannot %s process at %p (%s). "
				"Is PaX MPROTECT active? See security(7), "
				"sysctl(7), paxctl(8)\n", writebuf ? "write to" :
				"read from", piod.piod_offs,
				strerror(errno));
	    return TARGET_XFER_E_IO;	/* Some other error perhaps? */
	  }
	if (errno != EINVAL)
	  return TARGET_XFER_EOF;
      }
#endif
      *xfered_len = inf_ptrace_peek_poke (pid, readbuf, writebuf,
					  offset, len);
      return *xfered_len != 0 ? TARGET_XFER_OK : TARGET_XFER_EOF;

    case TARGET_OBJECT_UNWIND_TABLE:
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_AUXV:
#if defined (PT_IO) && defined (PIOD_READ_AUXV)
      /* OpenBSD 4.5 has a new PIOD_READ_AUXV operation for the PT_IO
	 request that allows us to read the auxilliary vector.  Other
	 BSD's may follow if they feel the need to support PIE.  */
      {
	struct ptrace_io_desc piod;

	if (writebuf)
	  return TARGET_XFER_E_IO;
	piod.piod_op = PIOD_READ_AUXV;
	piod.piod_addr = readbuf;
	piod.piod_offs = (void *) (long) offset;
	piod.piod_len = len;

	errno = 0;
	if (ptrace (PT_IO, pid, (caddr_t)&piod, 0) == 0)
	  {
	    /* Return the actual number of bytes read or written.  */
	    *xfered_len = piod.piod_len;
	    return (piod.piod_len == 0) ? TARGET_XFER_EOF : TARGET_XFER_OK;
	  }
      }
#endif
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_WCOOKIE:
      return TARGET_XFER_E_IO;

    default:
      return TARGET_XFER_E_IO;
    }
}

/* Return non-zero if the thread specified by PTID is alive.  */

static int
inf_ptrace_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  /* ??? Is kill the right way to do this?  */
  return (kill (ptid_get_pid (ptid), 0) != -1);
}

/* Print status information about what we're accessing.  */

static void
inf_ptrace_files_info (struct target_ops *ignore)
{
  struct inferior *inf = current_inferior ();

  printf_filtered (_("\tUsing the running image of %s %s.\n"),
		   inf->attach_flag ? "attached" : "child",
		   target_pid_to_str (inferior_ptid));
}

static const char *
inf_ptrace_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  return normal_pid_to_str (ptid);
}

#if defined (PT_IO) && defined (PIOD_READ_AUXV)

/* Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
   Return 0 if *READPTR is already at the end of the buffer.
   Return -1 if there is insufficient buffer for a whole entry.
   Return 1 if an entry was read into *TYPEP and *VALP.  */

static int
inf_ptrace_auxv_parse (struct target_ops *ops, gdb_byte **readptr,
		       gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
{
  struct type *int_type = builtin_type (target_gdbarch ())->builtin_int;
  struct type *ptr_type = builtin_type (target_gdbarch ())->builtin_data_ptr;
  const int sizeof_auxv_type = TYPE_LENGTH (int_type);
  const int sizeof_auxv_val = TYPE_LENGTH (ptr_type);
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  gdb_byte *ptr = *readptr;

  if (endptr == ptr)
    return 0;

  if (endptr - ptr < 2 * sizeof_auxv_val)
    return -1;

  *typep = extract_unsigned_integer (ptr, sizeof_auxv_type, byte_order);
  ptr += sizeof_auxv_val;	/* Alignment.  */
  *valp = extract_unsigned_integer (ptr, sizeof_auxv_val, byte_order);
  ptr += sizeof_auxv_val;

  *readptr = ptr;
  return 1;
}

#endif

/* Create a prototype ptrace target.  The client can override it with
   local methods.  */

struct target_ops *
inf_ptrace_target (void)
{
  struct target_ops *t = inf_child_target ();

  t->to_attach = inf_ptrace_attach;
  t->to_detach = inf_ptrace_detach;
  t->to_resume = inf_ptrace_resume;
  t->to_wait = inf_ptrace_wait;
  t->to_files_info = inf_ptrace_files_info;
  t->to_kill = inf_ptrace_kill;
  t->to_create_inferior = inf_ptrace_create_inferior;
#ifdef PT_GET_PROCESS_STATE
  t->to_follow_fork = inf_ptrace_follow_fork;
  t->to_insert_fork_catchpoint = inf_ptrace_insert_fork_catchpoint;
  t->to_remove_fork_catchpoint = inf_ptrace_remove_fork_catchpoint;
  t->to_post_startup_inferior = inf_ptrace_post_startup_inferior;
  t->to_post_attach = inf_ptrace_post_attach;
#endif
  t->to_mourn_inferior = inf_ptrace_mourn_inferior;
  t->to_thread_alive = inf_ptrace_thread_alive;
  t->to_pid_to_str = inf_ptrace_pid_to_str;
  t->to_interrupt = inf_ptrace_interrupt;
  t->to_xfer_partial = inf_ptrace_xfer_partial;
#if defined (PT_IO) && defined (PIOD_READ_AUXV)
  t->to_auxv_parse = inf_ptrace_auxv_parse;
#endif

  return t;
}


/* Pointer to a function that returns the offset within the user area
   where a particular register is stored.  */
static CORE_ADDR (*inf_ptrace_register_u_offset)(struct gdbarch *, int, int);

/* Fetch register REGNUM from the inferior.  */

static void
inf_ptrace_fetch_register (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  CORE_ADDR addr;
  size_t size;
  PTRACE_TYPE_RET *buf;
  pid_t pid;
  int i;

  /* This isn't really an address, but ptrace thinks of it as one.  */
  addr = inf_ptrace_register_u_offset (gdbarch, regnum, 0);
  if (addr == (CORE_ADDR)-1
      || gdbarch_cannot_fetch_register (gdbarch, regnum))
    {
      regcache_raw_supply (regcache, regnum, NULL);
      return;
    }

  pid = get_ptrace_pid (regcache_get_ptid (regcache));

  size = register_size (gdbarch, regnum);
  gdb_assert ((size % sizeof (PTRACE_TYPE_RET)) == 0);
  buf = (PTRACE_TYPE_RET *) alloca (size);

  /* Read the register contents from the inferior a chunk at a time.  */
  for (i = 0; i < size / sizeof (PTRACE_TYPE_RET); i++)
    {
      errno = 0;
      buf[i] = ptrace (PT_READ_U, pid, (PTRACE_TYPE_ARG3)(uintptr_t)addr, 0);
      if (errno != 0)
	error (_("Couldn't read register %s (#%d): %s."),
	       gdbarch_register_name (gdbarch, regnum),
	       regnum, safe_strerror (errno));

      addr += sizeof (PTRACE_TYPE_RET);
    }
  regcache_raw_supply (regcache, regnum, buf);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

static void
inf_ptrace_fetch_registers (struct target_ops *ops,
			    struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    for (regnum = 0;
	 regnum < gdbarch_num_regs (get_regcache_arch (regcache));
	 regnum++)
      inf_ptrace_fetch_register (regcache, regnum);
  else
    inf_ptrace_fetch_register (regcache, regnum);
}

/* Store register REGNUM into the inferior.  */

static void
inf_ptrace_store_register (const struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  CORE_ADDR addr;
  size_t size;
  PTRACE_TYPE_RET *buf;
  pid_t pid;
  int i;

  /* This isn't really an address, but ptrace thinks of it as one.  */
  addr = inf_ptrace_register_u_offset (gdbarch, regnum, 1);
  if (addr == (CORE_ADDR)-1 
      || gdbarch_cannot_store_register (gdbarch, regnum))
    return;

  pid = get_ptrace_pid (regcache_get_ptid (regcache));

  size = register_size (gdbarch, regnum);
  gdb_assert ((size % sizeof (PTRACE_TYPE_RET)) == 0);
  buf = (PTRACE_TYPE_RET *) alloca (size);

  /* Write the register contents into the inferior a chunk at a time.  */
  regcache_raw_collect (regcache, regnum, buf);
  for (i = 0; i < size / sizeof (PTRACE_TYPE_RET); i++)
    {
      errno = 0;
      ptrace (PT_WRITE_U, pid, (PTRACE_TYPE_ARG3)(uintptr_t)addr, buf[i]);
      if (errno != 0)
	error (_("Couldn't write register %s (#%d): %s."),
	       gdbarch_register_name (gdbarch, regnum),
	       regnum, safe_strerror (errno));

      addr += sizeof (PTRACE_TYPE_RET);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

static void
inf_ptrace_store_registers (struct target_ops *ops,
			    struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    for (regnum = 0;
	 regnum < gdbarch_num_regs (get_regcache_arch (regcache));
	 regnum++)
      inf_ptrace_store_register (regcache, regnum);
  else
    inf_ptrace_store_register (regcache, regnum);
}

/* Create a "traditional" ptrace target.  REGISTER_U_OFFSET should be
   a function returning the offset within the user area where a
   particular register is stored.  */

struct target_ops *
inf_ptrace_trad_target (CORE_ADDR (*register_u_offset)
					(struct gdbarch *, int, int))
{
  struct target_ops *t = inf_ptrace_target();

  gdb_assert (register_u_offset);
  inf_ptrace_register_u_offset = register_u_offset;
  t->to_fetch_registers = inf_ptrace_fetch_registers;
  t->to_store_registers = inf_ptrace_store_registers;

  return t;
}
