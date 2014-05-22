/* Target operations for the remote server for GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.

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

struct target_ops *the_target;

void
set_desired_inferior (int use_general)
{
  struct thread_info *found;

  if (use_general == 1)
    found = find_thread_ptid (general_thread);
  else
    found = find_thread_ptid (cont_thread);

  if (found == NULL)
    current_inferior = (struct thread_info *) all_threads.head;
  else
    current_inferior = found;
}

int
read_inferior_memory (CORE_ADDR memaddr, unsigned char *myaddr, int len)
{
  int res;
  res = (*the_target->read_memory) (memaddr, myaddr, len);
  check_mem_read (memaddr, myaddr, len);
  return res;
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

  buffer = xmalloc (len);
  memcpy (buffer, myaddr, len);
  check_mem_write (memaddr, buffer, myaddr, len);
  res = (*the_target->write_memory) (memaddr, buffer, len);
  free (buffer);
  buffer = NULL;

  return res;
}

ptid_t
mywait (ptid_t ptid, struct target_waitstatus *ourstatus, int options,
	int connected_wait)
{
  ptid_t ret;

  if (connected_wait)
    server_waiting = 1;

  ret = (*the_target->wait) (ptid, ourstatus, options);

  if (ourstatus->kind == TARGET_WAITKIND_EXITED)
    fprintf (stderr,
	     "\nChild exited with status %d\n", ourstatus->value.integer);
  else if (ourstatus->kind == TARGET_WAITKIND_SIGNALLED)
    fprintf (stderr, "\nChild terminated with signal = 0x%x (%s)\n",
	     gdb_signal_to_host (ourstatus->value.sig),
	     gdb_signal_to_name (ourstatus->value.sig));

  if (connected_wait)
    server_waiting = 0;

  return ret;
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
  the_target = (struct target_ops *) xmalloc (sizeof (*the_target));
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

/* Return a pretty printed form of target_waitstatus.  */

const char *
target_waitstatus_to_string (const struct target_waitstatus *ws)
{
  static char buf[200];
  const char *kind_str = "status->kind = ";

  switch (ws->kind)
    {
    case TARGET_WAITKIND_EXITED:
      sprintf (buf, "%sexited, status = %d",
	       kind_str, ws->value.integer);
      break;
    case TARGET_WAITKIND_STOPPED:
      sprintf (buf, "%sstopped, signal = %s",
	       kind_str, gdb_signal_to_name (ws->value.sig));
      break;
    case TARGET_WAITKIND_SIGNALLED:
      sprintf (buf, "%ssignalled, signal = %s",
	       kind_str, gdb_signal_to_name (ws->value.sig));
      break;
    case TARGET_WAITKIND_LOADED:
      sprintf (buf, "%sloaded", kind_str);
      break;
    case TARGET_WAITKIND_EXECD:
      sprintf (buf, "%sexecd", kind_str);
      break;
    case TARGET_WAITKIND_SPURIOUS:
      sprintf (buf, "%sspurious", kind_str);
      break;
    case TARGET_WAITKIND_IGNORE:
      sprintf (buf, "%signore", kind_str);
      break;
    default:
      sprintf (buf, "%sunknown???", kind_str);
      break;
    }

  return buf;
}

int
kill_inferior (int pid)
{
  gdb_agent_about_to_close (pid);

  return (*the_target->kill) (pid);
}
