/* DWARF 2 location expression support for GDB.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

   Contributed by Daniel Jacobowitz, MontaVista Software, Inc.

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
#include "ui-out.h"
#include "value.h"
#include "frame.h"
#include "gdbcore.h"
#include "target.h"
#include "inferior.h"
#include "ax.h"
#include "ax-gdb.h"
#include "regcache.h"
#include "objfiles.h"
#include "block.h"
#include "gdbcmd.h"
#include "complaints.h"
#include "dwarf2.h"
#include "dwarf2expr.h"
#include "dwarf2loc.h"
#include "dwarf2-frame.h"
#include "compile/compile.h"
#include "selftest.h"
#include <algorithm>
#include <vector>
#include <unordered_set>
#include "common/underlying.h"

extern int dwarf_always_disassemble;

static struct value *dwarf2_evaluate_loc_desc_full (struct type *type,
						    struct frame_info *frame,
						    const gdb_byte *data,
						    size_t size,
						    struct dwarf2_per_cu_data *per_cu,
						    struct type *subobj_type,
						    LONGEST subobj_byte_offset);

static struct call_site_parameter *dwarf_expr_reg_to_entry_parameter
    (struct frame_info *frame,
     enum call_site_parameter_kind kind,
     union call_site_parameter_u kind_u,
     struct dwarf2_per_cu_data **per_cu_return);

/* Until these have formal names, we define these here.
   ref: http://gcc.gnu.org/wiki/DebugFission
   Each entry in .debug_loc.dwo begins with a byte that describes the entry,
   and is then followed by data specific to that entry.  */

enum debug_loc_kind
{
  /* Indicates the end of the list of entries.  */
  DEBUG_LOC_END_OF_LIST = 0,

  /* This is followed by an unsigned LEB128 number that is an index into
     .debug_addr and specifies the base address for all following entries.  */
  DEBUG_LOC_BASE_ADDRESS = 1,

  /* This is followed by two unsigned LEB128 numbers that are indices into
     .debug_addr and specify the beginning and ending addresses, and then
     a normal location expression as in .debug_loc.  */
  DEBUG_LOC_START_END = 2,

  /* This is followed by an unsigned LEB128 number that is an index into
     .debug_addr and specifies the beginning address, and a 4 byte unsigned
     number that specifies the length, and then a normal location expression
     as in .debug_loc.  */
  DEBUG_LOC_START_LENGTH = 3,

  /* An internal value indicating there is insufficient data.  */
  DEBUG_LOC_BUFFER_OVERFLOW = -1,

  /* An internal value indicating an invalid kind of entry was found.  */
  DEBUG_LOC_INVALID_ENTRY = -2
};

/* Helper function which throws an error if a synthetic pointer is
   invalid.  */

static void
invalid_synthetic_pointer (void)
{
  error (_("access outside bounds of object "
	   "referenced via synthetic pointer"));
}

/* Decode the addresses in a non-dwo .debug_loc entry.
   A pointer to the next byte to examine is returned in *NEW_PTR.
   The encoded low,high addresses are return in *LOW,*HIGH.
   The result indicates the kind of entry found.  */

static enum debug_loc_kind
decode_debug_loc_addresses (const gdb_byte *loc_ptr, const gdb_byte *buf_end,
			    const gdb_byte **new_ptr,
			    CORE_ADDR *low, CORE_ADDR *high,
			    enum bfd_endian byte_order,
			    unsigned int addr_size,
			    int signed_addr_p)
{
  CORE_ADDR base_mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));

  if (buf_end - loc_ptr < 2 * addr_size)
    return DEBUG_LOC_BUFFER_OVERFLOW;

  if (signed_addr_p)
    *low = extract_signed_integer (loc_ptr, addr_size, byte_order);
  else
    *low = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
  loc_ptr += addr_size;

  if (signed_addr_p)
    *high = extract_signed_integer (loc_ptr, addr_size, byte_order);
  else
    *high = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
  loc_ptr += addr_size;

  *new_ptr = loc_ptr;

  /* A base-address-selection entry.  */
  if ((*low & base_mask) == base_mask)
    return DEBUG_LOC_BASE_ADDRESS;

  /* An end-of-list entry.  */
  if (*low == 0 && *high == 0)
    return DEBUG_LOC_END_OF_LIST;

  return DEBUG_LOC_START_END;
}

/* Decode the addresses in .debug_loclists entry.
   A pointer to the next byte to examine is returned in *NEW_PTR.
   The encoded low,high addresses are return in *LOW,*HIGH.
   The result indicates the kind of entry found.  */

static enum debug_loc_kind
decode_debug_loclists_addresses (struct dwarf2_per_cu_data *per_cu,
				 const gdb_byte *loc_ptr,
				 const gdb_byte *buf_end,
				 const gdb_byte **new_ptr,
				 CORE_ADDR *low, CORE_ADDR *high,
				 enum bfd_endian byte_order,
				 unsigned int addr_size,
				 int signed_addr_p)
{
  uint64_t u64;

  if (loc_ptr == buf_end)
    return DEBUG_LOC_BUFFER_OVERFLOW;

  switch (*loc_ptr++)
    {
    case DW_LLE_end_of_list:
      *new_ptr = loc_ptr;
      return DEBUG_LOC_END_OF_LIST;
    case DW_LLE_base_address:
      if (loc_ptr + addr_size > buf_end)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      if (signed_addr_p)
	*high = extract_signed_integer (loc_ptr, addr_size, byte_order);
      else
	*high = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
      loc_ptr += addr_size;
      *new_ptr = loc_ptr;
      return DEBUG_LOC_BASE_ADDRESS;
    case DW_LLE_offset_pair:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *low = u64;
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &u64);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *high = u64;
      *new_ptr = loc_ptr;
      return DEBUG_LOC_START_END;
    default:
      return DEBUG_LOC_INVALID_ENTRY;
    }
}

/* Decode the addresses in .debug_loc.dwo entry.
   A pointer to the next byte to examine is returned in *NEW_PTR.
   The encoded low,high addresses are return in *LOW,*HIGH.
   The result indicates the kind of entry found.  */

static enum debug_loc_kind
decode_debug_loc_dwo_addresses (struct dwarf2_per_cu_data *per_cu,
				const gdb_byte *loc_ptr,
				const gdb_byte *buf_end,
				const gdb_byte **new_ptr,
				CORE_ADDR *low, CORE_ADDR *high,
				enum bfd_endian byte_order)
{
  uint64_t low_index, high_index;

  if (loc_ptr == buf_end)
    return DEBUG_LOC_BUFFER_OVERFLOW;

  switch (*loc_ptr++)
    {
    case DW_LLE_GNU_end_of_list_entry:
      *new_ptr = loc_ptr;
      return DEBUG_LOC_END_OF_LIST;
    case DW_LLE_GNU_base_address_selection_entry:
      *low = 0;
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &high_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *high = dwarf2_read_addr_index (per_cu, high_index);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_BASE_ADDRESS;
    case DW_LLE_GNU_start_end_entry:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &low_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *low = dwarf2_read_addr_index (per_cu, low_index);
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &high_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *high = dwarf2_read_addr_index (per_cu, high_index);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_START_END;
    case DW_LLE_GNU_start_length_entry:
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &low_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *low = dwarf2_read_addr_index (per_cu, low_index);
      if (loc_ptr + 4 > buf_end)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *high = *low;
      *high += extract_unsigned_integer (loc_ptr, 4, byte_order);
      *new_ptr = loc_ptr + 4;
      return DEBUG_LOC_START_LENGTH;
    default:
      return DEBUG_LOC_INVALID_ENTRY;
    }
}

/* A function for dealing with location lists.  Given a
   symbol baton (BATON) and a pc value (PC), find the appropriate
   location expression, set *LOCEXPR_LENGTH, and return a pointer
   to the beginning of the expression.  Returns NULL on failure.

   For now, only return the first matching location expression; there
   can be more than one in the list.  */

const gdb_byte *
dwarf2_find_location_expression (struct dwarf2_loclist_baton *baton,
				 size_t *locexpr_length, CORE_ADDR pc)
{
  struct objfile *objfile = dwarf2_per_cu_objfile (baton->per_cu);
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int addr_size = dwarf2_per_cu_addr_size (baton->per_cu);
  int signed_addr_p = bfd_get_sign_extend_vma (objfile->obfd);
  /* Adjust base_address for relocatable objects.  */
  CORE_ADDR base_offset = dwarf2_per_cu_text_offset (baton->per_cu);
  CORE_ADDR base_address = baton->base_address + base_offset;
  const gdb_byte *loc_ptr, *buf_end;

  loc_ptr = baton->data;
  buf_end = baton->data + baton->size;

  while (1)
    {
      CORE_ADDR low = 0, high = 0; /* init for gcc -Wall */
      int length;
      enum debug_loc_kind kind;
      const gdb_byte *new_ptr = NULL; /* init for gcc -Wall */

      if (baton->from_dwo)
	kind = decode_debug_loc_dwo_addresses (baton->per_cu,
					       loc_ptr, buf_end, &new_ptr,
					       &low, &high, byte_order);
      else if (dwarf2_version (baton->per_cu) < 5)
	kind = decode_debug_loc_addresses (loc_ptr, buf_end, &new_ptr,
					   &low, &high,
					   byte_order, addr_size,
					   signed_addr_p);
      else
	kind = decode_debug_loclists_addresses (baton->per_cu,
						loc_ptr, buf_end, &new_ptr,
						&low, &high, byte_order,
						addr_size, signed_addr_p);

      loc_ptr = new_ptr;
      switch (kind)
	{
	case DEBUG_LOC_END_OF_LIST:
	  *locexpr_length = 0;
	  return NULL;
	case DEBUG_LOC_BASE_ADDRESS:
	  base_address = high + base_offset;
	  continue;
	case DEBUG_LOC_START_END:
	case DEBUG_LOC_START_LENGTH:
	  break;
	case DEBUG_LOC_BUFFER_OVERFLOW:
	case DEBUG_LOC_INVALID_ENTRY:
	  error (_("dwarf2_find_location_expression: "
		   "Corrupted DWARF expression."));
	default:
	  gdb_assert_not_reached ("bad debug_loc_kind");
	}

      /* Otherwise, a location expression entry.
	 If the entry is from a DWO, don't add base address: the entry is from
	 .debug_addr which already has the DWARF "base address".  We still add
	 base_offset in case we're debugging a PIE executable.  */
      if (baton->from_dwo)
	{
	  low += base_offset;
	  high += base_offset;
	}
      else
	{
	  low += base_address;
	  high += base_address;
	}

      if (dwarf2_version (baton->per_cu) < 5)
	{
	  length = extract_unsigned_integer (loc_ptr, 2, byte_order);
	  loc_ptr += 2;
	}
      else
	{
	  unsigned int bytes_read;

	  length = read_unsigned_leb128 (NULL, loc_ptr, &bytes_read);
	  loc_ptr += bytes_read;
	}

      if (low == high && pc == low)
	{
	  /* This is entry PC record present only at entry point
	     of a function.  Verify it is really the function entry point.  */

	  const struct block *pc_block = block_for_pc (pc);
	  struct symbol *pc_func = NULL;

	  if (pc_block)
	    pc_func = block_linkage_function (pc_block);

	  if (pc_func && pc == BLOCK_START (SYMBOL_BLOCK_VALUE (pc_func)))
	    {
	      *locexpr_length = length;
	      return loc_ptr;
	    }
	}

      if (pc >= low && pc < high)
	{
	  *locexpr_length = length;
	  return loc_ptr;
	}

      loc_ptr += length;
    }
}

/* This is the baton used when performing dwarf2 expression
   evaluation.  */
struct dwarf_expr_baton
{
  struct frame_info *frame;
  struct dwarf2_per_cu_data *per_cu;
  CORE_ADDR obj_address;
};

/* Implement find_frame_base_location method for LOC_BLOCK functions using
   DWARF expression for its DW_AT_frame_base.  */

static void
locexpr_find_frame_base_location (struct symbol *framefunc, CORE_ADDR pc,
				  const gdb_byte **start, size_t *length)
{
  struct dwarf2_locexpr_baton *symbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (framefunc);

  *length = symbaton->size;
  *start = symbaton->data;
}

/* Implement the struct symbol_block_ops::get_frame_base method for
   LOC_BLOCK functions using a DWARF expression as its DW_AT_frame_base.  */

static CORE_ADDR
locexpr_get_frame_base (struct symbol *framefunc, struct frame_info *frame)
{
  struct gdbarch *gdbarch;
  struct type *type;
  struct dwarf2_locexpr_baton *dlbaton;
  const gdb_byte *start;
  size_t length;
  struct value *result;

  /* If this method is called, then FRAMEFUNC is supposed to be a DWARF block.
     Thus, it's supposed to provide the find_frame_base_location method as
     well.  */
  gdb_assert (SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location != NULL);

  gdbarch = get_frame_arch (frame);
  type = builtin_type (gdbarch)->builtin_data_ptr;
  dlbaton = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (framefunc);

  SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location
    (framefunc, get_frame_pc (frame), &start, &length);
  result = dwarf2_evaluate_loc_desc (type, frame, start, length,
				     dlbaton->per_cu);

  /* The DW_AT_frame_base attribute contains a location description which
     computes the base address itself.  However, the call to
     dwarf2_evaluate_loc_desc returns a value representing a variable at
     that address.  The frame base address is thus this variable's
     address.  */
  return value_address (result);
}

/* Vector for inferior functions as represented by LOC_BLOCK, if the inferior
   function uses DWARF expression for its DW_AT_frame_base.  */

const struct symbol_block_ops dwarf2_block_frame_base_locexpr_funcs =
{
  locexpr_find_frame_base_location,
  locexpr_get_frame_base
};

/* Implement find_frame_base_location method for LOC_BLOCK functions using
   DWARF location list for its DW_AT_frame_base.  */

static void
loclist_find_frame_base_location (struct symbol *framefunc, CORE_ADDR pc,
				  const gdb_byte **start, size_t *length)
{
  struct dwarf2_loclist_baton *symbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (framefunc);

  *start = dwarf2_find_location_expression (symbaton, length, pc);
}

/* Implement the struct symbol_block_ops::get_frame_base method for
   LOC_BLOCK functions using a DWARF location list as its DW_AT_frame_base.  */

static CORE_ADDR
loclist_get_frame_base (struct symbol *framefunc, struct frame_info *frame)
{
  struct gdbarch *gdbarch;
  struct type *type;
  struct dwarf2_loclist_baton *dlbaton;
  const gdb_byte *start;
  size_t length;
  struct value *result;

  /* If this method is called, then FRAMEFUNC is supposed to be a DWARF block.
     Thus, it's supposed to provide the find_frame_base_location method as
     well.  */
  gdb_assert (SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location != NULL);

  gdbarch = get_frame_arch (frame);
  type = builtin_type (gdbarch)->builtin_data_ptr;
  dlbaton = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (framefunc);

  SYMBOL_BLOCK_OPS (framefunc)->find_frame_base_location
    (framefunc, get_frame_pc (frame), &start, &length);
  result = dwarf2_evaluate_loc_desc (type, frame, start, length,
				     dlbaton->per_cu);

  /* The DW_AT_frame_base attribute contains a location description which
     computes the base address itself.  However, the call to
     dwarf2_evaluate_loc_desc returns a value representing a variable at
     that address.  The frame base address is thus this variable's
     address.  */
  return value_address (result);
}

/* Vector for inferior functions as represented by LOC_BLOCK, if the inferior
   function uses DWARF location list for its DW_AT_frame_base.  */

const struct symbol_block_ops dwarf2_block_frame_base_loclist_funcs =
{
  loclist_find_frame_base_location,
  loclist_get_frame_base
};

/* See dwarf2loc.h.  */

void
func_get_frame_base_dwarf_block (struct symbol *framefunc, CORE_ADDR pc,
				 const gdb_byte **start, size_t *length)
{
  if (SYMBOL_BLOCK_OPS (framefunc) != NULL)
    {
      const struct symbol_block_ops *ops_block = SYMBOL_BLOCK_OPS (framefunc);

      ops_block->find_frame_base_location (framefunc, pc, start, length);
    }
  else
    *length = 0;

  if (*length == 0)
    error (_("Could not find the frame base for \"%s\"."),
	   SYMBOL_NATURAL_NAME (framefunc));
}

static CORE_ADDR
get_frame_pc_for_per_cu_dwarf_call (void *baton)
{
  dwarf_expr_context *ctx = (dwarf_expr_context *) baton;

  return ctx->get_frame_pc ();
}

static void
per_cu_dwarf_call (struct dwarf_expr_context *ctx, cu_offset die_offset,
		   struct dwarf2_per_cu_data *per_cu)
{
  struct dwarf2_locexpr_baton block;

  block = dwarf2_fetch_die_loc_cu_off (die_offset, per_cu,
				       get_frame_pc_for_per_cu_dwarf_call,
				       ctx);

  /* DW_OP_call_ref is currently not supported.  */
  gdb_assert (block.per_cu == per_cu);

  ctx->eval (block.data, block.size);
}

class dwarf_evaluate_loc_desc : public dwarf_expr_context
{
 public:

  struct frame_info *frame;
  struct dwarf2_per_cu_data *per_cu;
  CORE_ADDR obj_address;

  /* Helper function for dwarf2_evaluate_loc_desc.  Computes the CFA for
     the frame in BATON.  */

  CORE_ADDR get_frame_cfa () OVERRIDE
  {
    return dwarf2_frame_cfa (frame);
  }

  /* Helper function for dwarf2_evaluate_loc_desc.  Computes the PC for
     the frame in BATON.  */

  CORE_ADDR get_frame_pc () OVERRIDE
  {
    return get_frame_address_in_block (frame);
  }

  /* Using the objfile specified in BATON, find the address for the
     current thread's thread-local storage with offset OFFSET.  */
  CORE_ADDR get_tls_address (CORE_ADDR offset) OVERRIDE
  {
    struct objfile *objfile = dwarf2_per_cu_objfile (per_cu);

    return target_translate_tls_address (objfile, offset);
  }

  /* Helper interface of per_cu_dwarf_call for
     dwarf2_evaluate_loc_desc.  */

  void dwarf_call (cu_offset die_offset) OVERRIDE
  {
    per_cu_dwarf_call (this, die_offset, per_cu);
  }

  struct type *get_base_type (cu_offset die_offset, int size) OVERRIDE
  {
    struct type *result = dwarf2_get_die_type (die_offset, per_cu);
    if (result == NULL)
      error (_("Could not find type for DW_OP_const_type"));
    if (size != 0 && TYPE_LENGTH (result) != size)
      error (_("DW_OP_const_type has different sizes for type and data"));
    return result;
  }

  /* Callback function for dwarf2_evaluate_loc_desc.
     Fetch the address indexed by DW_OP_GNU_addr_index.  */

  CORE_ADDR get_addr_index (unsigned int index) OVERRIDE
  {
    return dwarf2_read_addr_index (per_cu, index);
  }

  /* Callback function for get_object_address. Return the address of the VLA
     object.  */

  CORE_ADDR get_object_address () OVERRIDE
  {
    if (obj_address == 0)
      error (_("Location address is not set."));
    return obj_address;
  }

  /* Execute DWARF block of call_site_parameter which matches KIND and
     KIND_U.  Choose DEREF_SIZE value of that parameter.  Search
     caller of this objects's frame.

     The caller can be from a different CU - per_cu_dwarf_call
     implementation can be more simple as it does not support cross-CU
     DWARF executions.  */

  void push_dwarf_reg_entry_value (enum call_site_parameter_kind kind,
				   union call_site_parameter_u kind_u,
				   int deref_size) OVERRIDE
  {
    struct frame_info *caller_frame;
    struct dwarf2_per_cu_data *caller_per_cu;
    struct call_site_parameter *parameter;
    const gdb_byte *data_src;
    size_t size;

    caller_frame = get_prev_frame (frame);

    parameter = dwarf_expr_reg_to_entry_parameter (frame, kind, kind_u,
						   &caller_per_cu);
    data_src = deref_size == -1 ? parameter->value : parameter->data_value;
    size = deref_size == -1 ? parameter->value_size : parameter->data_value_size;

    /* DEREF_SIZE size is not verified here.  */
    if (data_src == NULL)
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("Cannot resolve DW_AT_call_data_value"));

    scoped_restore save_frame = make_scoped_restore (&this->frame,
						     caller_frame);
    scoped_restore save_per_cu = make_scoped_restore (&this->per_cu,
						      caller_per_cu);
    scoped_restore save_obj_addr = make_scoped_restore (&this->obj_address,
							(CORE_ADDR) 0);

    scoped_restore save_arch = make_scoped_restore (&this->gdbarch);
    this->gdbarch
      = get_objfile_arch (dwarf2_per_cu_objfile (per_cu));
    scoped_restore save_addr_size = make_scoped_restore (&this->addr_size);
    this->addr_size = dwarf2_per_cu_addr_size (per_cu);
    scoped_restore save_offset = make_scoped_restore (&this->offset);
    this->offset = dwarf2_per_cu_text_offset (per_cu);

    this->eval (data_src, size);
  }

  /* Using the frame specified in BATON, find the location expression
     describing the frame base.  Return a pointer to it in START and
     its length in LENGTH.  */
  void get_frame_base (const gdb_byte **start, size_t * length) OVERRIDE
  {
    /* FIXME: cagney/2003-03-26: This code should be using
       get_frame_base_address(), and then implement a dwarf2 specific
       this_base method.  */
    struct symbol *framefunc;
    const struct block *bl = get_frame_block (frame, NULL);

    if (bl == NULL)
      error (_("frame address is not available."));

    /* Use block_linkage_function, which returns a real (not inlined)
       function, instead of get_frame_function, which may return an
       inlined function.  */
    framefunc = block_linkage_function (bl);

    /* If we found a frame-relative symbol then it was certainly within
       some function associated with a frame. If we can't find the frame,
       something has gone wrong.  */
    gdb_assert (framefunc != NULL);

    func_get_frame_base_dwarf_block (framefunc,
				     get_frame_address_in_block (frame),
				     start, length);
  }

  /* Read memory at ADDR (length LEN) into BUF.  */

  void read_mem (gdb_byte *buf, CORE_ADDR addr, size_t len) OVERRIDE
  {
    read_memory (addr, buf, len);
  }

  /* Using the frame specified in BATON, return the value of register
     REGNUM, treated as a pointer.  */
  CORE_ADDR read_addr_from_reg (int dwarf_regnum) OVERRIDE
  {
    struct gdbarch *gdbarch = get_frame_arch (frame);
    int regnum = dwarf_reg_to_regnum_or_error (gdbarch, dwarf_regnum);

    return address_from_register (regnum, frame);
  }

  /* Implement "get_reg_value" callback.  */

  struct value *get_reg_value (struct type *type, int dwarf_regnum) OVERRIDE
  {
    struct gdbarch *gdbarch = get_frame_arch (frame);
    int regnum = dwarf_reg_to_regnum_or_error (gdbarch, dwarf_regnum);

    return value_from_register (type, regnum, frame);
  }
};

/* See dwarf2loc.h.  */

unsigned int entry_values_debug = 0;

/* Helper to set entry_values_debug.  */

static void
show_entry_values_debug (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Entry values and tail call frames debugging is %s.\n"),
		    value);
}

/* Find DW_TAG_call_site's DW_AT_call_target address.
   CALLER_FRAME (for registers) can be NULL if it is not known.  This function
   always returns valid address or it throws NO_ENTRY_VALUE_ERROR.  */

static CORE_ADDR
call_site_to_target_addr (struct gdbarch *call_site_gdbarch,
			  struct call_site *call_site,
			  struct frame_info *caller_frame)
{
  switch (FIELD_LOC_KIND (call_site->target))
    {
    case FIELD_LOC_KIND_DWARF_BLOCK:
      {
	struct dwarf2_locexpr_baton *dwarf_block;
	struct value *val;
	struct type *caller_core_addr_type;
	struct gdbarch *caller_arch;

	dwarf_block = FIELD_DWARF_BLOCK (call_site->target);
	if (dwarf_block == NULL)
	  {
	    struct bound_minimal_symbol msym;
	    
	    msym = lookup_minimal_symbol_by_pc (call_site->pc - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("DW_AT_call_target is not specified at %s in %s"),
			 paddress (call_site_gdbarch, call_site->pc),
			 (msym.minsym == NULL ? "???"
			  : MSYMBOL_PRINT_NAME (msym.minsym)));
			
	  }
	if (caller_frame == NULL)
	  {
	    struct bound_minimal_symbol msym;
	    
	    msym = lookup_minimal_symbol_by_pc (call_site->pc - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("DW_AT_call_target DWARF block resolving "
			   "requires known frame which is currently not "
			   "available at %s in %s"),
			 paddress (call_site_gdbarch, call_site->pc),
			 (msym.minsym == NULL ? "???"
			  : MSYMBOL_PRINT_NAME (msym.minsym)));
			
	  }
	caller_arch = get_frame_arch (caller_frame);
	caller_core_addr_type = builtin_type (caller_arch)->builtin_func_ptr;
	val = dwarf2_evaluate_loc_desc (caller_core_addr_type, caller_frame,
					dwarf_block->data, dwarf_block->size,
					dwarf_block->per_cu);
	/* DW_AT_call_target is a DWARF expression, not a DWARF location.  */
	if (VALUE_LVAL (val) == lval_memory)
	  return value_address (val);
	else
	  return value_as_address (val);
      }

    case FIELD_LOC_KIND_PHYSNAME:
      {
	const char *physname;
	struct bound_minimal_symbol msym;

	physname = FIELD_STATIC_PHYSNAME (call_site->target);

	/* Handle both the mangled and demangled PHYSNAME.  */
	msym = lookup_minimal_symbol (physname, NULL, NULL);
	if (msym.minsym == NULL)
	  {
	    msym = lookup_minimal_symbol_by_pc (call_site->pc - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("Cannot find function \"%s\" for a call site target "
			   "at %s in %s"),
			 physname, paddress (call_site_gdbarch, call_site->pc),
			 (msym.minsym == NULL ? "???"
			  : MSYMBOL_PRINT_NAME (msym.minsym)));
			
	  }
	return BMSYMBOL_VALUE_ADDRESS (msym);
      }

    case FIELD_LOC_KIND_PHYSADDR:
      return FIELD_STATIC_PHYSADDR (call_site->target);

    default:
      internal_error (__FILE__, __LINE__, _("invalid call site target kind"));
    }
}

/* Convert function entry point exact address ADDR to the function which is
   compliant with TAIL_CALL_LIST_COMPLETE condition.  Throw
   NO_ENTRY_VALUE_ERROR otherwise.  */

static struct symbol *
func_addr_to_tail_call_list (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  struct symbol *sym = find_pc_function (addr);
  struct type *type;

  if (sym == NULL || BLOCK_START (SYMBOL_BLOCK_VALUE (sym)) != addr)
    throw_error (NO_ENTRY_VALUE_ERROR,
		 _("DW_TAG_call_site resolving failed to find function "
		   "name for address %s"),
		 paddress (gdbarch, addr));

  type = SYMBOL_TYPE (sym);
  gdb_assert (TYPE_CODE (type) == TYPE_CODE_FUNC);
  gdb_assert (TYPE_SPECIFIC_FIELD (type) == TYPE_SPECIFIC_FUNC);

  return sym;
}

/* Verify function with entry point exact address ADDR can never call itself
   via its tail calls (incl. transitively).  Throw NO_ENTRY_VALUE_ERROR if it
   can call itself via tail calls.

   If a funtion can tail call itself its entry value based parameters are
   unreliable.  There is no verification whether the value of some/all
   parameters is unchanged through the self tail call, we expect if there is
   a self tail call all the parameters can be modified.  */

static void
func_verify_no_selftailcall (struct gdbarch *gdbarch, CORE_ADDR verify_addr)
{
  CORE_ADDR addr;

  /* The verification is completely unordered.  Track here function addresses
     which still need to be iterated.  */
  std::vector<CORE_ADDR> todo;

  /* Track here CORE_ADDRs which were already visited.  */
  std::unordered_set<CORE_ADDR> addr_hash;

  todo.push_back (verify_addr);
  while (!todo.empty ())
    {
      struct symbol *func_sym;
      struct call_site *call_site;

      addr = todo.back ();
      todo.pop_back ();

      func_sym = func_addr_to_tail_call_list (gdbarch, addr);

      for (call_site = TYPE_TAIL_CALL_LIST (SYMBOL_TYPE (func_sym));
	   call_site; call_site = call_site->tail_call_next)
	{
	  CORE_ADDR target_addr;

	  /* CALLER_FRAME with registers is not available for tail-call jumped
	     frames.  */
	  target_addr = call_site_to_target_addr (gdbarch, call_site, NULL);

	  if (target_addr == verify_addr)
	    {
	      struct bound_minimal_symbol msym;
	      
	      msym = lookup_minimal_symbol_by_pc (verify_addr);
	      throw_error (NO_ENTRY_VALUE_ERROR,
			   _("DW_OP_entry_value resolving has found "
			     "function \"%s\" at %s can call itself via tail "
			     "calls"),
			   (msym.minsym == NULL ? "???"
			    : MSYMBOL_PRINT_NAME (msym.minsym)),
			   paddress (gdbarch, verify_addr));
	    }

	  if (addr_hash.insert (target_addr).second)
	    todo.push_back (target_addr);
	}
    }
}

/* Print user readable form of CALL_SITE->PC to gdb_stdlog.  Used only for
   ENTRY_VALUES_DEBUG.  */

static void
tailcall_dump (struct gdbarch *gdbarch, const struct call_site *call_site)
{
  CORE_ADDR addr = call_site->pc;
  struct bound_minimal_symbol msym = lookup_minimal_symbol_by_pc (addr - 1);

  fprintf_unfiltered (gdb_stdlog, " %s(%s)", paddress (gdbarch, addr),
		      (msym.minsym == NULL ? "???"
		       : MSYMBOL_PRINT_NAME (msym.minsym)));

}

/* Intersect RESULTP with CHAIN to keep RESULTP unambiguous, keep in RESULTP
   only top callers and bottom callees which are present in both.  GDBARCH is
   used only for ENTRY_VALUES_DEBUG.  RESULTP is NULL after return if there are
   no remaining possibilities to provide unambiguous non-trivial result.
   RESULTP should point to NULL on the first (initialization) call.  Caller is
   responsible for xfree of any RESULTP data.  */

static void
chain_candidate (struct gdbarch *gdbarch,
		 gdb::unique_xmalloc_ptr<struct call_site_chain> *resultp,
		 std::vector<struct call_site *> *chain)
{
  long length = chain->size ();
  int callers, callees, idx;

  if (*resultp == NULL)
    {
      /* Create the initial chain containing all the passed PCs.  */

      struct call_site_chain *result
	= ((struct call_site_chain *)
	   xmalloc (sizeof (*result)
		    + sizeof (*result->call_site) * (length - 1)));
      result->length = length;
      result->callers = result->callees = length;
      if (!chain->empty ())
	memcpy (result->call_site, chain->data (),
		sizeof (*result->call_site) * length);
      resultp->reset (result);

      if (entry_values_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "tailcall: initial:");
	  for (idx = 0; idx < length; idx++)
	    tailcall_dump (gdbarch, result->call_site[idx]);
	  fputc_unfiltered ('\n', gdb_stdlog);
	}

      return;
    }

  if (entry_values_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "tailcall: compare:");
      for (idx = 0; idx < length; idx++)
	tailcall_dump (gdbarch, chain->at (idx));
      fputc_unfiltered ('\n', gdb_stdlog);
    }

  /* Intersect callers.  */

  callers = std::min ((long) (*resultp)->callers, length);
  for (idx = 0; idx < callers; idx++)
    if ((*resultp)->call_site[idx] != chain->at (idx))
      {
	(*resultp)->callers = idx;
	break;
      }

  /* Intersect callees.  */

  callees = std::min ((long) (*resultp)->callees, length);
  for (idx = 0; idx < callees; idx++)
    if ((*resultp)->call_site[(*resultp)->length - 1 - idx]
	!= chain->at (length - 1 - idx))
      {
	(*resultp)->callees = idx;
	break;
      }

  if (entry_values_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "tailcall: reduced:");
      for (idx = 0; idx < (*resultp)->callers; idx++)
	tailcall_dump (gdbarch, (*resultp)->call_site[idx]);
      fputs_unfiltered (" |", gdb_stdlog);
      for (idx = 0; idx < (*resultp)->callees; idx++)
	tailcall_dump (gdbarch,
		       (*resultp)->call_site[(*resultp)->length
					     - (*resultp)->callees + idx]);
      fputc_unfiltered ('\n', gdb_stdlog);
    }

  if ((*resultp)->callers == 0 && (*resultp)->callees == 0)
    {
      /* There are no common callers or callees.  It could be also a direct
	 call (which has length 0) with ambiguous possibility of an indirect
	 call - CALLERS == CALLEES == 0 is valid during the first allocation
	 but any subsequence processing of such entry means ambiguity.  */
      resultp->reset (NULL);
      return;
    }

  /* See call_site_find_chain_1 why there is no way to reach the bottom callee
     PC again.  In such case there must be two different code paths to reach
     it.  CALLERS + CALLEES equal to LENGTH in the case of self tail-call.  */
  gdb_assert ((*resultp)->callers + (*resultp)->callees <= (*resultp)->length);
}

/* Create and return call_site_chain for CALLER_PC and CALLEE_PC.  All the
   assumed frames between them use GDBARCH.  Use depth first search so we can
   keep single CHAIN of call_site's back to CALLER_PC.  Function recursion
   would have needless GDB stack overhead.  Caller is responsible for xfree of
   the returned result.  Any unreliability results in thrown
   NO_ENTRY_VALUE_ERROR.  */

static struct call_site_chain *
call_site_find_chain_1 (struct gdbarch *gdbarch, CORE_ADDR caller_pc,
			CORE_ADDR callee_pc)
{
  CORE_ADDR save_callee_pc = callee_pc;
  gdb::unique_xmalloc_ptr<struct call_site_chain> retval;
  struct call_site *call_site;

  /* CHAIN contains only the intermediate CALL_SITEs.  Neither CALLER_PC's
     call_site nor any possible call_site at CALLEE_PC's function is there.
     Any CALL_SITE in CHAIN will be iterated to its siblings - via
     TAIL_CALL_NEXT.  This is inappropriate for CALLER_PC's call_site.  */
  std::vector<struct call_site *> chain;

  /* We are not interested in the specific PC inside the callee function.  */
  callee_pc = get_pc_function_start (callee_pc);
  if (callee_pc == 0)
    throw_error (NO_ENTRY_VALUE_ERROR, _("Unable to find function for PC %s"),
		 paddress (gdbarch, save_callee_pc));

  /* Mark CALL_SITEs so we do not visit the same ones twice.  */
  std::unordered_set<CORE_ADDR> addr_hash;

  /* Do not push CALL_SITE to CHAIN.  Push there only the first tail call site
     at the target's function.  All the possible tail call sites in the
     target's function will get iterated as already pushed into CHAIN via their
     TAIL_CALL_NEXT.  */
  call_site = call_site_for_pc (gdbarch, caller_pc);

  while (call_site)
    {
      CORE_ADDR target_func_addr;
      struct call_site *target_call_site;

      /* CALLER_FRAME with registers is not available for tail-call jumped
	 frames.  */
      target_func_addr = call_site_to_target_addr (gdbarch, call_site, NULL);

      if (target_func_addr == callee_pc)
	{
	  chain_candidate (gdbarch, &retval, &chain);
	  if (retval == NULL)
	    break;

	  /* There is no way to reach CALLEE_PC again as we would prevent
	     entering it twice as being already marked in ADDR_HASH.  */
	  target_call_site = NULL;
	}
      else
	{
	  struct symbol *target_func;

	  target_func = func_addr_to_tail_call_list (gdbarch, target_func_addr);
	  target_call_site = TYPE_TAIL_CALL_LIST (SYMBOL_TYPE (target_func));
	}

      do
	{
	  /* Attempt to visit TARGET_CALL_SITE.  */

	  if (target_call_site)
	    {
	      if (addr_hash.insert (target_call_site->pc).second)
		{
		  /* Successfully entered TARGET_CALL_SITE.  */

		  chain.push_back (target_call_site);
		  break;
		}
	    }

	  /* Backtrack (without revisiting the originating call_site).  Try the
	     callers's sibling; if there isn't any try the callers's callers's
	     sibling etc.  */

	  target_call_site = NULL;
	  while (!chain.empty ())
	    {
	      call_site = chain.back ();
	      chain.pop_back ();

	      size_t removed = addr_hash.erase (call_site->pc);
	      gdb_assert (removed == 1);

	      target_call_site = call_site->tail_call_next;
	      if (target_call_site)
		break;
	    }
	}
      while (target_call_site);

      if (chain.empty ())
	call_site = NULL;
      else
	call_site = chain.back ();
    }

  if (retval == NULL)
    {
      struct bound_minimal_symbol msym_caller, msym_callee;
      
      msym_caller = lookup_minimal_symbol_by_pc (caller_pc);
      msym_callee = lookup_minimal_symbol_by_pc (callee_pc);
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("There are no unambiguously determinable intermediate "
		     "callers or callees between caller function \"%s\" at %s "
		     "and callee function \"%s\" at %s"),
		   (msym_caller.minsym == NULL
		    ? "???" : MSYMBOL_PRINT_NAME (msym_caller.minsym)),
		   paddress (gdbarch, caller_pc),
		   (msym_callee.minsym == NULL
		    ? "???" : MSYMBOL_PRINT_NAME (msym_callee.minsym)),
		   paddress (gdbarch, callee_pc));
    }

  return retval.release ();
}

/* Create and return call_site_chain for CALLER_PC and CALLEE_PC.  All the
   assumed frames between them use GDBARCH.  If valid call_site_chain cannot be
   constructed return NULL.  Caller is responsible for xfree of the returned
   result.  */

struct call_site_chain *
call_site_find_chain (struct gdbarch *gdbarch, CORE_ADDR caller_pc,
		      CORE_ADDR callee_pc)
{
  struct call_site_chain *retval = NULL;

  TRY
    {
      retval = call_site_find_chain_1 (gdbarch, caller_pc, callee_pc);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
      if (e.error == NO_ENTRY_VALUE_ERROR)
	{
	  if (entry_values_debug)
	    exception_print (gdb_stdout, e);

	  return NULL;
	}
      else
	throw_exception (e);
    }
  END_CATCH

  return retval;
}

/* Return 1 if KIND and KIND_U match PARAMETER.  Return 0 otherwise.  */

static int
call_site_parameter_matches (struct call_site_parameter *parameter,
			     enum call_site_parameter_kind kind,
			     union call_site_parameter_u kind_u)
{
  if (kind == parameter->kind)
    switch (kind)
      {
      case CALL_SITE_PARAMETER_DWARF_REG:
	return kind_u.dwarf_reg == parameter->u.dwarf_reg;
      case CALL_SITE_PARAMETER_FB_OFFSET:
	return kind_u.fb_offset == parameter->u.fb_offset;
      case CALL_SITE_PARAMETER_PARAM_OFFSET:
	return kind_u.param_cu_off == parameter->u.param_cu_off;
      }
  return 0;
}

/* Fetch call_site_parameter from caller matching KIND and KIND_U.
   FRAME is for callee.

   Function always returns non-NULL, it throws NO_ENTRY_VALUE_ERROR
   otherwise.  */

static struct call_site_parameter *
dwarf_expr_reg_to_entry_parameter (struct frame_info *frame,
				   enum call_site_parameter_kind kind,
				   union call_site_parameter_u kind_u,
				   struct dwarf2_per_cu_data **per_cu_return)
{
  CORE_ADDR func_addr, caller_pc;
  struct gdbarch *gdbarch;
  struct frame_info *caller_frame;
  struct call_site *call_site;
  int iparams;
  /* Initialize it just to avoid a GCC false warning.  */
  struct call_site_parameter *parameter = NULL;
  CORE_ADDR target_addr;

  while (get_frame_type (frame) == INLINE_FRAME)
    {
      frame = get_prev_frame (frame);
      gdb_assert (frame != NULL);
    }

  func_addr = get_frame_func (frame);
  gdbarch = get_frame_arch (frame);
  caller_frame = get_prev_frame (frame);
  if (gdbarch != frame_unwind_arch (frame))
    {
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol_by_pc (func_addr);
      struct gdbarch *caller_gdbarch = frame_unwind_arch (frame);

      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("DW_OP_entry_value resolving callee gdbarch %s "
		     "(of %s (%s)) does not match caller gdbarch %s"),
		   gdbarch_bfd_arch_info (gdbarch)->printable_name,
		   paddress (gdbarch, func_addr),
		   (msym.minsym == NULL ? "???"
		    : MSYMBOL_PRINT_NAME (msym.minsym)),
		   gdbarch_bfd_arch_info (caller_gdbarch)->printable_name);
    }

  if (caller_frame == NULL)
    {
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol_by_pc (func_addr);

      throw_error (NO_ENTRY_VALUE_ERROR, _("DW_OP_entry_value resolving "
					   "requires caller of %s (%s)"),
		   paddress (gdbarch, func_addr),
		   (msym.minsym == NULL ? "???"
		    : MSYMBOL_PRINT_NAME (msym.minsym)));
    }
  caller_pc = get_frame_pc (caller_frame);
  call_site = call_site_for_pc (gdbarch, caller_pc);

  target_addr = call_site_to_target_addr (gdbarch, call_site, caller_frame);
  if (target_addr != func_addr)
    {
      struct minimal_symbol *target_msym, *func_msym;

      target_msym = lookup_minimal_symbol_by_pc (target_addr).minsym;
      func_msym = lookup_minimal_symbol_by_pc (func_addr).minsym;
      throw_error (NO_ENTRY_VALUE_ERROR,
		   _("DW_OP_entry_value resolving expects callee %s at %s "
		     "but the called frame is for %s at %s"),
		   (target_msym == NULL ? "???"
					: MSYMBOL_PRINT_NAME (target_msym)),
		   paddress (gdbarch, target_addr),
		   func_msym == NULL ? "???" : MSYMBOL_PRINT_NAME (func_msym),
		   paddress (gdbarch, func_addr));
    }

  /* No entry value based parameters would be reliable if this function can
     call itself via tail calls.  */
  func_verify_no_selftailcall (gdbarch, func_addr);

  for (iparams = 0; iparams < call_site->parameter_count; iparams++)
    {
      parameter = &call_site->parameter[iparams];
      if (call_site_parameter_matches (parameter, kind, kind_u))
	break;
    }
  if (iparams == call_site->parameter_count)
    {
      struct minimal_symbol *msym
	= lookup_minimal_symbol_by_pc (caller_pc).minsym;

      /* DW_TAG_call_site_parameter will be missing just if GCC could not
	 determine its value.  */
      throw_error (NO_ENTRY_VALUE_ERROR, _("Cannot find matching parameter "
					   "at DW_TAG_call_site %s at %s"),
		   paddress (gdbarch, caller_pc),
		   msym == NULL ? "???" : MSYMBOL_PRINT_NAME (msym)); 
    }

  *per_cu_return = call_site->per_cu;
  return parameter;
}

/* Return value for PARAMETER matching DEREF_SIZE.  If DEREF_SIZE is -1, return
   the normal DW_AT_call_value block.  Otherwise return the
   DW_AT_call_data_value (dereferenced) block.

   TYPE and CALLER_FRAME specify how to evaluate the DWARF block into returned
   struct value.

   Function always returns non-NULL, non-optimized out value.  It throws
   NO_ENTRY_VALUE_ERROR if it cannot resolve the value for any reason.  */

static struct value *
dwarf_entry_parameter_to_value (struct call_site_parameter *parameter,
				CORE_ADDR deref_size, struct type *type,
				struct frame_info *caller_frame,
				struct dwarf2_per_cu_data *per_cu)
{
  const gdb_byte *data_src;
  gdb_byte *data;
  size_t size;

  data_src = deref_size == -1 ? parameter->value : parameter->data_value;
  size = deref_size == -1 ? parameter->value_size : parameter->data_value_size;

  /* DEREF_SIZE size is not verified here.  */
  if (data_src == NULL)
    throw_error (NO_ENTRY_VALUE_ERROR,
		 _("Cannot resolve DW_AT_call_data_value"));

  /* DW_AT_call_value is a DWARF expression, not a DWARF
     location.  Postprocessing of DWARF_VALUE_MEMORY would lose the type from
     DWARF block.  */
  data = (gdb_byte *) alloca (size + 1);
  memcpy (data, data_src, size);
  data[size] = DW_OP_stack_value;

  return dwarf2_evaluate_loc_desc (type, caller_frame, data, size + 1, per_cu);
}

/* VALUE must be of type lval_computed with entry_data_value_funcs.  Perform
   the indirect method on it, that is use its stored target value, the sole
   purpose of entry_data_value_funcs..  */

static struct value *
entry_data_value_coerce_ref (const struct value *value)
{
  struct type *checked_type = check_typedef (value_type (value));
  struct value *target_val;

  if (!TYPE_IS_REFERENCE (checked_type))
    return NULL;

  target_val = (struct value *) value_computed_closure (value);
  value_incref (target_val);
  return target_val;
}

/* Implement copy_closure.  */

static void *
entry_data_value_copy_closure (const struct value *v)
{
  struct value *target_val = (struct value *) value_computed_closure (v);

  value_incref (target_val);
  return target_val;
}

/* Implement free_closure.  */

static void
entry_data_value_free_closure (struct value *v)
{
  struct value *target_val = (struct value *) value_computed_closure (v);

  value_free (target_val);
}

/* Vector for methods for an entry value reference where the referenced value
   is stored in the caller.  On the first dereference use
   DW_AT_call_data_value in the caller.  */

static const struct lval_funcs entry_data_value_funcs =
{
  NULL,	/* read */
  NULL,	/* write */
  NULL,	/* indirect */
  entry_data_value_coerce_ref,
  NULL,	/* check_synthetic_pointer */
  entry_data_value_copy_closure,
  entry_data_value_free_closure
};

/* Read parameter of TYPE at (callee) FRAME's function entry.  KIND and KIND_U
   are used to match DW_AT_location at the caller's
   DW_TAG_call_site_parameter.

   Function always returns non-NULL value.  It throws NO_ENTRY_VALUE_ERROR if it
   cannot resolve the parameter for any reason.  */

static struct value *
value_of_dwarf_reg_entry (struct type *type, struct frame_info *frame,
			  enum call_site_parameter_kind kind,
			  union call_site_parameter_u kind_u)
{
  struct type *checked_type = check_typedef (type);
  struct type *target_type = TYPE_TARGET_TYPE (checked_type);
  struct frame_info *caller_frame = get_prev_frame (frame);
  struct value *outer_val, *target_val, *val;
  struct call_site_parameter *parameter;
  struct dwarf2_per_cu_data *caller_per_cu;

  parameter = dwarf_expr_reg_to_entry_parameter (frame, kind, kind_u,
						 &caller_per_cu);

  outer_val = dwarf_entry_parameter_to_value (parameter, -1 /* deref_size */,
					      type, caller_frame,
					      caller_per_cu);

  /* Check if DW_AT_call_data_value cannot be used.  If it should be
     used and it is not available do not fall back to OUTER_VAL - dereferencing
     TYPE_CODE_REF with non-entry data value would give current value - not the
     entry value.  */

  if (!TYPE_IS_REFERENCE (checked_type)
      || TYPE_TARGET_TYPE (checked_type) == NULL)
    return outer_val;

  target_val = dwarf_entry_parameter_to_value (parameter,
					       TYPE_LENGTH (target_type),
					       target_type, caller_frame,
					       caller_per_cu);

  release_value (target_val);
  val = allocate_computed_value (type, &entry_data_value_funcs,
				 target_val /* closure */);

  /* Copy the referencing pointer to the new computed value.  */
  memcpy (value_contents_raw (val), value_contents_raw (outer_val),
	  TYPE_LENGTH (checked_type));
  set_value_lazy (val, 0);

  return val;
}

/* Read parameter of TYPE at (callee) FRAME's function entry.  DATA and
   SIZE are DWARF block used to match DW_AT_location at the caller's
   DW_TAG_call_site_parameter.

   Function always returns non-NULL value.  It throws NO_ENTRY_VALUE_ERROR if it
   cannot resolve the parameter for any reason.  */

static struct value *
value_of_dwarf_block_entry (struct type *type, struct frame_info *frame,
			    const gdb_byte *block, size_t block_len)
{
  union call_site_parameter_u kind_u;

  kind_u.dwarf_reg = dwarf_block_to_dwarf_reg (block, block + block_len);
  if (kind_u.dwarf_reg != -1)
    return value_of_dwarf_reg_entry (type, frame, CALL_SITE_PARAMETER_DWARF_REG,
				     kind_u);

  if (dwarf_block_to_fb_offset (block, block + block_len, &kind_u.fb_offset))
    return value_of_dwarf_reg_entry (type, frame, CALL_SITE_PARAMETER_FB_OFFSET,
                                     kind_u);

  /* This can normally happen - throw NO_ENTRY_VALUE_ERROR to get the message
     suppressed during normal operation.  The expression can be arbitrary if
     there is no caller-callee entry value binding expected.  */
  throw_error (NO_ENTRY_VALUE_ERROR,
	       _("DWARF-2 expression error: DW_OP_entry_value is supported "
		 "only for single DW_OP_reg* or for DW_OP_fbreg(*)"));
}

struct piece_closure
{
  /* Reference count.  */
  int refc;

  /* The CU from which this closure's expression came.  */
  struct dwarf2_per_cu_data *per_cu;

  /* The number of pieces used to describe this variable.  */
  int n_pieces;

  /* The target address size, used only for DWARF_VALUE_STACK.  */
  int addr_size;

  /* The pieces themselves.  */
  struct dwarf_expr_piece *pieces;

  /* Frame ID of frame to which a register value is relative, used
     only by DWARF_VALUE_REGISTER.  */
  struct frame_id frame_id;
};

/* Allocate a closure for a value formed from separately-described
   PIECES.  */

static struct piece_closure *
allocate_piece_closure (struct dwarf2_per_cu_data *per_cu,
			int n_pieces, struct dwarf_expr_piece *pieces,
			int addr_size, struct frame_info *frame)
{
  struct piece_closure *c = XCNEW (struct piece_closure);
  int i;

  c->refc = 1;
  c->per_cu = per_cu;
  c->n_pieces = n_pieces;
  c->addr_size = addr_size;
  c->pieces = XCNEWVEC (struct dwarf_expr_piece, n_pieces);
  if (frame == NULL)
    c->frame_id = null_frame_id;
  else
    c->frame_id = get_frame_id (frame);

  memcpy (c->pieces, pieces, n_pieces * sizeof (struct dwarf_expr_piece));
  for (i = 0; i < n_pieces; ++i)
    if (c->pieces[i].location == DWARF_VALUE_STACK)
      value_incref (c->pieces[i].v.value);

  return c;
}

/* Copy NBITS bits from SOURCE to DEST starting at the given bit
   offsets.  Use the bit order as specified by BITS_BIG_ENDIAN.
   Source and destination buffers must not overlap.  */

static void
copy_bitwise (gdb_byte *dest, ULONGEST dest_offset,
	      const gdb_byte *source, ULONGEST source_offset,
	      ULONGEST nbits, int bits_big_endian)
{
  unsigned int buf, avail;

  if (nbits == 0)
    return;

  if (bits_big_endian)
    {
      /* Start from the end, then work backwards.  */
      dest_offset += nbits - 1;
      dest += dest_offset / 8;
      dest_offset = 7 - dest_offset % 8;
      source_offset += nbits - 1;
      source += source_offset / 8;
      source_offset = 7 - source_offset % 8;
    }
  else
    {
      dest += dest_offset / 8;
      dest_offset %= 8;
      source += source_offset / 8;
      source_offset %= 8;
    }

  /* Fill BUF with DEST_OFFSET bits from the destination and 8 -
     SOURCE_OFFSET bits from the source.  */
  buf = *(bits_big_endian ? source-- : source++) >> source_offset;
  buf <<= dest_offset;
  buf |= *dest & ((1 << dest_offset) - 1);

  /* NBITS: bits yet to be written; AVAIL: BUF's fill level.  */
  nbits += dest_offset;
  avail = dest_offset + 8 - source_offset;

  /* Flush 8 bits from BUF, if appropriate.  */
  if (nbits >= 8 && avail >= 8)
    {
      *(bits_big_endian ? dest-- : dest++) = buf;
      buf >>= 8;
      avail -= 8;
      nbits -= 8;
    }

  /* Copy the middle part.  */
  if (nbits >= 8)
    {
      size_t len = nbits / 8;

      /* Use a faster method for byte-aligned copies.  */
      if (avail == 0)
	{
	  if (bits_big_endian)
	    {
	      dest -= len;
	      source -= len;
	      memcpy (dest + 1, source + 1, len);
	    }
	  else
	    {
	      memcpy (dest, source, len);
	      dest += len;
	      source += len;
	    }
	}
      else
	{
	  while (len--)
	    {
	      buf |= *(bits_big_endian ? source-- : source++) << avail;
	      *(bits_big_endian ? dest-- : dest++) = buf;
	      buf >>= 8;
	    }
	}
      nbits %= 8;
    }

  /* Write the last byte.  */
  if (nbits)
    {
      if (avail < nbits)
	buf |= *source << avail;

      buf &= (1 << nbits) - 1;
      *dest = (*dest & (~0 << nbits)) | buf;
    }
}

#if GDB_SELF_TEST

namespace selftests {

/* Helper function for the unit test of copy_bitwise.  Convert NBITS bits
   out of BITS, starting at OFFS, to the respective '0'/'1'-string.  MSB0
   specifies whether to assume big endian bit numbering.  Store the
   resulting (not null-terminated) string at STR.  */

static void
bits_to_str (char *str, const gdb_byte *bits, ULONGEST offs,
	     ULONGEST nbits, int msb0)
{
  unsigned int j;
  size_t i;

  for (i = offs / 8, j = offs % 8; nbits; i++, j = 0)
    {
      unsigned int ch = bits[i];
      for (; j < 8 && nbits; j++, nbits--)
	*str++ = (ch & (msb0 ? (1 << (7 - j)) : (1 << j))) ? '1' : '0';
    }
}

/* Check one invocation of copy_bitwise with the given parameters.  */

static void
check_copy_bitwise (const gdb_byte *dest, unsigned int dest_offset,
		    const gdb_byte *source, unsigned int source_offset,
		    unsigned int nbits, int msb0)
{
  size_t len = align_up (dest_offset + nbits, 8);
  char *expected = (char *) alloca (len + 1);
  char *actual = (char *) alloca (len + 1);
  gdb_byte *buf = (gdb_byte *) alloca (len / 8);

  /* Compose a '0'/'1'-string that represents the expected result of
     copy_bitwise below:
      Bits from [0, DEST_OFFSET) are filled from DEST.
      Bits from [DEST_OFFSET, DEST_OFFSET + NBITS) are filled from SOURCE.
      Bits from [DEST_OFFSET + NBITS, LEN) are filled from DEST.

     E.g., with:
      dest_offset: 4
      nbits:       2
      len:         8
      dest:        00000000
      source:      11111111

     We should end up with:
      buf:         00001100
                   DDDDSSDD (D=dest, S=source)
  */
  bits_to_str (expected, dest, 0, len, msb0);
  bits_to_str (expected + dest_offset, source, source_offset, nbits, msb0);

  /* Fill BUF with data from DEST, apply copy_bitwise, and convert the
     result to a '0'/'1'-string.  */
  memcpy (buf, dest, len / 8);
  copy_bitwise (buf, dest_offset, source, source_offset, nbits, msb0);
  bits_to_str (actual, buf, 0, len, msb0);

  /* Compare the resulting strings.  */
  expected[len] = actual[len] = '\0';
  if (strcmp (expected, actual) != 0)
    error (_("copy_bitwise %s != %s (%u+%u -> %u)"),
	   expected, actual, source_offset, nbits, dest_offset);
}

/* Unit test for copy_bitwise.  */

static void
copy_bitwise_tests (void)
{
  /* Data to be used as both source and destination buffers.  The two
     arrays below represent the lsb0- and msb0- encoded versions of the
     following bit string, respectively:
       00000000 00011111 11111111 01001000 10100101 11110010
     This pattern is chosen such that it contains:
     - constant 0- and 1- chunks of more than a full byte;
     - 0/1- and 1/0 transitions on all bit positions within a byte;
     - several sufficiently asymmetric bytes.
  */
  static const gdb_byte data_lsb0[] = {
    0x00, 0xf8, 0xff, 0x12, 0xa5, 0x4f
  };
  static const gdb_byte data_msb0[] = {
    0x00, 0x1f, 0xff, 0x48, 0xa5, 0xf2
  };

  constexpr size_t data_nbits = 8 * sizeof (data_lsb0);
  constexpr unsigned max_nbits = 24;

  /* Try all combinations of:
      lsb0/msb0 bit order (using the respective data array)
       X [0, MAX_NBITS] copy bit width
       X feasible source offsets for the given copy bit width
       X feasible destination offsets
  */
  for (int msb0 = 0; msb0 < 2; msb0++)
    {
      const gdb_byte *data = msb0 ? data_msb0 : data_lsb0;

      for (unsigned int nbits = 1; nbits <= max_nbits; nbits++)
	{
	  const unsigned int max_offset = data_nbits - nbits;

	  for (unsigned source_offset = 0;
	       source_offset <= max_offset;
	       source_offset++)
	    {
	      for (unsigned dest_offset = 0;
		   dest_offset <= max_offset;
		   dest_offset++)
		{
		  check_copy_bitwise (data + dest_offset / 8,
				      dest_offset % 8,
				      data + source_offset / 8,
				      source_offset % 8,
				      nbits, msb0);
		}
	    }
	}

      /* Special cases: copy all, copy nothing.  */
      check_copy_bitwise (data_lsb0, 0, data_msb0, 0, data_nbits, msb0);
      check_copy_bitwise (data_msb0, 0, data_lsb0, 0, data_nbits, msb0);
      check_copy_bitwise (data, data_nbits - 7, data, 9, 0, msb0);
    }
}

} /* namespace selftests */

#endif /* GDB_SELF_TEST */

static void
read_pieced_value (struct value *v)
{
  int i;
  long offset = 0;
  ULONGEST bits_to_skip;
  gdb_byte *contents;
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (v);
  size_t type_len;
  size_t buffer_size = 0;
  std::vector<gdb_byte> buffer;
  int bits_big_endian
    = gdbarch_bits_big_endian (get_type_arch (value_type (v)));

  if (value_type (v) != value_enclosing_type (v))
    internal_error (__FILE__, __LINE__,
		    _("Should not be able to create a lazy value with "
		      "an enclosing type"));

  contents = value_contents_raw (v);
  bits_to_skip = 8 * value_offset (v);
  if (value_bitsize (v))
    {
      bits_to_skip += value_bitpos (v);
      type_len = value_bitsize (v);
    }
  else
    type_len = 8 * TYPE_LENGTH (value_type (v));

  for (i = 0; i < c->n_pieces && offset < type_len; i++)
    {
      struct dwarf_expr_piece *p = &c->pieces[i];
      size_t this_size, this_size_bits;
      long dest_offset_bits, source_offset_bits, source_offset;
      const gdb_byte *intermediate_buffer;

      /* Compute size, source, and destination offsets for copying, in
	 bits.  */
      this_size_bits = p->size;
      if (bits_to_skip > 0 && bits_to_skip >= this_size_bits)
	{
	  bits_to_skip -= this_size_bits;
	  continue;
	}
      if (bits_to_skip > 0)
	{
	  dest_offset_bits = 0;
	  source_offset_bits = bits_to_skip;
	  this_size_bits -= bits_to_skip;
	  bits_to_skip = 0;
	}
      else
	{
	  dest_offset_bits = offset;
	  source_offset_bits = 0;
	}
      if (this_size_bits > type_len - offset)
	this_size_bits = type_len - offset;

      this_size = (this_size_bits + source_offset_bits % 8 + 7) / 8;
      source_offset = source_offset_bits / 8;
      if (buffer_size < this_size)
	{
	  buffer_size = this_size;
	  buffer.reserve (buffer_size);
	}
      intermediate_buffer = buffer.data ();

      /* Copy from the source to DEST_BUFFER.  */
      switch (p->location)
	{
	case DWARF_VALUE_REGISTER:
	  {
	    struct frame_info *frame = frame_find_by_id (c->frame_id);
	    struct gdbarch *arch = get_frame_arch (frame);
	    int gdb_regnum = dwarf_reg_to_regnum_or_error (arch, p->v.regno);
	    int optim, unavail;
	    LONGEST reg_offset = source_offset;

	    if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG
		&& this_size < register_size (arch, gdb_regnum))
	      {
		/* Big-endian, and we want less than full size.  */
		reg_offset = register_size (arch, gdb_regnum) - this_size;
		/* We want the lower-order THIS_SIZE_BITS of the bytes
		   we extract from the register.  */
		source_offset_bits += 8 * this_size - this_size_bits;
	      }

	    if (!get_frame_register_bytes (frame, gdb_regnum, reg_offset,
					   this_size, buffer.data (),
					   &optim, &unavail))
	      {
		/* Just so garbage doesn't ever shine through.  */
		memset (buffer.data (), 0, this_size);

		if (optim)
		  mark_value_bits_optimized_out (v, offset, this_size_bits);
		if (unavail)
		  mark_value_bits_unavailable (v, offset, this_size_bits);
	      }
	  }
	  break;

	case DWARF_VALUE_MEMORY:
	  read_value_memory (v, offset,
			     p->v.mem.in_stack_memory,
			     p->v.mem.addr + source_offset,
			     buffer.data (), this_size);
	  break;

	case DWARF_VALUE_STACK:
	  {
	    size_t n = this_size;

	    if (n > c->addr_size - source_offset)
	      n = (c->addr_size >= source_offset
		   ? c->addr_size - source_offset
		   : 0);
	    if (n == 0)
	      {
		/* Nothing.  */
	      }
	    else
	      {
		const gdb_byte *val_bytes = value_contents_all (p->v.value);

		intermediate_buffer = val_bytes + source_offset;
	      }
	  }
	  break;

	case DWARF_VALUE_LITERAL:
	  {
	    size_t n = this_size;

	    if (n > p->v.literal.length - source_offset)
	      n = (p->v.literal.length >= source_offset
		   ? p->v.literal.length - source_offset
		   : 0);
	    if (n != 0)
	      intermediate_buffer = p->v.literal.data + source_offset;
	  }
	  break;

	  /* These bits show up as zeros -- but do not cause the value
	     to be considered optimized-out.  */
	case DWARF_VALUE_IMPLICIT_POINTER:
	  break;

	case DWARF_VALUE_OPTIMIZED_OUT:
	  mark_value_bits_optimized_out (v, offset, this_size_bits);
	  break;

	default:
	  internal_error (__FILE__, __LINE__, _("invalid location type"));
	}

      if (p->location != DWARF_VALUE_OPTIMIZED_OUT
	  && p->location != DWARF_VALUE_IMPLICIT_POINTER)
	copy_bitwise (contents, dest_offset_bits,
		      intermediate_buffer, source_offset_bits % 8,
		      this_size_bits, bits_big_endian);

      offset += this_size_bits;
    }
}

static void
write_pieced_value (struct value *to, struct value *from)
{
  int i;
  long offset = 0;
  ULONGEST bits_to_skip;
  const gdb_byte *contents;
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (to);
  size_t type_len;
  size_t buffer_size = 0;
  std::vector<gdb_byte> buffer;
  int bits_big_endian
    = gdbarch_bits_big_endian (get_type_arch (value_type (to)));

  contents = value_contents (from);
  bits_to_skip = 8 * value_offset (to);
  if (value_bitsize (to))
    {
      bits_to_skip += value_bitpos (to);
      type_len = value_bitsize (to);
    }
  else
    type_len = 8 * TYPE_LENGTH (value_type (to));

  for (i = 0; i < c->n_pieces && offset < type_len; i++)
    {
      struct dwarf_expr_piece *p = &c->pieces[i];
      size_t this_size_bits, this_size;
      long dest_offset_bits, source_offset_bits, dest_offset, source_offset;
      int need_bitwise;
      const gdb_byte *source_buffer;

      this_size_bits = p->size;
      if (bits_to_skip > 0 && bits_to_skip >= this_size_bits)
	{
	  bits_to_skip -= this_size_bits;
	  continue;
	}
      if (this_size_bits > type_len - offset)
	this_size_bits = type_len - offset;
      if (bits_to_skip > 0)
	{
	  dest_offset_bits = bits_to_skip;
	  source_offset_bits = 0;
	  this_size_bits -= bits_to_skip;
	  bits_to_skip = 0;
	}
      else
	{
	  dest_offset_bits = 0;
	  source_offset_bits = offset;
	}

      this_size = (this_size_bits + source_offset_bits % 8 + 7) / 8;
      source_offset = source_offset_bits / 8;
      dest_offset = dest_offset_bits / 8;
      if (dest_offset_bits % 8 == 0 && source_offset_bits % 8 == 0)
	{
	  source_buffer = contents + source_offset;
	  need_bitwise = 0;
	}
      else
	{
	  if (buffer_size < this_size)
	    {
	      buffer_size = this_size;
	      buffer.reserve (buffer_size);
	    }
	  source_buffer = buffer.data ();
	  need_bitwise = 1;
	}

      switch (p->location)
	{
	case DWARF_VALUE_REGISTER:
	  {
	    struct frame_info *frame = frame_find_by_id (c->frame_id);
	    struct gdbarch *arch = get_frame_arch (frame);
	    int gdb_regnum = dwarf_reg_to_regnum_or_error (arch, p->v.regno);
	    int reg_offset = dest_offset;

	    if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG
		&& this_size <= register_size (arch, gdb_regnum))
	      {
		/* Big-endian, and we want less than full size.  */
		reg_offset = register_size (arch, gdb_regnum) - this_size;
	      }

	    if (need_bitwise)
	      {
		int optim, unavail;

		if (!get_frame_register_bytes (frame, gdb_regnum, reg_offset,
					       this_size, buffer.data (),
					       &optim, &unavail))
		  {
		    if (optim)
		      throw_error (OPTIMIZED_OUT_ERROR,
				   _("Can't do read-modify-write to "
				     "update bitfield; containing word "
				     "has been optimized out"));
		    if (unavail)
		      throw_error (NOT_AVAILABLE_ERROR,
				   _("Can't do read-modify-write to update "
				     "bitfield; containing word "
				     "is unavailable"));
		  }
		copy_bitwise (buffer.data (), dest_offset_bits,
			      contents, source_offset_bits,
			      this_size_bits,
			      bits_big_endian);
	      }

	    put_frame_register_bytes (frame, gdb_regnum, reg_offset, 
				      this_size, source_buffer);
	  }
	  break;
	case DWARF_VALUE_MEMORY:
	  if (need_bitwise)
	    {
	      /* Only the first and last bytes can possibly have any
		 bits reused.  */
	      read_memory (p->v.mem.addr + dest_offset, buffer.data (), 1);
	      read_memory (p->v.mem.addr + dest_offset + this_size - 1,
			   &buffer[this_size - 1], 1);
	      copy_bitwise (buffer.data (), dest_offset_bits,
			    contents, source_offset_bits,
			    this_size_bits,
			    bits_big_endian);
	    }

	  write_memory (p->v.mem.addr + dest_offset,
			source_buffer, this_size);
	  break;
	default:
	  mark_value_bytes_optimized_out (to, 0, TYPE_LENGTH (value_type (to)));
	  break;
	}
      offset += this_size_bits;
    }
}

/* An implementation of an lval_funcs method to see whether a value is
   a synthetic pointer.  */

static int
check_pieced_synthetic_pointer (const struct value *value, LONGEST bit_offset,
				int bit_length)
{
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (value);
  int i;

  bit_offset += 8 * value_offset (value);
  if (value_bitsize (value))
    bit_offset += value_bitpos (value);

  for (i = 0; i < c->n_pieces && bit_length > 0; i++)
    {
      struct dwarf_expr_piece *p = &c->pieces[i];
      size_t this_size_bits = p->size;

      if (bit_offset > 0)
	{
	  if (bit_offset >= this_size_bits)
	    {
	      bit_offset -= this_size_bits;
	      continue;
	    }

	  bit_length -= this_size_bits - bit_offset;
	  bit_offset = 0;
	}
      else
	bit_length -= this_size_bits;

      if (p->location != DWARF_VALUE_IMPLICIT_POINTER)
	return 0;
    }

  return 1;
}

/* A wrapper function for get_frame_address_in_block.  */

static CORE_ADDR
get_frame_address_in_block_wrapper (void *baton)
{
  return get_frame_address_in_block ((struct frame_info *) baton);
}

/* Fetch a DW_AT_const_value through a synthetic pointer.  */

static struct value *
fetch_const_value_from_synthetic_pointer (sect_offset die, LONGEST byte_offset,
					  struct dwarf2_per_cu_data *per_cu,
					  struct type *type)
{
  struct value *result = NULL;
  struct obstack temp_obstack;
  struct cleanup *cleanup;
  const gdb_byte *bytes;
  LONGEST len;

  obstack_init (&temp_obstack);
  cleanup = make_cleanup_obstack_free (&temp_obstack);
  bytes = dwarf2_fetch_constant_bytes (die, per_cu, &temp_obstack, &len);

  if (bytes != NULL)
    {
      if (byte_offset >= 0
	  && byte_offset + TYPE_LENGTH (TYPE_TARGET_TYPE (type)) <= len)
	{
	  bytes += byte_offset;
	  result = value_from_contents (TYPE_TARGET_TYPE (type), bytes);
	}
      else
	invalid_synthetic_pointer ();
    }
  else
    result = allocate_optimized_out_value (TYPE_TARGET_TYPE (type));

  do_cleanups (cleanup);

  return result;
}

/* Fetch the value pointed to by a synthetic pointer.  */

static struct value *
indirect_synthetic_pointer (sect_offset die, LONGEST byte_offset,
			    struct dwarf2_per_cu_data *per_cu,
			    struct frame_info *frame, struct type *type)
{
  /* Fetch the location expression of the DIE we're pointing to.  */
  struct dwarf2_locexpr_baton baton
    = dwarf2_fetch_die_loc_sect_off (die, per_cu,
				     get_frame_address_in_block_wrapper, frame);

  /* Get type of pointed-to DIE.  */
  struct type *orig_type = dwarf2_fetch_die_type_sect_off (die, per_cu);
  if (orig_type == NULL)
    invalid_synthetic_pointer ();

  /* If pointed-to DIE has a DW_AT_location, evaluate it and return the
     resulting value.  Otherwise, it may have a DW_AT_const_value instead,
     or it may've been optimized out.  */
  if (baton.data != NULL)
    return dwarf2_evaluate_loc_desc_full (orig_type, frame, baton.data,
					  baton.size, baton.per_cu,
					  TYPE_TARGET_TYPE (type),
					  byte_offset);
  else
    return fetch_const_value_from_synthetic_pointer (die, byte_offset, per_cu,
						     type);
}

/* An implementation of an lval_funcs method to indirect through a
   pointer.  This handles the synthetic pointer case when needed.  */

static struct value *
indirect_pieced_value (struct value *value)
{
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (value);
  struct type *type;
  struct frame_info *frame;
  struct dwarf2_locexpr_baton baton;
  int i, bit_length;
  LONGEST bit_offset;
  struct dwarf_expr_piece *piece = NULL;
  LONGEST byte_offset;
  enum bfd_endian byte_order;

  type = check_typedef (value_type (value));
  if (TYPE_CODE (type) != TYPE_CODE_PTR)
    return NULL;

  bit_length = 8 * TYPE_LENGTH (type);
  bit_offset = 8 * value_offset (value);
  if (value_bitsize (value))
    bit_offset += value_bitpos (value);

  for (i = 0; i < c->n_pieces && bit_length > 0; i++)
    {
      struct dwarf_expr_piece *p = &c->pieces[i];
      size_t this_size_bits = p->size;

      if (bit_offset > 0)
	{
	  if (bit_offset >= this_size_bits)
	    {
	      bit_offset -= this_size_bits;
	      continue;
	    }

	  bit_length -= this_size_bits - bit_offset;
	  bit_offset = 0;
	}
      else
	bit_length -= this_size_bits;

      if (p->location != DWARF_VALUE_IMPLICIT_POINTER)
	return NULL;

      if (bit_length != 0)
	error (_("Invalid use of DW_OP_implicit_pointer"));

      piece = p;
      break;
    }

  gdb_assert (piece != NULL);
  frame = get_selected_frame (_("No frame selected."));

  /* This is an offset requested by GDB, such as value subscripts.
     However, due to how synthetic pointers are implemented, this is
     always presented to us as a pointer type.  This means we have to
     sign-extend it manually as appropriate.  Use raw
     extract_signed_integer directly rather than value_as_address and
     sign extend afterwards on architectures that would need it
     (mostly everywhere except MIPS, which has signed addresses) as
     the later would go through gdbarch_pointer_to_address and thus
     return a CORE_ADDR with high bits set on architectures that
     encode address spaces and other things in CORE_ADDR.  */
  byte_order = gdbarch_byte_order (get_frame_arch (frame));
  byte_offset = extract_signed_integer (value_contents (value),
					TYPE_LENGTH (type), byte_order);
  byte_offset += piece->v.ptr.offset;

  return indirect_synthetic_pointer (piece->v.ptr.die_sect_off,
				     byte_offset, c->per_cu,
				     frame, type);
}

/* Implementation of the coerce_ref method of lval_funcs for synthetic C++
   references.  */

static struct value *
coerce_pieced_ref (const struct value *value)
{
  struct type *type = check_typedef (value_type (value));

  if (value_bits_synthetic_pointer (value, value_embedded_offset (value),
				    TARGET_CHAR_BIT * TYPE_LENGTH (type)))
    {
      const struct piece_closure *closure
	= (struct piece_closure *) value_computed_closure (value);
      struct frame_info *frame
	= get_selected_frame (_("No frame selected."));

      /* gdb represents synthetic pointers as pieced values with a single
	 piece.  */
      gdb_assert (closure != NULL);
      gdb_assert (closure->n_pieces == 1);

      return indirect_synthetic_pointer (closure->pieces->v.ptr.die_sect_off,
					 closure->pieces->v.ptr.offset,
					 closure->per_cu, frame, type);
    }
  else
    {
      /* Else: not a synthetic reference; do nothing.  */
      return NULL;
    }
}

static void *
copy_pieced_value_closure (const struct value *v)
{
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (v);
  
  ++c->refc;
  return c;
}

static void
free_pieced_value_closure (struct value *v)
{
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (v);

  --c->refc;
  if (c->refc == 0)
    {
      int i;

      for (i = 0; i < c->n_pieces; ++i)
	if (c->pieces[i].location == DWARF_VALUE_STACK)
	  value_free (c->pieces[i].v.value);

      xfree (c->pieces);
      xfree (c);
    }
}

/* Functions for accessing a variable described by DW_OP_piece.  */
static const struct lval_funcs pieced_value_funcs = {
  read_pieced_value,
  write_pieced_value,
  indirect_pieced_value,
  coerce_pieced_ref,
  check_pieced_synthetic_pointer,
  copy_pieced_value_closure,
  free_pieced_value_closure
};

/* Evaluate a location description, starting at DATA and with length
   SIZE, to find the current location of variable of TYPE in the
   context of FRAME.  If SUBOBJ_TYPE is non-NULL, return instead the
   location of the subobject of type SUBOBJ_TYPE at byte offset
   SUBOBJ_BYTE_OFFSET within the variable of type TYPE.  */

static struct value *
dwarf2_evaluate_loc_desc_full (struct type *type, struct frame_info *frame,
			       const gdb_byte *data, size_t size,
			       struct dwarf2_per_cu_data *per_cu,
			       struct type *subobj_type,
			       LONGEST subobj_byte_offset)
{
  struct value *retval;
  struct objfile *objfile = dwarf2_per_cu_objfile (per_cu);

  if (subobj_type == NULL)
    {
      subobj_type = type;
      subobj_byte_offset = 0;
    }
  else if (subobj_byte_offset < 0)
    invalid_synthetic_pointer ();

  if (size == 0)
    return allocate_optimized_out_value (subobj_type);

  dwarf_evaluate_loc_desc ctx;
  ctx.frame = frame;
  ctx.per_cu = per_cu;
  ctx.obj_address = 0;

  scoped_value_mark free_values;

  ctx.gdbarch = get_objfile_arch (objfile);
  ctx.addr_size = dwarf2_per_cu_addr_size (per_cu);
  ctx.ref_addr_size = dwarf2_per_cu_ref_addr_size (per_cu);
  ctx.offset = dwarf2_per_cu_text_offset (per_cu);

  TRY
    {
      ctx.eval (data, size);
    }
  CATCH (ex, RETURN_MASK_ERROR)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  free_values.free_to_mark ();
	  retval = allocate_value (subobj_type);
	  mark_value_bytes_unavailable (retval, 0,
					TYPE_LENGTH (subobj_type));
	  return retval;
	}
      else if (ex.error == NO_ENTRY_VALUE_ERROR)
	{
	  if (entry_values_debug)
	    exception_print (gdb_stdout, ex);
	  free_values.free_to_mark ();
	  return allocate_optimized_out_value (subobj_type);
	}
      else
	throw_exception (ex);
    }
  END_CATCH

  if (ctx.num_pieces > 0)
    {
      struct piece_closure *c;
      ULONGEST bit_size = 0;
      int i;

      for (i = 0; i < ctx.num_pieces; ++i)
	bit_size += ctx.pieces[i].size;
      if (8 * (subobj_byte_offset + TYPE_LENGTH (subobj_type)) > bit_size)
	invalid_synthetic_pointer ();

      c = allocate_piece_closure (per_cu, ctx.num_pieces, ctx.pieces,
				  ctx.addr_size, frame);
      /* We must clean up the value chain after creating the piece
	 closure but before allocating the result.  */
      free_values.free_to_mark ();
      retval = allocate_computed_value (subobj_type,
					&pieced_value_funcs, c);
      set_value_offset (retval, subobj_byte_offset);
    }
  else
    {
      switch (ctx.location)
	{
	case DWARF_VALUE_REGISTER:
	  {
	    struct gdbarch *arch = get_frame_arch (frame);
	    int dwarf_regnum
	      = longest_to_int (value_as_long (ctx.fetch (0)));
	    int gdb_regnum = dwarf_reg_to_regnum_or_error (arch, dwarf_regnum);

	    if (subobj_byte_offset != 0)
	      error (_("cannot use offset on synthetic pointer to register"));
	    free_values.free_to_mark ();
	    retval = value_from_register (subobj_type, gdb_regnum, frame);
	    if (value_optimized_out (retval))
	      {
		struct value *tmp;

		/* This means the register has undefined value / was
		   not saved.  As we're computing the location of some
		   variable etc. in the program, not a value for
		   inspecting a register ($pc, $sp, etc.), return a
		   generic optimized out value instead, so that we show
		   <optimized out> instead of <not saved>.  */
		tmp = allocate_value (subobj_type);
		value_contents_copy (tmp, 0, retval, 0,
				     TYPE_LENGTH (subobj_type));
		retval = tmp;
	      }
	  }
	  break;

	case DWARF_VALUE_MEMORY:
	  {
	    struct type *ptr_type;
	    CORE_ADDR address = ctx.fetch_address (0);
	    int in_stack_memory = ctx.fetch_in_stack_memory (0);

	    /* DW_OP_deref_size (and possibly other operations too) may
	       create a pointer instead of an address.  Ideally, the
	       pointer to address conversion would be performed as part
	       of those operations, but the type of the object to
	       which the address refers is not known at the time of
	       the operation.  Therefore, we do the conversion here
	       since the type is readily available.  */

	    switch (TYPE_CODE (subobj_type))
	      {
		case TYPE_CODE_FUNC:
		case TYPE_CODE_METHOD:
		  ptr_type = builtin_type (ctx.gdbarch)->builtin_func_ptr;
		  break;
		default:
		  ptr_type = builtin_type (ctx.gdbarch)->builtin_data_ptr;
		  break;
	      }
	    address = value_as_address (value_from_pointer (ptr_type, address));

	    free_values.free_to_mark ();
	    retval = value_at_lazy (subobj_type,
				    address + subobj_byte_offset);
	    if (in_stack_memory)
	      set_value_stack (retval, 1);
	  }
	  break;

	case DWARF_VALUE_STACK:
	  {
	    struct value *value = ctx.fetch (0);
	    size_t n = TYPE_LENGTH (value_type (value));
	    size_t len = TYPE_LENGTH (subobj_type);
	    size_t max = TYPE_LENGTH (type);
	    struct gdbarch *objfile_gdbarch = get_objfile_arch (objfile);
	    struct cleanup *cleanup;

	    if (subobj_byte_offset + len > max)
	      invalid_synthetic_pointer ();

	    /* Preserve VALUE because we are going to free values back
	       to the mark, but we still need the value contents
	       below.  */
	    value_incref (value);
	    free_values.free_to_mark ();
	    cleanup = make_cleanup_value_free (value);

	    retval = allocate_value (subobj_type);

	    /* The given offset is relative to the actual object.  */
	    if (gdbarch_byte_order (objfile_gdbarch) == BFD_ENDIAN_BIG)
	      subobj_byte_offset += n - max;

	    memcpy (value_contents_raw (retval),
		    value_contents_all (value) + subobj_byte_offset, len);

	    do_cleanups (cleanup);
	  }
	  break;

	case DWARF_VALUE_LITERAL:
	  {
	    bfd_byte *contents;
	    size_t n = TYPE_LENGTH (subobj_type);

	    if (subobj_byte_offset + n > ctx.len)
	      invalid_synthetic_pointer ();

	    free_values.free_to_mark ();
	    retval = allocate_value (subobj_type);
	    contents = value_contents_raw (retval);
	    memcpy (contents, ctx.data + subobj_byte_offset, n);
	  }
	  break;

	case DWARF_VALUE_OPTIMIZED_OUT:
	  free_values.free_to_mark ();
	  retval = allocate_optimized_out_value (subobj_type);
	  break;

	  /* DWARF_VALUE_IMPLICIT_POINTER was converted to a pieced
	     operation by execute_stack_op.  */
	case DWARF_VALUE_IMPLICIT_POINTER:
	  /* DWARF_VALUE_OPTIMIZED_OUT can't occur in this context --
	     it can only be encountered when making a piece.  */
	default:
	  internal_error (__FILE__, __LINE__, _("invalid location type"));
	}
    }

  set_value_initialized (retval, ctx.initialized);

  return retval;
}

/* The exported interface to dwarf2_evaluate_loc_desc_full; it always
   passes 0 as the byte_offset.  */

struct value *
dwarf2_evaluate_loc_desc (struct type *type, struct frame_info *frame,
			  const gdb_byte *data, size_t size,
			  struct dwarf2_per_cu_data *per_cu)
{
  return dwarf2_evaluate_loc_desc_full (type, frame, data, size, per_cu,
					NULL, 0);
}

/* Evaluates a dwarf expression and stores the result in VAL, expecting
   that the dwarf expression only produces a single CORE_ADDR.  FRAME is the
   frame in which the expression is evaluated.  ADDR is a context (location of
   a variable) and might be needed to evaluate the location expression.
   Returns 1 on success, 0 otherwise.   */

static int
dwarf2_locexpr_baton_eval (const struct dwarf2_locexpr_baton *dlbaton,
			   struct frame_info *frame,
			   CORE_ADDR addr,
			   CORE_ADDR *valp)
{
  struct objfile *objfile;

  if (dlbaton == NULL || dlbaton->size == 0)
    return 0;

  dwarf_evaluate_loc_desc ctx;

  ctx.frame = frame;
  ctx.per_cu = dlbaton->per_cu;
  ctx.obj_address = addr;

  objfile = dwarf2_per_cu_objfile (dlbaton->per_cu);

  ctx.gdbarch = get_objfile_arch (objfile);
  ctx.addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);
  ctx.ref_addr_size = dwarf2_per_cu_ref_addr_size (dlbaton->per_cu);
  ctx.offset = dwarf2_per_cu_text_offset (dlbaton->per_cu);

  ctx.eval (dlbaton->data, dlbaton->size);

  switch (ctx.location)
    {
    case DWARF_VALUE_REGISTER:
    case DWARF_VALUE_MEMORY:
    case DWARF_VALUE_STACK:
      *valp = ctx.fetch_address (0);
      if (ctx.location == DWARF_VALUE_REGISTER)
	*valp = ctx.read_addr_from_reg (*valp);
      return 1;
    case DWARF_VALUE_LITERAL:
      *valp = extract_signed_integer (ctx.data, ctx.len,
				      gdbarch_byte_order (ctx.gdbarch));
      return 1;
      /* Unsupported dwarf values.  */
    case DWARF_VALUE_OPTIMIZED_OUT:
    case DWARF_VALUE_IMPLICIT_POINTER:
      break;
    }

  return 0;
}

/* See dwarf2loc.h.  */

int
dwarf2_evaluate_property (const struct dynamic_prop *prop,
			  struct frame_info *frame,
			  struct property_addr_info *addr_stack,
			  CORE_ADDR *value)
{
  if (prop == NULL)
    return 0;

  if (frame == NULL && has_stack_frames ())
    frame = get_selected_frame (NULL);

  switch (prop->kind)
    {
    case PROP_LOCEXPR:
      {
	const struct dwarf2_property_baton *baton
	  = (const struct dwarf2_property_baton *) prop->data.baton;

	if (dwarf2_locexpr_baton_eval (&baton->locexpr, frame,
				       addr_stack ? addr_stack->addr : 0,
				       value))
	  {
	    if (baton->referenced_type)
	      {
		struct value *val = value_at (baton->referenced_type, *value);

		*value = value_as_address (val);
	      }
	    return 1;
	  }
      }
      break;

    case PROP_LOCLIST:
      {
	struct dwarf2_property_baton *baton
	  = (struct dwarf2_property_baton *) prop->data.baton;
	CORE_ADDR pc = get_frame_address_in_block (frame);
	const gdb_byte *data;
	struct value *val;
	size_t size;

	data = dwarf2_find_location_expression (&baton->loclist, &size, pc);
	if (data != NULL)
	  {
	    val = dwarf2_evaluate_loc_desc (baton->referenced_type, frame, data,
					    size, baton->loclist.per_cu);
	    if (!value_optimized_out (val))
	      {
		*value = value_as_address (val);
		return 1;
	      }
	  }
      }
      break;

    case PROP_CONST:
      *value = prop->data.const_val;
      return 1;

    case PROP_ADDR_OFFSET:
      {
	struct dwarf2_property_baton *baton
	  = (struct dwarf2_property_baton *) prop->data.baton;
	struct property_addr_info *pinfo;
	struct value *val;

	for (pinfo = addr_stack; pinfo != NULL; pinfo = pinfo->next)
	  if (pinfo->type == baton->referenced_type)
	    break;
	if (pinfo == NULL)
	  error (_("cannot find reference address for offset property"));
	if (pinfo->valaddr != NULL)
	  val = value_from_contents
		  (baton->offset_info.type,
		   pinfo->valaddr + baton->offset_info.offset);
	else
	  val = value_at (baton->offset_info.type,
			  pinfo->addr + baton->offset_info.offset);
	*value = value_as_address (val);
	return 1;
      }
    }

  return 0;
}

/* See dwarf2loc.h.  */

void
dwarf2_compile_property_to_c (string_file &stream,
			      const char *result_name,
			      struct gdbarch *gdbarch,
			      unsigned char *registers_used,
			      const struct dynamic_prop *prop,
			      CORE_ADDR pc,
			      struct symbol *sym)
{
  struct dwarf2_property_baton *baton
    = (struct dwarf2_property_baton *) prop->data.baton;
  const gdb_byte *data;
  size_t size;
  struct dwarf2_per_cu_data *per_cu;

  if (prop->kind == PROP_LOCEXPR)
    {
      data = baton->locexpr.data;
      size = baton->locexpr.size;
      per_cu = baton->locexpr.per_cu;
    }
  else
    {
      gdb_assert (prop->kind == PROP_LOCLIST);

      data = dwarf2_find_location_expression (&baton->loclist, &size, pc);
      per_cu = baton->loclist.per_cu;
    }

  compile_dwarf_bounds_to_c (stream, result_name, prop, sym, pc,
			     gdbarch, registers_used,
			     dwarf2_per_cu_addr_size (per_cu),
			     data, data + size, per_cu);
}


/* Helper functions and baton for dwarf2_loc_desc_get_symbol_read_needs.  */

class symbol_needs_eval_context : public dwarf_expr_context
{
 public:

  enum symbol_needs_kind needs;
  struct dwarf2_per_cu_data *per_cu;

  /* Reads from registers do require a frame.  */
  CORE_ADDR read_addr_from_reg (int regnum) OVERRIDE
  {
    needs = SYMBOL_NEEDS_FRAME;
    return 1;
  }

  /* "get_reg_value" callback: Reads from registers do require a
     frame.  */

  struct value *get_reg_value (struct type *type, int regnum) OVERRIDE
  {
    needs = SYMBOL_NEEDS_FRAME;
    return value_zero (type, not_lval);
  }

  /* Reads from memory do not require a frame.  */
  void read_mem (gdb_byte *buf, CORE_ADDR addr, size_t len) OVERRIDE
  {
    memset (buf, 0, len);
  }

  /* Frame-relative accesses do require a frame.  */
  void get_frame_base (const gdb_byte **start, size_t *length) OVERRIDE
  {
    static gdb_byte lit0 = DW_OP_lit0;

    *start = &lit0;
    *length = 1;

    needs = SYMBOL_NEEDS_FRAME;
  }

  /* CFA accesses require a frame.  */
  CORE_ADDR get_frame_cfa () OVERRIDE
  {
    needs = SYMBOL_NEEDS_FRAME;
    return 1;
  }

  CORE_ADDR get_frame_pc () OVERRIDE
  {
    needs = SYMBOL_NEEDS_FRAME;
    return 1;
  }

  /* Thread-local accesses require registers, but not a frame.  */
  CORE_ADDR get_tls_address (CORE_ADDR offset) OVERRIDE
  {
    if (needs <= SYMBOL_NEEDS_REGISTERS)
      needs = SYMBOL_NEEDS_REGISTERS;
    return 1;
  }

  /* Helper interface of per_cu_dwarf_call for
     dwarf2_loc_desc_get_symbol_read_needs.  */

  void dwarf_call (cu_offset die_offset) OVERRIDE
  {
    per_cu_dwarf_call (this, die_offset, per_cu);
  }

  /* DW_OP_entry_value accesses require a caller, therefore a
     frame.  */

  void push_dwarf_reg_entry_value (enum call_site_parameter_kind kind,
				   union call_site_parameter_u kind_u,
				   int deref_size) OVERRIDE
  {
    needs = SYMBOL_NEEDS_FRAME;

    /* The expression may require some stub values on DWARF stack.  */
    push_address (0, 0);
  }

  /* DW_OP_GNU_addr_index doesn't require a frame.  */

   CORE_ADDR get_addr_index (unsigned int index) OVERRIDE
   {
     /* Nothing to do.  */
     return 1;
   }

   /* DW_OP_push_object_address has a frame already passed through.  */

   CORE_ADDR get_object_address () OVERRIDE
   {
     /* Nothing to do.  */
     return 1;
   }
};

/* Compute the correct symbol_needs_kind value for the location
   expression at DATA (length SIZE).  */

static enum symbol_needs_kind
dwarf2_loc_desc_get_symbol_read_needs (const gdb_byte *data, size_t size,
				       struct dwarf2_per_cu_data *per_cu)
{
  int in_reg;
  struct objfile *objfile = dwarf2_per_cu_objfile (per_cu);

  scoped_value_mark free_values;

  symbol_needs_eval_context ctx;

  ctx.needs = SYMBOL_NEEDS_NONE;
  ctx.per_cu = per_cu;
  ctx.gdbarch = get_objfile_arch (objfile);
  ctx.addr_size = dwarf2_per_cu_addr_size (per_cu);
  ctx.ref_addr_size = dwarf2_per_cu_ref_addr_size (per_cu);
  ctx.offset = dwarf2_per_cu_text_offset (per_cu);

  ctx.eval (data, size);

  in_reg = ctx.location == DWARF_VALUE_REGISTER;

  if (ctx.num_pieces > 0)
    {
      int i;

      /* If the location has several pieces, and any of them are in
         registers, then we will need a frame to fetch them from.  */
      for (i = 0; i < ctx.num_pieces; i++)
        if (ctx.pieces[i].location == DWARF_VALUE_REGISTER)
          in_reg = 1;
    }

  if (in_reg)
    ctx.needs = SYMBOL_NEEDS_FRAME;
  return ctx.needs;
}

/* A helper function that throws an unimplemented error mentioning a
   given DWARF operator.  */

static void
unimplemented (unsigned int op)
{
  const char *name = get_DW_OP_name (op);

  if (name)
    error (_("DWARF operator %s cannot be translated to an agent expression"),
	   name);
  else
    error (_("Unknown DWARF operator 0x%02x cannot be translated "
	     "to an agent expression"),
	   op);
}

/* See dwarf2loc.h.

   This is basically a wrapper on gdbarch_dwarf2_reg_to_regnum so that we
   can issue a complaint, which is better than having every target's
   implementation of dwarf2_reg_to_regnum do it.  */

int
dwarf_reg_to_regnum (struct gdbarch *arch, int dwarf_reg)
{
  int reg = gdbarch_dwarf2_reg_to_regnum (arch, dwarf_reg);

  if (reg == -1)
    {
      complaint (&symfile_complaints,
		 _("bad DWARF register number %d"), dwarf_reg);
    }
  return reg;
}

/* Subroutine of dwarf_reg_to_regnum_or_error to simplify it.
   Throw an error because DWARF_REG is bad.  */

static void
throw_bad_regnum_error (ULONGEST dwarf_reg)
{
  /* Still want to print -1 as "-1".
     We *could* have int and ULONGEST versions of dwarf2_reg_to_regnum_or_error
     but that's overkill for now.  */
  if ((int) dwarf_reg == dwarf_reg)
    error (_("Unable to access DWARF register number %d"), (int) dwarf_reg);
  error (_("Unable to access DWARF register number %s"),
	 pulongest (dwarf_reg));
}

/* See dwarf2loc.h.  */

int
dwarf_reg_to_regnum_or_error (struct gdbarch *arch, ULONGEST dwarf_reg)
{
  int reg;

  if (dwarf_reg > INT_MAX)
    throw_bad_regnum_error (dwarf_reg);
  /* Yes, we will end up issuing a complaint and an error if DWARF_REG is
     bad, but that's ok.  */
  reg = dwarf_reg_to_regnum (arch, (int) dwarf_reg);
  if (reg == -1)
    throw_bad_regnum_error (dwarf_reg);
  return reg;
}

/* A helper function that emits an access to memory.  ARCH is the
   target architecture.  EXPR is the expression which we are building.
   NBITS is the number of bits we want to read.  This emits the
   opcodes needed to read the memory and then extract the desired
   bits.  */

static void
access_memory (struct gdbarch *arch, struct agent_expr *expr, ULONGEST nbits)
{
  ULONGEST nbytes = (nbits + 7) / 8;

  gdb_assert (nbytes > 0 && nbytes <= sizeof (LONGEST));

  if (expr->tracing)
    ax_trace_quick (expr, nbytes);

  if (nbits <= 8)
    ax_simple (expr, aop_ref8);
  else if (nbits <= 16)
    ax_simple (expr, aop_ref16);
  else if (nbits <= 32)
    ax_simple (expr, aop_ref32);
  else
    ax_simple (expr, aop_ref64);

  /* If we read exactly the number of bytes we wanted, we're done.  */
  if (8 * nbytes == nbits)
    return;

  if (gdbarch_bits_big_endian (arch))
    {
      /* On a bits-big-endian machine, we want the high-order
	 NBITS.  */
      ax_const_l (expr, 8 * nbytes - nbits);
      ax_simple (expr, aop_rsh_unsigned);
    }
  else
    {
      /* On a bits-little-endian box, we want the low-order NBITS.  */
      ax_zero_ext (expr, nbits);
    }
}

/* A helper function to return the frame's PC.  */

static CORE_ADDR
get_ax_pc (void *baton)
{
  struct agent_expr *expr = (struct agent_expr *) baton;

  return expr->scope;
}

/* Compile a DWARF location expression to an agent expression.
   
   EXPR is the agent expression we are building.
   LOC is the agent value we modify.
   ARCH is the architecture.
   ADDR_SIZE is the size of addresses, in bytes.
   OP_PTR is the start of the location expression.
   OP_END is one past the last byte of the location expression.
   
   This will throw an exception for various kinds of errors -- for
   example, if the expression cannot be compiled, or if the expression
   is invalid.  */

void
dwarf2_compile_expr_to_ax (struct agent_expr *expr, struct axs_value *loc,
			   struct gdbarch *arch, unsigned int addr_size,
			   const gdb_byte *op_ptr, const gdb_byte *op_end,
			   struct dwarf2_per_cu_data *per_cu)
{
  int i;
  std::vector<int> dw_labels, patches;
  const gdb_byte * const base = op_ptr;
  const gdb_byte *previous_piece = op_ptr;
  enum bfd_endian byte_order = gdbarch_byte_order (arch);
  ULONGEST bits_collected = 0;
  unsigned int addr_size_bits = 8 * addr_size;
  int bits_big_endian = gdbarch_bits_big_endian (arch);

  std::vector<int> offsets (op_end - op_ptr, -1);

  /* By default we are making an address.  */
  loc->kind = axs_lvalue_memory;

  while (op_ptr < op_end)
    {
      enum dwarf_location_atom op = (enum dwarf_location_atom) *op_ptr;
      uint64_t uoffset, reg;
      int64_t offset;
      int i;

      offsets[op_ptr - base] = expr->len;
      ++op_ptr;

      /* Our basic approach to code generation is to map DWARF
	 operations directly to AX operations.  However, there are
	 some differences.

	 First, DWARF works on address-sized units, but AX always uses
	 LONGEST.  For most operations we simply ignore this
	 difference; instead we generate sign extensions as needed
	 before division and comparison operations.  It would be nice
	 to omit the sign extensions, but there is no way to determine
	 the size of the target's LONGEST.  (This code uses the size
	 of the host LONGEST in some cases -- that is a bug but it is
	 difficult to fix.)

	 Second, some DWARF operations cannot be translated to AX.
	 For these we simply fail.  See
	 http://sourceware.org/bugzilla/show_bug.cgi?id=11662.  */
      switch (op)
	{
	case DW_OP_lit0:
	case DW_OP_lit1:
	case DW_OP_lit2:
	case DW_OP_lit3:
	case DW_OP_lit4:
	case DW_OP_lit5:
	case DW_OP_lit6:
	case DW_OP_lit7:
	case DW_OP_lit8:
	case DW_OP_lit9:
	case DW_OP_lit10:
	case DW_OP_lit11:
	case DW_OP_lit12:
	case DW_OP_lit13:
	case DW_OP_lit14:
	case DW_OP_lit15:
	case DW_OP_lit16:
	case DW_OP_lit17:
	case DW_OP_lit18:
	case DW_OP_lit19:
	case DW_OP_lit20:
	case DW_OP_lit21:
	case DW_OP_lit22:
	case DW_OP_lit23:
	case DW_OP_lit24:
	case DW_OP_lit25:
	case DW_OP_lit26:
	case DW_OP_lit27:
	case DW_OP_lit28:
	case DW_OP_lit29:
	case DW_OP_lit30:
	case DW_OP_lit31:
	  ax_const_l (expr, op - DW_OP_lit0);
	  break;

	case DW_OP_addr:
	  uoffset = extract_unsigned_integer (op_ptr, addr_size, byte_order);
	  op_ptr += addr_size;
	  /* Some versions of GCC emit DW_OP_addr before
	     DW_OP_GNU_push_tls_address.  In this case the value is an
	     index, not an address.  We don't support things like
	     branching between the address and the TLS op.  */
	  if (op_ptr >= op_end || *op_ptr != DW_OP_GNU_push_tls_address)
	    uoffset += dwarf2_per_cu_text_offset (per_cu);
	  ax_const_l (expr, uoffset);
	  break;

	case DW_OP_const1u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 1, byte_order));
	  op_ptr += 1;
	  break;
	case DW_OP_const1s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 1, byte_order));
	  op_ptr += 1;
	  break;
	case DW_OP_const2u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 2, byte_order));
	  op_ptr += 2;
	  break;
	case DW_OP_const2s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 2, byte_order));
	  op_ptr += 2;
	  break;
	case DW_OP_const4u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 4, byte_order));
	  op_ptr += 4;
	  break;
	case DW_OP_const4s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 4, byte_order));
	  op_ptr += 4;
	  break;
	case DW_OP_const8u:
	  ax_const_l (expr, extract_unsigned_integer (op_ptr, 8, byte_order));
	  op_ptr += 8;
	  break;
	case DW_OP_const8s:
	  ax_const_l (expr, extract_signed_integer (op_ptr, 8, byte_order));
	  op_ptr += 8;
	  break;
	case DW_OP_constu:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &uoffset);
	  ax_const_l (expr, uoffset);
	  break;
	case DW_OP_consts:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  ax_const_l (expr, offset);
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  loc->u.reg = dwarf_reg_to_regnum_or_error (arch, op - DW_OP_reg0);
	  loc->kind = axs_lvalue_register;
	  break;

	case DW_OP_regx:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  loc->u.reg = dwarf_reg_to_regnum_or_error (arch, reg);
	  loc->kind = axs_lvalue_register;
	  break;

	case DW_OP_implicit_value:
	  {
	    uint64_t len;

	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &len);
	    if (op_ptr + len > op_end)
	      error (_("DW_OP_implicit_value: too few bytes available."));
	    if (len > sizeof (ULONGEST))
	      error (_("Cannot translate DW_OP_implicit_value of %d bytes"),
		     (int) len);

	    ax_const_l (expr, extract_unsigned_integer (op_ptr, len,
							byte_order));
	    op_ptr += len;
	    dwarf_expr_require_composition (op_ptr, op_end,
					    "DW_OP_implicit_value");

	    loc->kind = axs_rvalue;
	  }
	  break;

	case DW_OP_stack_value:
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_stack_value");
	  loc->kind = axs_rvalue;
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	  i = dwarf_reg_to_regnum_or_error (arch, op - DW_OP_breg0);
	  ax_reg (expr, i);
	  if (offset != 0)
	    {
	      ax_const_l (expr, offset);
	      ax_simple (expr, aop_add);
	    }
	  break;
	case DW_OP_bregx:
	  {
	    op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	    i = dwarf_reg_to_regnum_or_error (arch, reg);
	    ax_reg (expr, i);
	    if (offset != 0)
	      {
		ax_const_l (expr, offset);
		ax_simple (expr, aop_add);
	      }
	  }
	  break;
	case DW_OP_fbreg:
	  {
	    const gdb_byte *datastart;
	    size_t datalen;
	    const struct block *b;
	    struct symbol *framefunc;

	    b = block_for_pc (expr->scope);

	    if (!b)
	      error (_("No block found for address"));

	    framefunc = block_linkage_function (b);

	    if (!framefunc)
	      error (_("No function found for block"));

	    func_get_frame_base_dwarf_block (framefunc, expr->scope,
					     &datastart, &datalen);

	    op_ptr = safe_read_sleb128 (op_ptr, op_end, &offset);
	    dwarf2_compile_expr_to_ax (expr, loc, arch, addr_size, datastart,
				       datastart + datalen, per_cu);
	    if (loc->kind == axs_lvalue_register)
	      require_rvalue (expr, loc);

	    if (offset != 0)
	      {
		ax_const_l (expr, offset);
		ax_simple (expr, aop_add);
	      }

	    loc->kind = axs_lvalue_memory;
	  }
	  break;

	case DW_OP_dup:
	  ax_simple (expr, aop_dup);
	  break;

	case DW_OP_drop:
	  ax_simple (expr, aop_pop);
	  break;

	case DW_OP_pick:
	  offset = *op_ptr++;
	  ax_pick (expr, offset);
	  break;
	  
	case DW_OP_swap:
	  ax_simple (expr, aop_swap);
	  break;

	case DW_OP_over:
	  ax_pick (expr, 1);
	  break;

	case DW_OP_rot:
	  ax_simple (expr, aop_rot);
	  break;

	case DW_OP_deref:
	case DW_OP_deref_size:
	  {
	    int size;

	    if (op == DW_OP_deref_size)
	      size = *op_ptr++;
	    else
	      size = addr_size;

	    if (size != 1 && size != 2 && size != 4 && size != 8)
	      error (_("Unsupported size %d in %s"),
		     size, get_DW_OP_name (op));
	    access_memory (arch, expr, size * TARGET_CHAR_BIT);
	  }
	  break;

	case DW_OP_abs:
	  /* Sign extend the operand.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_dup);
	  ax_const_l (expr, 0);
	  ax_simple (expr, aop_less_signed);
	  ax_simple (expr, aop_log_not);
	  i = ax_goto (expr, aop_if_goto);
	  /* We have to emit 0 - X.  */
	  ax_const_l (expr, 0);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_sub);
	  ax_label (expr, i, expr->len);
	  break;

	case DW_OP_neg:
	  /* No need to sign extend here.  */
	  ax_const_l (expr, 0);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_sub);
	  break;

	case DW_OP_not:
	  /* Sign extend the operand.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_bit_not);
	  break;

	case DW_OP_plus_uconst:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  /* It would be really weird to emit `DW_OP_plus_uconst 0',
	     but we micro-optimize anyhow.  */
	  if (reg != 0)
	    {
	      ax_const_l (expr, reg);
	      ax_simple (expr, aop_add);
	    }
	  break;

	case DW_OP_and:
	  ax_simple (expr, aop_bit_and);
	  break;

	case DW_OP_div:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_div_signed);
	  break;

	case DW_OP_minus:
	  ax_simple (expr, aop_sub);
	  break;

	case DW_OP_mod:
	  ax_simple (expr, aop_rem_unsigned);
	  break;

	case DW_OP_mul:
	  ax_simple (expr, aop_mul);
	  break;

	case DW_OP_or:
	  ax_simple (expr, aop_bit_or);
	  break;

	case DW_OP_plus:
	  ax_simple (expr, aop_add);
	  break;

	case DW_OP_shl:
	  ax_simple (expr, aop_lsh);
	  break;

	case DW_OP_shr:
	  ax_simple (expr, aop_rsh_unsigned);
	  break;

	case DW_OP_shra:
	  ax_simple (expr, aop_rsh_signed);
	  break;

	case DW_OP_xor:
	  ax_simple (expr, aop_bit_xor);
	  break;

	case DW_OP_le:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* Note no swap here: A <= B is !(B < A).  */
	  ax_simple (expr, aop_less_signed);
	  ax_simple (expr, aop_log_not);
	  break;

	case DW_OP_ge:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  /* A >= B is !(A < B).  */
	  ax_simple (expr, aop_less_signed);
	  ax_simple (expr, aop_log_not);
	  break;

	case DW_OP_eq:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* No need for a second swap here.  */
	  ax_simple (expr, aop_equal);
	  break;

	case DW_OP_lt:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_simple (expr, aop_less_signed);
	  break;

	case DW_OP_gt:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* Note no swap here: A > B is B < A.  */
	  ax_simple (expr, aop_less_signed);
	  break;

	case DW_OP_ne:
	  /* Sign extend the operands.  */
	  ax_ext (expr, addr_size_bits);
	  ax_simple (expr, aop_swap);
	  ax_ext (expr, addr_size_bits);
	  /* No need for a swap here.  */
	  ax_simple (expr, aop_equal);
	  ax_simple (expr, aop_log_not);
	  break;

	case DW_OP_call_frame_cfa:
	  {
	    int regnum;
	    CORE_ADDR text_offset;
	    LONGEST off;
	    const gdb_byte *cfa_start, *cfa_end;

	    if (dwarf2_fetch_cfa_info (arch, expr->scope, per_cu,
				       &regnum, &off,
				       &text_offset, &cfa_start, &cfa_end))
	      {
		/* Register.  */
		ax_reg (expr, regnum);
		if (off != 0)
		  {
		    ax_const_l (expr, off);
		    ax_simple (expr, aop_add);
		  }
	      }
	    else
	      {
		/* Another expression.  */
		ax_const_l (expr, text_offset);
		dwarf2_compile_expr_to_ax (expr, loc, arch, addr_size,
					   cfa_start, cfa_end, per_cu);
	      }

	    loc->kind = axs_lvalue_memory;
	  }
	  break;

	case DW_OP_GNU_push_tls_address:
	case DW_OP_form_tls_address:
	  unimplemented (op);
	  break;

	case DW_OP_push_object_address:
	  unimplemented (op);
	  break;

	case DW_OP_skip:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  i = ax_goto (expr, aop_goto);
	  dw_labels.push_back (op_ptr + offset - base);
	  patches.push_back (i);
	  break;

	case DW_OP_bra:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  /* Zero extend the operand.  */
	  ax_zero_ext (expr, addr_size_bits);
	  i = ax_goto (expr, aop_if_goto);
	  dw_labels.push_back (op_ptr + offset - base);
	  patches.push_back (i);
	  break;

	case DW_OP_nop:
	  break;

        case DW_OP_piece:
	case DW_OP_bit_piece:
	  {
	    uint64_t size, offset;

	    if (op_ptr - 1 == previous_piece)
	      error (_("Cannot translate empty pieces to agent expressions"));
	    previous_piece = op_ptr - 1;

            op_ptr = safe_read_uleb128 (op_ptr, op_end, &size);
	    if (op == DW_OP_piece)
	      {
		size *= 8;
		offset = 0;
	      }
	    else
	      op_ptr = safe_read_uleb128 (op_ptr, op_end, &offset);

	    if (bits_collected + size > 8 * sizeof (LONGEST))
	      error (_("Expression pieces exceed word size"));

	    /* Access the bits.  */
	    switch (loc->kind)
	      {
	      case axs_lvalue_register:
		ax_reg (expr, loc->u.reg);
		break;

	      case axs_lvalue_memory:
		/* Offset the pointer, if needed.  */
		if (offset > 8)
		  {
		    ax_const_l (expr, offset / 8);
		    ax_simple (expr, aop_add);
		    offset %= 8;
		  }
		access_memory (arch, expr, size);
		break;
	      }

	    /* For a bits-big-endian target, shift up what we already
	       have.  For a bits-little-endian target, shift up the
	       new data.  Note that there is a potential bug here if
	       the DWARF expression leaves multiple values on the
	       stack.  */
	    if (bits_collected > 0)
	      {
		if (bits_big_endian)
		  {
		    ax_simple (expr, aop_swap);
		    ax_const_l (expr, size);
		    ax_simple (expr, aop_lsh);
		    /* We don't need a second swap here, because
		       aop_bit_or is symmetric.  */
		  }
		else
		  {
		    ax_const_l (expr, size);
		    ax_simple (expr, aop_lsh);
		  }
		ax_simple (expr, aop_bit_or);
	      }

	    bits_collected += size;
	    loc->kind = axs_rvalue;
	  }
	  break;

	case DW_OP_GNU_uninit:
	  unimplemented (op);

	case DW_OP_call2:
	case DW_OP_call4:
	  {
	    struct dwarf2_locexpr_baton block;
	    int size = (op == DW_OP_call2 ? 2 : 4);

	    uoffset = extract_unsigned_integer (op_ptr, size, byte_order);
	    op_ptr += size;

	    cu_offset offset = (cu_offset) uoffset;
	    block = dwarf2_fetch_die_loc_cu_off (offset, per_cu,
						 get_ax_pc, expr);

	    /* DW_OP_call_ref is currently not supported.  */
	    gdb_assert (block.per_cu == per_cu);

	    dwarf2_compile_expr_to_ax (expr, loc, arch, addr_size,
				       block.data, block.data + block.size,
				       per_cu);
	  }
	  break;

	case DW_OP_call_ref:
	  unimplemented (op);

	default:
	  unimplemented (op);
	}
    }

  /* Patch all the branches we emitted.  */
  for (i = 0; i < patches.size (); ++i)
    {
      int targ = offsets[dw_labels[i]];
      if (targ == -1)
	internal_error (__FILE__, __LINE__, _("invalid label"));
      ax_label (expr, patches[i], targ);
    }
}


/* Return the value of SYMBOL in FRAME using the DWARF-2 expression
   evaluator to calculate the location.  */
static struct value *
locexpr_read_variable (struct symbol *symbol, struct frame_info *frame)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);
  struct value *val;

  val = dwarf2_evaluate_loc_desc (SYMBOL_TYPE (symbol), frame, dlbaton->data,
				  dlbaton->size, dlbaton->per_cu);

  return val;
}

/* Return the value of SYMBOL in FRAME at (callee) FRAME's function
   entry.  SYMBOL should be a function parameter, otherwise NO_ENTRY_VALUE_ERROR
   will be thrown.  */

static struct value *
locexpr_read_variable_at_entry (struct symbol *symbol, struct frame_info *frame)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);

  return value_of_dwarf_block_entry (SYMBOL_TYPE (symbol), frame, dlbaton->data,
				     dlbaton->size);
}

/* Implementation of get_symbol_read_needs from
   symbol_computed_ops.  */

static enum symbol_needs_kind
locexpr_get_symbol_read_needs (struct symbol *symbol)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);

  return dwarf2_loc_desc_get_symbol_read_needs (dlbaton->data, dlbaton->size,
						dlbaton->per_cu);
}

/* Return true if DATA points to the end of a piece.  END is one past
   the last byte in the expression.  */

static int
piece_end_p (const gdb_byte *data, const gdb_byte *end)
{
  return data == end || data[0] == DW_OP_piece || data[0] == DW_OP_bit_piece;
}

/* Helper for locexpr_describe_location_piece that finds the name of a
   DWARF register.  */

static const char *
locexpr_regname (struct gdbarch *gdbarch, int dwarf_regnum)
{
  int regnum;

  /* This doesn't use dwarf_reg_to_regnum_or_error on purpose.
     We'd rather print *something* here than throw an error.  */
  regnum = dwarf_reg_to_regnum (gdbarch, dwarf_regnum);
  /* gdbarch_register_name may just return "", return something more
     descriptive for bad register numbers.  */
  if (regnum == -1)
    {
      /* The text is output as "$bad_register_number".
	 That is why we use the underscores.  */
      return _("bad_register_number");
    }
  return gdbarch_register_name (gdbarch, regnum);
}

/* Nicely describe a single piece of a location, returning an updated
   position in the bytecode sequence.  This function cannot recognize
   all locations; if a location is not recognized, it simply returns
   DATA.  If there is an error during reading, e.g. we run off the end
   of the buffer, an error is thrown.  */

static const gdb_byte *
locexpr_describe_location_piece (struct symbol *symbol, struct ui_file *stream,
				 CORE_ADDR addr, struct objfile *objfile,
				 struct dwarf2_per_cu_data *per_cu,
				 const gdb_byte *data, const gdb_byte *end,
				 unsigned int addr_size)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  size_t leb128_size;

  if (data[0] >= DW_OP_reg0 && data[0] <= DW_OP_reg31)
    {
      fprintf_filtered (stream, _("a variable in $%s"),
			locexpr_regname (gdbarch, data[0] - DW_OP_reg0));
      data += 1;
    }
  else if (data[0] == DW_OP_regx)
    {
      uint64_t reg;

      data = safe_read_uleb128 (data + 1, end, &reg);
      fprintf_filtered (stream, _("a variable in $%s"),
			locexpr_regname (gdbarch, reg));
    }
  else if (data[0] == DW_OP_fbreg)
    {
      const struct block *b;
      struct symbol *framefunc;
      int frame_reg = 0;
      int64_t frame_offset;
      const gdb_byte *base_data, *new_data, *save_data = data;
      size_t base_size;
      int64_t base_offset = 0;

      new_data = safe_read_sleb128 (data + 1, end, &frame_offset);
      if (!piece_end_p (new_data, end))
	return data;
      data = new_data;

      b = block_for_pc (addr);

      if (!b)
	error (_("No block found for address for symbol \"%s\"."),
	       SYMBOL_PRINT_NAME (symbol));

      framefunc = block_linkage_function (b);

      if (!framefunc)
	error (_("No function found for block for symbol \"%s\"."),
	       SYMBOL_PRINT_NAME (symbol));

      func_get_frame_base_dwarf_block (framefunc, addr, &base_data, &base_size);

      if (base_data[0] >= DW_OP_breg0 && base_data[0] <= DW_OP_breg31)
	{
	  const gdb_byte *buf_end;
	  
	  frame_reg = base_data[0] - DW_OP_breg0;
	  buf_end = safe_read_sleb128 (base_data + 1, base_data + base_size,
				       &base_offset);
	  if (buf_end != base_data + base_size)
	    error (_("Unexpected opcode after "
		     "DW_OP_breg%u for symbol \"%s\"."),
		   frame_reg, SYMBOL_PRINT_NAME (symbol));
	}
      else if (base_data[0] >= DW_OP_reg0 && base_data[0] <= DW_OP_reg31)
	{
	  /* The frame base is just the register, with no offset.  */
	  frame_reg = base_data[0] - DW_OP_reg0;
	  base_offset = 0;
	}
      else
	{
	  /* We don't know what to do with the frame base expression,
	     so we can't trace this variable; give up.  */
	  return save_data;
	}

      fprintf_filtered (stream,
			_("a variable at frame base reg $%s offset %s+%s"),
			locexpr_regname (gdbarch, frame_reg),
			plongest (base_offset), plongest (frame_offset));
    }
  else if (data[0] >= DW_OP_breg0 && data[0] <= DW_OP_breg31
	   && piece_end_p (data, end))
    {
      int64_t offset;

      data = safe_read_sleb128 (data + 1, end, &offset);

      fprintf_filtered (stream,
			_("a variable at offset %s from base reg $%s"),
			plongest (offset),
			locexpr_regname (gdbarch, data[0] - DW_OP_breg0));
    }

  /* The location expression for a TLS variable looks like this (on a
     64-bit LE machine):

     DW_AT_location    : 10 byte block: 3 4 0 0 0 0 0 0 0 e0
                        (DW_OP_addr: 4; DW_OP_GNU_push_tls_address)

     0x3 is the encoding for DW_OP_addr, which has an operand as long
     as the size of an address on the target machine (here is 8
     bytes).  Note that more recent version of GCC emit DW_OP_const4u
     or DW_OP_const8u, depending on address size, rather than
     DW_OP_addr.  0xe0 is the encoding for DW_OP_GNU_push_tls_address.
     The operand represents the offset at which the variable is within
     the thread local storage.  */

  else if (data + 1 + addr_size < end
	   && (data[0] == DW_OP_addr
	       || (addr_size == 4 && data[0] == DW_OP_const4u)
	       || (addr_size == 8 && data[0] == DW_OP_const8u))
	   && (data[1 + addr_size] == DW_OP_GNU_push_tls_address
	       || data[1 + addr_size] == DW_OP_form_tls_address)
	   && piece_end_p (data + 2 + addr_size, end))
    {
      ULONGEST offset;
      offset = extract_unsigned_integer (data + 1, addr_size,
					 gdbarch_byte_order (gdbarch));

      fprintf_filtered (stream, 
			_("a thread-local variable at offset 0x%s "
			  "in the thread-local storage for `%s'"),
			phex_nz (offset, addr_size), objfile_name (objfile));

      data += 1 + addr_size + 1;
    }

  /* With -gsplit-dwarf a TLS variable can also look like this:
     DW_AT_location    : 3 byte block: fc 4 e0
                        (DW_OP_GNU_const_index: 4;
			 DW_OP_GNU_push_tls_address)  */
  else if (data + 3 <= end
	   && data + 1 + (leb128_size = skip_leb128 (data + 1, end)) < end
	   && data[0] == DW_OP_GNU_const_index
	   && leb128_size > 0
	   && (data[1 + leb128_size] == DW_OP_GNU_push_tls_address
	       || data[1 + leb128_size] == DW_OP_form_tls_address)
	   && piece_end_p (data + 2 + leb128_size, end))
    {
      uint64_t offset;

      data = safe_read_uleb128 (data + 1, end, &offset);
      offset = dwarf2_read_addr_index (per_cu, offset);
      fprintf_filtered (stream, 
			_("a thread-local variable at offset 0x%s "
			  "in the thread-local storage for `%s'"),
			phex_nz (offset, addr_size), objfile_name (objfile));
      ++data;
    }

  else if (data[0] >= DW_OP_lit0
	   && data[0] <= DW_OP_lit31
	   && data + 1 < end
	   && data[1] == DW_OP_stack_value)
    {
      fprintf_filtered (stream, _("the constant %d"), data[0] - DW_OP_lit0);
      data += 2;
    }

  return data;
}

/* Disassemble an expression, stopping at the end of a piece or at the
   end of the expression.  Returns a pointer to the next unread byte
   in the input expression.  If ALL is nonzero, then this function
   will keep going until it reaches the end of the expression.
   If there is an error during reading, e.g. we run off the end
   of the buffer, an error is thrown.  */

static const gdb_byte *
disassemble_dwarf_expression (struct ui_file *stream,
			      struct gdbarch *arch, unsigned int addr_size,
			      int offset_size, const gdb_byte *start,
			      const gdb_byte *data, const gdb_byte *end,
			      int indent, int all,
			      struct dwarf2_per_cu_data *per_cu)
{
  while (data < end
	 && (all
	     || (data[0] != DW_OP_piece && data[0] != DW_OP_bit_piece)))
    {
      enum dwarf_location_atom op = (enum dwarf_location_atom) *data++;
      uint64_t ul;
      int64_t l;
      const char *name;

      name = get_DW_OP_name (op);

      if (!name)
	error (_("Unrecognized DWARF opcode 0x%02x at %ld"),
	       op, (long) (data - 1 - start));
      fprintf_filtered (stream, "  %*ld: %s", indent + 4,
			(long) (data - 1 - start), name);

      switch (op)
	{
	case DW_OP_addr:
	  ul = extract_unsigned_integer (data, addr_size,
					 gdbarch_byte_order (arch));
	  data += addr_size;
	  fprintf_filtered (stream, " 0x%s", phex_nz (ul, addr_size));
	  break;

	case DW_OP_const1u:
	  ul = extract_unsigned_integer (data, 1, gdbarch_byte_order (arch));
	  data += 1;
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	case DW_OP_const1s:
	  l = extract_signed_integer (data, 1, gdbarch_byte_order (arch));
	  data += 1;
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;
	case DW_OP_const2u:
	  ul = extract_unsigned_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	case DW_OP_const2s:
	  l = extract_signed_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;
	case DW_OP_const4u:
	  ul = extract_unsigned_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	case DW_OP_const4s:
	  l = extract_signed_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;
	case DW_OP_const8u:
	  ul = extract_unsigned_integer (data, 8, gdbarch_byte_order (arch));
	  data += 8;
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	case DW_OP_const8s:
	  l = extract_signed_integer (data, 8, gdbarch_byte_order (arch));
	  data += 8;
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;
	case DW_OP_constu:
	  data = safe_read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	case DW_OP_consts:
	  data = safe_read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;

	case DW_OP_reg0:
	case DW_OP_reg1:
	case DW_OP_reg2:
	case DW_OP_reg3:
	case DW_OP_reg4:
	case DW_OP_reg5:
	case DW_OP_reg6:
	case DW_OP_reg7:
	case DW_OP_reg8:
	case DW_OP_reg9:
	case DW_OP_reg10:
	case DW_OP_reg11:
	case DW_OP_reg12:
	case DW_OP_reg13:
	case DW_OP_reg14:
	case DW_OP_reg15:
	case DW_OP_reg16:
	case DW_OP_reg17:
	case DW_OP_reg18:
	case DW_OP_reg19:
	case DW_OP_reg20:
	case DW_OP_reg21:
	case DW_OP_reg22:
	case DW_OP_reg23:
	case DW_OP_reg24:
	case DW_OP_reg25:
	case DW_OP_reg26:
	case DW_OP_reg27:
	case DW_OP_reg28:
	case DW_OP_reg29:
	case DW_OP_reg30:
	case DW_OP_reg31:
	  fprintf_filtered (stream, " [$%s]",
			    locexpr_regname (arch, op - DW_OP_reg0));
	  break;

	case DW_OP_regx:
	  data = safe_read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s [$%s]", pulongest (ul),
			    locexpr_regname (arch, (int) ul));
	  break;

	case DW_OP_implicit_value:
	  data = safe_read_uleb128 (data, end, &ul);
	  data += ul;
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_breg0:
	case DW_OP_breg1:
	case DW_OP_breg2:
	case DW_OP_breg3:
	case DW_OP_breg4:
	case DW_OP_breg5:
	case DW_OP_breg6:
	case DW_OP_breg7:
	case DW_OP_breg8:
	case DW_OP_breg9:
	case DW_OP_breg10:
	case DW_OP_breg11:
	case DW_OP_breg12:
	case DW_OP_breg13:
	case DW_OP_breg14:
	case DW_OP_breg15:
	case DW_OP_breg16:
	case DW_OP_breg17:
	case DW_OP_breg18:
	case DW_OP_breg19:
	case DW_OP_breg20:
	case DW_OP_breg21:
	case DW_OP_breg22:
	case DW_OP_breg23:
	case DW_OP_breg24:
	case DW_OP_breg25:
	case DW_OP_breg26:
	case DW_OP_breg27:
	case DW_OP_breg28:
	case DW_OP_breg29:
	case DW_OP_breg30:
	case DW_OP_breg31:
	  data = safe_read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " %s [$%s]", plongest (l),
			    locexpr_regname (arch, op - DW_OP_breg0));
	  break;

	case DW_OP_bregx:
	  data = safe_read_uleb128 (data, end, &ul);
	  data = safe_read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " register %s [$%s] offset %s",
			    pulongest (ul),
			    locexpr_regname (arch, (int) ul),
			    plongest (l));
	  break;

	case DW_OP_fbreg:
	  data = safe_read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;

	case DW_OP_xderef_size:
	case DW_OP_deref_size:
	case DW_OP_pick:
	  fprintf_filtered (stream, " %d", *data);
	  ++data;
	  break;

	case DW_OP_plus_uconst:
	  data = safe_read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;

	case DW_OP_skip:
	  l = extract_signed_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  fprintf_filtered (stream, " to %ld",
			    (long) (data + l - start));
	  break;

	case DW_OP_bra:
	  l = extract_signed_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  fprintf_filtered (stream, " %ld",
			    (long) (data + l - start));
	  break;

	case DW_OP_call2:
	  ul = extract_unsigned_integer (data, 2, gdbarch_byte_order (arch));
	  data += 2;
	  fprintf_filtered (stream, " offset %s", phex_nz (ul, 2));
	  break;

	case DW_OP_call4:
	  ul = extract_unsigned_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  fprintf_filtered (stream, " offset %s", phex_nz (ul, 4));
	  break;

	case DW_OP_call_ref:
	  ul = extract_unsigned_integer (data, offset_size,
					 gdbarch_byte_order (arch));
	  data += offset_size;
	  fprintf_filtered (stream, " offset %s", phex_nz (ul, offset_size));
	  break;

        case DW_OP_piece:
	  data = safe_read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s (bytes)", pulongest (ul));
	  break;

	case DW_OP_bit_piece:
	  {
	    uint64_t offset;

	    data = safe_read_uleb128 (data, end, &ul);
	    data = safe_read_uleb128 (data, end, &offset);
	    fprintf_filtered (stream, " size %s offset %s (bits)",
			      pulongest (ul), pulongest (offset));
	  }
	  break;

	case DW_OP_implicit_pointer:
	case DW_OP_GNU_implicit_pointer:
	  {
	    ul = extract_unsigned_integer (data, offset_size,
					   gdbarch_byte_order (arch));
	    data += offset_size;

	    data = safe_read_sleb128 (data, end, &l);

	    fprintf_filtered (stream, " DIE %s offset %s",
			      phex_nz (ul, offset_size),
			      plongest (l));
	  }
	  break;

	case DW_OP_deref_type:
	case DW_OP_GNU_deref_type:
	  {
	    int addr_size = *data++;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset offset = (cu_offset) ul;
	    type = dwarf2_get_die_type (offset, per_cu);
	    fprintf_filtered (stream, "<");
	    type_print (type, "", stream, -1);
	    fprintf_filtered (stream, " [0x%s]> %d",
			      phex_nz (to_underlying (offset), 0),
			      addr_size);
	  }
	  break;

	case DW_OP_const_type:
	case DW_OP_GNU_const_type:
	  {
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset type_die = (cu_offset) ul;
	    type = dwarf2_get_die_type (type_die, per_cu);
	    fprintf_filtered (stream, "<");
	    type_print (type, "", stream, -1);
	    fprintf_filtered (stream, " [0x%s]>",
			      phex_nz (to_underlying (type_die), 0));
	  }
	  break;

	case DW_OP_regval_type:
	case DW_OP_GNU_regval_type:
	  {
	    uint64_t reg;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &reg);
	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset type_die = (cu_offset) ul;

	    type = dwarf2_get_die_type (type_die, per_cu);
	    fprintf_filtered (stream, "<");
	    type_print (type, "", stream, -1);
	    fprintf_filtered (stream, " [0x%s]> [$%s]",
			      phex_nz (to_underlying (type_die), 0),
			      locexpr_regname (arch, reg));
	  }
	  break;

	case DW_OP_convert:
	case DW_OP_GNU_convert:
	case DW_OP_reinterpret:
	case DW_OP_GNU_reinterpret:
	  {
	    data = safe_read_uleb128 (data, end, &ul);
	    cu_offset type_die = (cu_offset) ul;

	    if (to_underlying (type_die) == 0)
	      fprintf_filtered (stream, "<0>");
	    else
	      {
		struct type *type;

		type = dwarf2_get_die_type (type_die, per_cu);
		fprintf_filtered (stream, "<");
		type_print (type, "", stream, -1);
		fprintf_filtered (stream, " [0x%s]>",
				  phex_nz (to_underlying (type_die), 0));
	      }
	  }
	  break;

	case DW_OP_entry_value:
	case DW_OP_GNU_entry_value:
	  data = safe_read_uleb128 (data, end, &ul);
	  fputc_filtered ('\n', stream);
	  disassemble_dwarf_expression (stream, arch, addr_size, offset_size,
					start, data, data + ul, indent + 2,
					all, per_cu);
	  data += ul;
	  continue;

	case DW_OP_GNU_parameter_ref:
	  ul = extract_unsigned_integer (data, 4, gdbarch_byte_order (arch));
	  data += 4;
	  fprintf_filtered (stream, " offset %s", phex_nz (ul, 4));
	  break;

	case DW_OP_GNU_addr_index:
	  data = safe_read_uleb128 (data, end, &ul);
	  ul = dwarf2_read_addr_index (per_cu, ul);
	  fprintf_filtered (stream, " 0x%s", phex_nz (ul, addr_size));
	  break;
	case DW_OP_GNU_const_index:
	  data = safe_read_uleb128 (data, end, &ul);
	  ul = dwarf2_read_addr_index (per_cu, ul);
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	}

      fprintf_filtered (stream, "\n");
    }

  return data;
}

/* Describe a single location, which may in turn consist of multiple
   pieces.  */

static void
locexpr_describe_location_1 (struct symbol *symbol, CORE_ADDR addr,
			     struct ui_file *stream,
			     const gdb_byte *data, size_t size,
			     struct objfile *objfile, unsigned int addr_size,
			     int offset_size, struct dwarf2_per_cu_data *per_cu)
{
  const gdb_byte *end = data + size;
  int first_piece = 1, bad = 0;

  while (data < end)
    {
      const gdb_byte *here = data;
      int disassemble = 1;

      if (first_piece)
	first_piece = 0;
      else
	fprintf_filtered (stream, _(", and "));

      if (!dwarf_always_disassemble)
	{
	  data = locexpr_describe_location_piece (symbol, stream,
						  addr, objfile, per_cu,
						  data, end, addr_size);
	  /* If we printed anything, or if we have an empty piece,
	     then don't disassemble.  */
	  if (data != here
	      || data[0] == DW_OP_piece
	      || data[0] == DW_OP_bit_piece)
	    disassemble = 0;
	}
      if (disassemble)
	{
	  fprintf_filtered (stream, _("a complex DWARF expression:\n"));
	  data = disassemble_dwarf_expression (stream,
					       get_objfile_arch (objfile),
					       addr_size, offset_size, data,
					       data, end, 0,
					       dwarf_always_disassemble,
					       per_cu);
	}

      if (data < end)
	{
	  int empty = data == here;
	      
	  if (disassemble)
	    fprintf_filtered (stream, "   ");
	  if (data[0] == DW_OP_piece)
	    {
	      uint64_t bytes;

	      data = safe_read_uleb128 (data + 1, end, &bytes);

	      if (empty)
		fprintf_filtered (stream, _("an empty %s-byte piece"),
				  pulongest (bytes));
	      else
		fprintf_filtered (stream, _(" [%s-byte piece]"),
				  pulongest (bytes));
	    }
	  else if (data[0] == DW_OP_bit_piece)
	    {
	      uint64_t bits, offset;

	      data = safe_read_uleb128 (data + 1, end, &bits);
	      data = safe_read_uleb128 (data, end, &offset);

	      if (empty)
		fprintf_filtered (stream,
				  _("an empty %s-bit piece"),
				  pulongest (bits));
	      else
		fprintf_filtered (stream,
				  _(" [%s-bit piece, offset %s bits]"),
				  pulongest (bits), pulongest (offset));
	    }
	  else
	    {
	      bad = 1;
	      break;
	    }
	}
    }

  if (bad || data > end)
    error (_("Corrupted DWARF2 expression for \"%s\"."),
	   SYMBOL_PRINT_NAME (symbol));
}

/* Print a natural-language description of SYMBOL to STREAM.  This
   version is for a symbol with a single location.  */

static void
locexpr_describe_location (struct symbol *symbol, CORE_ADDR addr,
			   struct ui_file *stream)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);
  struct objfile *objfile = dwarf2_per_cu_objfile (dlbaton->per_cu);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);
  int offset_size = dwarf2_per_cu_offset_size (dlbaton->per_cu);

  locexpr_describe_location_1 (symbol, addr, stream,
			       dlbaton->data, dlbaton->size,
			       objfile, addr_size, offset_size,
			       dlbaton->per_cu);
}

/* Describe the location of SYMBOL as an agent value in VALUE, generating
   any necessary bytecode in AX.  */

static void
locexpr_tracepoint_var_ref (struct symbol *symbol, struct gdbarch *gdbarch,
			    struct agent_expr *ax, struct axs_value *value)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (symbol);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);

  if (dlbaton->size == 0)
    value->optimized_out = 1;
  else
    dwarf2_compile_expr_to_ax (ax, value, gdbarch, addr_size,
			       dlbaton->data, dlbaton->data + dlbaton->size,
			       dlbaton->per_cu);
}

/* symbol_computed_ops 'generate_c_location' method.  */

static void
locexpr_generate_c_location (struct symbol *sym, string_file &stream,
			     struct gdbarch *gdbarch,
			     unsigned char *registers_used,
			     CORE_ADDR pc, const char *result_name)
{
  struct dwarf2_locexpr_baton *dlbaton
    = (struct dwarf2_locexpr_baton *) SYMBOL_LOCATION_BATON (sym);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);

  if (dlbaton->size == 0)
    error (_("symbol \"%s\" is optimized out"), SYMBOL_NATURAL_NAME (sym));

  compile_dwarf_expr_to_c (stream, result_name,
			   sym, pc, gdbarch, registers_used, addr_size,
			   dlbaton->data, dlbaton->data + dlbaton->size,
			   dlbaton->per_cu);
}

/* The set of location functions used with the DWARF-2 expression
   evaluator.  */
const struct symbol_computed_ops dwarf2_locexpr_funcs = {
  locexpr_read_variable,
  locexpr_read_variable_at_entry,
  locexpr_get_symbol_read_needs,
  locexpr_describe_location,
  0,	/* location_has_loclist */
  locexpr_tracepoint_var_ref,
  locexpr_generate_c_location
};


/* Wrapper functions for location lists.  These generally find
   the appropriate location expression and call something above.  */

/* Return the value of SYMBOL in FRAME using the DWARF-2 expression
   evaluator to calculate the location.  */
static struct value *
loclist_read_variable (struct symbol *symbol, struct frame_info *frame)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  struct value *val;
  const gdb_byte *data;
  size_t size;
  CORE_ADDR pc = frame ? get_frame_address_in_block (frame) : 0;

  data = dwarf2_find_location_expression (dlbaton, &size, pc);
  val = dwarf2_evaluate_loc_desc (SYMBOL_TYPE (symbol), frame, data, size,
				  dlbaton->per_cu);

  return val;
}

/* Read variable SYMBOL like loclist_read_variable at (callee) FRAME's function
   entry.  SYMBOL should be a function parameter, otherwise NO_ENTRY_VALUE_ERROR
   will be thrown.

   Function always returns non-NULL value, it may be marked optimized out if
   inferior frame information is not available.  It throws NO_ENTRY_VALUE_ERROR
   if it cannot resolve the parameter for any reason.  */

static struct value *
loclist_read_variable_at_entry (struct symbol *symbol, struct frame_info *frame)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  const gdb_byte *data;
  size_t size;
  CORE_ADDR pc;

  if (frame == NULL || !get_frame_func_if_available (frame, &pc))
    return allocate_optimized_out_value (SYMBOL_TYPE (symbol));

  data = dwarf2_find_location_expression (dlbaton, &size, pc);
  if (data == NULL)
    return allocate_optimized_out_value (SYMBOL_TYPE (symbol));

  return value_of_dwarf_block_entry (SYMBOL_TYPE (symbol), frame, data, size);
}

/* Implementation of get_symbol_read_needs from
   symbol_computed_ops.  */

static enum symbol_needs_kind
loclist_symbol_needs (struct symbol *symbol)
{
  /* If there's a location list, then assume we need to have a frame
     to choose the appropriate location expression.  With tracking of
     global variables this is not necessarily true, but such tracking
     is disabled in GCC at the moment until we figure out how to
     represent it.  */

  return SYMBOL_NEEDS_FRAME;
}

/* Print a natural-language description of SYMBOL to STREAM.  This
   version applies when there is a list of different locations, each
   with a specified address range.  */

static void
loclist_describe_location (struct symbol *symbol, CORE_ADDR addr,
			   struct ui_file *stream)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  const gdb_byte *loc_ptr, *buf_end;
  struct objfile *objfile = dwarf2_per_cu_objfile (dlbaton->per_cu);
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);
  int offset_size = dwarf2_per_cu_offset_size (dlbaton->per_cu);
  int signed_addr_p = bfd_get_sign_extend_vma (objfile->obfd);
  /* Adjust base_address for relocatable objects.  */
  CORE_ADDR base_offset = dwarf2_per_cu_text_offset (dlbaton->per_cu);
  CORE_ADDR base_address = dlbaton->base_address + base_offset;
  int done = 0;

  loc_ptr = dlbaton->data;
  buf_end = dlbaton->data + dlbaton->size;

  fprintf_filtered (stream, _("multi-location:\n"));

  /* Iterate through locations until we run out.  */
  while (!done)
    {
      CORE_ADDR low = 0, high = 0; /* init for gcc -Wall */
      int length;
      enum debug_loc_kind kind;
      const gdb_byte *new_ptr = NULL; /* init for gcc -Wall */

      if (dlbaton->from_dwo)
	kind = decode_debug_loc_dwo_addresses (dlbaton->per_cu,
					       loc_ptr, buf_end, &new_ptr,
					       &low, &high, byte_order);
      else
	kind = decode_debug_loc_addresses (loc_ptr, buf_end, &new_ptr,
					   &low, &high,
					   byte_order, addr_size,
					   signed_addr_p);
      loc_ptr = new_ptr;
      switch (kind)
	{
	case DEBUG_LOC_END_OF_LIST:
	  done = 1;
	  continue;
	case DEBUG_LOC_BASE_ADDRESS:
	  base_address = high + base_offset;
	  fprintf_filtered (stream, _("  Base address %s"),
			    paddress (gdbarch, base_address));
	  continue;
	case DEBUG_LOC_START_END:
	case DEBUG_LOC_START_LENGTH:
	  break;
	case DEBUG_LOC_BUFFER_OVERFLOW:
	case DEBUG_LOC_INVALID_ENTRY:
	  error (_("Corrupted DWARF expression for symbol \"%s\"."),
		 SYMBOL_PRINT_NAME (symbol));
	default:
	  gdb_assert_not_reached ("bad debug_loc_kind");
	}

      /* Otherwise, a location expression entry.  */
      low += base_address;
      high += base_address;

      low = gdbarch_adjust_dwarf2_addr (gdbarch, low);
      high = gdbarch_adjust_dwarf2_addr (gdbarch, high);

      length = extract_unsigned_integer (loc_ptr, 2, byte_order);
      loc_ptr += 2;

      /* (It would improve readability to print only the minimum
	 necessary digits of the second number of the range.)  */
      fprintf_filtered (stream, _("  Range %s-%s: "),
			paddress (gdbarch, low), paddress (gdbarch, high));

      /* Now describe this particular location.  */
      locexpr_describe_location_1 (symbol, low, stream, loc_ptr, length,
				   objfile, addr_size, offset_size,
				   dlbaton->per_cu);

      fprintf_filtered (stream, "\n");

      loc_ptr += length;
    }
}

/* Describe the location of SYMBOL as an agent value in VALUE, generating
   any necessary bytecode in AX.  */
static void
loclist_tracepoint_var_ref (struct symbol *symbol, struct gdbarch *gdbarch,
			    struct agent_expr *ax, struct axs_value *value)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (symbol);
  const gdb_byte *data;
  size_t size;
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);

  data = dwarf2_find_location_expression (dlbaton, &size, ax->scope);
  if (size == 0)
    value->optimized_out = 1;
  else
    dwarf2_compile_expr_to_ax (ax, value, gdbarch, addr_size, data, data + size,
			       dlbaton->per_cu);
}

/* symbol_computed_ops 'generate_c_location' method.  */

static void
loclist_generate_c_location (struct symbol *sym, string_file &stream,
			     struct gdbarch *gdbarch,
			     unsigned char *registers_used,
			     CORE_ADDR pc, const char *result_name)
{
  struct dwarf2_loclist_baton *dlbaton
    = (struct dwarf2_loclist_baton *) SYMBOL_LOCATION_BATON (sym);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);
  const gdb_byte *data;
  size_t size;

  data = dwarf2_find_location_expression (dlbaton, &size, pc);
  if (size == 0)
    error (_("symbol \"%s\" is optimized out"), SYMBOL_NATURAL_NAME (sym));

  compile_dwarf_expr_to_c (stream, result_name,
			   sym, pc, gdbarch, registers_used, addr_size,
			   data, data + size,
			   dlbaton->per_cu);
}

/* The set of location functions used with the DWARF-2 expression
   evaluator and location lists.  */
const struct symbol_computed_ops dwarf2_loclist_funcs = {
  loclist_read_variable,
  loclist_read_variable_at_entry,
  loclist_symbol_needs,
  loclist_describe_location,
  1,	/* location_has_loclist */
  loclist_tracepoint_var_ref,
  loclist_generate_c_location
};

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_dwarf2loc;

void
_initialize_dwarf2loc (void)
{
  add_setshow_zuinteger_cmd ("entry-values", class_maintenance,
			     &entry_values_debug,
			     _("Set entry values and tail call frames "
			       "debugging."),
			     _("Show entry values and tail call frames "
			       "debugging."),
			     _("When non-zero, the process of determining "
			       "parameter values from function entry point "
			       "and tail call frames will be printed."),
			     NULL,
			     show_entry_values_debug,
			     &setdebuglist, &showdebuglist);

#if GDB_SELF_TEST
  register_self_test (selftests::copy_bitwise_tests);
#endif
}
