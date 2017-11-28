/* Continuations for GDB, the GNU debugger.

   Copyright (C) 1986-2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "gdbthread.h"
#include "inferior.h"
#include "continuations.h"

struct continuation
{
  struct continuation *next;
  continuation_ftype *function;
  continuation_free_arg_ftype *free_arg;
  void *arg;
};

/* Add a new continuation to the continuation chain.  Args are
   FUNCTION to run the continuation up with, and ARG to pass to
   it.  */

static void
make_continuation (struct continuation **pmy_chain,
		   continuation_ftype *function,
		   void *arg,  void (*free_arg) (void *))
{
  struct continuation *newobj = XNEW (struct continuation);

  newobj->next = *pmy_chain;
  newobj->function = function;
  newobj->free_arg = free_arg;
  newobj->arg = arg;
  *pmy_chain = newobj;
}

static void
do_my_continuations_1 (struct continuation **pmy_chain, int err)
{
  struct continuation *ptr;

  while ((ptr = *pmy_chain) != NULL)
    {
      *pmy_chain = ptr->next;	/* Do this first in case of recursion.  */
      (*ptr->function) (ptr->arg, err);
      if (ptr->free_arg)
	(*ptr->free_arg) (ptr->arg);
      xfree (ptr);
    }
}

static void
do_my_continuations (struct continuation **list, int err)
{
  struct continuation *continuations;

  if (*list == NULL)
    return;

  /* Copy the list header into another pointer, and set the global
     list header to null, so that the global list can change as a side
     effect of invoking the continuations and the processing of the
     preexisting continuations will not be affected.  */

  continuations = *list;
  *list = NULL;

  /* Work now on the list we have set aside.  */
  do_my_continuations_1 (&continuations, err);
}

static void
discard_my_continuations_1 (struct continuation **pmy_chain)
{
  struct continuation *ptr;

  while ((ptr = *pmy_chain) != NULL)
    {
      *pmy_chain = ptr->next;
      if (ptr->free_arg)
	(*ptr->free_arg) (ptr->arg);
      xfree (ptr);
    }
}

static void
discard_my_continuations (struct continuation **list)
{
  discard_my_continuations_1 (list);
  *list = NULL;
}

/* Add a continuation to the continuation list of INFERIOR.  The new
   continuation will be added at the front.  */

void
add_inferior_continuation (continuation_ftype *hook, void *args,
			   continuation_free_arg_ftype *free_arg)
{
  struct inferior *inf = current_inferior ();

  make_continuation (&inf->continuations, hook, args, free_arg);
}

/* Do all continuations of the current inferior.  */

void
do_all_inferior_continuations (int err)
{
  struct inferior *inf = current_inferior ();
  do_my_continuations (&inf->continuations, err);
}

/* Get rid of all the inferior-wide continuations of INF.  */

void
discard_all_inferior_continuations (struct inferior *inf)
{
  discard_my_continuations (&inf->continuations);
}
