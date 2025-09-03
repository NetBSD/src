/* Copyright 2007-2024 Free Software Foundation, Inc.

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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <errno.h>

/* Expects 4 arguments:

   1. Either 'standard' or 'detached', where 'standard' tests
   a general gcore script spawn with its controlling terminal available
   and 'detached' tests gcore script spawn without its controlling
   terminal available.
   2. Path to the gcore script.
   3. Path to the data-directory to pass to the gcore script.
   4. The core file output name.  */

int
main (int argc, char **argv)
{
  pid_t pid = 0;
  pid_t ppid;
  char buf[1024*2 + 500];
  int gotint, res;

  assert (argc == 5);

  pid = fork ();

  switch (pid)
    {
    case 0:
      if (strcmp (argv[1], "detached") == 0)
	setpgrp ();
      ppid = getppid ();
      gotint = snprintf (buf, sizeof (buf), "%s -d %s -o %s %d",
			 argv[2], argv[3], argv[4], (int) ppid);
      assert (gotint < sizeof (buf));
      res = system (buf);
      assert (res != -1);
      break;

    case -1:
      perror ("fork err\n");
      exit (1);
      break;

    default:
      do
	{
	  res = waitpid (pid, NULL, 0);
	}
      while (res == -1 && errno == EINTR);

      assert (res == pid);
      break;
    }

  return 0;
}
