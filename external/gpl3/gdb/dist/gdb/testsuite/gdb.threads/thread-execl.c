/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2025 Free Software Foundation, Inc.

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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static const char *image;

void *
thread_execler (void *arg)
{
  /* Exec ourselves again.  Pass an extra argument so that the
     post-exec image knows to not re-exec yet again.  */
  if (execl (image, image, "1", NULL) == -1)
    {
      perror ("execl");
      abort ();
    }

  return NULL;
}

int
main (int argc, char **argv)
{
  pthread_t thread;

  /* An extra argument means we're in the post-exec image, so we're
     done.  Don't re-exec again.  */
  if (argc > 1)
    exit (0);

  image = argv[0];

  pthread_create (&thread, NULL, thread_execler, NULL);
  pthread_join (thread, NULL);

  return 0;
}
