/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2025 Free Software Foundation, Inc.

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
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM 4

static pthread_barrier_t threads_started_barrier;

static void
stop_here ()
{
}

static void
spin ()
{
  while (1)
    usleep (1);
}

static void *
work (void *arg)
{
  int id = *(int *) arg;

  pthread_barrier_wait (&threads_started_barrier);

  if (id % 2 == 0)
    stop_here ();
  else
    spin ();

  pthread_exit (NULL);
}

int
main ()
{
  /* Ensure we stop if GDB crashes and DejaGNU fails to kill us.  */
  alarm (10);

  pthread_t threads[NUM];
  int ids[NUM];

  pthread_barrier_init (&threads_started_barrier, NULL, NUM + 1);

  for (int i = 0; i < NUM; i++)
    {
      ids[i] = i;
      pthread_create (&threads[i], NULL, work, &ids[i]);
    }

  /* Wait until all threads are seen running.  */
  pthread_barrier_wait (&threads_started_barrier);

  stop_here ();

  return 0;
}
