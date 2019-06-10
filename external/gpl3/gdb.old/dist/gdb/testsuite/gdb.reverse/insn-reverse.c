/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2017 Free Software Foundation, Inc.

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

typedef void (*testcase_ftype) (void);

/* The arch-specific files need to implement both the initialize function
   and define the testcases array.  */

#if (defined __aarch64__)
#include "insn-reverse-aarch64.c"
#elif (defined __arm__)
#include "insn-reverse-arm.c"
#elif (defined __x86_64__) || (defined __i386__)
#include "insn-reverse-x86.c"
#else
/* We get here if the current architecture being tested doesn't have any
   record/replay instruction decoding tests implemented.  */
static testcase_ftype testcases[] = {};

/* Dummy implementation in case this target doesn't have any record/replay
   instruction decoding tests implemented.  */

static void
initialize (void)
{
}
#endif

/* GDB will read n_testcases to know how many functions to test.  The
   functions are implemented in arch-specific files and the testcases
   array is defined together with them.  */
static int n_testcases = (sizeof (testcases) / sizeof (testcase_ftype));

int
main ()
{
  int i = 0;

  /* Initialize any required arch-specific bits.  */
  initialize ();

  for (i = 0; i < n_testcases; i++)
    testcases[i] ();

  return 0;
}
