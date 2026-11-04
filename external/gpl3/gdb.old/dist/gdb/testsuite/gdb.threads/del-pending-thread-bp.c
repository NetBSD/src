/* Copyright 2023 Free Software Foundation, Inc.

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

#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>

pthread_barrier_t barrier;

static void
barrier_wait (pthread_barrier_t *b)
{
  int res = pthread_barrier_wait (b);
  if (res != 0 && res != PTHREAD_BARRIER_SERIAL_THREAD)
    abort ();
}

static void *
thread_worker (void *arg)
{
  barrier_wait (&barrier);
  return NULL;
}

void
breakpt (void)
{
  /* Nothing.  */
}

int
main (void)
{
  void *handle;
  void (*func)(int);
  pthread_t thread;

  if (pthread_barrier_init (&barrier, NULL, 2) != 0)
    abort ();

  if (pthread_create (&thread, NULL, thread_worker, NULL) != 0)
    abort ();

  breakpt ();

  /* Allow the worker thread to complete.  */
  barrier_wait (&barrier);

  if (pthread_join (thread, NULL) != 0)
    abort ();

  breakpt ();

  /* Now load the shared library.  */
  handle = dlopen (SHLIB_NAME, RTLD_LAZY);
  if (handle == NULL)
    abort ();

  /* Find the function symbol.  */
  func = (void (*)(int)) dlsym (handle, "foo");

  /* Call the library function.  */
  func (1);

  /* Unload the shared library.  */
  if (dlclose (handle) != 0)
    abort ();

  breakpt ();

  return 0;
}

