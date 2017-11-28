/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2016 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

int save_parent;

/* The fork child.  Just runs forever.  */

static int
fork_child (void)
{
  while (1)
    {
      sleep (1);

      /* Exit if GDB kills the parent.  */
      if (getppid () != save_parent)
	break;
      if (kill (getppid (), 0) != 0)
	break;
    }

  return 0;
}

/* The fork parent.  Just runs forever waiting for the child to
   exit.  */

static int
fork_parent (void)
{
  if (wait (NULL) == -1)
    {
      perror ("wait");
      return 1;
    }

  return 0;
}

int
main (void)
{
  pid_t pid;

  save_parent = getpid ();

  /* Don't run forever.  */
  alarm (180);

  /* The parent and child should basically run forever without
     tripping on any debug event.  We want to check that GDB updates
     the parent and child running states correctly right after the
     fork.  */
  pid = fork ();
  if (pid > 0)
    return fork_parent ();
  else if (pid == 0)
    return fork_child ();
  else
    {
      perror ("fork");
      return 1;
    }
}
