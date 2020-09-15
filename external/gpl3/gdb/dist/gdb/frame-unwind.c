/* Definitions for frame unwinder, for GDB, the GNU debugger.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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
#include "frame-unwind.h"
#include "dummy-frame.h"
#include "inline-frame.h"
#include "value.h"
#include "regcache.h"
#include "gdb_obstack.h"
#include "target.h"
#include "gdbarch.h"
#include "dwarf2/frame-tailcall.h"

static struct gdbarch_data *frame_unwind_data;

struct frame_unwind_table_entry
{
  const struct frame_unwind *unwinder;
  struct frame_unwind_table_entry *next;
};

struct frame_unwind_table
{
  struct frame_unwind_table_entry *list;
  /* The head of the OSABI part of the search list.  */
  struct frame_unwind_table_entry **osabi_head;
};

/* A helper function to add an unwinder to a list.  LINK says where to
   install the new unwinder.  The new link is returned.  */

static struct frame_unwind_table_entry **
add_unwinder (struct obstack *obstack, const struct frame_unwind *unwinder,
	      struct frame_unwind_table_entry **link)
{
  *link = OBSTACK_ZALLOC (obstack, struct frame_unwind_table_entry);
  (*link)->unwinder = unwinder;
  return &(*link)->next;
}

static void *
frame_unwind_init (struct obstack *obstack)
{
  struct frame_unwind_table *table
    = OBSTACK_ZALLOC (obstack, struct frame_unwind_table);

  /* Start the table out with a few default sniffers.  OSABI code
     can't override this.  */
  struct frame_unwind_table_entry **link = &table->list;

  link = add_unwinder (obstack, &dummy_frame_unwind, link);
  /* The DWARF tailcall sniffer must come before the inline sniffer.
     Otherwise, we can end up in a situation where a DWARF frame finds
     tailcall information, but then the inline sniffer claims a frame
     before the tailcall sniffer, resulting in confusion.  This is
     safe to do always because the tailcall sniffer can only ever be
     activated if the newer frame was created using the DWARF
     unwinder, and it also found tailcall information.  */
  link = add_unwinder (obstack, &dwarf2_tailcall_frame_unwind, link);
  link = add_unwinder (obstack, &inline_frame_unwind, link);

  /* The insertion point for OSABI sniffers.  */
  table->osabi_head = link;
  return table;
}

void
frame_unwind_prepend_unwinder (struct gdbarch *gdbarch,
				const struct frame_unwind *unwinder)
{
  struct frame_unwind_table *table
    = (struct frame_unwind_table *) gdbarch_data (gdbarch, frame_unwind_data);
  struct frame_unwind_table_entry *entry;

  /* Insert the new entry at the start of the list.  */
  entry = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct frame_unwind_table_entry);
  entry->unwinder = unwinder;
  entry->next = (*table->osabi_head);
  (*table->osabi_head) = entry;
}

void
frame_unwind_append_unwinder (struct gdbarch *gdbarch,
			      const struct frame_unwind *unwinder)
{
  struct frame_unwind_table *table
    = (struct frame_unwind_table *) gdbarch_data (gdbarch, frame_unwind_data);
  struct frame_unwind_table_entry **ip;

  /* Find the end of the list and insert the new entry there.  */
  for (ip = table->osabi_head; (*ip) != NULL; ip = &(*ip)->next);
  (*ip) = GDBARCH_OBSTACK_ZALLOC (gdbarch, struct frame_unwind_table_entry);
  (*ip)->unwinder = unwinder;
}

/* Call SNIFFER from UNWINDER.  If it succeeded set UNWINDER for
   THIS_FRAME and return 1.  Otherwise the function keeps THIS_FRAME
   unchanged and returns 0.  */

static int
frame_unwind_try_unwinder (struct frame_info *this_frame, void **this_cache,
                          const struct frame_unwind *unwinder)
{
  int res = 0;

  unsigned int entry_generation = get_frame_cache_generation ();

  frame_prepare_for_sniffer (this_frame, unwinder);

  try
    {
      res = unwinder->sniffer (unwinder, this_frame, this_cache);
    }
  catch (const gdb_exception &ex)
    {
      /* Catch all exceptions, caused by either interrupt or error.
	 Reset *THIS_CACHE, unless something reinitialized the frame
	 cache meanwhile, in which case THIS_FRAME/THIS_CACHE are now
	 dangling.  */
      if (get_frame_cache_generation () == entry_generation)
	{
	  *this_cache = NULL;
	  frame_cleanup_after_sniffer (this_frame);
	}

      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  /* This usually means that not even the PC is available,
	     thus most unwinders aren't able to determine if they're
	     the best fit.  Keep trying.  Fallback prologue unwinders
	     should always accept the frame.  */
	  return 0;
	}
      throw;
    }

  if (res)
    return 1;
  else
    {
      /* Don't set *THIS_CACHE to NULL here, because sniffer has to do
	 so.  */
      frame_cleanup_after_sniffer (this_frame);
      return 0;
    }
  gdb_assert_not_reached ("frame_unwind_try_unwinder");
}

/* Iterate through sniffers for THIS_FRAME frame until one returns with an
   unwinder implementation.  THIS_FRAME->UNWIND must be NULL, it will get set
   by this function.  Possibly initialize THIS_CACHE.  */

void
frame_unwind_find_by_frame (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct frame_unwind_table *table
    = (struct frame_unwind_table *) gdbarch_data (gdbarch, frame_unwind_data);
  struct frame_unwind_table_entry *entry;
  const struct frame_unwind *unwinder_from_target;

  unwinder_from_target = target_get_unwinder ();
  if (unwinder_from_target != NULL
      && frame_unwind_try_unwinder (this_frame, this_cache,
                                   unwinder_from_target))
    return;

  unwinder_from_target = target_get_tailcall_unwinder ();
  if (unwinder_from_target != NULL
      && frame_unwind_try_unwinder (this_frame, this_cache,
                                   unwinder_from_target))
    return;

  for (entry = table->list; entry != NULL; entry = entry->next)
    if (frame_unwind_try_unwinder (this_frame, this_cache, entry->unwinder))
      return;

  internal_error (__FILE__, __LINE__, _("frame_unwind_find_by_frame failed"));
}

/* A default frame sniffer which always accepts the frame.  Used by
   fallback prologue unwinders.  */

int
default_frame_sniffer (const struct frame_unwind *self,
		       struct frame_info *this_frame,
		       void **this_prologue_cache)
{
  return 1;
}

/* The default frame unwinder stop_reason callback.  */

enum unwind_stop_reason
default_frame_unwind_stop_reason (struct frame_info *this_frame,
				  void **this_cache)
{
  struct frame_id this_id = get_frame_id (this_frame);

  if (frame_id_eq (this_id, outer_frame_id))
    return UNWIND_OUTERMOST;
  else
    return UNWIND_NO_REASON;
}

/* See frame-unwind.h.  */

CORE_ADDR
default_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  int pc_regnum = gdbarch_pc_regnum (gdbarch);
  CORE_ADDR pc = frame_unwind_register_unsigned (next_frame, pc_regnum);
  pc = gdbarch_addr_bits_remove (gdbarch, pc);
  return pc;
}

/* See frame-unwind.h.  */

CORE_ADDR
default_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  int sp_regnum = gdbarch_sp_regnum (gdbarch);
  return frame_unwind_register_unsigned (next_frame, sp_regnum);
}

/* Helper functions for value-based register unwinding.  These return
   a (possibly lazy) value of the appropriate type.  */

/* Return a value which indicates that FRAME did not save REGNUM.  */

struct value *
frame_unwind_got_optimized (struct frame_info *frame, int regnum)
{
  struct gdbarch *gdbarch = frame_unwind_arch (frame);
  struct type *type = register_type (gdbarch, regnum);

  return allocate_optimized_out_value (type);
}

/* Return a value which indicates that FRAME copied REGNUM into
   register NEW_REGNUM.  */

struct value *
frame_unwind_got_register (struct frame_info *frame,
			   int regnum, int new_regnum)
{
  return value_of_register_lazy (frame, new_regnum);
}

/* Return a value which indicates that FRAME saved REGNUM in memory at
   ADDR.  */

struct value *
frame_unwind_got_memory (struct frame_info *frame, int regnum, CORE_ADDR addr)
{
  struct gdbarch *gdbarch = frame_unwind_arch (frame);
  struct value *v = value_at_lazy (register_type (gdbarch, regnum), addr);

  set_value_stack (v, 1);
  return v;
}

/* Return a value which indicates that FRAME's saved version of
   REGNUM has a known constant (computed) value of VAL.  */

struct value *
frame_unwind_got_constant (struct frame_info *frame, int regnum,
			   ULONGEST val)
{
  struct gdbarch *gdbarch = frame_unwind_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct value *reg_val;

  reg_val = value_zero (register_type (gdbarch, regnum), not_lval);
  store_unsigned_integer (value_contents_writeable (reg_val),
			  register_size (gdbarch, regnum), byte_order, val);
  return reg_val;
}

struct value *
frame_unwind_got_bytes (struct frame_info *frame, int regnum, gdb_byte *buf)
{
  struct gdbarch *gdbarch = frame_unwind_arch (frame);
  struct value *reg_val;

  reg_val = value_zero (register_type (gdbarch, regnum), not_lval);
  memcpy (value_contents_raw (reg_val), buf, register_size (gdbarch, regnum));
  return reg_val;
}

/* Return a value which indicates that FRAME's saved version of REGNUM
   has a known constant (computed) value of ADDR.  Convert the
   CORE_ADDR to a target address if necessary.  */

struct value *
frame_unwind_got_address (struct frame_info *frame, int regnum,
			  CORE_ADDR addr)
{
  struct gdbarch *gdbarch = frame_unwind_arch (frame);
  struct value *reg_val;

  reg_val = value_zero (register_type (gdbarch, regnum), not_lval);
  pack_long (value_contents_writeable (reg_val),
	     register_type (gdbarch, regnum), addr);
  return reg_val;
}

void _initialize_frame_unwind ();
void
_initialize_frame_unwind ()
{
  frame_unwind_data = gdbarch_data_register_pre_init (frame_unwind_init);
}
