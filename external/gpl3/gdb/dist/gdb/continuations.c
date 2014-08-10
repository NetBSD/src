/* Continuations for GDB, the GNU debugger.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
  struct continuation *new = XNEW (struct continuation);

  new->next = *pmy_chain;
  new->function = function;
  new->free_arg = free_arg;
  new->arg = arg;
  *pmy_chain = new;
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

/* Add a continuation to the continuation list of THREAD.  The new
   continuation will be added at the front.  */

void
add_continuation (struct thread_info *thread,
		  continuation_ftype *hook, void *args,
		  continuation_free_arg_ftype *free_arg)
{
  make_continuation (&thread->continuations, hook, args, free_arg);
}

static void
restore_thread_cleanup (void *arg)
{
  ptid_t *ptid_p = arg;

  switch_to_thread (*ptid_p);
}

/* Walk down the continuation list of PTID, and execute all the
   continuations.  There is a problem though.  In some cases new
   continuations may be added while we are in the middle of this loop.
   If this happens they will be added in the front, and done before we
   have a chance of exhausting those that were already there.  We need
   to then save the beginning of the list in a pointer and do the
   continuations from there on, instead of using the global beginning
   of list as our iteration pointer.  */

static void
do_all_continuations_ptid (ptid_t ptid,
			   struct continuation **continuations_p,
			   int err)
{
  struct cleanup *old_chain;
  ptid_t current_thread;

  if (*continuations_p == NULL)
    return;

  current_thread = inferior_ptid;

  /* Restore selected thread on exit.  Don't try to restore the frame
     as well, because:

     - When running continuations, the selected frame is always #0.

     - The continuations may trigger symbol file loads, which may
     change the frame layout (frame ids change), which would trigger
     a warning if we used make_cleanup_restore_current_thread.  */

  old_chain = make_cleanup (restore_thread_cleanup, &current_thread);

  /* Let the continuation see this thread as selected.  */
  switch_to_thread (ptid);

  do_my_continuations (continuations_p, err);

  do_cleanups (old_chain);
}

/* Callback for iterate over threads.  */

static int
do_all_continuations_thread_callback (struct thread_info *thread, void *data)
{
  int err = * (int *) data;
  do_all_continuations_ptid (thread->ptid, &thread->continuations, err);
  return 0;
}

/* Do all continuations of thread THREAD.  */

void
do_all_continuations_thread (struct thread_info *thread, int err)
{
  do_all_continuations_thread_callback (thread, &err);
}

/* Do all continuations of all threads.  */

void
do_all_continuations (int err)
{
  iterate_over_threads (do_all_continuations_thread_callback, &err);
}

/* Callback for iterate over threads.  */

static int
discard_all_continuations_thread_callback (struct thread_info *thread,
					   void *data)
{
  discard_my_continuations (&thread->continuations);
  return 0;
}

/* Get rid of all the continuations of THREAD.  */

void
discard_all_continuations_thread (struct thread_info *thread)
{
  discard_all_continuations_thread_callback (thread, NULL);
}

/* Get rid of all the continuations of all threads.  */

void
discard_all_continuations (void)
{
  iterate_over_threads (discard_all_continuations_thread_callback, NULL);
}


/* Add a continuation to the intermediate continuation list of THREAD.
   The new continuation will be added at the front.  */

void
add_intermediate_continuation (struct thread_info *thread,
			       continuation_ftype *hook,
			       void *args,
			       continuation_free_arg_ftype *free_arg)
{
  make_continuation (&thread->intermediate_continuations, hook,
		     args, free_arg);
}

/* Walk down the cmd_continuation list, and execute all the
   continuations.  There is a problem though.  In some cases new
   continuations may be added while we are in the middle of this
   loop.  If this happens they will be added in the front, and done
   before we have a chance of exhausting those that were already
   there.  We need to then save the beginning of the list in a pointer
   and do the continuations from there on, instead of using the
   global beginning of list as our iteration pointer.  */

static int
do_all_intermediate_continuations_thread_callback (struct thread_info *thread,
						   void *data)
{
  int err = * (int *) data;

  do_all_continuations_ptid (thread->ptid,
			     &thread->intermediate_continuations, err);
  return 0;
}

/* Do all intermediate continuations of thread THREAD.  */

void
do_all_intermediate_continuations_thread (struct thread_info *thread, int err)
{
  do_all_intermediate_continuations_thread_callback (thread, &err);
}

/* Do all intermediate continuations of all threads.  */

void
do_all_intermediate_continuations (int err)
{
  iterate_over_threads (do_all_intermediate_continuations_thread_callback,
			&err);
}

/* Callback for iterate over threads.  */

static int
discard_all_intermediate_continuations_thread_callback (struct thread_info *thread,
							void *data)
{
  discard_my_continuations (&thread->intermediate_continuations);
  return 0;
}

/* Get rid of all the intermediate continuations of THREAD.  */

void
discard_all_intermediate_continuations_thread (struct thread_info *thread)
{
  discard_all_intermediate_continuations_thread_callback (thread, NULL);
}

/* Get rid of all the intermediate continuations of all threads.  */

void
discard_all_intermediate_continuations (void)
{
  iterate_over_threads (discard_all_intermediate_continuations_thread_callback,
			NULL);
}
