/* Target operations for the remote server for GDB.
   Copyright (C) 2002-2016 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#include "server.h"
#include "tracepoint.h"

struct target_ops *the_target;

int
set_desired_thread (int use_general)
{
  struct thread_info *found;

  if (use_general == 1)
    found = find_thread_ptid (general_thread);
  else
    found = find_thread_ptid (cont_thread);

  current_thread = found;
  return (current_thread != NULL);
}

/* Structure used to look up a thread to use as current when accessing
   memory.  */

struct thread_search
{
  /* The PTID of the current general thread.  This is an input
     parameter.  */
  ptid_t current_gen_ptid;

  /* The first thread found.  */
  struct thread_info *first;

  /* The first stopped thread found.  */
  struct thread_info *stopped;

  /* The current general thread, if found.  */
  struct thread_info *current;
};

/* Callback for find_inferior.  Search for a thread to use as current
   when accessing memory.  */

static int
thread_search_callback (struct inferior_list_entry *entry, void *args)
{
  struct thread_info *thread = (struct thread_info *) entry;
  struct thread_search *s = (struct thread_search *) args;

  if (ptid_get_pid (entry->id) == ptid_get_pid (s->current_gen_ptid)
      && mythread_alive (ptid_of (thread)))
    {
      if (s->stopped == NULL
	  && the_target->thread_stopped != NULL
	  && thread_stopped (thread))
	s->stopped = thread;

      if (s->first == NULL)
	s->first = thread;

      if (s->current == NULL && ptid_equal (s->current_gen_ptid, entry->id))
	s->current = thread;
    }

  return 0;
}

/* The thread that was current before prepare_to_access_memory was
   called.  done_accessing_memory uses this to restore the previous
   selected thread.  */
static ptid_t prev_general_thread;

/* See target.h.  */

int
prepare_to_access_memory (void)
{
  struct thread_search search;
  struct thread_info *thread;

  memset (&search, 0, sizeof (search));
  search.current_gen_ptid = general_thread;
  prev_general_thread = general_thread;

  if (the_target->prepare_to_access_memory != NULL)
    {
      int res;

      res = the_target->prepare_to_access_memory ();
      if (res != 0)
	return res;
    }

  find_inferior (&all_threads, thread_search_callback, &search);

  /* Prefer a stopped thread.  If none is found, try the current
     thread.  Otherwise, take the first thread in the process.  If
     none is found, undo the effects of
     target->prepare_to_access_memory() and return error.  */
  if (search.stopped != NULL)
    thread = search.stopped;
  else if (search.current != NULL)
    thread = search.current;
  else if (search.first != NULL)
    thread = search.first;
  else
    {
      done_accessing_memory ();
      return 1;
    }

  current_thread = thread;
  general_thread = ptid_of (thread);

  return 0;
}

/* See target.h.  */

void
done_accessing_memory (void)
{
  if (the_target->done_accessing_memory != NULL)
    the_target->done_accessing_memory ();

  /* Restore the previous selected thread.  */
  general_thread = prev_general_thread;
  current_thread = find_thread_ptid (general_thread);
}

int
read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  int res;
  res = (*the_target->read_memory) (memaddr, myaddr, len);
  check_mem_read (memaddr, myaddr, len);
  return res;
}

/* See target/target.h.  */

int
target_read_memory (CORE_ADDR memaddr, gdb_byte *myaddr, ssize_t len)
{
  return read_inferior_memory (memaddr, myaddr, len);
}

/* See target/target.h.  */

int
target_read_uint32 (CORE_ADDR memaddr, uint32_t *result)
{
  return read_inferior_memory (memaddr, (gdb_byte *) result, sizeof (*result));
}

int
write_inferior_memory (CORE_ADDR memaddr, const unsigned char *myaddr,
		       int len)
{
  /* Lacking cleanups, there is some potential for a memory leak if the
     write fails and we go through error().  Make sure that no more than
     one buffer is ever pending by making BUFFER static.  */
  static unsigned char *buffer = 0;
  int res;

  if (buffer != NULL)
    free (buffer);

  buffer = (unsigned char *) xmalloc (len);
  memcpy (buffer, myaddr, len);
  check_mem_write (memaddr, buffer, myaddr, len);
  res = (*the_target->write_memory) (memaddr, buffer, len);
  free (buffer);
  buffer = NULL;

  return res;
}

/* See target/target.h.  */

int
target_write_memory (CORE_ADDR memaddr, const gdb_byte *myaddr, ssize_t len)
{
  return write_inferior_memory (memaddr, myaddr, len);
}

ptid_t
mywait (ptid_t ptid, struct target_waitstatus *ourstatus, int options,
	int connected_wait)
{
  ptid_t ret;

  if (connected_wait)
    server_waiting = 1;

  ret = (*the_target->wait) (ptid, ourstatus, options);

  /* We don't expose _LOADED events to gdbserver core.  See the
     `dlls_changed' global.  */
  if (ourstatus->kind == TARGET_WAITKIND_LOADED)
    ourstatus->kind = TARGET_WAITKIND_STOPPED;

  /* If GDB is connected through TCP/serial, then GDBserver will most
     probably be running on its own terminal/console, so it's nice to
     print there why is GDBserver exiting.  If however, GDB is
     connected through stdio, then there's no need to spam the GDB
     console with this -- the user will already see the exit through
     regular GDB output, in that same terminal.  */
  if (!remote_connection_is_stdio ())
    {
      if (ourstatus->kind == TARGET_WAITKIND_EXITED)
	fprintf (stderr,
		 "\nChild exited with status %d\n", ourstatus->value.integer);
      else if (ourstatus->kind == TARGET_WAITKIND_SIGNALLED)
	fprintf (stderr, "\nChild terminated with signal = 0x%x (%s)\n",
		 gdb_signal_to_host (ourstatus->value.sig),
		 gdb_signal_to_name (ourstatus->value.sig));
    }

  if (connected_wait)
    server_waiting = 0;

  return ret;
}

/* See target/target.h.  */

void
target_stop_and_wait (ptid_t ptid)
{
  struct target_waitstatus status;
  int was_non_stop = non_stop;
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_stop;
  resume_info.sig = GDB_SIGNAL_0;
  (*the_target->resume) (&resume_info, 1);

  non_stop = 1;
  mywait (ptid, &status, 0, 0);
  non_stop = was_non_stop;
}

/* See target/target.h.  */

void
target_continue_no_signal (ptid_t ptid)
{
  struct thread_resume resume_info;

  resume_info.thread = ptid;
  resume_info.kind = resume_continue;
  resume_info.sig = GDB_SIGNAL_0;
  (*the_target->resume) (&resume_info, 1);
}

int
start_non_stop (int nonstop)
{
  if (the_target->start_non_stop == NULL)
    {
      if (nonstop)
	return -1;
      else
	return 0;
    }

  return (*the_target->start_non_stop) (nonstop);
}

void
set_target_ops (struct target_ops *target)
{
  the_target = XNEW (struct target_ops);
  memcpy (the_target, target, sizeof (*the_target));
}

/* Convert pid to printable format.  */

const char *
target_pid_to_str (ptid_t ptid)
{
  static char buf[80];

  if (ptid_equal (ptid, minus_one_ptid))
    xsnprintf (buf, sizeof (buf), "<all threads>");
  else if (ptid_equal (ptid, null_ptid))
    xsnprintf (buf, sizeof (buf), "<null thread>");
  else if (ptid_get_tid (ptid) != 0)
    xsnprintf (buf, sizeof (buf), "Thread %d.0x%lx",
	       ptid_get_pid (ptid), ptid_get_tid (ptid));
  else if (ptid_get_lwp (ptid) != 0)
    xsnprintf (buf, sizeof (buf), "LWP %d.%ld",
	       ptid_get_pid (ptid), ptid_get_lwp (ptid));
  else
    xsnprintf (buf, sizeof (buf), "Process %d",
	       ptid_get_pid (ptid));

  return buf;
}

int
kill_inferior (int pid)
{
  gdb_agent_about_to_close (pid);

  return (*the_target->kill) (pid);
}

/* Target can do hardware single step.  */

int
target_can_do_hardware_single_step (void)
{
  return 1;
}

/* Default implementation for breakpoint_kind_for_pc.

   The default behavior for targets that don't implement breakpoint_kind_for_pc
   is to use the size of a breakpoint as the kind.  */

int
default_breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  int size = 0;

  gdb_assert (the_target->sw_breakpoint_from_kind != NULL);

  (*the_target->sw_breakpoint_from_kind) (0, &size);
  return size;
}
