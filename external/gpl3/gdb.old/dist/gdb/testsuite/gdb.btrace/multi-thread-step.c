/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2016 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

static pthread_barrier_t barrier;
static int global;

static void *
test (void *arg)
{
  pthread_barrier_wait (&barrier);

  global = 42; /* bp.1 */

  pthread_barrier_wait (&barrier);

  global = 42; /* bp.2 */

  return arg;
}

int
main (void)
{
  pthread_t th;

  pthread_barrier_init (&barrier, NULL, 2);
  pthread_create (&th, NULL, test, NULL);

  test (NULL);

  pthread_join (th, NULL);
  pthread_barrier_destroy (&barrier);

  return 0; /* bp.3 */
}
