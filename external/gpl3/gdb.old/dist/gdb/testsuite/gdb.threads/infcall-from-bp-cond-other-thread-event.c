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
#include <stdlib.h>
#include <sched.h>

#define NUM_THREADS 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Some global variables to poke, just for something to do.  */
volatile int global_var_0 = 0;
volatile int global_var_1 = 0;

/* This flag is updated from GDB.  */
volatile int raise_signal = 0;

/* Implement the breakpoint condition function.  Release the other thread
   and try to give the other thread a chance to run.  Then return ANSWER.  */
int
condition_core_func (int answer)
{
  /* This unlock should release the other thread.  */
  if (pthread_mutex_unlock (&mutex) != 0)
    abort ();

  /* And this yield and sleep should (hopefully) give the other thread a
     chance to run.  This isn't guaranteed of course, but once the other
     thread does run it should hit a breakpoint, which GDB should
     (temporarily) ignore, so there's no easy way for us to know the other
     thread has done what it needs to, thus, yielding and sleeping is the
     best we can do.  */
  sched_yield ();
  sleep (2);

  return answer;
}

void
stop_marker ()
{
  int a = 100;	/* Final breakpoint here.  */
}

/* A breakpoint condition function that always returns true.  */
int
condition_true_func ()
{
  return condition_core_func (1);
}

/* A breakpoint condition function that always returns false.  */
int
condition_false_func ()
{
  return condition_core_func (0);
}

void *
worker_func (void *arg)
{
  volatile int *ptr = 0;
  int tid = *((int *) arg);

  switch (tid)
    {
    case 0:
      global_var_0 = 11;	/* First thread breakpoint.  */
      break;

    case 1:
      if (pthread_mutex_lock (&mutex) != 0)
	abort ();
      if (raise_signal)
	global_var_1 = *ptr;	/* Signal here.  */
      else
	global_var_1 = 99;	/* Other thread breakpoint.  */
      break;

    default:
      abort ();
    }

  return NULL;
}

int
main ()
{
  pthread_t threads[NUM_THREADS];
  int args[NUM_THREADS];

  /* Set an alarm, just in case the test deadlocks.  */
  alarm (300);

  /* We want the mutex to start locked.  */
  if (pthread_mutex_lock (&mutex) != 0)
    abort ();

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

  /* Unlock once we're done, just for cleanliness.  */
  if (pthread_mutex_unlock (&mutex) != 0)
    abort ();

  stop_marker ();

  return 0;
}
