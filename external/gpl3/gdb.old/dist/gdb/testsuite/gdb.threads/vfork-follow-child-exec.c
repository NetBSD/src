/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2023 Free Software Foundation, Inc.

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

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

static char *program_name;

static void *
f (void *arg)
{
  int res = vfork ();

  if (res == -1)
    {
      perror ("vfork");
      return NULL;
    }
  else if (res == 0)
    {
      /* Child.  */
      execl (program_name, program_name, "1", NULL);
      perror ("exec");
      abort ();
    }
  else
    {
      /* Parent.  */
      return NULL;
    }
}

int
main (int argc, char **argv)
{
  pthread_t tid;

  if (argc > 1)
    {
      /* Getting here via execl.  */
      return 0;
    }

  program_name = argv[0];

  pthread_create (&tid, NULL, f, NULL);
  pthread_join (tid, NULL);
  return 0;
}
