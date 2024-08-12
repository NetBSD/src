/* Copyright 2022-2024 Free Software Foundation, Inc.

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

#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 3

int
is_matching_tid (int *tid_ptr, int tid_value)
{
  return *tid_ptr == tid_value;
}

int
return_true ()
{
  return 1;
}

int
return_false ()
{
  return 0;
}

int
function_that_segfaults ()
{
  int *p = 0;
  *p = 1;	/* Segfault happens here.   */
}

int
function_with_breakpoint ()
{
  return 1;	/* Nested breakpoint.  */
}

void *
worker_func (void *arg)
{
  int a = 42;	/* Breakpoint here.  */
}

void
stop_marker ()
{
  int b = 99;	/* Stop marker.  */
}

int
main ()
{
  pthread_t threads[NUM_THREADS];
  int args[NUM_THREADS];

  alarm (300);

  for (int i = 0; i < NUM_THREADS; i++)
    {
      args[i] = i;
      pthread_create (&threads[i], NULL, worker_func, &args[i]);
    }

  for (int i = 0; i < NUM_THREADS; i++)
    {
      void *retval;
      pthread_join (threads[i], &retval);
    }

  stop_marker ();

  return 0;
}
