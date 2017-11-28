/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2017 Free Software Foundation, Inc.

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

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

static void
marker ()
{}

#define STACK_SIZE 0x1000

static int
clone_fn (void *unused)
{
  return 0;
}

int
main (void)
{
  int i, pid;
  unsigned char *stack[6];

  for (i = 0; i < (sizeof (stack) / sizeof (stack[0])); i++)
    stack[i] = malloc (STACK_SIZE);

  for (i = 0; i < (sizeof (stack) / sizeof (stack[0])); i++)
    {
      pid = clone (clone_fn, stack[i] + STACK_SIZE, CLONE_FILES | CLONE_VM,
		   NULL);
    }

  for (i = 0; i < (sizeof (stack) / sizeof (stack[0])); i++)
    free (stack[i]);

  marker ();
}
