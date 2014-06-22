/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2014 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Do not use threads as we need to exploit a bug in LWP code masked by the
   threads code otherwise.

   INFERIOR_PTID must point to exited LWP.  Here we use the initial LWP as it
   is automatically INFERIOR_PTID for GDB.

   Finally we need to call target_resume (RESUME_ALL, ...) which we invoke by
   NEW_THREAD_EVENT (called from the new LWP as initial LWP is exited now).  */

#define _GNU_SOURCE
#include <sched.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

#include <features.h>
#ifdef __UCLIBC__
#if !(defined(__UCLIBC_HAS_MMU__) || defined(__ARCH_HAS_MMU__))
#define HAS_NOMMU
#endif
#endif

#define STACK_SIZE 0x1000

static int
fn_return (void *unused)
{
  return 0;	/* at-fn_return */
}

static int
fn (void *unused)
{
  int i;
  unsigned char *stack;
  int new_pid;

  i = sleep (1);
  assert (i == 0);

  stack = malloc (STACK_SIZE);
  assert (stack != NULL);

  new_pid = clone (fn_return, stack + STACK_SIZE, CLONE_FILES
#if defined(__UCLIBC__) && defined(HAS_NOMMU)
		   | CLONE_VM
#endif /* defined(__UCLIBC__) && defined(HAS_NOMMU) */
		   , NULL, NULL, NULL, NULL);
  assert (new_pid > 0);

  return 0;
}

int
main (int argc, char **argv)
{
  unsigned char *stack;
  int new_pid;

  stack = malloc (STACK_SIZE);
  assert (stack != NULL);

  new_pid = clone (fn, stack + STACK_SIZE, CLONE_FILES
#if defined(__UCLIBC__) && defined(HAS_NOMMU)
		   | CLONE_VM
#endif /* defined(__UCLIBC__) && defined(HAS_NOMMU) */
		   , NULL, NULL, NULL, NULL);
  assert (new_pid > 0);

  return 0;
}
