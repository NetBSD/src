/* Linux-specific PROCFS manipulation routines.
   Copyright (C) 2009-2014 Free Software Foundation, Inc.

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
#include <string.h>
#endif

#include "linux-procfs.h"
#include "filestuff.h"

/* Return the TGID of LWPID from /proc/pid/status.  Returns -1 if not
   found.  */

static int
linux_proc_get_int (pid_t lwpid, const char *field)
{
  size_t field_len = strlen (field);
  FILE *status_file;
  char buf[100];
  int retval = -1;

  snprintf (buf, sizeof (buf), "/proc/%d/status", (int) lwpid);
  status_file = gdb_fopen_cloexec (buf, "r");
  if (status_file == NULL)
    {
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
  return linux_proc_get_int (lwpid, "Tgid");
}

/* See linux-procfs.h.  */

pid_t
linux_proc_get_tracerpid (pid_t lwpid)
{
  return linux_proc_get_int (lwpid, "TracerPid");
}

/* Return non-zero if 'State' of /proc/PID/status contains STATE.  */

static int
linux_proc_pid_has_state (pid_t pid, const char *state)
{
  char buffer[100];
  FILE *procfile;
  int retval;
  int have_state;

  xsnprintf (buffer, sizeof (buffer), "/proc/%d/status", (int) pid);
  procfile = gdb_fopen_cloexec (buffer, "r");
  if (procfile == NULL)
    {
      warning (_("unable to open /proc file '%s'"), buffer);
      return 0;
    }

  have_state = 0;
  while (fgets (buffer, sizeof (buffer), procfile) != NULL)
    if (strncmp (buffer, "State:", 6) == 0)
      {
	have_state = 1;
	break;
      }
  retval = (have_state && strstr (buffer, state) != NULL);
  fclose (procfile);
  return retval;
}

/* Detect `T (stopped)' in `/proc/PID/status'.
   Other states including `T (tracing stop)' are reported as false.  */

int
linux_proc_pid_is_stopped (pid_t pid)
{
  return linux_proc_pid_has_state (pid, "T (stopped)");
}

/* See linux-procfs.h declaration.  */

int
linux_proc_pid_is_zombie (pid_t pid)
{
  return linux_proc_pid_has_state (pid, "Z (zombie)");
}
