/* Host support routines for MinGW, for GDB, the GNU debugger.

   Copyright (C) 2006-2013 Free Software Foundation, Inc.

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

#include "gdb_assert.h"
#include "gdb_select.h"
#include "gdb_string.h"
#include "readline/readline.h"

#include <windows.h>

/* This event is signalled whenever an asynchronous SIGINT handler
   needs to perform an action in the main thread.  */
static HANDLE sigint_event;

/* When SIGINT_EVENT is signalled, gdb_select will call this
   function.  */
struct async_signal_handler *sigint_handler;

/* The strerror() function can return NULL for errno values that are
   out of range.  Provide a "safe" version that always returns a
   printable string.

   The Windows runtime implementation of strerror never returns NULL,
   but does return a useless string for anything above sys_nerr;
   unfortunately this includes all socket-related error codes.
   This replacement tries to find a system-provided error message.  */

char *
safe_strerror (int errnum)
{
  static char *buffer;
  int len;

  if (errnum >= 0 && errnum < sys_nerr)
    return strerror (errnum);

  if (buffer)
    {
      LocalFree (buffer);
      buffer = NULL;
    }

  if (FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER
		     | FORMAT_MESSAGE_FROM_SYSTEM,
		     NULL, errnum,
		     MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		     (LPTSTR) &buffer, 0, NULL) == 0)
    {
      static char buf[32];
      xsnprintf (buf, sizeof buf, "(undocumented errno %d)", errnum);
      return buf;
    }

  /* Windows error messages end with a period and a CR-LF; strip that
     out.  */
  len = strlen (buffer);
  if (len > 3 && strcmp (buffer + len - 3, ".\r\n") == 0)
    buffer[len - 3] = '\0';

  return buffer;
}

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

  gdb_assert (num_handles < MAXIMUM_WAIT_OBJECTS);
  handles[num_handles++] = sigint_event;

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

  if (h == sigint_event
      || WaitForSingleObject (sigint_event, 0) == WAIT_OBJECT_0)
    {
      if (sigint_handler != NULL)
	call_async_signal_handler (sigint_handler);

      if (num_ready == 0)
	{
	  errno = EINTR;
	  return -1;
	}
    }

  return num_ready;
}

/* Wrapper for the body of signal handlers.  On Windows systems, a
   SIGINT handler runs in its own thread.  We can't longjmp from
   there, and we shouldn't even prompt the user.  Delay HANDLER
   until the main thread is next in gdb_select.  */

void
gdb_call_async_signal_handler (struct async_signal_handler *handler,
			       int immediate_p)
{
  if (immediate_p)
    sigint_handler = handler;
  else
    {
      mark_async_signal_handler (handler);
      sigint_handler = NULL;
    }
  SetEvent (sigint_event);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_mingw_hdep;

void
_initialize_mingw_hdep (void)
{
  sigint_event = CreateEvent (0, FALSE, FALSE, 0);
}
