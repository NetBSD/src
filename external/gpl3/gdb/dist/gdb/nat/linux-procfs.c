/* Linux-specific PROCFS manipulation routines.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include "linux-procfs.h"
#include "gdbsupport/filestuff.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unordered_set>
#include <utility>

/* Return the TGID of LWPID from /proc/pid/status.  Returns -1 if not
   found.  */

static int
linux_proc_get_int (pid_t lwpid, const char *field, int warn)
{
  size_t field_len = strlen (field);
  char buf[100];
  int retval = -1;

  snprintf (buf, sizeof (buf), "/proc/%d/status", (int) lwpid);
  gdb_file_up status_file = gdb_fopen_cloexec (buf, "r");
  if (status_file == NULL)
    {
      if (warn)
	warning (_("unable to open /proc file '%s'"), buf);
      return -1;
    }

  while (fgets (buf, sizeof (buf), status_file.get ()))
    if (strncmp (buf, field, field_len) == 0 && buf[field_len] == ':')
      {
	retval = strtol (&buf[field_len + 1], NULL, 10);
	break;
      }

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

/* Process states as discovered in the 'State' line of
   /proc/PID/status.  Not all possible states are represented here,
   only those that we care about.  */

enum proc_state
{
  /* Some state we don't handle.  */
  PROC_STATE_UNKNOWN,

  /* Stopped on a signal.  */
  PROC_STATE_STOPPED,

  /* Tracing stop.  */
  PROC_STATE_TRACING_STOP,

  /* Dead.  */
  PROC_STATE_DEAD,

  /* Zombie.  */
  PROC_STATE_ZOMBIE,
};

/* Parse a PROC_STATE out of STATE, a buffer with the state found in
   the 'State:' line of /proc/PID/status.  */

static enum proc_state
parse_proc_status_state (const char *state)
{
  state = skip_spaces (state);

  switch (state[0])
    {
    case 't':
      return PROC_STATE_TRACING_STOP;
    case 'T':
      /* Before Linux 2.6.33, tracing stop used uppercase T.  */
      if (strcmp (state, "T (stopped)\n") == 0)
	return PROC_STATE_STOPPED;
      else /* "T (tracing stop)\n" */
	return PROC_STATE_TRACING_STOP;
    case 'X':
      return PROC_STATE_DEAD;
    case 'Z':
      return PROC_STATE_ZOMBIE;
    }

  return PROC_STATE_UNKNOWN;
}


/* Fill in STATE, a buffer with BUFFER_SIZE bytes with the 'State'
   line of /proc/PID/status.  Returns -1 on failure to open the /proc
   file, 1 if the line is found, and 0 if not found.  If WARN, warn on
   failure to open the /proc file.  */

static int
linux_proc_pid_get_state (pid_t pid, int warn, enum proc_state *state)
{
  int have_state;
  char buffer[100];

  xsnprintf (buffer, sizeof (buffer), "/proc/%d/status", (int) pid);
  gdb_file_up procfile = gdb_fopen_cloexec (buffer, "r");
  if (procfile == NULL)
    {
      if (warn)
	warning (_("unable to open /proc file '%s'"), buffer);
      return -1;
    }

  have_state = 0;
  while (fgets (buffer, sizeof (buffer), procfile.get ()) != NULL)
    if (startswith (buffer, "State:"))
      {
	have_state = 1;
	*state = parse_proc_status_state (buffer + sizeof ("State:") - 1);
	break;
      }
  return have_state;
}

/* See linux-procfs.h declaration.  */

int
linux_proc_pid_is_gone (pid_t pid)
{
  int have_state;
  enum proc_state state;

  have_state = linux_proc_pid_get_state (pid, 0, &state);
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
    return (state == PROC_STATE_ZOMBIE || state == PROC_STATE_DEAD);
}

/* Return non-zero if 'State' of /proc/PID/status contains STATE.  If
   WARN, warn on failure to open the /proc file.  */

static int
linux_proc_pid_has_state (pid_t pid, enum proc_state state, int warn)
{
  int have_state;
  enum proc_state cur_state;

  have_state = linux_proc_pid_get_state (pid, warn, &cur_state);
  return (have_state > 0 && cur_state == state);
}

/* Detect `T (stopped)' in `/proc/PID/status'.
   Other states including `T (tracing stop)' are reported as false.  */

int
linux_proc_pid_is_stopped (pid_t pid)
{
  return linux_proc_pid_has_state (pid, PROC_STATE_STOPPED, 1);
}

/* Detect `t (tracing stop)' in `/proc/PID/status'.
   Other states including `T (stopped)' are reported as false.  */

int
linux_proc_pid_is_trace_stopped_nowarn (pid_t pid)
{
  return linux_proc_pid_has_state (pid, PROC_STATE_TRACING_STOP, 1);
}

/* Return non-zero if PID is a zombie.  If WARN, warn on failure to
   open the /proc file.  */

static int
linux_proc_pid_is_zombie_maybe_warn (pid_t pid, int warn)
{
  return linux_proc_pid_has_state (pid, PROC_STATE_ZOMBIE, warn);
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

std::optional<std::string>
linux_proc_get_stat_field (ptid_t ptid, int field)
{
  /* We never need to read PID from the stat file, and there's
     command_from_pid to read the comm field.  */
  gdb_assert (field >= LINUX_PROC_STAT_STATE);

  std::string filename = string_printf ("/proc/%ld/task/%ld/stat",
					(long) ptid.pid (), (long) ptid.lwp ());

  std::optional<std::string> content
    = read_text_file_to_string (filename.c_str ());
  if (!content.has_value ())
    return {};

  /* ps command also relies on no trailing fields ever containing ')'.  */
  std::string::size_type pos = content->find_last_of (')');
  if (pos == std::string::npos)
    return {};

  /* The first field after program name is LINUX_PROC_STAT_STATE.  */
  for (int i = LINUX_PROC_STAT_STATE; i <= field; ++i)
    {
      /* Find separator.  */
      pos = content->find_first_of (' ', pos);
      if (pos == std::string::npos)
	return {};

      /* Find beginning of field.  */
      pos = content->find_first_not_of (' ', pos);
      if (pos == std::string::npos)
	return {};
    }

  /* Find end of field.  */
  std::string::size_type end_pos = content->find_first_of (' ', pos);
  if (end_pos == std::string::npos)
    return content->substr (pos);
  else
    return content->substr (pos, end_pos - pos);
}

/* Get the start time of thread PTID.  */

static std::optional<ULONGEST>
linux_proc_get_starttime (ptid_t ptid)
{
  std::optional<std::string> field
    = linux_proc_get_stat_field (ptid, LINUX_PROC_STAT_STARTTIME);

  if (!field.has_value ())
    return {};

  errno = 0;
  const char *trailer;
  ULONGEST starttime = strtoulst (field->c_str (), &trailer, 10);
  if (starttime == ULONGEST_MAX && errno == ERANGE)
    return {};
  else if (*trailer != '\0')
    /* There were unexpected characters.  */
    return {};

  return starttime;
}

/* See linux-procfs.h.  */

const char *
linux_proc_tid_get_name (ptid_t ptid)
{
#define TASK_COMM_LEN 16  /* As defined in the kernel's sched.h.  */

  static char comm_buf[TASK_COMM_LEN];
  char comm_path[100];
  const char *comm_val;
  pid_t pid = ptid.pid ();
  pid_t tid = ptid.lwp_p () ? ptid.lwp () : ptid.pid ();

  xsnprintf (comm_path, sizeof (comm_path),
	     "/proc/%ld/task/%ld/comm", (long) pid, (long) tid);

  gdb_file_up comm_file = gdb_fopen_cloexec (comm_path, "r");
  if (comm_file == NULL)
    return NULL;

  comm_val = fgets (comm_buf, sizeof (comm_buf), comm_file.get ());

  if (comm_val != NULL)
    {
      int i;

      /* Make sure there is no newline at the end.  */
      for (i = 0; i < sizeof (comm_buf); i++)
	{
	  if (comm_buf[i] == '\n')
	    {
	      comm_buf[i] = '\0';
	      break;
	    }
	}
    }

  return comm_val;
}

/* See linux-procfs.h.  */

void
linux_proc_attach_tgid_threads (pid_t pid,
				linux_proc_attach_lwp_func attach_lwp)
{
  char pathname[128];
  int new_threads_found;
  int iterations;

  if (linux_proc_get_tgid (pid) != pid)
    return;

  xsnprintf (pathname, sizeof (pathname), "/proc/%ld/task", (long) pid);
  gdb_dir_up dir (opendir (pathname));
  if (dir == NULL)
    {
      warning (_("Could not open %s."), pathname);
      return;
    }

  /* Callable object to hash elements in visited_lpws.  */
  struct pair_hash
  {
    std::size_t operator() (const std::pair<unsigned long, ULONGEST> &v) const
    {
      return (std::hash<unsigned long>() (v.first)
	      ^ std::hash<ULONGEST>() (v.second));
    }
  };

  /* Keeps track of the LWPs we have already visited in /proc,
     identified by their PID and starttime to detect PID reuse.  */
  std::unordered_set<std::pair<unsigned long, ULONGEST>,
		     pair_hash> visited_lwps;

  /* Scan the task list for existing threads.  While we go through the
     threads, new threads may be spawned.  Cycle through the list of
     threads until we have done two iterations without finding new
     threads.  */
  for (iterations = 0; iterations < 2; iterations++)
    {
      struct dirent *dp;

      new_threads_found = 0;
      while ((dp = readdir (dir.get ())) != NULL)
	{
	  unsigned long lwp;

	  /* Fetch one lwp.  */
	  lwp = strtoul (dp->d_name, NULL, 10);
	  if (lwp != 0)
	    {
	      ptid_t ptid = ptid_t (pid, lwp);
	      std::optional<ULONGEST> starttime
		= linux_proc_get_starttime (ptid);

	      if (starttime.has_value ())
		{
		  std::pair<unsigned long, ULONGEST> key (lwp, *starttime);

		  /* If we already visited this LWP, skip it this time.  */
		  if (visited_lwps.find (key) != visited_lwps.cend ())
		    continue;

		  visited_lwps.insert (key);
		}

	      if (attach_lwp (ptid))
		new_threads_found = 1;
	    }
	}

      if (new_threads_found)
	{
	  /* Start over.  */
	  iterations = -1;
	}

      rewinddir (dir.get ());
    }
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

const char *
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

  /* Use /proc/PID/exe if the actual file can't be read, but /proc/PID/exe
     can be.  */
  if (access (buf, R_OK) != 0 && access (name, R_OK) == 0)
    strcpy (buf, name);

  return buf;
}

/* See linux-procfs.h.  */

void
linux_proc_init_warnings ()
{
  static bool warned = false;

  if (warned)
    return;
  warned = true;

  struct stat st;

  if (stat ("/proc/self", &st) != 0)
    warning (_("/proc is not accessible."));
}
