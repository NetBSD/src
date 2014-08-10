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

   Contributed by Markus Deuling <deuling@de.ibm.com>  */

#include <stdio.h>
#include <libspe2.h>
#include <pthread.h>
#include <sys/wait.h>

extern spe_program_handle_t size_spu;
#define nr_t 1

void *
spe_thread (void *arg)
{
  int flags = 0;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  spe_context_ptr_t *ctx = (spe_context_ptr_t *) arg;

  spe_program_load (*ctx, &size_spu);
  spe_context_run (*ctx, &entry, flags, NULL, NULL, NULL);

  pthread_exit (NULL);
}

int
main (void)
{
  int thread_id[nr_t];
  pthread_t pts[nr_t];
  spe_context_ptr_t ctx[nr_t];
  int cnt;

  int c  = sizeof (char);
  printf ("sizeof(char)=%d\n", c);

  int s  = sizeof (short);
  printf ("sizeof(short)=%d\n", s);

  int i  = sizeof (int);
  printf ("sizeof(int)=%d\n", i);

  int l  = sizeof (long);
  printf ("sizeof(long)=%d\n", l);

  int ll = sizeof (long long);
  printf ("sizeof(long long)=%d\n", ll);

  int f  = sizeof (float);
  printf ("sizeof(float)=%d\n", f);

  int d  = sizeof (double);
  printf ("sizeof(double)=%d\n", d);

  int ld = sizeof (long double);
  printf ("sizeof(long double)=%d\n", ld);

  for (cnt = 0; cnt < nr_t; cnt++)
    {
      ctx[cnt] = spe_context_create (0, NULL);
      thread_id[cnt]
	= pthread_create (&pts[cnt], NULL, &spe_thread, &ctx[cnt]);
    }

  for (cnt = 0; cnt < nr_t; cnt++)
    pthread_join (pts[cnt], NULL);

  for (cnt = 0; cnt < nr_t; cnt++)
    spe_context_destroy (ctx[cnt]);

  return 0;
}
