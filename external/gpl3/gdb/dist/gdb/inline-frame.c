/* Inline frame unwinder for GDB.

   Copyright (C) 2008-2013 Free Software Foundation, Inc.

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
#include "inline-frame.h"
#include "addrmap.h"
#include "block.h"
#include "frame-unwind.h"
#include "inferior.h"
#include "regcache.h"
#include "symtab.h"
#include "vec.h"

#include "gdb_assert.h"

/* We need to save a few variables for every thread stopped at the
   virtual call site of an inlined function.  If there was always a
   "struct thread_info", we could hang it off that; in the mean time,
   keep our own list.  */
struct inline_state
{
  /* The thread this data relates to.  It should be a currently
     stopped thread; we assume thread IDs never change while the
     thread is stopped.  */
  ptid_t ptid;

  /* The number of inlined functions we are skipping.  Each of these
     functions can be stepped in to.  */
  int skipped_frames;

  /* Only valid if SKIPPED_FRAMES is non-zero.  This is the PC used
     when calculating SKIPPED_FRAMES; used to check whether we have
     moved to a new location by user request.  If so, we invalidate
     any skipped frames.  */
  CORE_ADDR saved_pc;

  /* Only valid if SKIPPED_FRAMES is non-zero.  This is the symbol
     of the outermost skipped inline function.  It's used to find the
     call site of the current frame.  */
  struct symbol *skipped_symbol;
};

typedef struct inline_state inline_state_s;
DEF_VEC_O(inline_state_s);

static VEC(inline_state_s) *inline_states;

/* Locate saved inlined frame state for PTID, if it exists
   and is valid.  */

static struct inline_state *
find_inline_frame_state (ptid_t ptid)
{
  struct inline_state *state;
  int ix;

  for (ix = 0; VEC_iterate (inline_state_s, inline_states, ix, state); ix++)
    {
      if (ptid_equal (state->ptid, ptid))
	{
	  struct regcache *regcache = get_thread_regcache (ptid);
	  CORE_ADDR current_pc = regcache_read_pc (regcache);

	  if (current_pc != state->saved_pc)
	    {
	      /* PC has changed - this context is invalid.  Use the
		 default behavior.  */
	      VEC_unordered_remove (inline_state_s, inline_states, ix);
	      return NULL;
	    }
	  else
	    return state;
	}
    }

  return NULL;
}

/* Allocate saved inlined frame state for PTID.  */

static struct inline_state *
allocate_inline_frame_state (ptid_t ptid)
{
  struct inline_state *state;

  state = VEC_safe_push (inline_state_s, inline_states, NULL);
  memset (state, 0, sizeof (*state));
  state->ptid = ptid;

  return state;
}

/* Forget about any hidden inlined functions in PTID, which is new or
   about to be resumed.  PTID may be minus_one_ptid (all processes)
   or a PID (all threads in this process).  */

void
clear_inline_frame_state (ptid_t ptid)
{
  struct inline_state *state;
  int ix;

  if (ptid_equal (ptid, minus_one_ptid))
    {
      VEC_free (inline_state_s, inline_states);
      return;
    }

  if (ptid_is_pid (ptid))
    {
      VEC (inline_state_s) *new_states = NULL;
      int pid = ptid_get_pid (ptid);

      for (ix = 0;
	   VEC_iterate (inline_state_s, inline_states, ix, state);
	   ix++)
	if (pid != ptid_get_pid (state->ptid))
	  VEC_safe_push (inline_state_s, new_states, state);
      VEC_free (inline_state_s, inline_states);
      inline_states = new_states;
      return;
    }

  for (ix = 0; VEC_iterate (inline_state_s, inline_states, ix, state); ix++)
    if (ptid_equal (state->ptid, ptid))
      {
	VEC_unordered_remove (inline_state_s, inline_states, ix);
	return;
      }
}

static void
inline_frame_this_id (struct frame_info *this_frame,
		      void **this_cache,
		      struct frame_id *this_id)
{
  struct symbol *func;

  /* In order to have a stable frame ID for a given inline function,
     we must get the stack / special addresses from the underlying
     real frame's this_id method.  So we must call get_prev_frame.
     Because we are inlined into some function, there must be previous
     frames, so this is safe - as long as we're careful not to
     create any cycles.  */
  *this_id = get_frame_id (get_prev_frame (this_frame));

  /* We need a valid frame ID, so we need to be based on a valid
     frame.  FSF submission NOTE: this would be a good assertion to
     apply to all frames, all the time.  That would fix the ambiguity
     of null_frame_id (between "no/any frame" and "the outermost
     frame").  This will take work.  */
  gdb_assert (frame_id_p (*this_id));

  /* For now, require we don't match outer_frame_id either (see
     comment above).  */
  gdb_assert (!frame_id_eq (*this_id, outer_frame_id));

  /* Future work NOTE: Alexandre Oliva applied a patch to GCC 4.3
     which generates DW_AT_entry_pc for inlined functions when
     possible.  If this attribute is available, we should use it
     in the frame ID (and eventually, to set breakpoints).  */
  func = get_frame_function (this_frame);
  gdb_assert (func != NULL);
  (*this_id).code_addr = BLOCK_START (SYMBOL_BLOCK_VALUE (func));
  (*this_id).artificial_depth++;
}

static struct value *
inline_frame_prev_register (struct frame_info *this_frame, void **this_cache,
			    int regnum)
{
  /* Use get_frame_register_value instead of
     frame_unwind_got_register, to avoid requiring this frame's ID.
     This frame's ID depends on the previous frame's ID (unusual), and
     the previous frame's ID depends on this frame's unwound
     registers.  If unwinding registers from this frame called
     get_frame_id, there would be a loop.

     Do not copy this code into any other unwinder!  Inlined functions
     are special; other unwinders must not have a dependency on the
     previous frame's ID, and therefore can and should use
     frame_unwind_got_register instead.  */
  return get_frame_register_value (this_frame, regnum);
}

/* Check whether we are at an inlining site that does not already
   have an associated frame.  */

static int
inline_frame_sniffer (const struct frame_unwind *self,
		      struct frame_info *this_frame,
		      void **this_cache)
{
  CORE_ADDR this_pc;
  struct block *frame_block, *cur_block;
  int depth;
  struct frame_info *next_frame;
  struct inline_state *state = find_inline_frame_state (inferior_ptid);

  this_pc = get_frame_address_in_block (this_frame);
  frame_block = block_for_pc (this_pc);
  if (frame_block == NULL)
    return 0;

  /* Calculate DEPTH, the number of inlined functions at this
     location.  */
  depth = 0;
  cur_block = frame_block;
  while (BLOCK_SUPERBLOCK (cur_block))
    {
      if (block_inlined_p (cur_block))
	depth++;

      cur_block = BLOCK_SUPERBLOCK (cur_block);
    }

  /* Check how many inlined functions already have frames.  */
  for (next_frame = get_next_frame (this_frame);
       next_frame && get_frame_type (next_frame) == INLINE_FRAME;
       next_frame = get_next_frame (next_frame))
    {
      gdb_assert (depth > 0);
      depth--;
    }

  /* If this is the topmost frame, or all frames above us are inlined,
     then check whether we were requested to skip some frames (so they
     can be stepped into later).  */
  if (state != NULL && state->skipped_frames > 0 && next_frame == NULL)
    {
      gdb_assert (depth >= state->skipped_frames);
      depth -= state->skipped_frames;
    }

  /* If all the inlined functions here already have frames, then pass
     to the normal unwinder for this PC.  */
  if (depth == 0)
    return 0;

  /* If the next frame is an inlined function, but not the outermost, then
     we are the next outer.  If it is not an inlined function, then we
     are the innermost inlined function of a different real frame.  */
  return 1;
}

const struct frame_unwind inline_frame_unwind = {
  INLINE_FRAME,
  default_frame_unwind_stop_reason,
  inline_frame_this_id,
  inline_frame_prev_register,
  NULL,
  inline_frame_sniffer
};

/* Return non-zero if BLOCK, an inlined function block containing PC,
   has a group of contiguous instructions starting at PC (but not
   before it).  */

static int
block_starting_point_at (CORE_ADDR pc, struct block *block)
{
  struct blockvector *bv;
  struct block *new_block;

  bv = blockvector_for_pc (pc, NULL);
  if (BLOCKVECTOR_MAP (bv) == NULL)
    return 0;

  new_block = addrmap_find (BLOCKVECTOR_MAP (bv), pc - 1);
  if (new_block == NULL)
    return 1;

  if (new_block == block || contained_in (new_block, block))
    return 0;

  /* The immediately preceding address belongs to a different block,
     which is not a child of this one.  Treat this as an entrance into
     BLOCK.  */
  return 1;
}

/* Skip all inlined functions whose call sites are at the current PC.
   Frames for the hidden functions will not appear in the backtrace until the
   user steps into them.  */

void
skip_inline_frames (ptid_t ptid)
{
  CORE_ADDR this_pc;
  struct block *frame_block, *cur_block;
  struct symbol *last_sym = NULL;
  int skip_count = 0;
  struct inline_state *state;

  /* This function is called right after reinitializing the frame
     cache.  We try not to do more unwinding than absolutely
     necessary, for performance.  */
  this_pc = get_frame_pc (get_current_frame ());
  frame_block = block_for_pc (this_pc);

  if (frame_block != NULL)
    {
      cur_block = frame_block;
      while (BLOCK_SUPERBLOCK (cur_block))
	{
	  if (block_inlined_p (cur_block))
	    {
	      /* See comments in inline_frame_this_id about this use
		 of BLOCK_START.  */
	      if (BLOCK_START (cur_block) == this_pc
		  || block_starting_point_at (this_pc, cur_block))
		{
		  skip_count++;
		  last_sym = BLOCK_FUNCTION (cur_block);
		}
	      else
		break;
	    }
	  cur_block = BLOCK_SUPERBLOCK (cur_block);
	}
    }

  gdb_assert (find_inline_frame_state (ptid) == NULL);
  state = allocate_inline_frame_state (ptid);
  state->skipped_frames = skip_count;
  state->saved_pc = this_pc;
  state->skipped_symbol = last_sym;

  if (skip_count != 0)
    reinit_frame_cache ();
}

/* Step into an inlined function by unhiding it.  */

void
step_into_inline_frame (ptid_t ptid)
{
  struct inline_state *state = find_inline_frame_state (ptid);

  gdb_assert (state != NULL && state->skipped_frames > 0);
  state->skipped_frames--;
  reinit_frame_cache ();
}

/* Return the number of hidden functions inlined into the current
   frame.  */

int
inline_skipped_frames (ptid_t ptid)
{
  struct inline_state *state = find_inline_frame_state (ptid);

  if (state == NULL)
    return 0;
  else
    return state->skipped_frames;
}

/* If one or more inlined functions are hidden, return the symbol for
   the function inlined into the current frame.  */

struct symbol *
inline_skipped_symbol (ptid_t ptid)
{
  struct inline_state *state = find_inline_frame_state (ptid);

  gdb_assert (state != NULL);
  return state->skipped_symbol;
}

/* Return the number of functions inlined into THIS_FRAME.  Some of
   the callees may not have associated frames (see
   skip_inline_frames).  */

int
frame_inlined_callees (struct frame_info *this_frame)
{
  struct frame_info *next_frame;
  int inline_count = 0;

  /* First count how many inlined functions at this PC have frames
     above FRAME (are inlined into FRAME).  */
  for (next_frame = get_next_frame (this_frame);
       next_frame && get_frame_type (next_frame) == INLINE_FRAME;
       next_frame = get_next_frame (next_frame))
    inline_count++;

  /* Simulate some most-inner inlined frames which were suppressed, so
     they can be stepped into later.  If we are unwinding already
     outer frames from some non-inlined frame this does not apply.  */
  if (next_frame == NULL)
    inline_count += inline_skipped_frames (inferior_ptid);

  return inline_count;
}
