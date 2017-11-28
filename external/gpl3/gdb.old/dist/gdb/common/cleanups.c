/* Cleanup routines for GDB, the GNU debugger.

   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "cleanups.h"

/* The cleanup list records things that have to be undone
   if an error happens (descriptors to be closed, memory to be freed, etc.)
   Each link in the chain records a function to call and an
   argument to give it.

   Use make_cleanup to add an element to the cleanup chain.
   Use do_cleanups to do all cleanup actions back to a given
   point in the chain.  Use discard_cleanups to remove cleanups
   from the chain back to a given point, not doing them.

   If the argument is pointer to allocated memory, then you need
   to additionally set the 'free_arg' member to a function that will
   free that memory.  This function will be called both when the cleanup
   is executed and when it's discarded.  */

struct cleanup
{
  struct cleanup *next;
  void (*function) (void *);
  void (*free_arg) (void *);
  void *arg;
};

/* Used to mark the end of a cleanup chain.
   The value is chosen so that it:
   - is non-NULL so that make_cleanup never returns NULL,
   - causes a segv if dereferenced
     [though this won't catch errors that a value of, say,
     ((struct cleanup *) -1) will]
   - displays as something useful when printed in gdb.
   This is const for a bit of extra robustness.
   It is initialized to coax gcc into putting it into .rodata.
   All fields are initialized to survive -Wextra.  */
static const struct cleanup sentinel_cleanup = { 0, 0, 0, 0 };

/* Handy macro to use when referring to sentinel_cleanup.  */
#define SENTINEL_CLEANUP ((struct cleanup *) &sentinel_cleanup)

/* Chain of cleanup actions established with make_cleanup,
   to be executed if an error happens.  */
static struct cleanup *cleanup_chain = SENTINEL_CLEANUP;

/* Chain of cleanup actions established with make_final_cleanup,
   to be executed when gdb exits.  */
static struct cleanup *final_cleanup_chain = SENTINEL_CLEANUP;

/* Main worker routine to create a cleanup.
   PMY_CHAIN is a pointer to either cleanup_chain or final_cleanup_chain.
   FUNCTION is the function to call to perform the cleanup.
   ARG is passed to FUNCTION when called.
   FREE_ARG, if non-NULL, is called after the cleanup is performed.

   The result is a pointer to the previous chain pointer
   to be passed later to do_cleanups or discard_cleanups.  */

static struct cleanup *
make_my_cleanup2 (struct cleanup **pmy_chain, make_cleanup_ftype *function,
		  void *arg,  void (*free_arg) (void *))
{
  struct cleanup *newobj = XNEW (struct cleanup);
  struct cleanup *old_chain = *pmy_chain;

  newobj->next = *pmy_chain;
  newobj->function = function;
  newobj->free_arg = free_arg;
  newobj->arg = arg;
  *pmy_chain = newobj;

  gdb_assert (old_chain != NULL);
  return old_chain;
}

/* Worker routine to create a cleanup without a destructor.
   PMY_CHAIN is a pointer to either cleanup_chain or final_cleanup_chain.
   FUNCTION is the function to call to perform the cleanup.
   ARG is passed to FUNCTION when called.

   The result is a pointer to the previous chain pointer
   to be passed later to do_cleanups or discard_cleanups.  */

static struct cleanup *
make_my_cleanup (struct cleanup **pmy_chain, make_cleanup_ftype *function,
		 void *arg)
{
  return make_my_cleanup2 (pmy_chain, function, arg, NULL);
}

/* Add a new cleanup to the cleanup_chain,
   and return the previous chain pointer
   to be passed later to do_cleanups or discard_cleanups.
   Args are FUNCTION to clean up with, and ARG to pass to it.  */

struct cleanup *
make_cleanup (make_cleanup_ftype *function, void *arg)
{
  return make_my_cleanup (&cleanup_chain, function, arg);
}

/* Same as make_cleanup except also includes DTOR, a destructor to free ARG.
   DTOR is invoked when the cleanup is performed or when it is discarded.  */

struct cleanup *
make_cleanup_dtor (make_cleanup_ftype *function, void *arg,
		   make_cleanup_dtor_ftype *dtor)
{
  return make_my_cleanup2 (&cleanup_chain,
			   function, arg, dtor);
}

/* Same as make_cleanup except the cleanup is added to final_cleanup_chain.  */

struct cleanup *
make_final_cleanup (make_cleanup_ftype *function, void *arg)
{
  return make_my_cleanup (&final_cleanup_chain, function, arg);
}

/* Worker routine to perform cleanups.
   PMY_CHAIN is a pointer to either cleanup_chain or final_cleanup_chain.
   OLD_CHAIN is the result of a "make" cleanup routine.
   Cleanups are performed until we get back to the old end of the chain.  */

static void
do_my_cleanups (struct cleanup **pmy_chain,
		struct cleanup *old_chain)
{
  struct cleanup *ptr;

  while ((ptr = *pmy_chain) != old_chain)
    {
      *pmy_chain = ptr->next;	/* Do this first in case of recursion.  */
      (*ptr->function) (ptr->arg);
      if (ptr->free_arg)
	(*ptr->free_arg) (ptr->arg);
      xfree (ptr);
    }
}

/* Return a value that can be passed to do_cleanups, do_final_cleanups to
   indicate perform all cleanups.  */

struct cleanup *
all_cleanups (void)
{
  return SENTINEL_CLEANUP;
}

/* Discard cleanups and do the actions they describe
   until we get back to the point OLD_CHAIN in the cleanup_chain.  */

void
do_cleanups (struct cleanup *old_chain)
{
  do_my_cleanups (&cleanup_chain, old_chain);
}

/* Discard cleanups and do the actions they describe
   until we get back to the point OLD_CHAIN in the final_cleanup_chain.  */

void
do_final_cleanups (struct cleanup *old_chain)
{
  do_my_cleanups (&final_cleanup_chain, old_chain);
}

/* Main worker routine to discard cleanups.
   PMY_CHAIN is a pointer to either cleanup_chain or final_cleanup_chain.
   OLD_CHAIN is the result of a "make" cleanup routine.
   Cleanups are discarded until we get back to the old end of the chain.  */

static void
discard_my_cleanups (struct cleanup **pmy_chain,
		     struct cleanup *old_chain)
{
  struct cleanup *ptr;

  while ((ptr = *pmy_chain) != old_chain)
    {
      *pmy_chain = ptr->next;
      if (ptr->free_arg)
	(*ptr->free_arg) (ptr->arg);
      xfree (ptr);
    }
}

/* Discard cleanups, not doing the actions they describe,
   until we get back to the point OLD_CHAIN in the cleanup chain.  */

void
discard_cleanups (struct cleanup *old_chain)
{
  discard_my_cleanups (&cleanup_chain, old_chain);
}

/* Discard final cleanups, not doing the actions they describe,
   until we get back to the point OLD_CHAIN in the final cleanup chain.  */

void
discard_final_cleanups (struct cleanup *old_chain)
{
  discard_my_cleanups (&final_cleanup_chain, old_chain);
}

/* Main worker routine to save cleanups.
   PMY_CHAIN is a pointer to either cleanup_chain or final_cleanup_chain.
   The chain is emptied and the result is a pointer to the old chain.  */

static struct cleanup *
save_my_cleanups (struct cleanup **pmy_chain)
{
  struct cleanup *old_chain = *pmy_chain;

  *pmy_chain = SENTINEL_CLEANUP;
  return old_chain;
}

/* Set the cleanup_chain to 0, and return the old cleanup_chain.  */

struct cleanup *
save_cleanups (void)
{
  return save_my_cleanups (&cleanup_chain);
}

/* Set the final_cleanup_chain to 0, and return the old
   final_cleanup_chain.  */

struct cleanup *
save_final_cleanups (void)
{
  return save_my_cleanups (&final_cleanup_chain);
}

/* Main worker routine to save cleanups.
   PMY_CHAIN is a pointer to either cleanup_chain or final_cleanup_chain.
   The chain is restored from CHAIN.  */

static void
restore_my_cleanups (struct cleanup **pmy_chain, struct cleanup *chain)
{
  if (*pmy_chain != SENTINEL_CLEANUP)
    internal_warning (__FILE__, __LINE__,
		      _("restore_my_cleanups has found a stale cleanup"));

  *pmy_chain = chain;
}

/* Restore the cleanup chain from a previously saved chain.  */

void
restore_cleanups (struct cleanup *chain)
{
  restore_my_cleanups (&cleanup_chain, chain);
}

/* Restore the final cleanup chain from a previously saved chain.  */

void
restore_final_cleanups (struct cleanup *chain)
{
  restore_my_cleanups (&final_cleanup_chain, chain);
}

/* Provide a known function that does nothing, to use as a base for
   a possibly long chain of cleanups.  This is useful where we
   use the cleanup chain for handling normal cleanups as well as dealing
   with cleanups that need to be done as a result of a call to error().
   In such cases, we may not be certain where the first cleanup is, unless
   we have a do-nothing one to always use as the base.  */

void
null_cleanup (void *arg)
{
}
