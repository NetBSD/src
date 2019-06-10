/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2017 Free Software Foundation, Inc.

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

/* Support program for testing unwindonsignal.  */

#include <signal.h>
#include <unistd.h>

void
gen_signal ()
{
  /* According to sigall.exp, SIGABRT is always supported.  */
  kill (getpid (), SIGABRT);
}

/* Easy place to set a breakpoint.  */

void
stop_here ()
{
}

int
main ()
{

#ifdef SIG_SETMASK
  /* Ensure all the signals aren't blocked.
     The environment in which the testsuite is run may have blocked some
     for whatever reason.  */
  {
    sigset_t newset;
    sigemptyset (&newset);
    sigprocmask (SIG_SETMASK, &newset, NULL);
  }
#endif

  /* Stop here so we can hand-call gen_signal.  */
  stop_here ();

  return 0;
}
