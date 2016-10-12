/* Wrapper implementation for waitpid for GNU/Linux (LWP layer).

   Copyright (C) 2001-2015 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
/* FIXME: server.h is required for the definition of debug_threads
   which is used in the gdbserver-specific debug printing in
   linux_debug.  This code should be made available to GDB also,
   but the lack of a suitable flag to enable it prevents this.  */
#include "server.h"
#endif

#include "linux-nat.h"
#include "linux-waitpid.h"
#include "gdb_wait.h"

/* Print debugging output based on the format string FORMAT and
   its parameters.  */

static inline void
linux_debug (const char *format, ...)
{
#ifdef GDBSERVER
  if (debug_threads)
    {
      va_list args;
      va_start (args, format);
      vfprintf (stderr, format, args);
      va_end (args);
    }
#endif
}

/* Convert wait status STATUS to a string.  Used for printing debug
   messages only.  */

char *
status_to_str (int status)
{
  static char buf[64];

  if (WIFSTOPPED (status))
    {
      if (WSTOPSIG (status) == SYSCALL_SIGTRAP)
	snprintf (buf, sizeof (buf), "%s (stopped at syscall)",
		  strsignal (SIGTRAP));
      else
	snprintf (buf, sizeof (buf), "%s (stopped)",
		  strsignal (WSTOPSIG (status)));
    }
  else if (WIFSIGNALED (status))
    snprintf (buf, sizeof (buf), "%s (terminated)",
	      strsignal (WTERMSIG (status)));
  else
    snprintf (buf, sizeof (buf), "%d (exited)", WEXITSTATUS (status));

  return buf;
}

/* Wrapper function for waitpid which handles EINTR, and emulates
   __WALL for systems where that is not available.  */

int
my_waitpid (int pid, int *status, int flags)
{
  int ret, out_errno;

  linux_debug ("my_waitpid (%d, 0x%x)\n", pid, flags);

  if (flags & __WALL)
    {
      sigset_t block_mask, org_mask, wake_mask;
      int wnohang;

      wnohang = (flags & WNOHANG) != 0;
      flags &= ~(__WALL | __WCLONE);

      if (!wnohang)
	{
	  flags |= WNOHANG;

	  /* Block all signals while here.  This avoids knowing about
	     LinuxThread's signals.  */
	  sigfillset (&block_mask);
	  sigprocmask (SIG_BLOCK, &block_mask, &org_mask);

	  /* ... except during the sigsuspend below.  */
	  sigemptyset (&wake_mask);
	}

      while (1)
	{
	  /* Since all signals are blocked, there's no need to check
	     for EINTR here.  */
	  ret = waitpid (pid, status, flags);
	  out_errno = errno;

	  if (ret == -1 && out_errno != ECHILD)
	    break;
	  else if (ret > 0)
	    break;

	  if (flags & __WCLONE)
	    {
	      /* We've tried both flavors now.  If WNOHANG is set,
		 there's nothing else to do, just bail out.  */
	      if (wnohang)
		break;

	      linux_debug ("blocking\n");

	      /* Block waiting for signals.  */
	      sigsuspend (&wake_mask);
	    }
	  flags ^= __WCLONE;
	}

      if (!wnohang)
	sigprocmask (SIG_SETMASK, &org_mask, NULL);
    }
  else
    {
      do
	ret = waitpid (pid, status, flags);
      while (ret == -1 && errno == EINTR);
      out_errno = errno;
    }

  linux_debug ("my_waitpid (%d, 0x%x): status(%x), %d\n",
	       pid, flags, (ret > 0 && status != NULL) ? *status : -1, ret);

  errno = out_errno;
  return ret;
}
