/* This file is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

/* Set a watchdog that aborts the testcase after a timeout.  */

#ifndef GDB_WATCHDOG_H
#define GDB_WATCHDOG_H

/* Forward declaration to make sure the definitions have the right
   prototype, at least in C.  */
static void gdb_watchdog (unsigned int seconds);

static const char _gdb_watchdog_msg[]
  = "gdb_watchdog: timeout expired - aborting test\n";

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

static VOID CALLBACK
_gdb_watchdog_timer_routine (PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
  fputs (_gdb_watchdog_msg, stderr);
  abort ();
}

static void
gdb_watchdog (unsigned int seconds)
{
  HANDLE timer;

  if (!CreateTimerQueueTimer (&timer, NULL,
			      _gdb_watchdog_timer_routine, NULL,
			      seconds * 1000, 0, 0))
    abort ();
}

#else /* POSIX systems */

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

static void
_gdb_sigalrm_handler (int signo)
{
  write (2, _gdb_watchdog_msg, sizeof (_gdb_watchdog_msg) - 1);
  abort ();
}

static void
gdb_watchdog (unsigned int seconds)
{
  signal (SIGALRM, _gdb_sigalrm_handler);
  alarm (seconds);
}

#endif

#endif /* GDB_WATCHDOG_H */
