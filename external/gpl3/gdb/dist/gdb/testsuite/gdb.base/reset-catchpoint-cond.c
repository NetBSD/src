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

extern void lib_func_test_syscall (void);
extern void lib_func_test_signal (void);
extern void lib_func_test_fork (void);

/* We use this to perform some filler work.  */
volatile int global_var = 0;

/* Just somewhere for GDB to put a breakpoint.  */
void
breakpt_before_exit (void)
{
  /* Nothing.  */
}

int
main (void)
{
#if defined TEST_SYSCALL
  lib_func_test_syscall ();
#elif defined TEST_SIGNAL
  lib_func_test_signal ();
#elif defined TEST_FORK
  lib_func_test_fork ();
#else
# error compile with suitable -DTEST_xxx macro defined
#endif

  ++global_var;

  breakpt_before_exit ();

  return 0;
}
