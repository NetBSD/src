/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2016 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <libspe2.h>
#include <pthread.h>
#include <sys/wait.h>

extern spe_program_handle_t coremaker_spu;
#define nr_t 5

void *
spe_thread (void * arg)
{
  int flags = 0;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  spe_context_ptr_t *ctx = (spe_context_ptr_t *) arg;

  spe_program_load (*ctx, &coremaker_spu);
  spe_context_run (*ctx, &entry, flags, NULL, NULL, NULL);

  pthread_exit (NULL);
}

int
main (void)
{
  int thread_id[nr_t];
  pthread_attr_t attr;
  pthread_t pts[nr_t];
  spe_context_ptr_t ctx[nr_t];
  unsigned int value;
  int cnt;

  /* Use small thread stacks to speed up writing out core file.  */
  pthread_attr_init (&attr);
  pthread_attr_setstacksize (&attr, 2*PTHREAD_STACK_MIN);

  for (cnt = 0; cnt < nr_t; cnt++)
    {
      ctx[cnt] = spe_context_create (0, NULL);
      thread_id[cnt]
	= pthread_create (&pts[cnt], &attr, &spe_thread, &ctx[cnt]);
    }

  pthread_attr_destroy (&attr);

  for (cnt = 0; cnt < nr_t; cnt++)
    spe_out_intr_mbox_read (ctx[cnt], &value, 1, SPE_MBOX_ALL_BLOCKING);

  abort ();
}

