/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2015 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <signal.h>

#define NUM_THREADS 10
const int num_threads = NUM_THREADS;

pthread_t child_thread[NUM_THREADS];
volatile pthread_t signal_thread;
volatile int got_sig;

void
handler (int signo)
{
  got_sig = 1;
}

void
loop_broke (void)
{
}

#define INF_LOOP				\
  do						\
    {						\
      while (!got_sig)				\
	;					\
    }						\
  while (0)

void *
child_function (void *arg)
{
  pthread_t self = pthread_self ();

  while (1)
    {
      INF_LOOP; /* set thread breakpoint here */
      loop_broke ();
    }
}

int
main (void)
{
  int res;
  int i;

  alarm (60);

  signal (SIGUSR1, handler);

  for (i = 0; i < NUM_THREADS; i++)
    {
      res = pthread_create (&child_thread[i], NULL, child_function, NULL);
    }

  while (1)
    {
      pthread_kill (signal_thread, SIGUSR1); /* set kill breakpoint here */
      INF_LOOP;
      loop_broke ();
    }

  exit(EXIT_SUCCESS);
}
