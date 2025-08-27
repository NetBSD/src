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
#include <semaphore.h>
#include <stdlib.h>

#define NUM_THREADS 5

/* Semaphores, used to track when threads have started, and to control
   when the threads finish.  */
sem_t startup_semaphore;
sem_t finish_semaphore;

/* Mutex to control when the first worker thread hit a breakpoint
   location.  */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Global variable to poke, just so threads have something to do.  */
volatile int global_var = 0;

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

void *
worker_func (void *arg)
{
  int tid = *((int *) arg);

  switch (tid)
    {
    case 0:
      /* Wait for MUTEX to become available, then pass through the
	 conditional breakpoint location.  */
      if (pthread_mutex_lock (&mutex) != 0)
	abort ();
      global_var = 99;	/* Conditional breakpoint here.  */
      if (pthread_mutex_unlock (&mutex) != 0)
	abort ();
      break;

    default:
      /* Notify the main thread that the thread has started, then wait for
	 the main thread to tell us to finish.  */
      sem_post (&startup_semaphore);
      if (sem_wait (&finish_semaphore) != 0)
	abort ();
      break;
    }
}

void
stop_marker ()
{
  global_var = 99;	/* Stop marker.  */
}

int
main ()
{
  pthread_t threads[NUM_THREADS];
  int args[NUM_THREADS];
  void *retval;

  /* An alarm, just in case the thread deadlocks.  */
  alarm (300);

  /* Semaphore initialization.  */
  if (sem_init (&startup_semaphore, 0, 0) != 0)
    abort ();
  if (sem_init (&finish_semaphore, 0, 0) != 0)
    abort ();

  /* Lock MUTEX, this prevents the first worker thread from rushing ahead.  */
  if (pthread_mutex_lock (&mutex) != 0)
    abort ();

  /* Worker thread creation.  */
  for (int i = 0; i < NUM_THREADS; i++)
    {
      args[i] = i;
      pthread_create (&threads[i], NULL, worker_func, &args[i]);
    }

  /* Wait for every thread (other than the first) to tell us it has started
     up.  */
  for (int i = 1; i < NUM_THREADS; i++)
    {
      if (sem_wait (&startup_semaphore) != 0)
	abort ();
    }

  /* Unlock the first thread so it can proceed.  */
  if (pthread_mutex_unlock (&mutex) != 0)
    abort ();

  /* Wait for the first thread only.  */
  pthread_join (threads[0], &retval);

  /* Now post FINISH_SEMAPHORE to allow all the other threads to finish.  */
  for (int i = 1; i < NUM_THREADS; i++)
    sem_post (&finish_semaphore);

  /* Now wait for the remaining threads to complete.  */
  for (int i = 1; i < NUM_THREADS; i++)
    pthread_join (threads[i], &retval);

  /* Semaphore cleanup.  */
  sem_destroy (&finish_semaphore);
  sem_destroy (&startup_semaphore);

  stop_marker ();

  return 0;
}
