/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2013 Free Software Foundation, Inc.

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

   Contributed by Ulrich Weigand <uweigand@de.ibm.com>  */

#include <stdio.h>
#include <stdlib.h>
#include <libspe2.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern spe_program_handle_t fork_spu;

void *
spe_thread (void * arg)
{
  int flags = 0;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  spe_context_ptr_t *ctx = (spe_context_ptr_t *) arg;

  spe_program_load (*ctx, &fork_spu);
  spe_context_run (*ctx, &entry, flags, NULL, NULL, NULL);

  pthread_exit (NULL);
}

int
main (void)
{
  pthread_t pts;
  spe_context_ptr_t ctx;
  unsigned int value;
  unsigned int pid;

  ctx = spe_context_create (0, NULL);
  pthread_create (&pts, NULL, &spe_thread, &ctx);

  /* Wait until the SPU thread is running.  */
  spe_out_intr_mbox_read (ctx, &value, 1, SPE_MBOX_ALL_BLOCKING);

  pid = fork ();
  if (pid == 0)
    {
      /* This is the child.  Just exit immediately.  */
      exit (0);
    }
  else
    {
      /* This is the parent.  Wait for the child to exit.  */
      waitpid (pid, NULL, 0);
    }

  /* Tell SPU to continue.  */
  spe_in_mbox_write (ctx, &value, 1, SPE_MBOX_ALL_BLOCKING);
  
  pthread_join (pts, NULL);
  spe_context_destroy (ctx);

  return 0;
}

