/* Copyright (C) 2016 Free Software Foundation, Inc.

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
#include "signals-state-save-restore.h"

#include <signal.h>

/* The original signal actions and mask.  */

#ifdef HAVE_SIGACTION
static struct sigaction original_signal_actions[NSIG];

/* Note that we use sigprocmask without worrying about threads because
   the save/restore functions are called either from main, or after a
   fork.  In both cases, we know the calling process is single
   threaded.  */
static sigset_t original_signal_mask;
#endif

/* See signals-state-save-restore.h.   */

void
save_original_signals_state (void)
{
#ifdef HAVE_SIGACTION
  int i;
  int res;

  res = sigprocmask (0,  NULL, &original_signal_mask);
  if (res == -1)
    perror_with_name (("sigprocmask"));

  for (i = 1; i < NSIG; i++)
    {
      struct sigaction *oldact = &original_signal_actions[i];

      res = sigaction (i, NULL, oldact);
      if (res == -1 && errno == EINVAL)
	{
	  /* Some signal numbers in the range are invalid.  */
	  continue;
	}
      else if (res == -1)
	perror_with_name (("sigaction"));

      /* If we find a custom signal handler already installed, then
	 this function was called too late.  */
      if (oldact->sa_handler != SIG_DFL && oldact->sa_handler != SIG_IGN)
	internal_error (__FILE__, __LINE__, _("unexpected signal handler"));
    }
#endif
}

/* See signals-state-save-restore.h.   */

void
restore_original_signals_state (void)
{
#ifdef HAVE_SIGACTION
  int i;
  int res;

  for (i = 1; i < NSIG; i++)
    {
      res = sigaction (i, &original_signal_actions[i], NULL);
      if (res == -1 && errno == EINVAL)
	{
	  /* Some signal numbers in the range are invalid.  */
	  continue;
	}
      else if (res == -1)
	perror_with_name (("sigaction"));
    }

  res = sigprocmask (SIG_SETMASK,  &original_signal_mask, NULL);
  if (res == -1)
    perror_with_name (("sigprocmask"));
#endif
}
