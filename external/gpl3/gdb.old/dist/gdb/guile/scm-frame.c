/* Scheme interface to stack frames.

   Copyright (C) 2008-2015 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "block.h"
#include "frame.h"
#include "inferior.h"
#include "objfiles.h"
#include "symfile.h"
#include "symtab.h"
#include "stack.h"
#include "value.h"
#include "guile-internal.h"

/* The <gdb:frame> smob.
   The typedef for this struct is in guile-internal.h.  */

struct _frame_smob
{
  /* This always appears first.  */
  eqable_gdb_smob base;

  struct frame_id frame_id;
  struct gdbarch *gdbarch;

  /* Frames are tracked by inferior.
     We need some place to put the eq?-able hash table, and this feels as
     good a place as any.  Frames in one inferior shouldn't be considered
     equal to frames in a different inferior.  The frame becomes invalid if
     this becomes NULL (the inferior has been deleted from gdb).
     It's easier to relax restrictions than impose them after the fact.
     N.B. It is an outstanding question whether a frame survives reruns of
     the inferior.  Intuitively the answer is "No", but currently a frame
     also survives, e.g., multiple invocations of the same function from
     the same point.  Even different threads can have the same frame, e.g.,
     if a thread dies and a new thread gets the same stack.  */
  struct inferior *inferior;

  /* Marks that the FRAME_ID member actually holds the ID of the frame next
     to this, and not this frame's ID itself.  This is a hack to permit Scheme
     frame objects which represent invalid frames (i.e., the last frame_info
     in a corrupt stack).  The problem arises from the fact that this code
     relies on FRAME_ID to uniquely identify a frame, which is not always true
     for the last "frame" in a corrupt stack (it can have a null ID, or the
     same ID as the  previous frame).  Whenever get_prev_frame returns NULL, we
     record the frame_id of the next frame and set FRAME_ID_IS_NEXT to 1.  */
  int frame_id_is_next;
};

static const char frame_smob_name[] = "gdb:frame";

/* The tag Guile knows the frame smob by.  */
static scm_t_bits frame_smob_tag;

/* Keywords used in argument passing.  */
static SCM block_keyword;

static const struct inferior_data *frscm_inferior_data_key;

/* Administrivia for frame smobs.  */

/* Helper function to hash a frame_smob.  */

static hashval_t
frscm_hash_frame_smob (const void *p)
{
  const frame_smob *f_smob = p;
  const struct frame_id *fid = &f_smob->frame_id;
  hashval_t hash = htab_hash_pointer (f_smob->inferior);

  if (fid->stack_status == FID_STACK_VALID)
    hash = iterative_hash (&fid->stack_addr, sizeof (fid->stack_addr), hash);
  if (fid->code_addr_p)
    hash = iterative_hash (&fid->code_addr, sizeof (fid->code_addr), hash);
  if (fid->special_addr_p)
    hash = iterative_hash (&fid->special_addr, sizeof (fid->special_addr),
			   hash);

  return hash;
}

/* Helper function to compute equality of frame_smobs.  */

static int
frscm_eq_frame_smob (const void *ap, const void *bp)
{
  const frame_smob *a = ap;
  const frame_smob *b = bp;

  return (frame_id_eq (a->frame_id, b->frame_id)
	  && a->inferior == b->inferior
	  && a->inferior != NULL);
}

/* Return the frame -> SCM mapping table.
   It is created if necessary.  */

static htab_t
frscm_inferior_frame_map (struct inferior *inferior)
{
  htab_t htab = inferior_data (inferior, frscm_inferior_data_key);

  if (htab == NULL)
    {
      htab = gdbscm_create_eqable_gsmob_ptr_map (frscm_hash_frame_smob,
						 frscm_eq_frame_smob);
      set_inferior_data (inferior, frscm_inferior_data_key, htab);
    }

  return htab;
}

/* The smob "free" function for <gdb:frame>.  */

static size_t
frscm_free_frame_smob (SCM self)
{
  frame_smob *f_smob = (frame_smob *) SCM_SMOB_DATA (self);

  if (f_smob->inferior != NULL)
    {
      htab_t htab = frscm_inferior_frame_map (f_smob->inferior);

      gdbscm_clear_eqable_gsmob_ptr_slot (htab, &f_smob->base);
    }

  /* Not necessary, done to catch bugs.  */
  f_smob->inferior = NULL;

  return 0;
}

/* The smob "print" function for <gdb:frame>.  */

static int
frscm_print_frame_smob (SCM self, SCM port, scm_print_state *pstate)
{
  frame_smob *f_smob = (frame_smob *) SCM_SMOB_DATA (self);
  struct ui_file *strfile;
  char *s;

  gdbscm_printf (port, "#<%s ", frame_smob_name);

  strfile = mem_fileopen ();
  fprint_frame_id (strfile, f_smob->frame_id);
  s = ui_file_xstrdup (strfile, NULL);
  gdbscm_printf (port, "%s", s);
  ui_file_delete (strfile);
  xfree (s);

  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:frame> object.  */

static SCM
frscm_make_frame_smob (void)
{
  frame_smob *f_smob = (frame_smob *)
    scm_gc_malloc (sizeof (frame_smob), frame_smob_name);
  SCM f_scm;

  f_smob->frame_id = null_frame_id;
  f_smob->gdbarch = NULL;
  f_smob->inferior = NULL;
  f_smob->frame_id_is_next = 0;
  f_scm = scm_new_smob (frame_smob_tag, (scm_t_bits) f_smob);
  gdbscm_init_eqable_gsmob (&f_smob->base, f_scm);

  return f_scm;
}

/* Return non-zero if SCM is a <gdb:frame> object.  */

int
frscm_is_frame (SCM scm)
{
  return SCM_SMOB_PREDICATE (frame_smob_tag, scm);
}

/* (frame? object) -> boolean */

static SCM
gdbscm_frame_p (SCM scm)
{
  return scm_from_bool (frscm_is_frame (scm));
}

/* Create a new <gdb:frame> object that encapsulates FRAME.
   Returns a <gdb:exception> object if there is an error.  */

static SCM
frscm_scm_from_frame (struct frame_info *frame, struct inferior *inferior)
{
  frame_smob *f_smob, f_smob_for_lookup;
  SCM f_scm;
  htab_t htab;
  eqable_gdb_smob **slot;
  volatile struct gdb_exception except;
  struct frame_id frame_id = null_frame_id;
  struct gdbarch *gdbarch = NULL;
  int frame_id_is_next = 0;

  /* If we've already created a gsmob for this frame, return it.
     This makes frames eq?-able.  */
  htab = frscm_inferior_frame_map (inferior);
  f_smob_for_lookup.frame_id = get_frame_id (frame);
  f_smob_for_lookup.inferior = inferior;
  slot = gdbscm_find_eqable_gsmob_ptr_slot (htab, &f_smob_for_lookup.base);
  if (*slot != NULL)
    return (*slot)->containing_scm;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      /* Try to get the previous frame, to determine if this is the last frame
	 in a corrupt stack.  If so, we need to store the frame_id of the next
	 frame and not of this one (which is possibly invalid).  */
      if (get_prev_frame (frame) == NULL
	  && get_frame_unwind_stop_reason (frame) != UNWIND_NO_REASON
	  && get_next_frame (frame) != NULL)
	{
	  frame_id = get_frame_id (get_next_frame (frame));
	  frame_id_is_next = 1;
	}
      else
	{
	  frame_id = get_frame_id (frame);
	  frame_id_is_next = 0;
	}
      gdbarch = get_frame_arch (frame);
    }
  if (except.reason < 0)
    return gdbscm_scm_from_gdb_exception (except);

  f_scm = frscm_make_frame_smob ();
  f_smob = (frame_smob *) SCM_SMOB_DATA (f_scm);
  f_smob->frame_id = frame_id;
  f_smob->gdbarch = gdbarch;
  f_smob->inferior = inferior;
  f_smob->frame_id_is_next = frame_id_is_next;

  gdbscm_fill_eqable_gsmob_ptr_slot (slot, &f_smob->base);

  return f_scm;
}

/* Create a new <gdb:frame> object that encapsulates FRAME.
   A Scheme exception is thrown if there is an error.  */

static SCM
frscm_scm_from_frame_unsafe (struct frame_info *frame,
			     struct inferior *inferior)
{
  SCM f_scm = frscm_scm_from_frame (frame, inferior);

  if (gdbscm_is_exception (f_scm))
    gdbscm_throw (f_scm);

  return f_scm;
}

/* Returns the <gdb:frame> object in SELF.
   Throws an exception if SELF is not a <gdb:frame> object.  */

static SCM
frscm_get_frame_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (frscm_is_frame (self), self, arg_pos, func_name,
		   frame_smob_name);

  return self;
}

/* There is no gdbscm_scm_to_frame function because translating
   a frame SCM object to a struct frame_info * can throw a GDB error.
   Thus code working with frames has to handle both Scheme errors (e.g., the
   object is not a frame) and GDB errors (e.g., the frame lookup failed).

   To help keep things clear we split gdbscm_scm_to_frame into two:

   gdbscm_get_frame_smob_arg_unsafe
     - throws a Scheme error if object is not a frame,
       or if the inferior is gone or is no longer current

   gdbscm_frame_smob_to_frame
     - may throw a gdb error if the conversion fails
     - it's not clear when it will and won't throw a GDB error,
       but for robustness' sake we assume that whenever we call out to GDB
       a GDB error may get thrown (and thus the call must be wrapped in a
       TRY_CATCH)  */

/* Returns the frame_smob for the object wrapped by FRAME_SCM.
   A Scheme error is thrown if FRAME_SCM is not a frame.  */

frame_smob *
frscm_get_frame_smob_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM f_scm = frscm_get_frame_arg_unsafe (self, arg_pos, func_name);
  frame_smob *f_smob = (frame_smob *) SCM_SMOB_DATA (f_scm);

  if (f_smob->inferior == NULL)
    {
      gdbscm_invalid_object_error (func_name, arg_pos, self,
				   _("inferior"));
    }
  if (f_smob->inferior != current_inferior ())
    scm_misc_error (func_name, _("inferior has changed"), SCM_EOL);

  return f_smob;
}

/* Returns the frame_info object wrapped by F_SMOB.
   If the frame doesn't exist anymore (the frame id doesn't
   correspond to any frame in the inferior), returns NULL.
   This function calls GDB routines, so don't assume a GDB error will
   not be thrown.  */

struct frame_info *
frscm_frame_smob_to_frame (frame_smob *f_smob)
{
  struct frame_info *frame;

  frame = frame_find_by_id (f_smob->frame_id);
  if (frame == NULL)
    return NULL;

  if (f_smob->frame_id_is_next)
    frame = get_prev_frame (frame);

  return frame;
}

/* Helper function for frscm_del_inferior_frames to mark the frame
   as invalid.  */

static int
frscm_mark_frame_invalid (void **slot, void *info)
{
  frame_smob *f_smob = (frame_smob *) *slot;

  f_smob->inferior = NULL;
  return 1;
}

/* This function is called when an inferior is about to be freed.
   Invalidate the frame as further actions on the frame could result
   in bad data.  All access to the frame should be gated by
   frscm_get_frame_smob_arg_unsafe which will raise an exception on
   invalid frames.  */

static void
frscm_del_inferior_frames (struct inferior *inferior, void *datum)
{
  htab_t htab = datum;

  if (htab != NULL)
    {
      htab_traverse_noresize (htab, frscm_mark_frame_invalid, NULL);
      htab_delete (htab);
    }
}

/* Frame methods.  */

/* (frame-valid? <gdb:frame>) -> bool
   Returns #t if the frame corresponding to the frame_id of this
   object still exists in the inferior.  */

static SCM
gdbscm_frame_valid_p (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return scm_from_bool (frame != NULL);
}

/* (frame-name <gdb:frame>) -> string
   Returns the name of the function corresponding to this frame,
   or #f if there is no function.  */

static SCM
gdbscm_frame_name (SCM self)
{
  frame_smob *f_smob;
  char *name = NULL;
  enum language lang = language_minimal;
  struct frame_info *frame = NULL;
  SCM result;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	find_frame_funname (frame, &name, &lang, NULL);
    }
  if (except.reason < 0)
    xfree (name);
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (name != NULL)
    {
      result = gdbscm_scm_from_c_string (name);
      xfree (name);
    }
  else
    result = SCM_BOOL_F;

  return result;
}

/* (frame-type <gdb:frame>) -> integer
   Returns the frame type, namely one of the gdb:*_FRAME constants.  */

static SCM
gdbscm_frame_type (SCM self)
{
  frame_smob *f_smob;
  enum frame_type type = NORMAL_FRAME;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	type = get_frame_type (frame);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return scm_from_int (type);
}

/* (frame-arch <gdb:frame>) -> <gdb:architecture>
   Returns the frame's architecture as a gdb:architecture object.  */

static SCM
gdbscm_frame_arch (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return arscm_scm_from_arch (f_smob->gdbarch);
}

/* (frame-unwind-stop-reason <gdb:frame>) -> integer
   Returns one of the gdb:FRAME_UNWIND_* constants.  */

static SCM
gdbscm_frame_unwind_stop_reason (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;
  enum unwind_stop_reason stop_reason;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  stop_reason = get_frame_unwind_stop_reason (frame);

  return scm_from_int (stop_reason);
}

/* (frame-pc <gdb:frame>) -> integer
   Returns the frame's resume address.  */

static SCM
gdbscm_frame_pc (SCM self)
{
  frame_smob *f_smob;
  CORE_ADDR pc = 0;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	pc = get_frame_pc (frame);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return gdbscm_scm_from_ulongest (pc);
}

/* (frame-block <gdb:frame>) -> <gdb:block>
   Returns the frame's code block, or #f if one cannot be found.  */

static SCM
gdbscm_frame_block (SCM self)
{
  frame_smob *f_smob;
  const struct block *block = NULL, *fn_block;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	block = get_frame_block (frame, NULL);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  for (fn_block = block;
       fn_block != NULL && BLOCK_FUNCTION (fn_block) == NULL;
       fn_block = BLOCK_SUPERBLOCK (fn_block))
    continue;

  if (block == NULL || fn_block == NULL || BLOCK_FUNCTION (fn_block) == NULL)
    {
      scm_misc_error (FUNC_NAME, _("cannot find block for frame"),
		      scm_list_1 (self));
    }

  if (block != NULL)
    {
      return bkscm_scm_from_block
	(block, symbol_objfile (BLOCK_FUNCTION (fn_block)));
    }

  return SCM_BOOL_F;
}

/* (frame-function <gdb:frame>) -> <gdb:symbol>
   Returns the symbol for the function corresponding to this frame,
   or #f if there isn't one.  */

static SCM
gdbscm_frame_function (SCM self)
{
  frame_smob *f_smob;
  struct symbol *sym = NULL;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	sym = find_pc_function (get_frame_address_in_block (frame));
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (sym != NULL)
    return syscm_scm_from_symbol (sym);

  return SCM_BOOL_F;
}

/* (frame-older <gdb:frame>) -> <gdb:frame>
   Returns the frame immediately older (outer) to this frame,
   or #f if there isn't one.  */

static SCM
gdbscm_frame_older (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *prev = NULL;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	prev = get_prev_frame (frame);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (prev != NULL)
    return frscm_scm_from_frame_unsafe (prev, f_smob->inferior);

  return SCM_BOOL_F;
}

/* (frame-newer <gdb:frame>) -> <gdb:frame>
   Returns the frame immediately newer (inner) to this frame,
   or #f if there isn't one.  */

static SCM
gdbscm_frame_newer (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *next = NULL;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	next = get_next_frame (frame);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  if (next != NULL)
    return frscm_scm_from_frame_unsafe (next, f_smob->inferior);

  return SCM_BOOL_F;
}

/* (frame-sal <gdb:frame>) -> <gdb:sal>
   Returns the frame's symtab and line.  */

static SCM
gdbscm_frame_sal (SCM self)
{
  frame_smob *f_smob;
  struct symtab_and_line sal;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	find_frame_sal (frame, &sal);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return stscm_scm_from_sal (sal);
}

/* (frame-read-var <gdb:frame> <gdb:symbol>) -> <gdb:value>
   (frame-read-var <gdb:frame> string [#:block <gdb:block>]) -> <gdb:value>
   If the optional block argument is provided start the search from that block,
   otherwise search from the frame's current block (determined by examining
   the resume address of the frame).  The variable argument must be a string
   or an instance of a <gdb:symbol>.  The block argument must be an instance of
   <gdb:block>.  */

static SCM
gdbscm_frame_read_var (SCM self, SCM symbol_scm, SCM rest)
{
  SCM keywords[] = { block_keyword, SCM_BOOL_F };
  int rc;
  frame_smob *f_smob;
  int block_arg_pos = -1;
  SCM block_scm = SCM_UNDEFINED;
  struct frame_info *frame = NULL;
  struct symbol *var = NULL;
  struct value *value = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG3, keywords, "#O",
			      rest, &block_arg_pos, &block_scm);

  if (syscm_is_symbol (symbol_scm))
    {
      var = syscm_get_valid_symbol_arg_unsafe (symbol_scm, SCM_ARG2,
					       FUNC_NAME);
      SCM_ASSERT (SCM_UNBNDP (block_scm), block_scm, SCM_ARG3, FUNC_NAME);
    }
  else if (scm_is_string (symbol_scm))
    {
      char *var_name;
      const struct block *block = NULL;
      struct cleanup *cleanup;
      volatile struct gdb_exception except;

      if (! SCM_UNBNDP (block_scm))
	{
	  SCM except_scm;

	  gdb_assert (block_arg_pos > 0);
	  block = bkscm_scm_to_block (block_scm, block_arg_pos, FUNC_NAME,
				      &except_scm);
	  if (block == NULL)
	    gdbscm_throw (except_scm);
	}

      var_name = gdbscm_scm_to_c_string (symbol_scm);
      cleanup = make_cleanup (xfree, var_name);
      /* N.B. Between here and the call to do_cleanups, don't do anything
	 to cause a Scheme exception without performing the cleanup.  */

      TRY_CATCH (except, RETURN_MASK_ALL)
	{
	  if (block == NULL)
	    block = get_frame_block (frame, NULL);
	  var = lookup_symbol (var_name, block, VAR_DOMAIN, NULL);
	}
      if (except.reason < 0)
	do_cleanups (cleanup);
      GDBSCM_HANDLE_GDB_EXCEPTION (except);

      if (var == NULL)
	{
	  do_cleanups (cleanup);
	  gdbscm_out_of_range_error (FUNC_NAME, 0, symbol_scm,
				     _("variable not found"));
	}

      do_cleanups (cleanup);
    }
  else
    {
      /* Use SCM_ASSERT_TYPE for more consistent error messages.  */
      SCM_ASSERT_TYPE (0, symbol_scm, SCM_ARG1, FUNC_NAME,
		       _("gdb:symbol or string"));
    }

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      value = read_var_value (var, frame);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return vlscm_scm_from_value (value);
}

/* (frame-select <gdb:frame>) -> unspecified
   Select this frame.  */

static SCM
gdbscm_frame_select (SCM self)
{
  frame_smob *f_smob;
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  f_smob = frscm_get_frame_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = frscm_frame_smob_to_frame (f_smob);
      if (frame != NULL)
	select_frame (frame);
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  if (frame == NULL)
    {
      gdbscm_invalid_object_error (FUNC_NAME, SCM_ARG1, self,
				   _("<gdb:frame>"));
    }

  return SCM_UNSPECIFIED;
}

/* (newest-frame) -> <gdb:frame>
   Returns the newest frame.  */

static SCM
gdbscm_newest_frame (void)
{
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = get_current_frame ();
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return frscm_scm_from_frame_unsafe (frame, current_inferior ());
}

/* (selected-frame) -> <gdb:frame>
   Returns the selected frame.  */

static SCM
gdbscm_selected_frame (void)
{
  struct frame_info *frame = NULL;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      frame = get_selected_frame (_("No frame is currently selected"));
    }
  GDBSCM_HANDLE_GDB_EXCEPTION (except);

  return frscm_scm_from_frame_unsafe (frame, current_inferior ());
}

/* (unwind-stop-reason-string integer) -> string
   Return a string explaining the unwind stop reason.  */

static SCM
gdbscm_unwind_stop_reason_string (SCM reason_scm)
{
  int reason;
  const char *str;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG1, NULL, "i",
			      reason_scm, &reason);

  if (reason < UNWIND_FIRST || reason > UNWIND_LAST)
    scm_out_of_range (FUNC_NAME, reason_scm);

  str = unwind_stop_reason_to_string (reason);
  return gdbscm_scm_from_c_string (str);
}

/* Initialize the Scheme frame support.  */

static const scheme_integer_constant frame_integer_constants[] =
{
#define ENTRY(X) { #X, X }

  ENTRY (NORMAL_FRAME),
  ENTRY (DUMMY_FRAME),
  ENTRY (INLINE_FRAME),
  ENTRY (TAILCALL_FRAME),
  ENTRY (SIGTRAMP_FRAME),
  ENTRY (ARCH_FRAME),
  ENTRY (SENTINEL_FRAME),

#undef ENTRY

#define SET(name, description) \
  { "FRAME_" #name, name },
#include "unwind_stop_reasons.def"
#undef SET

  END_INTEGER_CONSTANTS
};

static const scheme_function frame_functions[] =
{
  { "frame?", 1, 0, 0, gdbscm_frame_p,
    "\
Return #t if the object is a <gdb:frame> object." },

  { "frame-valid?", 1, 0, 0, gdbscm_frame_valid_p,
    "\
Return #t if the object is a valid <gdb:frame> object.\n\
Frames become invalid when the inferior returns to its caller." },

  { "frame-name", 1, 0, 0, gdbscm_frame_name,
    "\
Return the name of the function corresponding to this frame,\n\
or #f if there is no function." },

  { "frame-arch", 1, 0, 0, gdbscm_frame_arch,
    "\
Return the frame's architecture as a <gdb:arch> object." },

  { "frame-type", 1, 0, 0, gdbscm_frame_type,
    "\
Return the frame type, namely one of the gdb:*_FRAME constants." },

  { "frame-unwind-stop-reason", 1, 0, 0, gdbscm_frame_unwind_stop_reason,
    "\
Return one of the gdb:FRAME_UNWIND_* constants explaining why\n\
it's not possible to find frames older than this." },

  { "frame-pc", 1, 0, 0, gdbscm_frame_pc,
    "\
Return the frame's resume address." },

  { "frame-block", 1, 0, 0, gdbscm_frame_block,
    "\
Return the frame's code block, or #f if one cannot be found." },

  { "frame-function", 1, 0, 0, gdbscm_frame_function,
    "\
Return the <gdb:symbol> for the function corresponding to this frame,\n\
or #f if there isn't one." },

  { "frame-older", 1, 0, 0, gdbscm_frame_older,
    "\
Return the frame immediately older (outer) to this frame,\n\
or #f if there isn't one." },

  { "frame-newer", 1, 0, 0, gdbscm_frame_newer,
    "\
Return the frame immediately newer (inner) to this frame,\n\
or #f if there isn't one." },

  { "frame-sal", 1, 0, 0, gdbscm_frame_sal,
    "\
Return the frame's symtab-and-line <gdb:sal> object." },

  { "frame-read-var", 2, 0, 1, gdbscm_frame_read_var,
    "\
Return the value of the symbol in the frame.\n\
\n\
  Arguments: <gdb:frame> <gdb:symbol>\n\
         Or: <gdb:frame> string [#:block <gdb:block>]" },

  { "frame-select", 1, 0, 0, gdbscm_frame_select,
    "\
Select this frame." },

  { "newest-frame", 0, 0, 0, gdbscm_newest_frame,
    "\
Return the newest frame." },

  { "selected-frame", 0, 0, 0, gdbscm_selected_frame,
    "\
Return the selected frame." },

  { "unwind-stop-reason-string", 1, 0, 0, gdbscm_unwind_stop_reason_string,
    "\
Return a string explaining the unwind stop reason.\n\
\n\
  Arguments: integer (the result of frame-unwind-stop-reason)" },

  END_FUNCTIONS
};

void
gdbscm_initialize_frames (void)
{
  frame_smob_tag
    = gdbscm_make_smob_type (frame_smob_name, sizeof (frame_smob));
  scm_set_smob_free (frame_smob_tag, frscm_free_frame_smob);
  scm_set_smob_print (frame_smob_tag, frscm_print_frame_smob);

  gdbscm_define_integer_constants (frame_integer_constants, 1);
  gdbscm_define_functions (frame_functions, 1);

  block_keyword = scm_from_latin1_keyword ("block");

  /* Register an inferior "free" callback so we can properly
     invalidate frames when an inferior file is about to be deleted.  */
  frscm_inferior_data_key
    = register_inferior_data_with_cleanup (NULL, frscm_del_inferior_frames);
}
