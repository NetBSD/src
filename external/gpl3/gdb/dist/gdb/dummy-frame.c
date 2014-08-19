/* Code dealing with dummy stack frames, for GDB, the GNU debugger.

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
#include "dummy-frame.h"
#include "regcache.h"
#include "frame.h"
#include "inferior.h"
#include "gdb_assert.h"
#include "frame-unwind.h"
#include "command.h"
#include "gdbcmd.h"
#include <string.h>
#include "observer.h"
#include "gdbthread.h"

/* Dummy frame.  This saves the processor state just prior to setting
   up the inferior function call.  Older targets save the registers
   on the target stack (but that really slows down function calls).  */

struct dummy_frame
{
  struct dummy_frame *next;
  /* This frame's ID.  Must match the value returned by
     gdbarch_dummy_id.  */
  struct frame_id id;
  /* The caller's state prior to the call.  */
  struct infcall_suspend_state *caller_state;
};

static struct dummy_frame *dummy_frame_stack = NULL;

/* Push the caller's state, along with the dummy frame info, onto the
   dummy-frame stack.  */

void
dummy_frame_push (struct infcall_suspend_state *caller_state,
		  const struct frame_id *dummy_id)
{
  struct dummy_frame *dummy_frame;

  dummy_frame = XZALLOC (struct dummy_frame);
  dummy_frame->caller_state = caller_state;
  dummy_frame->id = (*dummy_id);
  dummy_frame->next = dummy_frame_stack;
  dummy_frame_stack = dummy_frame;
}

/* Remove *DUMMY_PTR from the dummy frame stack.  */

static void
remove_dummy_frame (struct dummy_frame **dummy_ptr)
{
  struct dummy_frame *dummy = *dummy_ptr;

  *dummy_ptr = dummy->next;
  discard_infcall_suspend_state (dummy->caller_state);
  xfree (dummy);
}

/* Delete any breakpoint B which is a momentary breakpoint for return from
   inferior call matching DUMMY_VOIDP.  */

static int
pop_dummy_frame_bpt (struct breakpoint *b, void *dummy_voidp)
{
  struct dummy_frame *dummy = dummy_voidp;

  if (b->thread == pid_to_thread_id (inferior_ptid)
      && b->disposition == disp_del && frame_id_eq (b->frame_id, dummy->id))
    {
      while (b->related_breakpoint != b)
	delete_breakpoint (b->related_breakpoint);

      delete_breakpoint (b);

      /* Stop the traversal.  */
      return 1;
    }

  /* Continue the traversal.  */
  return 0;
}

/* Pop *DUMMY_PTR, restoring program state to that before the
   frame was created.  */

static void
pop_dummy_frame (struct dummy_frame **dummy_ptr)
{
  struct dummy_frame *dummy = *dummy_ptr;

  restore_infcall_suspend_state (dummy->caller_state);

  iterate_over_breakpoints (pop_dummy_frame_bpt, dummy);

  /* restore_infcall_control_state frees inf_state,
     all that remains is to pop *dummy_ptr.  */
  *dummy_ptr = dummy->next;
  xfree (dummy);

  /* We've made right mess of GDB's local state, just discard
     everything.  */
  reinit_frame_cache ();
}

/* Look up DUMMY_ID.
   Return NULL if not found.  */

static struct dummy_frame **
lookup_dummy_frame (struct frame_id dummy_id)
{
  struct dummy_frame **dp;

  for (dp = &dummy_frame_stack; *dp != NULL; dp = &(*dp)->next)
    {
      if (frame_id_eq ((*dp)->id, dummy_id))
	return dp;
    }

  return NULL;
}

/* Pop the dummy frame DUMMY_ID, restoring program state to that before the
   frame was created.
   On return reinit_frame_cache has been called.
   If the frame isn't found, flag an internal error.

   NOTE: This can only pop the one frame, even if it is in the middle of the
   stack, because the other frames may be for different threads, and there's
   currently no way to tell which stack frame is for which thread.  */

void
dummy_frame_pop (struct frame_id dummy_id)
{
  struct dummy_frame **dp;

  dp = lookup_dummy_frame (dummy_id);
  gdb_assert (dp != NULL);

  pop_dummy_frame (dp);
}

/* Drop dummy frame DUMMY_ID.  Do nothing if it is not found.  Do not restore
   its state into inferior, just free its memory.  */

void
dummy_frame_discard (struct frame_id dummy_id)
{
  struct dummy_frame **dp;

  dp = lookup_dummy_frame (dummy_id);
  if (dp)
    remove_dummy_frame (dp);
}

/* There may be stale dummy frames, perhaps left over from when an uncaught
   longjmp took us out of a function that was called by the debugger.  Clean
   them up at least once whenever we start a new inferior.  */

static void
cleanup_dummy_frames (struct target_ops *target, int from_tty)
{
  while (dummy_frame_stack != NULL)
    remove_dummy_frame (&dummy_frame_stack);
}

/* Return the dummy frame cache, it contains both the ID, and a
   pointer to the regcache.  */
struct dummy_frame_cache
{
  struct frame_id this_id;
  struct regcache *prev_regcache;
};

static int
dummy_frame_sniffer (const struct frame_unwind *self,
		     struct frame_info *this_frame,
		     void **this_prologue_cache)
{
  struct dummy_frame *dummyframe;
  struct frame_id this_id;

  /* When unwinding a normal frame, the stack structure is determined
     by analyzing the frame's function's code (be it using brute force
     prologue analysis, or the dwarf2 CFI).  In the case of a dummy
     frame, that simply isn't possible.  The PC is either the program
     entry point, or some random address on the stack.  Trying to use
     that PC to apply standard frame ID unwind techniques is just
     asking for trouble.  */
  
  /* Don't bother unless there is at least one dummy frame.  */
  if (dummy_frame_stack != NULL)
    {
      /* Use an architecture specific method to extract this frame's
	 dummy ID, assuming it is a dummy frame.  */
      this_id = gdbarch_dummy_id (get_frame_arch (this_frame), this_frame);

      /* Use that ID to find the corresponding cache entry.  */
      for (dummyframe = dummy_frame_stack;
	   dummyframe != NULL;
	   dummyframe = dummyframe->next)
	{
	  if (frame_id_eq (dummyframe->id, this_id))
	    {
	      struct dummy_frame_cache *cache;

	      cache = FRAME_OBSTACK_ZALLOC (struct dummy_frame_cache);
	      cache->prev_regcache = get_infcall_suspend_state_regcache
						   (dummyframe->caller_state);
	      cache->this_id = this_id;
	      (*this_prologue_cache) = cache;
	      return 1;
	    }
	}
    }
  return 0;
}

/* Given a call-dummy dummy-frame, return the registers.  Here the
   register value is taken from the local copy of the register buffer.  */

static struct value *
dummy_frame_prev_register (struct frame_info *this_frame,
			   void **this_prologue_cache,
			   int regnum)
{
  struct dummy_frame_cache *cache = (*this_prologue_cache);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct value *reg_val;

  /* The dummy-frame sniffer always fills in the cache.  */
  gdb_assert (cache != NULL);

  /* Describe the register's location.  Generic dummy frames always
     have the register value in an ``expression''.  */
  reg_val = value_zero (register_type (gdbarch, regnum), not_lval);

  /* Use the regcache_cooked_read() method so that it, on the fly,
     constructs either a raw or pseudo register from the raw
     register cache.  */
  regcache_cooked_read (cache->prev_regcache, regnum,
			value_contents_writeable (reg_val));
  return reg_val;
}

/* Assuming that THIS_FRAME is a dummy, return its ID.  That ID is
   determined by examining the NEXT frame's unwound registers using
   the method dummy_id().  As a side effect, THIS dummy frame's
   dummy cache is located and saved in THIS_PROLOGUE_CACHE.  */

static void
dummy_frame_this_id (struct frame_info *this_frame,
		     void **this_prologue_cache,
		     struct frame_id *this_id)
{
  /* The dummy-frame sniffer always fills in the cache.  */
  struct dummy_frame_cache *cache = (*this_prologue_cache);

  gdb_assert (cache != NULL);
  (*this_id) = cache->this_id;
}

const struct frame_unwind dummy_frame_unwind =
{
  DUMMY_FRAME,
  default_frame_unwind_stop_reason,
  dummy_frame_this_id,
  dummy_frame_prev_register,
  NULL,
  dummy_frame_sniffer,
};

static void
fprint_dummy_frames (struct ui_file *file)
{
  struct dummy_frame *s;

  for (s = dummy_frame_stack; s != NULL; s = s->next)
    {
      gdb_print_host_address (s, file);
      fprintf_unfiltered (file, ":");
      fprintf_unfiltered (file, " id=");
      fprint_frame_id (file, s->id);
      fprintf_unfiltered (file, "\n");
    }
}

static void
maintenance_print_dummy_frames (char *args, int from_tty)
{
  if (args == NULL)
    fprint_dummy_frames (gdb_stdout);
  else
    {
      struct cleanup *cleanups;
      struct ui_file *file = gdb_fopen (args, "w");

      if (file == NULL)
	perror_with_name (_("maintenance print dummy-frames"));
      cleanups = make_cleanup_ui_file_delete (file);
      fprint_dummy_frames (file);    
      do_cleanups (cleanups);
    }
}

extern void _initialize_dummy_frame (void);

void
_initialize_dummy_frame (void)
{
  add_cmd ("dummy-frames", class_maintenance, maintenance_print_dummy_frames,
	   _("Print the contents of the internal dummy-frame stack."),
	   &maintenanceprintlist);

  observer_attach_inferior_created (cleanup_dummy_frames);
}
