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
#include <libspe2.h>
#include <pthread.h>
#include <sys/wait.h>

extern spe_program_handle_t ea_cache_spu;
int int_var = 23;

void *
spe_thread (void *arg)
{
  int flags = 0;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  spe_context_ptr_t *ctx = (spe_context_ptr_t *) arg;

  spe_program_load (*ctx, &ea_cache_spu);
  spe_context_run (*ctx, &entry, flags, &int_var, NULL, NULL);

  pthread_exit (NULL);
}

int
main (void)
{
  spe_context_ptr_t ctx;
  pthread_t pts;
  int thread_id;

  printf ("ppe.c | int_var vor %d | adr int_var %p\n", int_var, &int_var);

  /* Create SPE context and pthread.  */
  ctx = spe_context_create (0, NULL);
  thread_id = pthread_create (&pts, NULL, &spe_thread, &ctx);

  /* Join the pthread.  */
  pthread_join (pts, NULL);

  /* Destroy the SPE context.  */
  spe_context_destroy (ctx);

  printf ("ppe.c | int_var nach %d\n", int_var);

  return 0;
}

