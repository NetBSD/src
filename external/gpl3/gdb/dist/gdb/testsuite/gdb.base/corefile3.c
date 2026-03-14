/* Copyright 1992-2025 Free Software Foundation, Inc.

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

/* This file is based on coremaker.c.  */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAPSIZE (8 * 1024)

/* Global pointers so it's easier to access them from GDB.  */

char *rw_mapping = NULL;
char *malloc_buffer = NULL;
char *anon_mapping = NULL;
char *shm_mapping = NULL;

/* Create mappings within this process.  */

void
mmapdata ()
{
  /* Allocate and initialize a buffer that will be used to write the file
     that is later mapped in.  */

  malloc_buffer = (char *) malloc (MAPSIZE);
  for (int j = 0; j < MAPSIZE; ++j)
    malloc_buffer[j] = j;

  /* Write the file to map in.  */

  int fd = open ("coremmap.data", O_CREAT | O_RDWR, 0666);
  assert (fd != -1);
  write (fd, malloc_buffer, MAPSIZE);

  /* Now map the file into our address space as RW_MAPPING.  */

  rw_mapping
    = (char *) mmap (0, MAPSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  assert (rw_mapping != (char *) MAP_FAILED);

  /* Verify that the original data and the mapped data are identical.  If
     not, we'd rather fail now than when trying to access the mapped data
     from the core file. */

  for (int j = 0; j < MAPSIZE; ++j)
    assert (malloc_buffer[j] == rw_mapping[j]);

  /* Touch RW_MAPPING so the kernel writes it out into 'core'.  */
  rw_mapping[0] = malloc_buffer[0];

  /* Create yet another region which is allocated, but not written to.  */
  anon_mapping = mmap (NULL, MAPSIZE, PROT_READ | PROT_WRITE,
		       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert (anon_mapping != MAP_FAILED);

  /* Create a shared memory mapping.  */
  int sid = shmget (IPC_PRIVATE, MAPSIZE, IPC_CREAT | IPC_EXCL | 0777);
  assert (sid != -1);
  shm_mapping = (char *) shmat (sid, NULL, 0);
  int res = shmctl (sid, IPC_RMID, NULL);
  assert (res == 0);
  assert (shm_mapping != MAP_FAILED);
}

void
func2 ()
{
#ifdef SA_FULLDUMP
  /* Force a corefile that includes the data section for AIX.  */
  {
    struct sigaction sa;

    sigaction (SIGABRT, (struct sigaction *)0, &sa);
    sa.sa_flags |= SA_FULLDUMP;
    sigaction (SIGABRT, &sa, (struct sigaction *)0);
  }
#endif

  abort ();
}

void
func1 ()
{
  func2 ();
}

int
main (void)
{
  mmapdata ();
  func1 ();
  return 0;
}
