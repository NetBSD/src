/* This testcase is part of GDB, the GNU debugger.

   Copyright 2002-2016 Free Software Foundation, Inc.

   Copyright 1992, 1993, 1994, 1995, 1999, 2002, 2003, 2007, 2008, 2009
   Free Software Foundation, Inc.

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

   This file is copied from schedlock.c.  */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

void *thread_function (void *arg); /* Pointer to function executed by each thread */

static pthread_barrier_t threads_started_barrier;

#define NUM 15

static int num_threads = NUM;

static unsigned int shared_var = 1;

int main () {
    int res;
    pthread_t threads[NUM];
    void *thread_result;
    long i;

    pthread_barrier_init (&threads_started_barrier, NULL, NUM + 1);

    for (i = 0; i < NUM; i++)
      {
        res = pthread_create (&threads[i],
                             NULL,
                             thread_function,
			     (void *) i);
      }

    pthread_barrier_wait (&threads_started_barrier);

    sleep (180); /* all threads started */

    exit (EXIT_SUCCESS);
}

void
loop (void)
{
}

void *thread_function (void *arg) {
    int my_number = (long) arg;

    pthread_barrier_wait (&threads_started_barrier);

    /* Don't run forever.  Run just short of it :)  */
    while (shared_var > 0)
      {
        shared_var++;
	usleep (1); /* Loop increment.  */
	loop ();
      }

    pthread_exit (NULL);
}

