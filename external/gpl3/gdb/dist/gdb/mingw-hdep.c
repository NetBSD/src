/* Host support routines for MinGW, for GDB, the GNU debugger.

   Copyright (C) 2006-2017 Free Software Foundation, Inc.

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
#include "main.h"
#include "serial.h"
#include "event-loop.h"

#include "gdb_select.h"
#include "readline/readline.h"

#include <windows.h>

/* Return an absolute file name of the running GDB, if possible, or
   ARGV0 if not.  The return value is in malloc'ed storage.  */

char *
windows_get_absolute_argv0 (const char *argv0)
{
  char full_name[PATH_MAX];

  if (GetModuleFileName (NULL, full_name, PATH_MAX))
    return xstrdup (full_name);
  return xstrdup (argv0);
}

/* Wrapper for select.  On Windows systems, where the select interface
   only works for sockets, this uses the GDB serial abstraction to
   handle sockets, consoles, pipes, and serial ports.

   The arguments to this function are the same as the traditional
   arguments to select on POSIX platforms.  */

int
gdb_select (int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	    struct timeval *timeout)
{
  static HANDLE never_handle;
  HANDLE handles[MAXIMUM_WAIT_OBJECTS];
  HANDLE h;
  DWORD event;
  DWORD num_handles;
  /* SCBS contains serial control objects corresponding to file
     descriptors in READFDS and WRITEFDS.  */
  struct serial *scbs[MAXIMUM_WAIT_OBJECTS];
  /* The number of valid entries in SCBS.  */
  size_t num_scbs;
  int fd;
  int num_ready;
  size_t indx;

  num_ready = 0;
  num_handles = 0;
  num_scbs = 0;
  for (fd = 0; fd < n; ++fd)
    {
      HANDLE read = NULL, except = NULL;
      struct serial *scb;

      /* There is no support yet for WRITEFDS.  At present, this isn't
	 used by GDB -- but we do not want to silently ignore WRITEFDS
	 if something starts using it.  */
      gdb_assert (!writefds || !FD_ISSET (fd, writefds));

      if ((!readfds || !FD_ISSET (fd, readfds))
	  && (!exceptfds || !FD_ISSET (fd, exceptfds)))
	continue;

      scb = serial_for_fd (fd);
      if (scb)
	{
	  serial_wait_handle (scb, &read, &except);
	  scbs[num_scbs++] = scb;
	}

      if (read == NULL)
	read = (HANDLE) _get_osfhandle (fd);
      if (except == NULL)
	{
	  if (!never_handle)
	    never_handle = CreateEvent (0, FALSE, FALSE, 0);

	  except = never_handle;
	}

      if (readfds && FD_ISSET (fd, readfds))
	{
	  gdb_assert (num_handles < MAXIMUM_WAIT_OBJECTS);
	  handles[num_handles++] = read;
	}

      if (exceptfds && FD_ISSET (fd, exceptfds))
	{
	  gdb_assert (num_handles < MAXIMUM_WAIT_OBJECTS);
	  handles[num_handles++] = except;
	}
    }

  gdb_assert (num_handles <= MAXIMUM_WAIT_OBJECTS);

  event = WaitForMultipleObjects (num_handles,
				  handles,
				  FALSE,
				  timeout
				  ? (timeout->tv_sec * 1000
				     + timeout->tv_usec / 1000)
				  : INFINITE);
  /* EVENT can only be a value in the WAIT_ABANDONED_0 range if the
     HANDLES included an abandoned mutex.  Since GDB doesn't use
     mutexes, that should never occur.  */
  gdb_assert (!(WAIT_ABANDONED_0 <= event
		&& event < WAIT_ABANDONED_0 + num_handles));
  /* We no longer need the helper threads to check for activity.  */
  for (indx = 0; indx < num_scbs; ++indx)
    serial_done_wait_handle (scbs[indx]);
  if (event == WAIT_FAILED)
    return -1;
  if (event == WAIT_TIMEOUT)
    return 0;
  /* Run through the READFDS, clearing bits corresponding to descriptors
     for which input is unavailable.  */
  h = handles[event - WAIT_OBJECT_0];
  for (fd = 0, indx = 0; fd < n; ++fd)
    {
      HANDLE fd_h;

      if ((!readfds || !FD_ISSET (fd, readfds))
	  && (!exceptfds || !FD_ISSET (fd, exceptfds)))
	continue;

      if (readfds && FD_ISSET (fd, readfds))
	{
	  fd_h = handles[indx++];
	  /* This handle might be ready, even though it wasn't the handle
	     returned by WaitForMultipleObjects.  */
	  if (fd_h != h && WaitForSingleObject (fd_h, 0) != WAIT_OBJECT_0)
	    FD_CLR (fd, readfds);
	  else
	    num_ready++;
	}

      if (exceptfds && FD_ISSET (fd, exceptfds))
	{
	  fd_h = handles[indx++];
	  /* This handle might be ready, even though it wasn't the handle
	     returned by WaitForMultipleObjects.  */
	  if (fd_h != h && WaitForSingleObject (fd_h, 0) != WAIT_OBJECT_0)
	    FD_CLR (fd, exceptfds);
	  else
	    num_ready++;
	}
    }

  /* With multi-threaded SIGINT handling, there is a race between the
     readline signal handler and GDB.  It may still be in
     rl_prep_terminal in another thread.  Do not return until it is
     done; we can check the state here because we never longjmp from
     signal handlers on Windows.  */
  while (RL_ISSTATE (RL_STATE_SIGHANDLER))
    Sleep (1);

  return num_ready;
}
