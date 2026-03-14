/* This testcase is part of GDB, the GNU debugger.

   Copyright 2025 Free Software Foundation, Inc.

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

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

void
breakpt (void)
{
  /* Nothing.  */
}

int
main (void)
{
  /* Create a shared memory mapping.  */
  int sid = shmget (IPC_PRIVATE, 0x1000, IPC_CREAT | IPC_EXCL | 0777);
  if (sid == -1)
    {
      perror ("shmget");
      exit (1);
    }

  /* Attach the shared memory mapping.  */
  void *addr = shmat (sid, NULL, SHM_RND);
  if (addr == (void *) -1L)
    {
      perror ("shmat");
      exit (1);
    }

  breakpt ();

  /* Mark the shared memory mapping as deleted -- once the last user
     has finished with it.  */
  if (shmctl (sid, IPC_RMID, NULL) != 0)
    {
      perror ("shmctl");
      exit (1);
    }

  return 0;
}
