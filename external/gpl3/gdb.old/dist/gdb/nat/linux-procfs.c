/* Linux-specific PROCFS manipulation routines.
   Copyright (C) 2009-2015 Free Software Foundation, Inc.

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
#include "linux-procfs.h"
#include "filestuff.h"
#include <dirent.h>
#include <sys/stat.h>

/* Return the TGID of LWPID from /proc/pid/status.  Returns -1 if not
   found.  */

static int
linux_proc_get_int (pid_t lwpid, const char *field, int warn)
{
  size_t field_len = strlen (field);
  FILE *status_file;
  char buf[100];
  int retval = -1;

  snprintf (buf, sizeof (buf), "/proc/%d/status", (int) lwpid);
  status_file = gdb_fopen_cloexec (buf, "r");
  if (status_file == NULL)
    {
      if (warn)
	warning (_("unable to open /proc file '%s'"), buf);
      return -1;
    }

  while (fgets (buf, sizeof (buf), status_file))
    if (strncmp (buf, field, field_len) == 0 && buf[field_len] == ':')
      {
	retval = strtol (&buf[field_len + 1], NULL, 10);
	break;
      }

  fclose (status_file);
  return retval;
}

/* Return the TGID of LWPID from /proc/pid/status.  Returns -1 if not
   found.  */

int
linux_proc_get_tgid (pid_t lwpid)
{
  return linux_proc_get_int (lwpid, "Tgid", 1);
}

/* See linux-procfs.h.  */

pid_t
linux_proc_get_tracerpid_nowarn (pid_t lwpid)
{
  return linux_proc_get_int (lwpid, "TracerPid", 0);
}

/* Fill in BUFFER, a buffer with BUFFER_SIZE bytes with the 'State'
   line of /proc/PID/status.  Returns -1 on failure to open the /proc
   file, 1 if the line is found, and 0 if not found.  If WARN, warn on
   failure to open the /proc file.  */

static int
linux_proc_pid_get_state (pid_t pid, char *buffer, size_t buffer_size,
			  int warn)
{
  FILE *procfile;
  int have_state;

  xsnprintf (buffer, buffer_size, "/proc/%d/status", (int) pid);
  procfile = gdb_fopen_cloexec (buffer, "r");
  if (procfile == NULL)
    {
      if (warn)
	warning (_("unable to open /proc file '%s'"), buffer);
      return -1;
    }

  have_state = 0;
  while (fgets (buffer, buffer_size, procfile) != NULL)
    if (startswith (buffer, "State:"))
      {
	have_state = 1;
	break;
      }
  fclose (procfile);
  return have_state;
}

/* See linux-procfs.h declaration.  */

int
linux_proc_pid_is_gone (pid_t pid)
{
  char buffer[100];
  int have_state;

  have_state = linux_proc_pid_get_state (pid, buffer, sizeof buffer, 0);
  if (have_state < 0)
    {
      /* If we can't open the status file, assume the thread has
	 disappeared.  */
      return 1;
    }
  else if (have_state == 0)
    {
      /* No "State:" line, assume thread is alive.  */
      return 0;
    }
  else
    {
      return (strstr (buffer, "Z (") != NULL
	      || strstr (buffer, "X (") != NULL);
    }
}

/* Return non-zero if 'State' of /proc/PID/status contains STATE.  If
   WARN, warn on failure to open the /proc file.  */

static int
linux_proc_pid_has_state (pid_t pid, const char *state, int warn)
{
  char buffer[100];
  int have_state;

  have_state = linux_proc_pid_get_state (pid, buffer, sizeof buffer, warn);
  return (have_state > 0 && strstr (buffer, state) != NULL);
}

/* Detect `T (stopped)' in `/proc/PID/status'.
   Other states including `T (tracing stop)' are reported as false.  */

int
linux_proc_pid_is_stopped (pid_t pid)
{
  return linux_proc_pid_has_state (pid, "T (stopped)", 1);
}

/* Detect `T (tracing stop)' in `/proc/PID/status'.
   Other states including `T (stopped)' are reported as false.  */

int
linux_proc_pid_is_trace_stopped_nowarn (pid_t pid)
{
  return linux_proc_pid_has_state (pid, "T (tracing stop)", 1);
}

/* Return non-zero if PID is a zombie.  If WARN, warn on failure to
   open the /proc file.  */

static int
linux_proc_pid_is_zombie_maybe_warn (pid_t pid, int warn)
{
  return linux_proc_pid_has_state (pid, "Z (zombie)", warn);
}

/* See linux-procfs.h declaration.  */

int
linux_proc_pid_is_zombie_nowarn (pid_t pid)
{
  return linux_proc_pid_is_zombie_maybe_warn (pid, 0);
}

/* See linux-procfs.h declaration.  */

int
linux_proc_pid_is_zombie (pid_t pid)
{
  return linux_proc_pid_is_zombie_maybe_warn (pid, 1);
}

/* See linux-procfs.h.  */

void
linux_proc_attach_tgid_threads (pid_t pid,
				linux_proc_attach_lwp_func attach_lwp)
{
  DIR *dir;
  char pathname[128];
  int new_threads_found;
  int iterations;

  if (linux_proc_get_tgid (pid) != pid)
    return;

  xsnprintf (pathname, sizeof (pathname), "/proc/%ld/task", (long) pid);
  dir = opendir (pathname);
  if (dir == NULL)
    {
      warning (_("Could not open /proc/%ld/task."), (long) pid);
      return;
    }

  /* Scan the task list for existing threads.  While we go through the
     threads, new threads may be spawned.  Cycle through the list of
     threads until we have done two iterations without finding new
     threads.  */
  for (iterations = 0; iterations < 2; iterations++)
    {
      struct dirent *dp;

      new_threads_found = 0;
      while ((dp = readdir (dir)) != NULL)
	{
	  unsigned long lwp;

	  /* Fetch one lwp.  */
	  lwp = strtoul (dp->d_name, NULL, 10);
	  if (lwp != 0)
	    {
	      ptid_t ptid = ptid_build (pid, lwp, 0);

	      if (attach_lwp (ptid))
		new_threads_found = 1;
	    }
	}

      if (new_threads_found)
	{
	  /* Start over.  */
	  iterations = -1;
	}

      rewinddir (dir);
    }

  closedir (dir);
}

/* See linux-procfs.h.  */

int
linux_proc_task_list_dir_exists (pid_t pid)
{
  char pathname[128];
  struct stat buf;

  xsnprintf (pathname, sizeof (pathname), "/proc/%ld/task", (long) pid);
  return (stat (pathname, &buf) == 0);
}

/* See linux-procfs.h.  */

char *
linux_proc_pid_to_exec_file (int pid)
{
  static char buf[PATH_MAX];
  char name[PATH_MAX];
  ssize_t len;

  xsnprintf (name, PATH_MAX, "/proc/%d/exe", pid);
  len = readlink (name, buf, PATH_MAX - 1);
  if (len <= 0)
    strcpy (buf, name);
  else
    buf[len] = '\0';

  return buf;
}
