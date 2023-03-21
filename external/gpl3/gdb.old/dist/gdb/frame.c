/* Cache and manage frames for GDB, the GNU debugger.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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
#include "frame.h"
#include "target.h"
#include "value.h"
#include "inferior.h"	/* for inferior_ptid */
#include "regcache.h"
#include "user-regs.h"
#include "gdb_obstack.h"
#include "dummy-frame.h"
#include "sentinel-frame.h"
#include "gdbcore.h"
#include "annotate.h"
#include "language.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "command.h"
#include "gdbcmd.h"
#include "observable.h"
#include "objfiles.h"
#include "gdbthread.h"
#include "block.h"
#include "inline-frame.h"
#include "tracepoint.h"
#include "hashtab.h"
#include "valprint.h"
#include "cli/cli-option.h"

/* The sentinel frame terminates the innermost end of the frame chain.
   If unwound, it returns the information needed to construct an
   innermost frame.

   The current frame, which is the innermost frame, can be found at
   sentinel_frame->prev.  */

static struct frame_info *sentinel_frame;

/* Number of calls to reinit_frame_cache.  */
static unsigned int frame_cache_generation = 0;

/* See frame.h.  */

unsigned int
get_frame_cache_generation ()
{
  return frame_cache_generation;
}

/* The values behind the global "set backtrace ..." settings.  */
set_backtrace_options user_set_backtrace_options;

static struct frame_info *get_prev_frame_raw (struct frame_info *this_frame);
static const char *frame_stop_reason_symbol_string (enum unwind_stop_reason reason);

/* Status of some values cached in the frame_info object.  */

enum cached_copy_status
{
  /* Value is unknown.  */
  CC_UNKNOWN,

  /* We have a value.  */
  CC_VALUE,

  /* Value was not saved.  */
  CC_NOT_SAVED,

  /* Value is unavailable.  */
  CC_UNAVAILABLE
};

enum class frame_id_status
{
  /* Frame id is not computed.  */
  NOT_COMPUTED = 0,

  /* Frame id is being computed (compute_frame_id is active).  */
  COMPUTING,

  /* Frame id has been computed.  */
  COMPUTED,
};

/* We keep a cache of stack frames, each of which is a "struct
   frame_info".  The innermost one gets allocated (in
   wait_for_inferior) each time the inferior stops; sentinel_frame
   points to it.  Additional frames get allocated (in get_prev_frame)
   as needed, and are chained through the next and prev fields.  Any
   time that the frame cache becomes invalid (most notably when we
   execute something, but also if we change how we interpret the
   frames (e.g. "set heuristic-fence-post" in mips-tdep.c, or anything
   which reads new symbols)), we should call reinit_frame_cache.  */

struct frame_info
{
  /* Level of this frame.  The inner-most (youngest) frame is at level
     0.  As you move towards the outer-most (oldest) frame, the level
     increases.  This is a cached value.  It could just as easily be
     computed by counting back from the selected frame to the inner
     most frame.  */
  /* NOTE: cagney/2002-04-05: Perhaps a level of ``-1'' should be
     reserved to indicate a bogus frame - one that has been created
     just to keep GDB happy (GDB always needs a frame).  For the
     moment leave this as speculation.  */
  int level;

  /* The frame's program space.  */
  struct program_space *pspace;

  /* The frame's address space.  */
  const address_space *aspace;

  /* The frame's low-level unwinder and corresponding cache.  The
     low-level unwinder is responsible for unwinding register values
     for the previous frame.  The low-level unwind methods are
     selected based on the presence, or otherwise, of register unwind
     information such as CFI.  */
  void *prologue_cache;
  const struct frame_unwind *unwind;

  /* Cached copy of the previous frame's architecture.  */
  struct
  {
    bool p;
    struct gdbarch *arch;
  } prev_arch;

  /* Cached copy of the previous frame's resume address.  */
  struct {
    cached_copy_status status;
    /* Did VALUE require unmasking when being read.  */
    bool masked;
    CORE_ADDR value;
  } prev_pc;

  /* Cached copy of the previous frame's function address.  */
  struct
  {
    CORE_ADDR addr;
    cached_copy_status status;
  } prev_func;

  /* This frame's ID.  */
  struct
  {
    frame_id_status p;
    struct frame_id value;
  } this_id;

  /* The frame's high-level base methods, and corresponding cache.
     The high level base methods are selected based on the frame's
     debug info.  */
  const struct frame_base *base;
  void *base_cache;

  /* Pointers to the next (down, inner, younger) and previous (up,
     outer, older) frame_info's in the frame cache.  */
  struct frame_info *next; /* down, inner, younger */
  bool prev_p;
  struct frame_info *prev; /* up, outer, older */

  /* The reason why we could not set PREV, or UNWIND_NO_REASON if we
     could.  Only valid when PREV_P is set.  */
  enum unwind_stop_reason stop_reason;

  /* A frame specific string describing the STOP_REASON in more detail.
     Only valid when PREV_P is set, but even then may still be NULL.  */
  const char *stop_string;
};

/* See frame.h.  */

void
set_frame_previous_pc_masked (struct frame_info *frame)
{
  frame->prev_pc.masked = true;
}

/* See frame.h.  */

bool
get_frame_pc_masked (const struct frame_info *frame)
{
  gdb_assert (frame->next != nullptr);
  gdb_assert (frame->next->prev_pc.status == CC_VALUE);

  return frame->next->prev_pc.masked;
}

/* A frame stash used to speed up frame lookups.  Create a hash table
   to stash frames previously accessed from the frame cache for
   quicker subsequent retrieval.  The hash table is emptied whenever
   the frame cache is invalidated.  */

static htab_t frame_stash;

/* Internal function to calculate a hash from the frame_id addresses,
   using as many valid addresses as possible.  Frames below level 0
   are not stored in the hash table.  */

static hashval_t
frame_addr_hash (const void *ap)
{
  const struct frame_info *frame = (const struct frame_info *) ap;
  const struct frame_id f_id = frame->this_id.value;
  hashval_t hash = 0;

  gdb_assert (f_id.stack_status != FID_STACK_INVALID
	      || f_id.code_addr_p
	      || f_id.special_addr_p);

  if (f_id.stack_status == FID_STACK_VALID)
    hash = iterative_hash (&f_id.stack_addr,
			   sizeof (f_id.stack_addr), hash);
  if (f_id.code_addr_p)
    hash = iterative_hash (&f_id.code_addr,
			   sizeof (f_id.code_addr), hash);
  if (f_id.special_addr_p)
    hash = iterative_hash (&f_id.special_addr,
			   sizeof (f_id.special_addr), hash);

  return hash;
}

/* Internal equality function for the hash table.  This function
   defers equality operations to frame_id_eq.  */

static int
frame_addr_hash_eq (const void *a, const void *b)
{
  const struct frame_info *f_entry = (const struct frame_info *) a;
  const struct frame_info *f_element = (const struct frame_info *) b;

  return frame_id_eq (f_entry->this_id.value,
		      f_element->this_id.value);
}

/* Internal function to create the frame_stash hash table.  100 seems
   to be a good compromise to start the hash table at.  */

static void
frame_stash_create (void)
{
  frame_stash = htab_create (100,
			     frame_addr_hash,
			     frame_addr_hash_eq,
			     NULL);
}

/* Internal function to add a frame to the frame_stash hash table.
   Returns false if a frame with the same ID was already stashed, true
   otherwise.  */

static bool
frame_stash_add (frame_info *frame)
{
  /* Do not try to stash the sentinel frame.  */
  gdb_assert (frame->level >= 0);

  frame_info **slot = (struct frame_info **) htab_find_slot (frame_stash,
							     frame, INSERT);

  /* If we already have a frame in the stack with the same id, we
     either have a stack cycle (corrupted stack?), or some bug
     elsewhere in GDB.  In any case, ignore the duplicate and return
     an indication to the caller.  */
  if (*slot != nullptr)
    return false;

  *slot = frame;
  return true;
}

/* Internal function to search the frame stash for an entry with the
   given frame ID.  If found, return that frame.  Otherwise return
   NULL.  */

static struct frame_info *
frame_stash_find (struct frame_id id)
{
  struct frame_info dummy;
  struct frame_info *frame;

  dummy.this_id.value = id;
  frame = (struct frame_info *) htab_find (frame_stash, &dummy);
  return frame;
}

/* Internal function to invalidate the frame stash by removing all
   entries in it.  This only occurs when the frame cache is
   invalidated.  */

static void
frame_stash_invalidate (void)
{
  htab_empty (frame_stash);
}

/* See frame.h  */
scoped_restore_selected_frame::scoped_restore_selected_frame ()
{
  m_fid = get_frame_id (get_selected_frame (NULL));
}

/* See frame.h  */
scoped_restore_selected_frame::~scoped_restore_selected_frame ()
{
  frame_info *frame = frame_find_by_id (m_fid);
  if (frame == NULL)
    warning (_("Unable to restore previously selected frame."));
  else
    select_frame (frame);
}

/* Flag to control debugging.  */

unsigned int frame_debug;
static void
show_frame_debug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Frame debugging is %s.\n"), value);
}

/* Implementation of "show backtrace past-main".  */

static void
show_backtrace_past_main (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Whether backtraces should "
		      "continue past \"main\" is %s.\n"),
		    value);
}

/* Implementation of "show backtrace past-entry".  */

static void
show_backtrace_past_entry (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Whether backtraces should continue past the "
			    "entry point of a program is %s.\n"),
		    value);
}

/* Implementation of "show backtrace limit".  */

static void
show_backtrace_limit (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("An upper bound on the number "
		      "of backtrace levels is %s.\n"),
		    value);
}


static void
fprint_field (struct ui_file *file, const char *name, int p, CORE_ADDR addr)
{
  if (p)
    fprintf_unfiltered (file, "%s=%s", name, hex_string (addr));
  else
    fprintf_unfiltered (file, "!%s", name);
}

void
fprint_frame_id (struct ui_file *file, struct frame_id id)
{
  fprintf_unfiltered (file, "{");

  if (id.stack_status == FID_STACK_INVALID)
    fprintf_unfiltered (file, "!stack");
  else if (id.stack_status == FID_STACK_UNAVAILABLE)
    fprintf_unfiltered (file, "stack=<unavailable>");
  else if (id.stack_status == FID_STACK_SENTINEL)
    fprintf_unfiltered (file, "stack=<sentinel>");
  else if (id.stack_status == FID_STACK_OUTER)
    fprintf_unfiltered (file, "stack=<outer>");
  else
    fprintf_unfiltered (file, "stack=%s", hex_string (id.stack_addr));

  fprintf_unfiltered (file, ",");

  fprint_field (file, "code", id.code_addr_p, id.code_addr);
  fprintf_unfiltered (file, ",");

  fprint_field (file, "special", id.special_addr_p, id.special_addr);

  if (id.artificial_depth)
    fprintf_unfiltered (file, ",artificial=%d", id.artificial_depth);

  fprintf_unfiltered (file, "}");
}

static void
fprint_frame_type (struct ui_file *file, enum frame_type type)
{
  switch (type)
    {
    case NORMAL_FRAME:
      fprintf_unfiltered (file, "NORMAL_FRAME");
      return;
    case DUMMY_FRAME:
      fprintf_unfiltered (file, "DUMMY_FRAME");
      return;
    case INLINE_FRAME:
      fprintf_unfiltered (file, "INLINE_FRAME");
      return;
    case TAILCALL_FRAME:
      fprintf_unfiltered (file, "TAILCALL_FRAME");
      return;
    case SIGTRAMP_FRAME:
      fprintf_unfiltered (file, "SIGTRAMP_FRAME");
      return;
    case ARCH_FRAME:
      fprintf_unfiltered (file, "ARCH_FRAME");
      return;
    case SENTINEL_FRAME:
      fprintf_unfiltered (file, "SENTINEL_FRAME");
      return;
    default:
      fprintf_unfiltered (file, "<unknown type>");
      return;
    };
}

static void
fprint_frame (struct ui_file *file, struct frame_info *fi)
{
  if (fi == NULL)
    {
      fprintf_unfiltered (file, "<NULL frame>");
      return;
    }

  fprintf_unfiltered (file, "{");
  fprintf_unfiltered (file, "level=%d", fi->level);
  fprintf_unfiltered (file, ",");

  fprintf_unfiltered (file, "type=");
  if (fi->unwind != NULL)
    fprint_frame_type (file, fi->unwind->type);
  else
    fprintf_unfiltered (file, "<unknown>");
  fprintf_unfiltered (file, ",");

  fprintf_unfiltered (file, "unwind=");
  if (fi->unwind != NULL)
    gdb_print_host_address (fi->unwind, file);
  else
    fprintf_unfiltered (file, "<unknown>");
  fprintf_unfiltered (file, ",");

  fprintf_unfiltered (file, "pc=");
  if (fi->next == NULL || fi->next->prev_pc.status == CC_UNKNOWN)
    fprintf_unfiltered (file, "<unknown>");
  else if (fi->next->prev_pc.status == CC_VALUE)
    {
      fprintf_unfiltered (file, "%s", hex_string (fi->next->prev_pc.value));
      if (fi->next->prev_pc.masked)
	fprintf_unfiltered (file, "[PAC]");
    }
  else if (fi->next->prev_pc.status == CC_NOT_SAVED)
    val_print_not_saved (file);
  else if (fi->next->prev_pc.status == CC_UNAVAILABLE)
    val_print_unavailable (file);
  fprintf_unfiltered (file, ",");

  fprintf_unfiltered (file, "id=");
  if (fi->this_id.p == frame_id_status::NOT_COMPUTED)
    fprintf_unfiltered (file, "<not computed>");
  else if (fi->this_id.p == frame_id_status::COMPUTING)
    fprintf_unfiltered (file, "<computing>");
  else
    fprint_frame_id (file, fi->this_id.value);
  fprintf_unfiltered (file, ",");

  fprintf_unfiltered (file, "func=");
  if (fi->next != NULL && fi->next->prev_func.status == CC_VALUE)
    fprintf_unfiltered (file, "%s", hex_string (fi->next->prev_func.addr));
  else
    fprintf_unfiltered (file, "<unknown>");
  fprintf_unfiltered (file, "}");
}

/* Given FRAME, return the enclosing frame as found in real frames read-in from
   inferior memory.  Skip any previous frames which were made up by GDB.
   Return FRAME if FRAME is a non-artificial frame.
   Return NULL if FRAME is the start of an artificial-only chain.  */

static struct frame_info *
skip_artificial_frames (struct frame_info *frame)
{
  /* Note we use get_prev_frame_always, and not get_prev_frame.  The
     latter will truncate the frame chain, leading to this function
     unintentionally returning a null_frame_id (e.g., when the user
     sets a backtrace limit).

     Note that for record targets we may get a frame chain that consists
     of artificial frames only.  */
  while (get_frame_type (frame) == INLINE_FRAME
	 || get_frame_type (frame) == TAILCALL_FRAME)
    {
      frame = get_prev_frame_always (frame);
      if (frame == NULL)
	break;
    }

  return frame;
}

struct frame_info *
skip_unwritable_frames (struct frame_info *frame)
{
  while (gdbarch_code_of_frame_writable (get_frame_arch (frame), frame) == 0)
    {
      frame = get_prev_frame (frame);
      if (frame == NULL)
	break;
    }

  return frame;
}

/* See frame.h.  */

struct frame_info *
skip_tailcall_frames (struct frame_info *frame)
{
  while (get_frame_type (frame) == TAILCALL_FRAME)
    {
      /* Note that for record targets we may get a frame chain that consists of
	 tailcall frames only.  */
      frame = get_prev_frame (frame);
      if (frame == NULL)
	break;
    }

  return frame;
}

/* Compute the frame's uniq ID that can be used to, later, re-find the
   frame.  */

static void
compute_frame_id (struct frame_info *fi)
{
  gdb_assert (fi->this_id.p == frame_id_status::NOT_COMPUTED);

  unsigned int entry_generation = get_frame_cache_generation ();

  try
    {
      /* Mark this frame's id as "being computed.  */
      fi->this_id.p = frame_id_status::COMPUTING;

      if (frame_debug)
	fprintf_unfiltered (gdb_stdlog, "{ compute_frame_id (fi=%d) ",
			    fi->level);

      /* Find the unwinder.  */
      if (fi->unwind == NULL)
	frame_unwind_find_by_frame (fi, &fi->prologue_cache);

      /* Find THIS frame's ID.  */
      /* Default to outermost if no ID is found.  */
      fi->this_id.value = outer_frame_id;
      fi->unwind->this_id (fi, &fi->prologue_cache, &fi->this_id.value);
      gdb_assert (frame_id_p (fi->this_id.value));

      /* Mark this frame's id as "computed".  */
      fi->this_id.p = frame_id_status::COMPUTED;

      if (frame_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "-> ");
	  fprint_frame_id (gdb_stdlog, fi->this_id.value);
	  fprintf_unfiltered (gdb_stdlog, " }\n");
	}
    }
  catch (const gdb_exception &ex)
    {
      /* On error, revert the frame id status to not computed.  If the frame
         cache generation changed, the frame object doesn't exist anymore, so
	 don't touch it.  */
      if (get_frame_cache_generation () == entry_generation)
	fi->this_id.p = frame_id_status::NOT_COMPUTED;

      throw;
    }
}

/* Return a frame uniq ID that can be used to, later, re-find the
   frame.  */

struct frame_id
get_frame_id (struct frame_info *fi)
{
  if (fi == NULL)
    return null_frame_id;

  /* It's always invalid to try to get a frame's id while it is being
     computed.  */
  gdb_assert (fi->this_id.p != frame_id_status::COMPUTING);

  if (fi->this_id.p == frame_id_status::NOT_COMPUTED)
    {
      /* If we haven't computed the frame id yet, then it must be that
	 this is the current frame.  Compute it now, and stash the
	 result.  The IDs of other frames are computed as soon as
	 they're created, in order to detect cycles.  See
	 get_prev_frame_if_no_cycle.  */
      gdb_assert (fi->level == 0);

      /* Compute.  */
      compute_frame_id (fi);

      /* Since this is the first frame in the chain, this should
	 always succeed.  */
      bool stashed = frame_stash_add (fi);
      gdb_assert (stashed);
    }

  return fi->this_id.value;
}

struct frame_id
get_stack_frame_id (struct frame_info *next_frame)
{
  return get_frame_id (skip_artificial_frames (next_frame));
}

struct frame_id
frame_unwind_caller_id (struct frame_info *next_frame)
{
  struct frame_info *this_frame;

  /* Use get_prev_frame_always, and not get_prev_frame.  The latter
     will truncate the frame chain, leading to this function
     unintentionally returning a null_frame_id (e.g., when a caller
     requests the frame ID of "main()"s caller.  */

  next_frame = skip_artificial_frames (next_frame);
  if (next_frame == NULL)
    return null_frame_id;

  this_frame = get_prev_frame_always (next_frame);
  if (this_frame)
    return get_frame_id (skip_artificial_frames (this_frame));
  else
    return null_frame_id;
}

const struct frame_id null_frame_id = { 0 }; /* All zeros.  */
const struct frame_id sentinel_frame_id = { 0, 0, 0, FID_STACK_SENTINEL, 0, 1, 0 };
const struct frame_id outer_frame_id = { 0, 0, 0, FID_STACK_OUTER, 0, 1, 0 };

struct frame_id
frame_id_build_special (CORE_ADDR stack_addr, CORE_ADDR code_addr,
                        CORE_ADDR special_addr)
{
  struct frame_id id = null_frame_id;

  id.stack_addr = stack_addr;
  id.stack_status = FID_STACK_VALID;
  id.code_addr = code_addr;
  id.code_addr_p = true;
  id.special_addr = special_addr;
  id.special_addr_p = true;
  return id;
}

/* See frame.h.  */

struct frame_id
frame_id_build_unavailable_stack (CORE_ADDR code_addr)
{
  struct frame_id id = null_frame_id;

  id.stack_status = FID_STACK_UNAVAILABLE;
  id.code_addr = code_addr;
  id.code_addr_p = true;
  return id;
}

/* See frame.h.  */

struct frame_id
frame_id_build_unavailable_stack_special (CORE_ADDR code_addr,
					  CORE_ADDR special_addr)
{
  struct frame_id id = null_frame_id;

  id.stack_status = FID_STACK_UNAVAILABLE;
  id.code_addr = code_addr;
  id.code_addr_p = true;
  id.special_addr = special_addr;
  id.special_addr_p = true;
  return id;
}

struct frame_id
frame_id_build (CORE_ADDR stack_addr, CORE_ADDR code_addr)
{
  struct frame_id id = null_frame_id;

  id.stack_addr = stack_addr;
  id.stack_status = FID_STACK_VALID;
  id.code_addr = code_addr;
  id.code_addr_p = true;
  return id;
}

struct frame_id
frame_id_build_wild (CORE_ADDR stack_addr)
{
  struct frame_id id = null_frame_id;

  id.stack_addr = stack_addr;
  id.stack_status = FID_STACK_VALID;
  return id;
}

bool
frame_id_p (frame_id l)
{
  /* The frame is valid iff it has a valid stack address.  */
  bool p = l.stack_status != FID_STACK_INVALID;

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "{ frame_id_p (l=");
      fprint_frame_id (gdb_stdlog, l);
      fprintf_unfiltered (gdb_stdlog, ") -> %d }\n", p);
    }

  return p;
}

bool
frame_id_artificial_p (frame_id l)
{
  if (!frame_id_p (l))
    return false;

  return l.artificial_depth != 0;
}

bool
frame_id_eq (frame_id l, frame_id r)
{
  bool eq;

  if (l.stack_status == FID_STACK_INVALID
      || r.stack_status == FID_STACK_INVALID)
    /* Like a NaN, if either ID is invalid, the result is false.
       Note that a frame ID is invalid iff it is the null frame ID.  */
    eq = false;
  else if (l.stack_status != r.stack_status || l.stack_addr != r.stack_addr)
    /* If .stack addresses are different, the frames are different.  */
    eq = false;
  else if (l.code_addr_p && r.code_addr_p && l.code_addr != r.code_addr)
    /* An invalid code addr is a wild card.  If .code addresses are
       different, the frames are different.  */
    eq = false;
  else if (l.special_addr_p && r.special_addr_p
	   && l.special_addr != r.special_addr)
    /* An invalid special addr is a wild card (or unused).  Otherwise
       if special addresses are different, the frames are different.  */
    eq = false;
  else if (l.artificial_depth != r.artificial_depth)
    /* If artificial depths are different, the frames must be different.  */
    eq = false;
  else
    /* Frames are equal.  */
    eq = true;

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "{ frame_id_eq (l=");
      fprint_frame_id (gdb_stdlog, l);
      fprintf_unfiltered (gdb_stdlog, ",r=");
      fprint_frame_id (gdb_stdlog, r);
      fprintf_unfiltered (gdb_stdlog, ") -> %d }\n", eq);
    }

  return eq;
}

/* Safety net to check whether frame ID L should be inner to
   frame ID R, according to their stack addresses.

   This method cannot be used to compare arbitrary frames, as the
   ranges of valid stack addresses may be discontiguous (e.g. due
   to sigaltstack).

   However, it can be used as safety net to discover invalid frame
   IDs in certain circumstances.  Assuming that NEXT is the immediate
   inner frame to THIS and that NEXT and THIS are both NORMAL frames:

   * The stack address of NEXT must be inner-than-or-equal to the stack
     address of THIS.

     Therefore, if frame_id_inner (THIS, NEXT) holds, some unwind
     error has occurred.

   * If NEXT and THIS have different stack addresses, no other frame
     in the frame chain may have a stack address in between.

     Therefore, if frame_id_inner (TEST, THIS) holds, but
     frame_id_inner (TEST, NEXT) does not hold, TEST cannot refer
     to a valid frame in the frame chain.

   The sanity checks above cannot be performed when a SIGTRAMP frame
   is involved, because signal handlers might be executed on a different
   stack than the stack used by the routine that caused the signal
   to be raised.  This can happen for instance when a thread exceeds
   its maximum stack size.  In this case, certain compilers implement
   a stack overflow strategy that cause the handler to be run on a
   different stack.  */

static bool
frame_id_inner (struct gdbarch *gdbarch, struct frame_id l, struct frame_id r)
{
  bool inner;

  if (l.stack_status != FID_STACK_VALID || r.stack_status != FID_STACK_VALID)
    /* Like NaN, any operation involving an invalid ID always fails.
       Likewise if either ID has an unavailable stack address.  */
    inner = false;
  else if (l.artificial_depth > r.artificial_depth
	   && l.stack_addr == r.stack_addr
	   && l.code_addr_p == r.code_addr_p
	   && l.special_addr_p == r.special_addr_p
	   && l.special_addr == r.special_addr)
    {
      /* Same function, different inlined functions.  */
      const struct block *lb, *rb;

      gdb_assert (l.code_addr_p && r.code_addr_p);

      lb = block_for_pc (l.code_addr);
      rb = block_for_pc (r.code_addr);

      if (lb == NULL || rb == NULL)
	/* Something's gone wrong.  */
	inner = false;
      else
	/* This will return true if LB and RB are the same block, or
	   if the block with the smaller depth lexically encloses the
	   block with the greater depth.  */
	inner = contained_in (lb, rb);
    }
  else
    /* Only return non-zero when strictly inner than.  Note that, per
       comment in "frame.h", there is some fuzz here.  Frameless
       functions are not strictly inner than (same .stack but
       different .code and/or .special address).  */
    inner = gdbarch_inner_than (gdbarch, l.stack_addr, r.stack_addr);

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "{ frame_id_inner (l=");
      fprint_frame_id (gdb_stdlog, l);
      fprintf_unfiltered (gdb_stdlog, ",r=");
      fprint_frame_id (gdb_stdlog, r);
      fprintf_unfiltered (gdb_stdlog, ") -> %d }\n", inner);
    }

  return inner;
}

struct frame_info *
frame_find_by_id (struct frame_id id)
{
  struct frame_info *frame, *prev_frame;

  /* ZERO denotes the null frame, let the caller decide what to do
     about it.  Should it instead return get_current_frame()?  */
  if (!frame_id_p (id))
    return NULL;

  /* Check for the sentinel frame.  */
  if (frame_id_eq (id, sentinel_frame_id))
    return sentinel_frame;

  /* Try using the frame stash first.  Finding it there removes the need
     to perform the search by looping over all frames, which can be very
     CPU-intensive if the number of frames is very high (the loop is O(n)
     and get_prev_frame performs a series of checks that are relatively
     expensive).  This optimization is particularly useful when this function
     is called from another function (such as value_fetch_lazy, case
     VALUE_LVAL (val) == lval_register) which already loops over all frames,
     making the overall behavior O(n^2).  */
  frame = frame_stash_find (id);
  if (frame)
    return frame;

  for (frame = get_current_frame (); ; frame = prev_frame)
    {
      struct frame_id self = get_frame_id (frame);

      if (frame_id_eq (id, self))
	/* An exact match.  */
	return frame;

      prev_frame = get_prev_frame (frame);
      if (!prev_frame)
	return NULL;

      /* As a safety net to avoid unnecessary backtracing while trying
	 to find an invalid ID, we check for a common situation where
	 we can detect from comparing stack addresses that no other
	 frame in the current frame chain can have this ID.  See the
	 comment at frame_id_inner for details.   */
      if (get_frame_type (frame) == NORMAL_FRAME
	  && !frame_id_inner (get_frame_arch (frame), id, self)
	  && frame_id_inner (get_frame_arch (prev_frame), id,
			     get_frame_id (prev_frame)))
	return NULL;
    }
  return NULL;
}

static CORE_ADDR
frame_unwind_pc (struct frame_info *this_frame)
{
  if (this_frame->prev_pc.status == CC_UNKNOWN)
    {
      struct gdbarch *prev_gdbarch;
      CORE_ADDR pc = 0;
      bool pc_p = false;

      /* The right way.  The `pure' way.  The one true way.  This
	 method depends solely on the register-unwind code to
	 determine the value of registers in THIS frame, and hence
	 the value of this frame's PC (resume address).  A typical
	 implementation is no more than:

	 frame_unwind_register (this_frame, ISA_PC_REGNUM, buf);
	 return extract_unsigned_integer (buf, size of ISA_PC_REGNUM);

	 Note: this method is very heavily dependent on a correct
	 register-unwind implementation, it pays to fix that
	 method first; this method is frame type agnostic, since
	 it only deals with register values, it works with any
	 frame.  This is all in stark contrast to the old
	 FRAME_SAVED_PC which would try to directly handle all the
	 different ways that a PC could be unwound.  */
      prev_gdbarch = frame_unwind_arch (this_frame);

      try
	{
	  pc = gdbarch_unwind_pc (prev_gdbarch, this_frame);
	  pc_p = true;
	}
      catch (const gdb_exception_error &ex)
	{
	  if (ex.error == NOT_AVAILABLE_ERROR)
	    {
	      this_frame->prev_pc.status = CC_UNAVAILABLE;

	      if (frame_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "{ frame_unwind_pc (this_frame=%d)"
				    " -> <unavailable> }\n",
				    this_frame->level);
	    }
	  else if (ex.error == OPTIMIZED_OUT_ERROR)
	    {
	      this_frame->prev_pc.status = CC_NOT_SAVED;

	      if (frame_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "{ frame_unwind_pc (this_frame=%d)"
				    " -> <not saved> }\n",
				    this_frame->level);
	    }
	  else
	    throw;
	}

      if (pc_p)
	{
	  this_frame->prev_pc.value = pc;
	  this_frame->prev_pc.status = CC_VALUE;
	  if (frame_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"{ frame_unwind_pc (this_frame=%d) "
				"-> %s }\n",
				this_frame->level,
				hex_string (this_frame->prev_pc.value));
	}
    }

  if (this_frame->prev_pc.status == CC_VALUE)
    return this_frame->prev_pc.value;
  else if (this_frame->prev_pc.status == CC_UNAVAILABLE)
    throw_error (NOT_AVAILABLE_ERROR, _("PC not available"));
  else if (this_frame->prev_pc.status == CC_NOT_SAVED)
    throw_error (OPTIMIZED_OUT_ERROR, _("PC not saved"));
  else
    internal_error (__FILE__, __LINE__,
		    "unexpected prev_pc status: %d",
		    (int) this_frame->prev_pc.status);
}

CORE_ADDR
frame_unwind_caller_pc (struct frame_info *this_frame)
{
  this_frame = skip_artificial_frames (this_frame);

  /* We must have a non-artificial frame.  The caller is supposed to check
     the result of frame_unwind_caller_id (), which returns NULL_FRAME_ID
     in this case.  */
  gdb_assert (this_frame != NULL);

  return frame_unwind_pc (this_frame);
}

bool
get_frame_func_if_available (frame_info *this_frame, CORE_ADDR *pc)
{
  struct frame_info *next_frame = this_frame->next;

  if (next_frame->prev_func.status == CC_UNKNOWN)
    {
      CORE_ADDR addr_in_block;

      /* Make certain that this, and not the adjacent, function is
         found.  */
      if (!get_frame_address_in_block_if_available (this_frame, &addr_in_block))
	{
	  next_frame->prev_func.status = CC_UNAVAILABLE;
	  if (frame_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"{ get_frame_func (this_frame=%d)"
				" -> unavailable }\n",
				this_frame->level);
	}
      else
	{
	  next_frame->prev_func.status = CC_VALUE;
	  next_frame->prev_func.addr = get_pc_function_start (addr_in_block);
	  if (frame_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"{ get_frame_func (this_frame=%d) -> %s }\n",
				this_frame->level,
				hex_string (next_frame->prev_func.addr));
	}
    }

  if (next_frame->prev_func.status == CC_UNAVAILABLE)
    {
      *pc = -1;
      return false;
    }
  else
    {
      gdb_assert (next_frame->prev_func.status == CC_VALUE);

      *pc = next_frame->prev_func.addr;
      return true;
    }
}

CORE_ADDR
get_frame_func (struct frame_info *this_frame)
{
  CORE_ADDR pc;

  if (!get_frame_func_if_available (this_frame, &pc))
    throw_error (NOT_AVAILABLE_ERROR, _("PC not available"));

  return pc;
}

std::unique_ptr<readonly_detached_regcache>
frame_save_as_regcache (struct frame_info *this_frame)
{
  auto cooked_read = [this_frame] (int regnum, gdb_byte *buf)
    {
      if (!deprecated_frame_register_read (this_frame, regnum, buf))
	return REG_UNAVAILABLE;
      else
	return REG_VALID;
    };

  std::unique_ptr<readonly_detached_regcache> regcache
    (new readonly_detached_regcache (get_frame_arch (this_frame), cooked_read));

  return regcache;
}

void
frame_pop (struct frame_info *this_frame)
{
  struct frame_info *prev_frame;

  if (get_frame_type (this_frame) == DUMMY_FRAME)
    {
      /* Popping a dummy frame involves restoring more than just registers.
	 dummy_frame_pop does all the work.  */
      dummy_frame_pop (get_frame_id (this_frame), inferior_thread ());
      return;
    }

  /* Ensure that we have a frame to pop to.  */
  prev_frame = get_prev_frame_always (this_frame);

  if (!prev_frame)
    error (_("Cannot pop the initial frame."));

  /* Ignore TAILCALL_FRAME type frames, they were executed already before
     entering THISFRAME.  */
  prev_frame = skip_tailcall_frames (prev_frame);

  if (prev_frame == NULL)
    error (_("Cannot find the caller frame."));

  /* Make a copy of all the register values unwound from this frame.
     Save them in a scratch buffer so that there isn't a race between
     trying to extract the old values from the current regcache while
     at the same time writing new values into that same cache.  */
  std::unique_ptr<readonly_detached_regcache> scratch
    = frame_save_as_regcache (prev_frame);

  /* FIXME: cagney/2003-03-16: It should be possible to tell the
     target's register cache that it is about to be hit with a burst
     register transfer and that the sequence of register writes should
     be batched.  The pair target_prepare_to_store() and
     target_store_registers() kind of suggest this functionality.
     Unfortunately, they don't implement it.  Their lack of a formal
     definition can lead to targets writing back bogus values
     (arguably a bug in the target code mind).  */
  /* Now copy those saved registers into the current regcache.  */
  get_current_regcache ()->restore (scratch.get ());

  /* We've made right mess of GDB's local state, just discard
     everything.  */
  reinit_frame_cache ();
}

void
frame_register_unwind (frame_info *next_frame, int regnum,
		       int *optimizedp, int *unavailablep,
		       enum lval_type *lvalp, CORE_ADDR *addrp,
		       int *realnump, gdb_byte *bufferp)
{
  struct value *value;

  /* Require all but BUFFERP to be valid.  A NULL BUFFERP indicates
     that the value proper does not need to be fetched.  */
  gdb_assert (optimizedp != NULL);
  gdb_assert (lvalp != NULL);
  gdb_assert (addrp != NULL);
  gdb_assert (realnump != NULL);
  /* gdb_assert (bufferp != NULL); */

  value = frame_unwind_register_value (next_frame, regnum);

  gdb_assert (value != NULL);

  *optimizedp = value_optimized_out (value);
  *unavailablep = !value_entirely_available (value);
  *lvalp = VALUE_LVAL (value);
  *addrp = value_address (value);
  if (*lvalp == lval_register)
    *realnump = VALUE_REGNUM (value);
  else
    *realnump = -1;

  if (bufferp)
    {
      if (!*optimizedp && !*unavailablep)
	memcpy (bufferp, value_contents_all (value),
		TYPE_LENGTH (value_type (value)));
      else
	memset (bufferp, 0, TYPE_LENGTH (value_type (value)));
    }

  /* Dispose of the new value.  This prevents watchpoints from
     trying to watch the saved frame pointer.  */
  release_value (value);
}

void
frame_register (struct frame_info *frame, int regnum,
		int *optimizedp, int *unavailablep, enum lval_type *lvalp,
		CORE_ADDR *addrp, int *realnump, gdb_byte *bufferp)
{
  /* Require all but BUFFERP to be valid.  A NULL BUFFERP indicates
     that the value proper does not need to be fetched.  */
  gdb_assert (optimizedp != NULL);
  gdb_assert (lvalp != NULL);
  gdb_assert (addrp != NULL);
  gdb_assert (realnump != NULL);
  /* gdb_assert (bufferp != NULL); */

  /* Obtain the register value by unwinding the register from the next
     (more inner frame).  */
  gdb_assert (frame != NULL && frame->next != NULL);
  frame_register_unwind (frame->next, regnum, optimizedp, unavailablep,
			 lvalp, addrp, realnump, bufferp);
}

void
frame_unwind_register (frame_info *next_frame, int regnum, gdb_byte *buf)
{
  int optimized;
  int unavailable;
  CORE_ADDR addr;
  int realnum;
  enum lval_type lval;

  frame_register_unwind (next_frame, regnum, &optimized, &unavailable,
			 &lval, &addr, &realnum, buf);

  if (optimized)
    throw_error (OPTIMIZED_OUT_ERROR,
		 _("Register %d was not saved"), regnum);
  if (unavailable)
    throw_error (NOT_AVAILABLE_ERROR,
		 _("Register %d is not available"), regnum);
}

void
get_frame_register (struct frame_info *frame,
		    int regnum, gdb_byte *buf)
{
  frame_unwind_register (frame->next, regnum, buf);
}

struct value *
frame_unwind_register_value (frame_info *next_frame, int regnum)
{
  struct gdbarch *gdbarch;
  struct value *value;

  gdb_assert (next_frame != NULL);
  gdbarch = frame_unwind_arch (next_frame);

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "{ frame_unwind_register_value "
			  "(frame=%d,regnum=%d(%s),...) ",
			  next_frame->level, regnum,
			  user_reg_map_regnum_to_name (gdbarch, regnum));
    }

  /* Find the unwinder.  */
  if (next_frame->unwind == NULL)
    frame_unwind_find_by_frame (next_frame, &next_frame->prologue_cache);

  /* Ask this frame to unwind its register.  */
  value = next_frame->unwind->prev_register (next_frame,
					     &next_frame->prologue_cache,
					     regnum);

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "->");
      if (value_optimized_out (value))
	{
	  fprintf_unfiltered (gdb_stdlog, " ");
	  val_print_not_saved (gdb_stdlog);
	}
      else
	{
	  if (VALUE_LVAL (value) == lval_register)
	    fprintf_unfiltered (gdb_stdlog, " register=%d",
				VALUE_REGNUM (value));
	  else if (VALUE_LVAL (value) == lval_memory)
	    fprintf_unfiltered (gdb_stdlog, " address=%s",
				paddress (gdbarch,
					  value_address (value)));
	  else
	    fprintf_unfiltered (gdb_stdlog, " computed");

	  if (value_lazy (value))
	    fprintf_unfiltered (gdb_stdlog, " lazy");
	  else
	    {
	      int i;
	      const gdb_byte *buf = value_contents (value);

	      fprintf_unfiltered (gdb_stdlog, " bytes=");
	      fprintf_unfiltered (gdb_stdlog, "[");
	      for (i = 0; i < register_size (gdbarch, regnum); i++)
		fprintf_unfiltered (gdb_stdlog, "%02x", buf[i]);
	      fprintf_unfiltered (gdb_stdlog, "]");
	    }
	}

      fprintf_unfiltered (gdb_stdlog, " }\n");
    }

  return value;
}

struct value *
get_frame_register_value (struct frame_info *frame, int regnum)
{
  return frame_unwind_register_value (frame->next, regnum);
}

LONGEST
frame_unwind_register_signed (frame_info *next_frame, int regnum)
{
  struct gdbarch *gdbarch = frame_unwind_arch (next_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int size = register_size (gdbarch, regnum);
  struct value *value = frame_unwind_register_value (next_frame, regnum);

  gdb_assert (value != NULL);

  if (value_optimized_out (value))
    {
      throw_error (OPTIMIZED_OUT_ERROR,
		   _("Register %d was not saved"), regnum);
    }
  if (!value_entirely_available (value))
    {
      throw_error (NOT_AVAILABLE_ERROR,
		   _("Register %d is not available"), regnum);
    }

  LONGEST r = extract_signed_integer (value_contents_all (value), size,
				      byte_order);

  release_value (value);
  return r;
}

LONGEST
get_frame_register_signed (struct frame_info *frame, int regnum)
{
  return frame_unwind_register_signed (frame->next, regnum);
}

ULONGEST
frame_unwind_register_unsigned (frame_info *next_frame, int regnum)
{
  struct gdbarch *gdbarch = frame_unwind_arch (next_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int size = register_size (gdbarch, regnum);
  struct value *value = frame_unwind_register_value (next_frame, regnum);

  gdb_assert (value != NULL);

  if (value_optimized_out (value))
    {
      throw_error (OPTIMIZED_OUT_ERROR,
		   _("Register %d was not saved"), regnum);
    }
  if (!value_entirely_available (value))
    {
      throw_error (NOT_AVAILABLE_ERROR,
		   _("Register %d is not available"), regnum);
    }

  ULONGEST r = extract_unsigned_integer (value_contents_all (value), size,
					 byte_order);

  release_value (value);
  return r;
}

ULONGEST
get_frame_register_unsigned (struct frame_info *frame, int regnum)
{
  return frame_unwind_register_unsigned (frame->next, regnum);
}

bool
read_frame_register_unsigned (frame_info *frame, int regnum,
			      ULONGEST *val)
{
  struct value *regval = get_frame_register_value (frame, regnum);

  if (!value_optimized_out (regval)
      && value_entirely_available (regval))
    {
      struct gdbarch *gdbarch = get_frame_arch (frame);
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      int size = register_size (gdbarch, VALUE_REGNUM (regval));

      *val = extract_unsigned_integer (value_contents (regval), size, byte_order);
      return true;
    }

  return false;
}

void
put_frame_register (struct frame_info *frame, int regnum,
		    const gdb_byte *buf)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int realnum;
  int optim;
  int unavail;
  enum lval_type lval;
  CORE_ADDR addr;

  frame_register (frame, regnum, &optim, &unavail,
		  &lval, &addr, &realnum, NULL);
  if (optim)
    error (_("Attempt to assign to a register that was not saved."));
  switch (lval)
    {
    case lval_memory:
      {
	write_memory (addr, buf, register_size (gdbarch, regnum));
	break;
      }
    case lval_register:
      get_current_regcache ()->cooked_write (realnum, buf);
      break;
    default:
      error (_("Attempt to assign to an unmodifiable value."));
    }
}

/* This function is deprecated.  Use get_frame_register_value instead,
   which provides more accurate information.

   Find and return the value of REGNUM for the specified stack frame.
   The number of bytes copied is REGISTER_SIZE (REGNUM).

   Returns 0 if the register value could not be found.  */

bool
deprecated_frame_register_read (frame_info *frame, int regnum,
				gdb_byte *myaddr)
{
  int optimized;
  int unavailable;
  enum lval_type lval;
  CORE_ADDR addr;
  int realnum;

  frame_register (frame, regnum, &optimized, &unavailable,
		  &lval, &addr, &realnum, myaddr);

  return !optimized && !unavailable;
}

bool
get_frame_register_bytes (frame_info *frame, int regnum,
			  CORE_ADDR offset, int len, gdb_byte *myaddr,
			  int *optimizedp, int *unavailablep)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int i;
  int maxsize;
  int numregs;

  /* Skip registers wholly inside of OFFSET.  */
  while (offset >= register_size (gdbarch, regnum))
    {
      offset -= register_size (gdbarch, regnum);
      regnum++;
    }

  /* Ensure that we will not read beyond the end of the register file.
     This can only ever happen if the debug information is bad.  */
  maxsize = -offset;
  numregs = gdbarch_num_cooked_regs (gdbarch);
  for (i = regnum; i < numregs; i++)
    {
      int thissize = register_size (gdbarch, i);

      if (thissize == 0)
	break;	/* This register is not available on this architecture.  */
      maxsize += thissize;
    }
  if (len > maxsize)
    error (_("Bad debug information detected: "
	     "Attempt to read %d bytes from registers."), len);

  /* Copy the data.  */
  while (len > 0)
    {
      int curr_len = register_size (gdbarch, regnum) - offset;

      if (curr_len > len)
	curr_len = len;

      if (curr_len == register_size (gdbarch, regnum))
	{
	  enum lval_type lval;
	  CORE_ADDR addr;
	  int realnum;

	  frame_register (frame, regnum, optimizedp, unavailablep,
			  &lval, &addr, &realnum, myaddr);
	  if (*optimizedp || *unavailablep)
	    return false;
	}
      else
	{
	  struct value *value = frame_unwind_register_value (frame->next,
							     regnum);
	  gdb_assert (value != NULL);
	  *optimizedp = value_optimized_out (value);
	  *unavailablep = !value_entirely_available (value);

	  if (*optimizedp || *unavailablep)
	    {
	      release_value (value);
	      return false;
	    }

	  memcpy (myaddr, value_contents_all (value) + offset, curr_len);
	  release_value (value);
	}

      myaddr += curr_len;
      len -= curr_len;
      offset = 0;
      regnum++;
    }

  *optimizedp = 0;
  *unavailablep = 0;

  return true;
}

void
put_frame_register_bytes (struct frame_info *frame, int regnum,
			  CORE_ADDR offset, int len, const gdb_byte *myaddr)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  /* Skip registers wholly inside of OFFSET.  */
  while (offset >= register_size (gdbarch, regnum))
    {
      offset -= register_size (gdbarch, regnum);
      regnum++;
    }

  /* Copy the data.  */
  while (len > 0)
    {
      int curr_len = register_size (gdbarch, regnum) - offset;

      if (curr_len > len)
	curr_len = len;

      if (curr_len == register_size (gdbarch, regnum))
	{
	  put_frame_register (frame, regnum, myaddr);
	}
      else
	{
	  struct value *value = frame_unwind_register_value (frame->next,
							     regnum);
	  gdb_assert (value != NULL);

	  memcpy ((char *) value_contents_writeable (value) + offset, myaddr,
		  curr_len);
	  put_frame_register (frame, regnum, value_contents_raw (value));
	  release_value (value);
	}

      myaddr += curr_len;
      len -= curr_len;
      offset = 0;
      regnum++;
    }
}

/* Create a sentinel frame.  */

static struct frame_info *
create_sentinel_frame (struct program_space *pspace, struct regcache *regcache)
{
  struct frame_info *frame = FRAME_OBSTACK_ZALLOC (struct frame_info);

  frame->level = -1;
  frame->pspace = pspace;
  frame->aspace = regcache->aspace ();
  /* Explicitly initialize the sentinel frame's cache.  Provide it
     with the underlying regcache.  In the future additional
     information, such as the frame's thread will be added.  */
  frame->prologue_cache = sentinel_frame_cache (regcache);
  /* For the moment there is only one sentinel frame implementation.  */
  frame->unwind = &sentinel_frame_unwind;
  /* Link this frame back to itself.  The frame is self referential
     (the unwound PC is the same as the pc), so make it so.  */
  frame->next = frame;
  /* The sentinel frame has a special ID.  */
  frame->this_id.p = frame_id_status::COMPUTED;
  frame->this_id.value = sentinel_frame_id;
  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "{ create_sentinel_frame (...) -> ");
      fprint_frame (gdb_stdlog, frame);
      fprintf_unfiltered (gdb_stdlog, " }\n");
    }
  return frame;
}

/* Cache for frame addresses already read by gdb.  Valid only while
   inferior is stopped.  Control variables for the frame cache should
   be local to this module.  */

static struct obstack frame_cache_obstack;

void *
frame_obstack_zalloc (unsigned long size)
{
  void *data = obstack_alloc (&frame_cache_obstack, size);

  memset (data, 0, size);
  return data;
}

static struct frame_info *get_prev_frame_always_1 (struct frame_info *this_frame);

struct frame_info *
get_current_frame (void)
{
  struct frame_info *current_frame;

  /* First check, and report, the lack of registers.  Having GDB
     report "No stack!" or "No memory" when the target doesn't even
     have registers is very confusing.  Besides, "printcmd.exp"
     explicitly checks that ``print $pc'' with no registers prints "No
     registers".  */
  if (!target_has_registers)
    error (_("No registers."));
  if (!target_has_stack)
    error (_("No stack."));
  if (!target_has_memory)
    error (_("No memory."));
  /* Traceframes are effectively a substitute for the live inferior.  */
  if (get_traceframe_number () < 0)
    validate_registers_access ();

  if (sentinel_frame == NULL)
    sentinel_frame =
      create_sentinel_frame (current_program_space, get_current_regcache ());

  /* Set the current frame before computing the frame id, to avoid
     recursion inside compute_frame_id, in case the frame's
     unwinder decides to do a symbol lookup (which depends on the
     selected frame's block).

     This call must always succeed.  In particular, nothing inside
     get_prev_frame_always_1 should try to unwind from the
     sentinel frame, because that could fail/throw, and we always
     want to leave with the current frame created and linked in --
     we should never end up with the sentinel frame as outermost
     frame.  */
  current_frame = get_prev_frame_always_1 (sentinel_frame);
  gdb_assert (current_frame != NULL);

  return current_frame;
}

/* The "selected" stack frame is used by default for local and arg
   access.  May be zero, for no selected frame.  */

static struct frame_info *selected_frame;

bool
has_stack_frames ()
{
  if (!target_has_registers || !target_has_stack || !target_has_memory)
    return false;

  /* Traceframes are effectively a substitute for the live inferior.  */
  if (get_traceframe_number () < 0)
    {
      /* No current inferior, no frame.  */
      if (inferior_ptid == null_ptid)
	return false;

      thread_info *tp = inferior_thread ();
      /* Don't try to read from a dead thread.  */
      if (tp->state == THREAD_EXITED)
	return false;

      /* ... or from a spinning thread.  */
      if (tp->executing)
	return false;
    }

  return true;
}

/* Return the selected frame.  Always non-NULL (unless there isn't an
   inferior sufficient for creating a frame) in which case an error is
   thrown.  */

struct frame_info *
get_selected_frame (const char *message)
{
  if (selected_frame == NULL)
    {
      if (message != NULL && !has_stack_frames ())
	error (("%s"), message);
      /* Hey!  Don't trust this.  It should really be re-finding the
	 last selected frame of the currently selected thread.  This,
	 though, is better than nothing.  */
      select_frame (get_current_frame ());
    }
  /* There is always a frame.  */
  gdb_assert (selected_frame != NULL);
  return selected_frame;
}

/* If there is a selected frame, return it.  Otherwise, return NULL.  */

struct frame_info *
get_selected_frame_if_set (void)
{
  return selected_frame;
}

/* This is a variant of get_selected_frame() which can be called when
   the inferior does not have a frame; in that case it will return
   NULL instead of calling error().  */

struct frame_info *
deprecated_safe_get_selected_frame (void)
{
  if (!has_stack_frames ())
    return NULL;
  return get_selected_frame (NULL);
}

/* Select frame FI (or NULL - to invalidate the current frame).  */

void
select_frame (struct frame_info *fi)
{
  selected_frame = fi;
  /* NOTE: cagney/2002-05-04: FI can be NULL.  This occurs when the
     frame is being invalidated.  */

  /* FIXME: kseitz/2002-08-28: It would be nice to call
     selected_frame_level_changed_event() right here, but due to limitations
     in the current interfaces, we would end up flooding UIs with events
     because select_frame() is used extensively internally.

     Once we have frame-parameterized frame (and frame-related) commands,
     the event notification can be moved here, since this function will only
     be called when the user's selected frame is being changed.  */

  /* Ensure that symbols for this frame are read in.  Also, determine the
     source language of this frame, and switch to it if desired.  */
  if (fi)
    {
      CORE_ADDR pc;

      /* We retrieve the frame's symtab by using the frame PC.
	 However we cannot use the frame PC as-is, because it usually
	 points to the instruction following the "call", which is
	 sometimes the first instruction of another function.  So we
	 rely on get_frame_address_in_block() which provides us with a
	 PC which is guaranteed to be inside the frame's code
	 block.  */
      if (get_frame_address_in_block_if_available (fi, &pc))
	{
	  struct compunit_symtab *cust = find_pc_compunit_symtab (pc);

	  if (cust != NULL
	      && compunit_language (cust) != current_language->la_language
	      && compunit_language (cust) != language_unknown
	      && language_mode == language_mode_auto)
	    set_language (compunit_language (cust));
	}
    }
}

/* Create an arbitrary (i.e. address specified by user) or innermost frame.
   Always returns a non-NULL value.  */

struct frame_info *
create_new_frame (CORE_ADDR addr, CORE_ADDR pc)
{
  struct frame_info *fi;

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "{ create_new_frame (addr=%s, pc=%s) ",
			  hex_string (addr), hex_string (pc));
    }

  fi = FRAME_OBSTACK_ZALLOC (struct frame_info);

  fi->next = create_sentinel_frame (current_program_space,
				    get_current_regcache ());

  /* Set/update this frame's cached PC value, found in the next frame.
     Do this before looking for this frame's unwinder.  A sniffer is
     very likely to read this, and the corresponding unwinder is
     entitled to rely that the PC doesn't magically change.  */
  fi->next->prev_pc.value = pc;
  fi->next->prev_pc.status = CC_VALUE;

  /* We currently assume that frame chain's can't cross spaces.  */
  fi->pspace = fi->next->pspace;
  fi->aspace = fi->next->aspace;

  /* Select/initialize both the unwind function and the frame's type
     based on the PC.  */
  frame_unwind_find_by_frame (fi, &fi->prologue_cache);

  fi->this_id.p = frame_id_status::COMPUTED;
  fi->this_id.value = frame_id_build (addr, pc);

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "-> ");
      fprint_frame (gdb_stdlog, fi);
      fprintf_unfiltered (gdb_stdlog, " }\n");
    }

  return fi;
}

/* Return the frame that THIS_FRAME calls (NULL if THIS_FRAME is the
   innermost frame).  Be careful to not fall off the bottom of the
   frame chain and onto the sentinel frame.  */

struct frame_info *
get_next_frame (struct frame_info *this_frame)
{
  if (this_frame->level > 0)
    return this_frame->next;
  else
    return NULL;
}

/* Return the frame that THIS_FRAME calls.  If THIS_FRAME is the
   innermost (i.e. current) frame, return the sentinel frame.  Thus,
   unlike get_next_frame(), NULL will never be returned.  */

struct frame_info *
get_next_frame_sentinel_okay (struct frame_info *this_frame)
{
  gdb_assert (this_frame != NULL);

  /* Note that, due to the manner in which the sentinel frame is
     constructed, this_frame->next still works even when this_frame
     is the sentinel frame.  But we disallow it here anyway because
     calling get_next_frame_sentinel_okay() on the sentinel frame
     is likely a coding error.  */
  gdb_assert (this_frame != sentinel_frame);

  return this_frame->next;
}

/* Observer for the target_changed event.  */

static void
frame_observer_target_changed (struct target_ops *target)
{
  reinit_frame_cache ();
}

/* Flush the entire frame cache.  */

void
reinit_frame_cache (void)
{
  struct frame_info *fi;

  ++frame_cache_generation;

  /* Tear down all frame caches.  */
  for (fi = sentinel_frame; fi != NULL; fi = fi->prev)
    {
      if (fi->prologue_cache && fi->unwind->dealloc_cache)
	fi->unwind->dealloc_cache (fi, fi->prologue_cache);
      if (fi->base_cache && fi->base->unwind->dealloc_cache)
	fi->base->unwind->dealloc_cache (fi, fi->base_cache);
    }

  /* Since we can't really be sure what the first object allocated was.  */
  obstack_free (&frame_cache_obstack, 0);
  obstack_init (&frame_cache_obstack);

  if (sentinel_frame != NULL)
    annotate_frames_invalid ();

  sentinel_frame = NULL;		/* Invalidate cache */
  select_frame (NULL);
  frame_stash_invalidate ();
  if (frame_debug)
    fprintf_unfiltered (gdb_stdlog, "{ reinit_frame_cache () }\n");
}

/* Find where a register is saved (in memory or another register).
   The result of frame_register_unwind is just where it is saved
   relative to this particular frame.  */

static void
frame_register_unwind_location (struct frame_info *this_frame, int regnum,
				int *optimizedp, enum lval_type *lvalp,
				CORE_ADDR *addrp, int *realnump)
{
  gdb_assert (this_frame == NULL || this_frame->level >= 0);

  while (this_frame != NULL)
    {
      int unavailable;

      frame_register_unwind (this_frame, regnum, optimizedp, &unavailable,
			     lvalp, addrp, realnump, NULL);

      if (*optimizedp)
	break;

      if (*lvalp != lval_register)
	break;

      regnum = *realnump;
      this_frame = get_next_frame (this_frame);
    }
}

/* Get the previous raw frame, and check that it is not identical to
   same other frame frame already in the chain.  If it is, there is
   most likely a stack cycle, so we discard it, and mark THIS_FRAME as
   outermost, with UNWIND_SAME_ID stop reason.  Unlike the other
   validity tests, that compare THIS_FRAME and the next frame, we do
   this right after creating the previous frame, to avoid ever ending
   up with two frames with the same id in the frame chain.  */

static struct frame_info *
get_prev_frame_if_no_cycle (struct frame_info *this_frame)
{
  struct frame_info *prev_frame;

  prev_frame = get_prev_frame_raw (this_frame);

  /* Don't compute the frame id of the current frame yet.  Unwinding
     the sentinel frame can fail (e.g., if the thread is gone and we
     can't thus read its registers).  If we let the cycle detection
     code below try to compute a frame ID, then an error thrown from
     within the frame ID computation would result in the sentinel
     frame as outermost frame, which is bogus.  Instead, we'll compute
     the current frame's ID lazily in get_frame_id.  Note that there's
     no point in doing cycle detection when there's only one frame, so
     nothing is lost here.  */
  if (prev_frame->level == 0)
    return prev_frame;

  unsigned int entry_generation = get_frame_cache_generation ();

  try
    {
      compute_frame_id (prev_frame);
      if (!frame_stash_add (prev_frame))
	{
	  /* Another frame with the same id was already in the stash.  We just
	     detected a cycle.  */
	  if (frame_debug)
	    {
	      fprintf_unfiltered (gdb_stdlog, "-> ");
	      fprint_frame (gdb_stdlog, NULL);
	      fprintf_unfiltered (gdb_stdlog, " // this frame has same ID }\n");
	    }
	  this_frame->stop_reason = UNWIND_SAME_ID;
	  /* Unlink.  */
	  prev_frame->next = NULL;
	  this_frame->prev = NULL;
	  prev_frame = NULL;
	}
    }
  catch (const gdb_exception &ex)
    {
      if (get_frame_cache_generation () == entry_generation)
	{
	  prev_frame->next = NULL;
	  this_frame->prev = NULL;
	}

      throw;
    }

  return prev_frame;
}

/* Helper function for get_prev_frame_always, this is called inside a
   TRY_CATCH block.  Return the frame that called THIS_FRAME or NULL if
   there is no such frame.  This may throw an exception.  */

static struct frame_info *
get_prev_frame_always_1 (struct frame_info *this_frame)
{
  struct gdbarch *gdbarch;

  gdb_assert (this_frame != NULL);
  gdbarch = get_frame_arch (this_frame);

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "{ get_prev_frame_always (this_frame=");
      if (this_frame != NULL)
	fprintf_unfiltered (gdb_stdlog, "%d", this_frame->level);
      else
	fprintf_unfiltered (gdb_stdlog, "<NULL>");
      fprintf_unfiltered (gdb_stdlog, ") ");
    }

  /* Only try to do the unwind once.  */
  if (this_frame->prev_p)
    {
      if (frame_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "-> ");
	  fprint_frame (gdb_stdlog, this_frame->prev);
	  fprintf_unfiltered (gdb_stdlog, " // cached \n");
	}
      return this_frame->prev;
    }

  /* If the frame unwinder hasn't been selected yet, we must do so
     before setting prev_p; otherwise the check for misbehaved
     sniffers will think that this frame's sniffer tried to unwind
     further (see frame_cleanup_after_sniffer).  */
  if (this_frame->unwind == NULL)
    frame_unwind_find_by_frame (this_frame, &this_frame->prologue_cache);

  this_frame->prev_p = true;
  this_frame->stop_reason = UNWIND_NO_REASON;

  /* If we are unwinding from an inline frame, all of the below tests
     were already performed when we unwound from the next non-inline
     frame.  We must skip them, since we can not get THIS_FRAME's ID
     until we have unwound all the way down to the previous non-inline
     frame.  */
  if (get_frame_type (this_frame) == INLINE_FRAME)
    return get_prev_frame_if_no_cycle (this_frame);

  /* Check that this frame is unwindable.  If it isn't, don't try to
     unwind to the prev frame.  */
  this_frame->stop_reason
    = this_frame->unwind->stop_reason (this_frame,
				       &this_frame->prologue_cache);

  if (this_frame->stop_reason != UNWIND_NO_REASON)
    {
      if (frame_debug)
	{
	  enum unwind_stop_reason reason = this_frame->stop_reason;

	  fprintf_unfiltered (gdb_stdlog, "-> ");
	  fprint_frame (gdb_stdlog, NULL);
	  fprintf_unfiltered (gdb_stdlog, " // %s }\n",
			      frame_stop_reason_symbol_string (reason));
	}
      return NULL;
    }

  /* Check that this frame's ID isn't inner to (younger, below, next)
     the next frame.  This happens when a frame unwind goes backwards.
     This check is valid only if this frame and the next frame are NORMAL.
     See the comment at frame_id_inner for details.  */
  if (get_frame_type (this_frame) == NORMAL_FRAME
      && this_frame->next->unwind->type == NORMAL_FRAME
      && frame_id_inner (get_frame_arch (this_frame->next),
			 get_frame_id (this_frame),
			 get_frame_id (this_frame->next)))
    {
      CORE_ADDR this_pc_in_block;
      struct minimal_symbol *morestack_msym;
      const char *morestack_name = NULL;
      
      /* gcc -fsplit-stack __morestack can continue the stack anywhere.  */
      this_pc_in_block = get_frame_address_in_block (this_frame);
      morestack_msym = lookup_minimal_symbol_by_pc (this_pc_in_block).minsym;
      if (morestack_msym)
	morestack_name = morestack_msym->linkage_name ();
      if (!morestack_name || strcmp (morestack_name, "__morestack") != 0)
	{
	  if (frame_debug)
	    {
	      fprintf_unfiltered (gdb_stdlog, "-> ");
	      fprint_frame (gdb_stdlog, NULL);
	      fprintf_unfiltered (gdb_stdlog,
				  " // this frame ID is inner }\n");
	    }
	  this_frame->stop_reason = UNWIND_INNER_ID;
	  return NULL;
	}
    }

  /* Check that this and the next frame do not unwind the PC register
     to the same memory location.  If they do, then even though they
     have different frame IDs, the new frame will be bogus; two
     functions can't share a register save slot for the PC.  This can
     happen when the prologue analyzer finds a stack adjustment, but
     no PC save.

     This check does assume that the "PC register" is roughly a
     traditional PC, even if the gdbarch_unwind_pc method adjusts
     it (we do not rely on the value, only on the unwound PC being
     dependent on this value).  A potential improvement would be
     to have the frame prev_pc method and the gdbarch unwind_pc
     method set the same lval and location information as
     frame_register_unwind.  */
  if (this_frame->level > 0
      && gdbarch_pc_regnum (gdbarch) >= 0
      && get_frame_type (this_frame) == NORMAL_FRAME
      && (get_frame_type (this_frame->next) == NORMAL_FRAME
	  || get_frame_type (this_frame->next) == INLINE_FRAME))
    {
      int optimized, realnum, nrealnum;
      enum lval_type lval, nlval;
      CORE_ADDR addr, naddr;

      frame_register_unwind_location (this_frame,
				      gdbarch_pc_regnum (gdbarch),
				      &optimized, &lval, &addr, &realnum);
      frame_register_unwind_location (get_next_frame (this_frame),
				      gdbarch_pc_regnum (gdbarch),
				      &optimized, &nlval, &naddr, &nrealnum);

      if ((lval == lval_memory && lval == nlval && addr == naddr)
	  || (lval == lval_register && lval == nlval && realnum == nrealnum))
	{
	  if (frame_debug)
	    {
	      fprintf_unfiltered (gdb_stdlog, "-> ");
	      fprint_frame (gdb_stdlog, NULL);
	      fprintf_unfiltered (gdb_stdlog, " // no saved PC }\n");
	    }

	  this_frame->stop_reason = UNWIND_NO_SAVED_PC;
	  this_frame->prev = NULL;
	  return NULL;
	}
    }

  return get_prev_frame_if_no_cycle (this_frame);
}

/* Return a "struct frame_info" corresponding to the frame that called
   THIS_FRAME.  Returns NULL if there is no such frame.

   Unlike get_prev_frame, this function always tries to unwind the
   frame.  */

struct frame_info *
get_prev_frame_always (struct frame_info *this_frame)
{
  struct frame_info *prev_frame = NULL;

  try
    {
      prev_frame = get_prev_frame_always_1 (this_frame);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == MEMORY_ERROR)
	{
	  this_frame->stop_reason = UNWIND_MEMORY_ERROR;
	  if (ex.message != NULL)
	    {
	      char *stop_string;
	      size_t size;

	      /* The error needs to live as long as the frame does.
	         Allocate using stack local STOP_STRING then assign the
	         pointer to the frame, this allows the STOP_STRING on the
	         frame to be of type 'const char *'.  */
	      size = ex.message->size () + 1;
	      stop_string = (char *) frame_obstack_zalloc (size);
	      memcpy (stop_string, ex.what (), size);
	      this_frame->stop_string = stop_string;
	    }
	  prev_frame = NULL;
	}
      else
	throw;
    }

  return prev_frame;
}

/* Construct a new "struct frame_info" and link it previous to
   this_frame.  */

static struct frame_info *
get_prev_frame_raw (struct frame_info *this_frame)
{
  struct frame_info *prev_frame;

  /* Allocate the new frame but do not wire it in to the frame chain.
     Some (bad) code in INIT_FRAME_EXTRA_INFO tries to look along
     frame->next to pull some fancy tricks (of course such code is, by
     definition, recursive).  Try to prevent it.

     There is no reason to worry about memory leaks, should the
     remainder of the function fail.  The allocated memory will be
     quickly reclaimed when the frame cache is flushed, and the `we've
     been here before' check above will stop repeated memory
     allocation calls.  */
  prev_frame = FRAME_OBSTACK_ZALLOC (struct frame_info);
  prev_frame->level = this_frame->level + 1;

  /* For now, assume we don't have frame chains crossing address
     spaces.  */
  prev_frame->pspace = this_frame->pspace;
  prev_frame->aspace = this_frame->aspace;

  /* Don't yet compute ->unwind (and hence ->type).  It is computed
     on-demand in get_frame_type, frame_register_unwind, and
     get_frame_id.  */

  /* Don't yet compute the frame's ID.  It is computed on-demand by
     get_frame_id().  */

  /* The unwound frame ID is validate at the start of this function,
     as part of the logic to decide if that frame should be further
     unwound, and not here while the prev frame is being created.
     Doing this makes it possible for the user to examine a frame that
     has an invalid frame ID.

     Some very old VAX code noted: [...]  For the sake of argument,
     suppose that the stack is somewhat trashed (which is one reason
     that "info frame" exists).  So, return 0 (indicating we don't
     know the address of the arglist) if we don't know what frame this
     frame calls.  */

  /* Link it in.  */
  this_frame->prev = prev_frame;
  prev_frame->next = this_frame;

  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "-> ");
      fprint_frame (gdb_stdlog, prev_frame);
      fprintf_unfiltered (gdb_stdlog, " }\n");
    }

  return prev_frame;
}

/* Debug routine to print a NULL frame being returned.  */

static void
frame_debug_got_null_frame (struct frame_info *this_frame,
			    const char *reason)
{
  if (frame_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "{ get_prev_frame (this_frame=");
      if (this_frame != NULL)
	fprintf_unfiltered (gdb_stdlog, "%d", this_frame->level);
      else
	fprintf_unfiltered (gdb_stdlog, "<NULL>");
      fprintf_unfiltered (gdb_stdlog, ") -> // %s}\n", reason);
    }
}

/* Is this (non-sentinel) frame in the "main"() function?  */

static bool
inside_main_func (frame_info *this_frame)
{
  if (symfile_objfile == nullptr)
    return false;

  bound_minimal_symbol msymbol
    = lookup_minimal_symbol (main_name (), NULL, symfile_objfile);
  if (msymbol.minsym == nullptr)
    return false;

  /* Make certain that the code, and not descriptor, address is
     returned.  */
  CORE_ADDR maddr
    = gdbarch_convert_from_func_ptr_addr (get_frame_arch (this_frame),
					  BMSYMBOL_VALUE_ADDRESS (msymbol),
					  current_top_target ());

  return maddr == get_frame_func (this_frame);
}

/* Test whether THIS_FRAME is inside the process entry point function.  */

static bool
inside_entry_func (frame_info *this_frame)
{
  CORE_ADDR entry_point;

  if (!entry_point_address_query (&entry_point))
    return false;

  return get_frame_func (this_frame) == entry_point;
}

/* Return a structure containing various interesting information about
   the frame that called THIS_FRAME.  Returns NULL if there is entier
   no such frame or the frame fails any of a set of target-independent
   condition that should terminate the frame chain (e.g., as unwinding
   past main()).

   This function should not contain target-dependent tests, such as
   checking whether the program-counter is zero.  */

struct frame_info *
get_prev_frame (struct frame_info *this_frame)
{
  CORE_ADDR frame_pc;
  int frame_pc_p;

  /* There is always a frame.  If this assertion fails, suspect that
     something should be calling get_selected_frame() or
     get_current_frame().  */
  gdb_assert (this_frame != NULL);
  
  /* If this_frame is the current frame, then compute and stash
     its frame id prior to fetching and computing the frame id of the
     previous frame.  Otherwise, the cycle detection code in
     get_prev_frame_if_no_cycle() will not work correctly.  When
     get_frame_id() is called later on, an assertion error will
     be triggered in the event of a cycle between the current
     frame and its previous frame.  */
  if (this_frame->level == 0)
    get_frame_id (this_frame);

  frame_pc_p = get_frame_pc_if_available (this_frame, &frame_pc);

  /* tausq/2004-12-07: Dummy frames are skipped because it doesn't make much
     sense to stop unwinding at a dummy frame.  One place where a dummy
     frame may have an address "inside_main_func" is on HPUX.  On HPUX, the
     pcsqh register (space register for the instruction at the head of the
     instruction queue) cannot be written directly; the only way to set it
     is to branch to code that is in the target space.  In order to implement
     frame dummies on HPUX, the called function is made to jump back to where 
     the inferior was when the user function was called.  If gdb was inside 
     the main function when we created the dummy frame, the dummy frame will 
     point inside the main function.  */
  if (this_frame->level >= 0
      && get_frame_type (this_frame) == NORMAL_FRAME
      && !user_set_backtrace_options.backtrace_past_main
      && frame_pc_p
      && inside_main_func (this_frame))
    /* Don't unwind past main().  Note, this is done _before_ the
       frame has been marked as previously unwound.  That way if the
       user later decides to enable unwinds past main(), that will
       automatically happen.  */
    {
      frame_debug_got_null_frame (this_frame, "inside main func");
      return NULL;
    }

  /* If the user's backtrace limit has been exceeded, stop.  We must
     add two to the current level; one of those accounts for backtrace_limit
     being 1-based and the level being 0-based, and the other accounts for
     the level of the new frame instead of the level of the current
     frame.  */
  if (this_frame->level + 2 > user_set_backtrace_options.backtrace_limit)
    {
      frame_debug_got_null_frame (this_frame, "backtrace limit exceeded");
      return NULL;
    }

  /* If we're already inside the entry function for the main objfile,
     then it isn't valid.  Don't apply this test to a dummy frame -
     dummy frame PCs typically land in the entry func.  Don't apply
     this test to the sentinel frame.  Sentinel frames should always
     be allowed to unwind.  */
  /* NOTE: cagney/2003-07-07: Fixed a bug in inside_main_func() -
     wasn't checking for "main" in the minimal symbols.  With that
     fixed asm-source tests now stop in "main" instead of halting the
     backtrace in weird and wonderful ways somewhere inside the entry
     file.  Suspect that tests for inside the entry file/func were
     added to work around that (now fixed) case.  */
  /* NOTE: cagney/2003-07-15: danielj (if I'm reading it right)
     suggested having the inside_entry_func test use the
     inside_main_func() msymbol trick (along with entry_point_address()
     I guess) to determine the address range of the start function.
     That should provide a far better stopper than the current
     heuristics.  */
  /* NOTE: tausq/2004-10-09: this is needed if, for example, the compiler
     applied tail-call optimizations to main so that a function called 
     from main returns directly to the caller of main.  Since we don't
     stop at main, we should at least stop at the entry point of the
     application.  */
  if (this_frame->level >= 0
      && get_frame_type (this_frame) == NORMAL_FRAME
      && !user_set_backtrace_options.backtrace_past_entry
      && frame_pc_p
      && inside_entry_func (this_frame))
    {
      frame_debug_got_null_frame (this_frame, "inside entry func");
      return NULL;
    }

  /* Assume that the only way to get a zero PC is through something
     like a SIGSEGV or a dummy frame, and hence that NORMAL frames
     will never unwind a zero PC.  */
  if (this_frame->level > 0
      && (get_frame_type (this_frame) == NORMAL_FRAME
	  || get_frame_type (this_frame) == INLINE_FRAME)
      && get_frame_type (get_next_frame (this_frame)) == NORMAL_FRAME
      && frame_pc_p && frame_pc == 0)
    {
      frame_debug_got_null_frame (this_frame, "zero PC");
      return NULL;
    }

  return get_prev_frame_always (this_frame);
}

struct frame_id
get_prev_frame_id_by_id (struct frame_id id)
{
  struct frame_id prev_id;
  struct frame_info *frame;
  
  frame = frame_find_by_id (id);

  if (frame != NULL)
    prev_id = get_frame_id (get_prev_frame (frame));
  else
    prev_id = null_frame_id;

  return prev_id;
}

CORE_ADDR
get_frame_pc (struct frame_info *frame)
{
  gdb_assert (frame->next != NULL);
  return frame_unwind_pc (frame->next);
}

bool
get_frame_pc_if_available (frame_info *frame, CORE_ADDR *pc)
{

  gdb_assert (frame->next != NULL);

  try
    {
      *pc = frame_unwind_pc (frame->next);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	return false;
      else
	throw;
    }

  return true;
}

/* Return an address that falls within THIS_FRAME's code block.  */

CORE_ADDR
get_frame_address_in_block (struct frame_info *this_frame)
{
  /* A draft address.  */
  CORE_ADDR pc = get_frame_pc (this_frame);

  struct frame_info *next_frame = this_frame->next;

  /* Calling get_frame_pc returns the resume address for THIS_FRAME.
     Normally the resume address is inside the body of the function
     associated with THIS_FRAME, but there is a special case: when
     calling a function which the compiler knows will never return
     (for instance abort), the call may be the very last instruction
     in the calling function.  The resume address will point after the
     call and may be at the beginning of a different function
     entirely.

     If THIS_FRAME is a signal frame or dummy frame, then we should
     not adjust the unwound PC.  For a dummy frame, GDB pushed the
     resume address manually onto the stack.  For a signal frame, the
     OS may have pushed the resume address manually and invoked the
     handler (e.g. GNU/Linux), or invoked the trampoline which called
     the signal handler - but in either case the signal handler is
     expected to return to the trampoline.  So in both of these
     cases we know that the resume address is executable and
     related.  So we only need to adjust the PC if THIS_FRAME
     is a normal function.

     If the program has been interrupted while THIS_FRAME is current,
     then clearly the resume address is inside the associated
     function.  There are three kinds of interruption: debugger stop
     (next frame will be SENTINEL_FRAME), operating system
     signal or exception (next frame will be SIGTRAMP_FRAME),
     or debugger-induced function call (next frame will be
     DUMMY_FRAME).  So we only need to adjust the PC if
     NEXT_FRAME is a normal function.

     We check the type of NEXT_FRAME first, since it is already
     known; frame type is determined by the unwinder, and since
     we have THIS_FRAME we've already selected an unwinder for
     NEXT_FRAME.

     If the next frame is inlined, we need to keep going until we find
     the real function - for instance, if a signal handler is invoked
     while in an inlined function, then the code address of the
     "calling" normal function should not be adjusted either.  */

  while (get_frame_type (next_frame) == INLINE_FRAME)
    next_frame = next_frame->next;

  if ((get_frame_type (next_frame) == NORMAL_FRAME
       || get_frame_type (next_frame) == TAILCALL_FRAME)
      && (get_frame_type (this_frame) == NORMAL_FRAME
	  || get_frame_type (this_frame) == TAILCALL_FRAME
	  || get_frame_type (this_frame) == INLINE_FRAME))
    return pc - 1;

  return pc;
}

bool
get_frame_address_in_block_if_available (frame_info *this_frame,
					 CORE_ADDR *pc)
{

  try
    {
      *pc = get_frame_address_in_block (this_frame);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	return false;
      throw;
    }

  return true;
}

symtab_and_line
find_frame_sal (frame_info *frame)
{
  struct frame_info *next_frame;
  int notcurrent;
  CORE_ADDR pc;

  if (frame_inlined_callees (frame) > 0)
    {
      struct symbol *sym;

      /* If the current frame has some inlined callees, and we have a next
	 frame, then that frame must be an inlined frame.  In this case
	 this frame's sal is the "call site" of the next frame's inlined
	 function, which can not be inferred from get_frame_pc.  */
      next_frame = get_next_frame (frame);
      if (next_frame)
	sym = get_frame_function (next_frame);
      else
	sym = inline_skipped_symbol (inferior_thread ());

      /* If frame is inline, it certainly has symbols.  */
      gdb_assert (sym);

      symtab_and_line sal;
      if (SYMBOL_LINE (sym) != 0)
	{
	  sal.symtab = symbol_symtab (sym);
	  sal.line = SYMBOL_LINE (sym);
	}
      else
	/* If the symbol does not have a location, we don't know where
	   the call site is.  Do not pretend to.  This is jarring, but
	   we can't do much better.  */
	sal.pc = get_frame_pc (frame);

      sal.pspace = get_frame_program_space (frame);
      return sal;
    }

  /* If FRAME is not the innermost frame, that normally means that
     FRAME->pc points at the return instruction (which is *after* the
     call instruction), and we want to get the line containing the
     call (because the call is where the user thinks the program is).
     However, if the next frame is either a SIGTRAMP_FRAME or a
     DUMMY_FRAME, then the next frame will contain a saved interrupt
     PC and such a PC indicates the current (rather than next)
     instruction/line, consequently, for such cases, want to get the
     line containing fi->pc.  */
  if (!get_frame_pc_if_available (frame, &pc))
    return {};

  notcurrent = (pc != get_frame_address_in_block (frame));
  return find_pc_line (pc, notcurrent);
}

/* Per "frame.h", return the ``address'' of the frame.  Code should
   really be using get_frame_id().  */
CORE_ADDR
get_frame_base (struct frame_info *fi)
{
  return get_frame_id (fi).stack_addr;
}

/* High-level offsets into the frame.  Used by the debug info.  */

CORE_ADDR
get_frame_base_address (struct frame_info *fi)
{
  if (get_frame_type (fi) != NORMAL_FRAME)
    return 0;
  if (fi->base == NULL)
    fi->base = frame_base_find_by_frame (fi);
  /* Sneaky: If the low-level unwind and high-level base code share a
     common unwinder, let them share the prologue cache.  */
  if (fi->base->unwind == fi->unwind)
    return fi->base->this_base (fi, &fi->prologue_cache);
  return fi->base->this_base (fi, &fi->base_cache);
}

CORE_ADDR
get_frame_locals_address (struct frame_info *fi)
{
  if (get_frame_type (fi) != NORMAL_FRAME)
    return 0;
  /* If there isn't a frame address method, find it.  */
  if (fi->base == NULL)
    fi->base = frame_base_find_by_frame (fi);
  /* Sneaky: If the low-level unwind and high-level base code share a
     common unwinder, let them share the prologue cache.  */
  if (fi->base->unwind == fi->unwind)
    return fi->base->this_locals (fi, &fi->prologue_cache);
  return fi->base->this_locals (fi, &fi->base_cache);
}

CORE_ADDR
get_frame_args_address (struct frame_info *fi)
{
  if (get_frame_type (fi) != NORMAL_FRAME)
    return 0;
  /* If there isn't a frame address method, find it.  */
  if (fi->base == NULL)
    fi->base = frame_base_find_by_frame (fi);
  /* Sneaky: If the low-level unwind and high-level base code share a
     common unwinder, let them share the prologue cache.  */
  if (fi->base->unwind == fi->unwind)
    return fi->base->this_args (fi, &fi->prologue_cache);
  return fi->base->this_args (fi, &fi->base_cache);
}

/* Return true if the frame unwinder for frame FI is UNWINDER; false
   otherwise.  */

bool
frame_unwinder_is (frame_info *fi, const frame_unwind *unwinder)
{
  if (fi->unwind == nullptr)
    frame_unwind_find_by_frame (fi, &fi->prologue_cache);

  return fi->unwind == unwinder;
}

/* Level of the selected frame: 0 for innermost, 1 for its caller, ...
   or -1 for a NULL frame.  */

int
frame_relative_level (struct frame_info *fi)
{
  if (fi == NULL)
    return -1;
  else
    return fi->level;
}

enum frame_type
get_frame_type (struct frame_info *frame)
{
  if (frame->unwind == NULL)
    /* Initialize the frame's unwinder because that's what
       provides the frame's type.  */
    frame_unwind_find_by_frame (frame, &frame->prologue_cache);
  return frame->unwind->type;
}

struct program_space *
get_frame_program_space (struct frame_info *frame)
{
  return frame->pspace;
}

struct program_space *
frame_unwind_program_space (struct frame_info *this_frame)
{
  gdb_assert (this_frame);

  /* This is really a placeholder to keep the API consistent --- we
     assume for now that we don't have frame chains crossing
     spaces.  */
  return this_frame->pspace;
}

const address_space *
get_frame_address_space (struct frame_info *frame)
{
  return frame->aspace;
}

/* Memory access methods.  */

void
get_frame_memory (struct frame_info *this_frame, CORE_ADDR addr,
		  gdb_byte *buf, int len)
{
  read_memory (addr, buf, len);
}

LONGEST
get_frame_memory_signed (struct frame_info *this_frame, CORE_ADDR addr,
			 int len)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  return read_memory_integer (addr, len, byte_order);
}

ULONGEST
get_frame_memory_unsigned (struct frame_info *this_frame, CORE_ADDR addr,
			   int len)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  return read_memory_unsigned_integer (addr, len, byte_order);
}

bool
safe_frame_unwind_memory (struct frame_info *this_frame,
			  CORE_ADDR addr, gdb_byte *buf, int len)
{
  /* NOTE: target_read_memory returns zero on success!  */
  return target_read_memory (addr, buf, len) == 0;
}

/* Architecture methods.  */

struct gdbarch *
get_frame_arch (struct frame_info *this_frame)
{
  return frame_unwind_arch (this_frame->next);
}

struct gdbarch *
frame_unwind_arch (struct frame_info *next_frame)
{
  if (!next_frame->prev_arch.p)
    {
      struct gdbarch *arch;

      if (next_frame->unwind == NULL)
	frame_unwind_find_by_frame (next_frame, &next_frame->prologue_cache);

      if (next_frame->unwind->prev_arch != NULL)
	arch = next_frame->unwind->prev_arch (next_frame,
					      &next_frame->prologue_cache);
      else
	arch = get_frame_arch (next_frame);

      next_frame->prev_arch.arch = arch;
      next_frame->prev_arch.p = true;
      if (frame_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "{ frame_unwind_arch (next_frame=%d) -> %s }\n",
			    next_frame->level,
			    gdbarch_bfd_arch_info (arch)->printable_name);
    }

  return next_frame->prev_arch.arch;
}

struct gdbarch *
frame_unwind_caller_arch (struct frame_info *next_frame)
{
  next_frame = skip_artificial_frames (next_frame);

  /* We must have a non-artificial frame.  The caller is supposed to check
     the result of frame_unwind_caller_id (), which returns NULL_FRAME_ID
     in this case.  */
  gdb_assert (next_frame != NULL);

  return frame_unwind_arch (next_frame);
}

/* Gets the language of FRAME.  */

enum language
get_frame_language (struct frame_info *frame)
{
  CORE_ADDR pc = 0;
  bool pc_p = false;

  gdb_assert (frame!= NULL);

    /* We determine the current frame language by looking up its
       associated symtab.  To retrieve this symtab, we use the frame
       PC.  However we cannot use the frame PC as is, because it
       usually points to the instruction following the "call", which
       is sometimes the first instruction of another function.  So
       we rely on get_frame_address_in_block(), it provides us with
       a PC that is guaranteed to be inside the frame's code
       block.  */

  try
    {
      pc = get_frame_address_in_block (frame);
      pc_p = true;
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  if (pc_p)
    {
      struct compunit_symtab *cust = find_pc_compunit_symtab (pc);

      if (cust != NULL)
	return compunit_language (cust);
    }

  return language_unknown;
}

/* Stack pointer methods.  */

CORE_ADDR
get_frame_sp (struct frame_info *this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  /* NOTE drow/2008-06-28: gdbarch_unwind_sp could be converted to
     operate on THIS_FRAME now.  */
  return gdbarch_unwind_sp (gdbarch, this_frame->next);
}

/* Return the reason why we can't unwind past FRAME.  */

enum unwind_stop_reason
get_frame_unwind_stop_reason (struct frame_info *frame)
{
  /* Fill-in STOP_REASON.  */
  get_prev_frame_always (frame);
  gdb_assert (frame->prev_p);

  return frame->stop_reason;
}

/* Return a string explaining REASON.  */

const char *
unwind_stop_reason_to_string (enum unwind_stop_reason reason)
{
  switch (reason)
    {
#define SET(name, description) \
    case name: return _(description);
#include "unwind_stop_reasons.def"
#undef SET

    default:
      internal_error (__FILE__, __LINE__,
		      "Invalid frame stop reason");
    }
}

const char *
frame_stop_reason_string (struct frame_info *fi)
{
  gdb_assert (fi->prev_p);
  gdb_assert (fi->prev == NULL);

  /* Return the specific string if we have one.  */
  if (fi->stop_string != NULL)
    return fi->stop_string;

  /* Return the generic string if we have nothing better.  */
  return unwind_stop_reason_to_string (fi->stop_reason);
}

/* Return the enum symbol name of REASON as a string, to use in debug
   output.  */

static const char *
frame_stop_reason_symbol_string (enum unwind_stop_reason reason)
{
  switch (reason)
    {
#define SET(name, description) \
    case name: return #name;
#include "unwind_stop_reasons.def"
#undef SET

    default:
      internal_error (__FILE__, __LINE__,
		      "Invalid frame stop reason");
    }
}

/* Clean up after a failed (wrong unwinder) attempt to unwind past
   FRAME.  */

void
frame_cleanup_after_sniffer (struct frame_info *frame)
{
  /* The sniffer should not allocate a prologue cache if it did not
     match this frame.  */
  gdb_assert (frame->prologue_cache == NULL);

  /* No sniffer should extend the frame chain; sniff based on what is
     already certain.  */
  gdb_assert (!frame->prev_p);

  /* The sniffer should not check the frame's ID; that's circular.  */
  gdb_assert (frame->this_id.p != frame_id_status::COMPUTED);

  /* Clear cached fields dependent on the unwinder.

     The previous PC is independent of the unwinder, but the previous
     function is not (see get_frame_address_in_block).  */
  frame->prev_func.status = CC_UNKNOWN;
  frame->prev_func.addr = 0;

  /* Discard the unwinder last, so that we can easily find it if an assertion
     in this function triggers.  */
  frame->unwind = NULL;
}

/* Set FRAME's unwinder temporarily, so that we can call a sniffer.
   If sniffing fails, the caller should be sure to call
   frame_cleanup_after_sniffer.  */

void
frame_prepare_for_sniffer (struct frame_info *frame,
			   const struct frame_unwind *unwind)
{
  gdb_assert (frame->unwind == NULL);
  frame->unwind = unwind;
}

static struct cmd_list_element *set_backtrace_cmdlist;
static struct cmd_list_element *show_backtrace_cmdlist;

/* Definition of the "set backtrace" settings that are exposed as
   "backtrace" command options.  */

using boolean_option_def
  = gdb::option::boolean_option_def<set_backtrace_options>;
using uinteger_option_def
  = gdb::option::uinteger_option_def<set_backtrace_options>;

const gdb::option::option_def set_backtrace_option_defs[] = {

  boolean_option_def {
    "past-main",
    [] (set_backtrace_options *opt) { return &opt->backtrace_past_main; },
    show_backtrace_past_main, /* show_cmd_cb */
    N_("Set whether backtraces should continue past \"main\"."),
    N_("Show whether backtraces should continue past \"main\"."),
    N_("Normally the caller of \"main\" is not of interest, so GDB will terminate\n\
the backtrace at \"main\".  Set this if you need to see the rest\n\
of the stack trace."),
  },

  boolean_option_def {
    "past-entry",
    [] (set_backtrace_options *opt) { return &opt->backtrace_past_entry; },
    show_backtrace_past_entry, /* show_cmd_cb */
    N_("Set whether backtraces should continue past the entry point of a program."),
    N_("Show whether backtraces should continue past the entry point of a program."),
    N_("Normally there are no callers beyond the entry point of a program, so GDB\n\
will terminate the backtrace there.  Set this if you need to see\n\
the rest of the stack trace."),
  },
};

void _initialize_frame ();
void
_initialize_frame ()
{
  obstack_init (&frame_cache_obstack);

  frame_stash_create ();

  gdb::observers::target_changed.attach (frame_observer_target_changed);

  add_basic_prefix_cmd ("backtrace", class_maintenance, _("\
Set backtrace specific variables.\n\
Configure backtrace variables such as the backtrace limit"),
			&set_backtrace_cmdlist, "set backtrace ",
			0/*allow-unknown*/, &setlist);
  add_show_prefix_cmd ("backtrace", class_maintenance, _("\
Show backtrace specific variables.\n\
Show backtrace variables such as the backtrace limit."),
		       &show_backtrace_cmdlist, "show backtrace ",
		       0/*allow-unknown*/, &showlist);

  add_setshow_uinteger_cmd ("limit", class_obscure,
			    &user_set_backtrace_options.backtrace_limit, _("\
Set an upper bound on the number of backtrace levels."), _("\
Show the upper bound on the number of backtrace levels."), _("\
No more than the specified number of frames can be displayed or examined.\n\
Literal \"unlimited\" or zero means no limit."),
			    NULL,
			    show_backtrace_limit,
			    &set_backtrace_cmdlist,
			    &show_backtrace_cmdlist);

  gdb::option::add_setshow_cmds_for_options
    (class_stack, &user_set_backtrace_options,
     set_backtrace_option_defs, &set_backtrace_cmdlist, &show_backtrace_cmdlist);

  /* Debug this files internals.  */
  add_setshow_zuinteger_cmd ("frame", class_maintenance, &frame_debug,  _("\
Set frame debugging."), _("\
Show frame debugging."), _("\
When non-zero, frame specific internal debugging is enabled."),
			     NULL,
			     show_frame_debug,
			     &setdebuglist, &showdebuglist);
}
