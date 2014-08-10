/* This testcase is part of GDB, the GNU debugger.

   Copyright 2002-2014 Free Software Foundation, Inc.

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

   This file is based on schedlock.c.  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

int THREADS = 10;
unsigned int *args;
volatile int done = 0;

void *
thread_function (void *arg)
{
  int my_number = (long) arg;
  volatile int *myp = (volatile int *) &args[my_number];

  /* Don't run forever.  Run just short of it :)  */
  while (*myp > 0 && !done)
    {
      (*myp)++; /* set breakpoint here */
    }

  if (done)
    usleep (100); /* Some time to make sure we don't mask any bad
		     SIGTRAP handling.  */

  pthread_exit (NULL);
}

int
main (int argc, char **argv)
{
  int res;
  pthread_t *threads;
  void *thread_result;
  long i = 0;

  threads = malloc (THREADS * sizeof (pthread_t));
  args = malloc (THREADS * sizeof (unsigned int));

  for (i = 0; i < THREADS; i++)
    {
      args[i] = 1; /* Init value.  */
      res = pthread_create (&threads[i],
			    NULL,
			    thread_function,
			    (void *) i);
    }

  for (i = 0; i < THREADS; i++)
    pthread_join (threads[i], &thread_result);

  exit(EXIT_SUCCESS);
}
