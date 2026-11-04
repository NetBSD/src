/* This testcase is part of GDB, the GNU debugger.

   Copyright 2024 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>

/* This type is used by GDB.  */
struct lib_type
{
  int a;
  int b;
  int c;
};

/* Ensure the type above is used.  */
volatile struct lib_type global_lib_object = { 1, 2, 3 };

/* This pointer is checked by GDB.  */
volatile void *opaque_ptr = 0;

void
lib_func_test_syscall (void)
{
  puts ("Inside library\n");
  fflush (stdout);
}

static void
sig_handler (int signo)
{
  /* Nothing.  */
}

void
lib_func_test_signal (void)
{
  signal (SIGUSR1, sig_handler);

  kill (getpid (), SIGUSR1);
}

void
lib_func_test_fork (void)
{
  pid_t pid = fork ();
  assert (pid != -1);

  if (pid == 0)
    {
      /* Child: just exit.  */
      exit (0);
    }

  /* Parent.  */
  waitpid (pid, NULL, 0);
}
