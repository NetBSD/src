/* DWARF 2 location expression support for GDB.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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
#include "exceptions.h"
#include "block.h"
#include "gdbcmd.h"

#include "dwarf2.h"
#include "dwarf2expr.h"
#include "dwarf2loc.h"
#include "dwarf2-frame.h"

#include <string.h>
#include "gdb_assert.h"

extern int dwarf2_always_disassemble;

static void dwarf_expr_frame_base_1 (struct symbol *framefunc, CORE_ADDR pc,
				     const gdb_byte **start, size_t *length);

static const struct dwarf_expr_context_funcs dwarf_expr_ctx_funcs;

static struct value *dwarf2_evaluate_loc_desc_full (struct type *type,
						    struct frame_info *frame,
						    const gdb_byte *data,
						    size_t size,
						    struct dwarf2_per_cu_data *per_cu,
						    LONGEST byte_offset);

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
    case DEBUG_LOC_END_OF_LIST:
      *new_ptr = loc_ptr;
      return DEBUG_LOC_END_OF_LIST;
    case DEBUG_LOC_BASE_ADDRESS:
      *low = 0;
      loc_ptr = gdb_read_uleb128 (loc_ptr, buf_end, &high_index);
      if (loc_ptr == NULL)
	return DEBUG_LOC_BUFFER_OVERFLOW;
      *high = dwarf2_read_addr_index (per_cu, high_index);
      *new_ptr = loc_ptr;
      return DEBUG_LOC_BASE_ADDRESS;
    case DEBUG_LOC_START_END:
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
    case DEBUG_LOC_START_LENGTH:
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
      else
	kind = decode_debug_loc_addresses (loc_ptr, buf_end, &new_ptr,
					   &low, &high,
					   byte_order, addr_size,
					   signed_addr_p);
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
	 If the entry is from a DWO, don't add base address: the entry is
	 from .debug_addr which has absolute addresses.  */
      if (! baton->from_dwo)
	{
	  low += base_address;
	  high += base_address;
	}

      length = extract_unsigned_integer (loc_ptr, 2, byte_order);
      loc_ptr += 2;

      if (low == high && pc == low)
	{
	  /* This is entry PC record present only at entry point
	     of a function.  Verify it is really the function entry point.  */

	  struct block *pc_block = block_for_pc (pc);
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
};

/* Helper functions for dwarf2_evaluate_loc_desc.  */

/* Using the frame specified in BATON, return the value of register
   REGNUM, treated as a pointer.  */
static CORE_ADDR
dwarf_expr_read_addr_from_reg (void *baton, int dwarf_regnum)
{
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;
  struct gdbarch *gdbarch = get_frame_arch (debaton->frame);
  CORE_ADDR result;
  int regnum;

  regnum = gdbarch_dwarf2_reg_to_regnum (gdbarch, dwarf_regnum);
  result = address_from_register (builtin_type (gdbarch)->builtin_data_ptr,
				  regnum, debaton->frame);
  return result;
}

/* Implement struct dwarf_expr_context_funcs' "get_reg_value" callback.  */

static struct value *
dwarf_expr_get_reg_value (void *baton, struct type *type, int dwarf_regnum)
{
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;
  struct gdbarch *gdbarch = get_frame_arch (debaton->frame);
  int regnum = gdbarch_dwarf2_reg_to_regnum (gdbarch, dwarf_regnum);

  return value_from_register (type, regnum, debaton->frame);
}

/* Read memory at ADDR (length LEN) into BUF.  */

static void
dwarf_expr_read_mem (void *baton, gdb_byte *buf, CORE_ADDR addr, size_t len)
{
  read_memory (addr, buf, len);
}

/* Using the frame specified in BATON, find the location expression
   describing the frame base.  Return a pointer to it in START and
   its length in LENGTH.  */
static void
dwarf_expr_frame_base (void *baton, const gdb_byte **start, size_t * length)
{
  /* FIXME: cagney/2003-03-26: This code should be using
     get_frame_base_address(), and then implement a dwarf2 specific
     this_base method.  */
  struct symbol *framefunc;
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;
  struct block *bl = get_frame_block (debaton->frame, NULL);

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

  dwarf_expr_frame_base_1 (framefunc,
			   get_frame_address_in_block (debaton->frame),
			   start, length);
}

/* Implement find_frame_base_location method for LOC_BLOCK functions using
   DWARF expression for its DW_AT_frame_base.  */

static void
locexpr_find_frame_base_location (struct symbol *framefunc, CORE_ADDR pc,
				  const gdb_byte **start, size_t *length)
{
  struct dwarf2_locexpr_baton *symbaton = SYMBOL_LOCATION_BATON (framefunc);

  *length = symbaton->size;
  *start = symbaton->data;
}

/* Vector for inferior functions as represented by LOC_BLOCK, if the inferior
   function uses DWARF expression for its DW_AT_frame_base.  */

const struct symbol_block_ops dwarf2_block_frame_base_locexpr_funcs =
{
  locexpr_find_frame_base_location
};

/* Implement find_frame_base_location method for LOC_BLOCK functions using
   DWARF location list for its DW_AT_frame_base.  */

static void
loclist_find_frame_base_location (struct symbol *framefunc, CORE_ADDR pc,
				  const gdb_byte **start, size_t *length)
{
  struct dwarf2_loclist_baton *symbaton = SYMBOL_LOCATION_BATON (framefunc);

  *start = dwarf2_find_location_expression (symbaton, length, pc);
}

/* Vector for inferior functions as represented by LOC_BLOCK, if the inferior
   function uses DWARF location list for its DW_AT_frame_base.  */

const struct symbol_block_ops dwarf2_block_frame_base_loclist_funcs =
{
  loclist_find_frame_base_location
};

static void
dwarf_expr_frame_base_1 (struct symbol *framefunc, CORE_ADDR pc,
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

/* Helper function for dwarf2_evaluate_loc_desc.  Computes the CFA for
   the frame in BATON.  */

static CORE_ADDR
dwarf_expr_frame_cfa (void *baton)
{
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;

  return dwarf2_frame_cfa (debaton->frame);
}

/* Helper function for dwarf2_evaluate_loc_desc.  Computes the PC for
   the frame in BATON.  */

static CORE_ADDR
dwarf_expr_frame_pc (void *baton)
{
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;

  return get_frame_address_in_block (debaton->frame);
}

/* Using the objfile specified in BATON, find the address for the
   current thread's thread-local storage with offset OFFSET.  */
static CORE_ADDR
dwarf_expr_tls_address (void *baton, CORE_ADDR offset)
{
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;
  struct objfile *objfile = dwarf2_per_cu_objfile (debaton->per_cu);

  return target_translate_tls_address (objfile, offset);
}

/* Call DWARF subroutine from DW_AT_location of DIE at DIE_OFFSET in
   current CU (as is PER_CU).  State of the CTX is not affected by the
   call and return.  */

static void
per_cu_dwarf_call (struct dwarf_expr_context *ctx, cu_offset die_offset,
		   struct dwarf2_per_cu_data *per_cu,
		   CORE_ADDR (*get_frame_pc) (void *baton),
		   void *baton)
{
  struct dwarf2_locexpr_baton block;

  block = dwarf2_fetch_die_loc_cu_off (die_offset, per_cu, get_frame_pc, baton);

  /* DW_OP_call_ref is currently not supported.  */
  gdb_assert (block.per_cu == per_cu);

  dwarf_expr_eval (ctx, block.data, block.size);
}

/* Helper interface of per_cu_dwarf_call for dwarf2_evaluate_loc_desc.  */

static void
dwarf_expr_dwarf_call (struct dwarf_expr_context *ctx, cu_offset die_offset)
{
  struct dwarf_expr_baton *debaton = ctx->baton;

  per_cu_dwarf_call (ctx, die_offset, debaton->per_cu,
		     ctx->funcs->get_frame_pc, ctx->baton);
}

/* Callback function for dwarf2_evaluate_loc_desc.  */

static struct type *
dwarf_expr_get_base_type (struct dwarf_expr_context *ctx,
			  cu_offset die_offset)
{
  struct dwarf_expr_baton *debaton = ctx->baton;

  return dwarf2_get_die_type (die_offset, debaton->per_cu);
}

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

/* Find DW_TAG_GNU_call_site's DW_AT_GNU_call_site_target address.
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
			 _("DW_AT_GNU_call_site_target is not specified "
			   "at %s in %s"),
			 paddress (call_site_gdbarch, call_site->pc),
			 (msym.minsym == NULL ? "???"
			  : SYMBOL_PRINT_NAME (msym.minsym)));
			
	  }
	if (caller_frame == NULL)
	  {
	    struct bound_minimal_symbol msym;
	    
	    msym = lookup_minimal_symbol_by_pc (call_site->pc - 1);
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("DW_AT_GNU_call_site_target DWARF block resolving "
			   "requires known frame which is currently not "
			   "available at %s in %s"),
			 paddress (call_site_gdbarch, call_site->pc),
			 (msym.minsym == NULL ? "???"
			  : SYMBOL_PRINT_NAME (msym.minsym)));
			
	  }
	caller_arch = get_frame_arch (caller_frame);
	caller_core_addr_type = builtin_type (caller_arch)->builtin_func_ptr;
	val = dwarf2_evaluate_loc_desc (caller_core_addr_type, caller_frame,
					dwarf_block->data, dwarf_block->size,
					dwarf_block->per_cu);
	/* DW_AT_GNU_call_site_target is a DWARF expression, not a DWARF
	   location.  */
	if (VALUE_LVAL (val) == lval_memory)
	  return value_address (val);
	else
	  return value_as_address (val);
      }

    case FIELD_LOC_KIND_PHYSNAME:
      {
	const char *physname;
	struct minimal_symbol *msym;

	physname = FIELD_STATIC_PHYSNAME (call_site->target);

	/* Handle both the mangled and demangled PHYSNAME.  */
	msym = lookup_minimal_symbol (physname, NULL, NULL);
	if (msym == NULL)
	  {
	    msym = lookup_minimal_symbol_by_pc (call_site->pc - 1).minsym;
	    throw_error (NO_ENTRY_VALUE_ERROR,
			 _("Cannot find function \"%s\" for a call site target "
			   "at %s in %s"),
			 physname, paddress (call_site_gdbarch, call_site->pc),
			 msym == NULL ? "???" : SYMBOL_PRINT_NAME (msym));
			
	  }
	return SYMBOL_VALUE_ADDRESS (msym);
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
		 _("DW_TAG_GNU_call_site resolving failed to find function "
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
  struct obstack addr_obstack;
  struct cleanup *old_chain;
  CORE_ADDR addr;

  /* Track here CORE_ADDRs which were already visited.  */
  htab_t addr_hash;

  /* The verification is completely unordered.  Track here function addresses
     which still need to be iterated.  */
  VEC (CORE_ADDR) *todo = NULL;

  obstack_init (&addr_obstack);
  old_chain = make_cleanup_obstack_free (&addr_obstack);   
  addr_hash = htab_create_alloc_ex (64, core_addr_hash, core_addr_eq, NULL,
				    &addr_obstack, hashtab_obstack_allocate,
				    NULL);
  make_cleanup_htab_delete (addr_hash);

  make_cleanup (VEC_cleanup (CORE_ADDR), &todo);

  VEC_safe_push (CORE_ADDR, todo, verify_addr);
  while (!VEC_empty (CORE_ADDR, todo))
    {
      struct symbol *func_sym;
      struct call_site *call_site;

      addr = VEC_pop (CORE_ADDR, todo);

      func_sym = func_addr_to_tail_call_list (gdbarch, addr);

      for (call_site = TYPE_TAIL_CALL_LIST (SYMBOL_TYPE (func_sym));
	   call_site; call_site = call_site->tail_call_next)
	{
	  CORE_ADDR target_addr;
	  void **slot;

	  /* CALLER_FRAME with registers is not available for tail-call jumped
	     frames.  */
	  target_addr = call_site_to_target_addr (gdbarch, call_site, NULL);

	  if (target_addr == verify_addr)
	    {
	      struct bound_minimal_symbol msym;
	      
	      msym = lookup_minimal_symbol_by_pc (verify_addr);
	      throw_error (NO_ENTRY_VALUE_ERROR,
			   _("DW_OP_GNU_entry_value resolving has found "
			     "function \"%s\" at %s can call itself via tail "
			     "calls"),
			   (msym.minsym == NULL ? "???"
			    : SYMBOL_PRINT_NAME (msym.minsym)),
			   paddress (gdbarch, verify_addr));
	    }

	  slot = htab_find_slot (addr_hash, &target_addr, INSERT);
	  if (*slot == NULL)
	    {
	      *slot = obstack_copy (&addr_obstack, &target_addr,
				    sizeof (target_addr));
	      VEC_safe_push (CORE_ADDR, todo, target_addr);
	    }
	}
    }

  do_cleanups (old_chain);
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
		       : SYMBOL_PRINT_NAME (msym.minsym)));

}

/* vec.h needs single word type name, typedef it.  */
typedef struct call_site *call_sitep;

/* Define VEC (call_sitep) functions.  */
DEF_VEC_P (call_sitep);

/* Intersect RESULTP with CHAIN to keep RESULTP unambiguous, keep in RESULTP
   only top callers and bottom callees which are present in both.  GDBARCH is
   used only for ENTRY_VALUES_DEBUG.  RESULTP is NULL after return if there are
   no remaining possibilities to provide unambiguous non-trivial result.
   RESULTP should point to NULL on the first (initialization) call.  Caller is
   responsible for xfree of any RESULTP data.  */

static void
chain_candidate (struct gdbarch *gdbarch, struct call_site_chain **resultp,
		 VEC (call_sitep) *chain)
{
  struct call_site_chain *result = *resultp;
  long length = VEC_length (call_sitep, chain);
  int callers, callees, idx;

  if (result == NULL)
    {
      /* Create the initial chain containing all the passed PCs.  */

      result = xmalloc (sizeof (*result) + sizeof (*result->call_site)
					   * (length - 1));
      result->length = length;
      result->callers = result->callees = length;
      if (!VEC_empty (call_sitep, chain))
	memcpy (result->call_site, VEC_address (call_sitep, chain),
		sizeof (*result->call_site) * length);
      *resultp = result;

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
	tailcall_dump (gdbarch, VEC_index (call_sitep, chain, idx));
      fputc_unfiltered ('\n', gdb_stdlog);
    }

  /* Intersect callers.  */

  callers = min (result->callers, length);
  for (idx = 0; idx < callers; idx++)
    if (result->call_site[idx] != VEC_index (call_sitep, chain, idx))
      {
	result->callers = idx;
	break;
      }

  /* Intersect callees.  */

  callees = min (result->callees, length);
  for (idx = 0; idx < callees; idx++)
    if (result->call_site[result->length - 1 - idx]
	!= VEC_index (call_sitep, chain, length - 1 - idx))
      {
	result->callees = idx;
	break;
      }

  if (entry_values_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "tailcall: reduced:");
      for (idx = 0; idx < result->callers; idx++)
	tailcall_dump (gdbarch, result->call_site[idx]);
      fputs_unfiltered (" |", gdb_stdlog);
      for (idx = 0; idx < result->callees; idx++)
	tailcall_dump (gdbarch, result->call_site[result->length
						  - result->callees + idx]);
      fputc_unfiltered ('\n', gdb_stdlog);
    }

  if (result->callers == 0 && result->callees == 0)
    {
      /* There are no common callers or callees.  It could be also a direct
	 call (which has length 0) with ambiguous possibility of an indirect
	 call - CALLERS == CALLEES == 0 is valid during the first allocation
	 but any subsequence processing of such entry means ambiguity.  */
      xfree (result);
      *resultp = NULL;
      return;
    }

  /* See call_site_find_chain_1 why there is no way to reach the bottom callee
     PC again.  In such case there must be two different code paths to reach
     it, therefore some of the former determined intermediate PCs must differ
     and the unambiguous chain gets shortened.  */
  gdb_assert (result->callers + result->callees < result->length);
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
  struct obstack addr_obstack;
  struct cleanup *back_to_retval, *back_to_workdata;
  struct call_site_chain *retval = NULL;
  struct call_site *call_site;

  /* Mark CALL_SITEs so we do not visit the same ones twice.  */
  htab_t addr_hash;

  /* CHAIN contains only the intermediate CALL_SITEs.  Neither CALLER_PC's
     call_site nor any possible call_site at CALLEE_PC's function is there.
     Any CALL_SITE in CHAIN will be iterated to its siblings - via
     TAIL_CALL_NEXT.  This is inappropriate for CALLER_PC's call_site.  */
  VEC (call_sitep) *chain = NULL;

  /* We are not interested in the specific PC inside the callee function.  */
  callee_pc = get_pc_function_start (callee_pc);
  if (callee_pc == 0)
    throw_error (NO_ENTRY_VALUE_ERROR, _("Unable to find function for PC %s"),
		 paddress (gdbarch, save_callee_pc));

  back_to_retval = make_cleanup (free_current_contents, &retval);

  obstack_init (&addr_obstack);
  back_to_workdata = make_cleanup_obstack_free (&addr_obstack);   
  addr_hash = htab_create_alloc_ex (64, core_addr_hash, core_addr_eq, NULL,
				    &addr_obstack, hashtab_obstack_allocate,
				    NULL);
  make_cleanup_htab_delete (addr_hash);

  make_cleanup (VEC_cleanup (call_sitep), &chain);

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
	  chain_candidate (gdbarch, &retval, chain);
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
	      void **slot;

	      slot = htab_find_slot (addr_hash, &target_call_site->pc, INSERT);
	      if (*slot == NULL)
		{
		  /* Successfully entered TARGET_CALL_SITE.  */

		  *slot = &target_call_site->pc;
		  VEC_safe_push (call_sitep, chain, target_call_site);
		  break;
		}
	    }

	  /* Backtrack (without revisiting the originating call_site).  Try the
	     callers's sibling; if there isn't any try the callers's callers's
	     sibling etc.  */

	  target_call_site = NULL;
	  while (!VEC_empty (call_sitep, chain))
	    {
	      call_site = VEC_pop (call_sitep, chain);

	      gdb_assert (htab_find_slot (addr_hash, &call_site->pc,
					  NO_INSERT) != NULL);
	      htab_remove_elt (addr_hash, &call_site->pc);

	      target_call_site = call_site->tail_call_next;
	      if (target_call_site)
		break;
	    }
	}
      while (target_call_site);

      if (VEC_empty (call_sitep, chain))
	call_site = NULL;
      else
	call_site = VEC_last (call_sitep, chain);
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
		    ? "???" : SYMBOL_PRINT_NAME (msym_caller.minsym)),
		   paddress (gdbarch, caller_pc),
		   (msym_callee.minsym == NULL
		    ? "???" : SYMBOL_PRINT_NAME (msym_callee.minsym)),
		   paddress (gdbarch, callee_pc));
    }

  do_cleanups (back_to_workdata);
  discard_cleanups (back_to_retval);
  return retval;
}

/* Create and return call_site_chain for CALLER_PC and CALLEE_PC.  All the
   assumed frames between them use GDBARCH.  If valid call_site_chain cannot be
   constructed return NULL.  Caller is responsible for xfree of the returned
   result.  */

struct call_site_chain *
call_site_find_chain (struct gdbarch *gdbarch, CORE_ADDR caller_pc,
		      CORE_ADDR callee_pc)
{
  volatile struct gdb_exception e;
  struct call_site_chain *retval = NULL;

  TRY_CATCH (e, RETURN_MASK_ERROR)
    {
      retval = call_site_find_chain_1 (gdbarch, caller_pc, callee_pc);
    }
  if (e.reason < 0)
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
	return kind_u.param_offset.cu_off == parameter->u.param_offset.cu_off;
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
		   _("DW_OP_GNU_entry_value resolving callee gdbarch %s "
		     "(of %s (%s)) does not match caller gdbarch %s"),
		   gdbarch_bfd_arch_info (gdbarch)->printable_name,
		   paddress (gdbarch, func_addr),
		   (msym.minsym == NULL ? "???"
		    : SYMBOL_PRINT_NAME (msym.minsym)),
		   gdbarch_bfd_arch_info (caller_gdbarch)->printable_name);
    }

  if (caller_frame == NULL)
    {
      struct bound_minimal_symbol msym
	= lookup_minimal_symbol_by_pc (func_addr);

      throw_error (NO_ENTRY_VALUE_ERROR, _("DW_OP_GNU_entry_value resolving "
					   "requires caller of %s (%s)"),
		   paddress (gdbarch, func_addr),
		   (msym.minsym == NULL ? "???"
		    : SYMBOL_PRINT_NAME (msym.minsym)));
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
		   _("DW_OP_GNU_entry_value resolving expects callee %s at %s "
		     "but the called frame is for %s at %s"),
		   (target_msym == NULL ? "???"
					: SYMBOL_PRINT_NAME (target_msym)),
		   paddress (gdbarch, target_addr),
		   func_msym == NULL ? "???" : SYMBOL_PRINT_NAME (func_msym),
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

      /* DW_TAG_GNU_call_site_parameter will be missing just if GCC could not
	 determine its value.  */
      throw_error (NO_ENTRY_VALUE_ERROR, _("Cannot find matching parameter "
					   "at DW_TAG_GNU_call_site %s at %s"),
		   paddress (gdbarch, caller_pc),
		   msym == NULL ? "???" : SYMBOL_PRINT_NAME (msym)); 
    }

  *per_cu_return = call_site->per_cu;
  return parameter;
}

/* Return value for PARAMETER matching DEREF_SIZE.  If DEREF_SIZE is -1, return
   the normal DW_AT_GNU_call_site_value block.  Otherwise return the
   DW_AT_GNU_call_site_data_value (dereferenced) block.

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
		 _("Cannot resolve DW_AT_GNU_call_site_data_value"));

  /* DW_AT_GNU_call_site_value is a DWARF expression, not a DWARF
     location.  Postprocessing of DWARF_VALUE_MEMORY would lose the type from
     DWARF block.  */
  data = alloca (size + 1);
  memcpy (data, data_src, size);
  data[size] = DW_OP_stack_value;

  return dwarf2_evaluate_loc_desc (type, caller_frame, data, size + 1, per_cu);
}

/* Execute DWARF block of call_site_parameter which matches KIND and KIND_U.
   Choose DEREF_SIZE value of that parameter.  Search caller of the CTX's
   frame.  CTX must be of dwarf_expr_ctx_funcs kind.

   The CTX caller can be from a different CU - per_cu_dwarf_call implementation
   can be more simple as it does not support cross-CU DWARF executions.  */

static void
dwarf_expr_push_dwarf_reg_entry_value (struct dwarf_expr_context *ctx,
				       enum call_site_parameter_kind kind,
				       union call_site_parameter_u kind_u,
				       int deref_size)
{
  struct dwarf_expr_baton *debaton;
  struct frame_info *frame, *caller_frame;
  struct dwarf2_per_cu_data *caller_per_cu;
  struct dwarf_expr_baton baton_local;
  struct dwarf_expr_context saved_ctx;
  struct call_site_parameter *parameter;
  const gdb_byte *data_src;
  size_t size;

  gdb_assert (ctx->funcs == &dwarf_expr_ctx_funcs);
  debaton = ctx->baton;
  frame = debaton->frame;
  caller_frame = get_prev_frame (frame);

  parameter = dwarf_expr_reg_to_entry_parameter (frame, kind, kind_u,
						 &caller_per_cu);
  data_src = deref_size == -1 ? parameter->value : parameter->data_value;
  size = deref_size == -1 ? parameter->value_size : parameter->data_value_size;

  /* DEREF_SIZE size is not verified here.  */
  if (data_src == NULL)
    throw_error (NO_ENTRY_VALUE_ERROR,
		 _("Cannot resolve DW_AT_GNU_call_site_data_value"));

  baton_local.frame = caller_frame;
  baton_local.per_cu = caller_per_cu;

  saved_ctx.gdbarch = ctx->gdbarch;
  saved_ctx.addr_size = ctx->addr_size;
  saved_ctx.offset = ctx->offset;
  saved_ctx.baton = ctx->baton;
  ctx->gdbarch = get_objfile_arch (dwarf2_per_cu_objfile (baton_local.per_cu));
  ctx->addr_size = dwarf2_per_cu_addr_size (baton_local.per_cu);
  ctx->offset = dwarf2_per_cu_text_offset (baton_local.per_cu);
  ctx->baton = &baton_local;

  dwarf_expr_eval (ctx, data_src, size);

  ctx->gdbarch = saved_ctx.gdbarch;
  ctx->addr_size = saved_ctx.addr_size;
  ctx->offset = saved_ctx.offset;
  ctx->baton = saved_ctx.baton;
}

/* Callback function for dwarf2_evaluate_loc_desc.
   Fetch the address indexed by DW_OP_GNU_addr_index.  */

static CORE_ADDR
dwarf_expr_get_addr_index (void *baton, unsigned int index)
{
  struct dwarf_expr_baton *debaton = (struct dwarf_expr_baton *) baton;

  return dwarf2_read_addr_index (debaton->per_cu, index);
}

/* VALUE must be of type lval_computed with entry_data_value_funcs.  Perform
   the indirect method on it, that is use its stored target value, the sole
   purpose of entry_data_value_funcs..  */

static struct value *
entry_data_value_coerce_ref (const struct value *value)
{
  struct type *checked_type = check_typedef (value_type (value));
  struct value *target_val;

  if (TYPE_CODE (checked_type) != TYPE_CODE_REF)
    return NULL;

  target_val = value_computed_closure (value);
  value_incref (target_val);
  return target_val;
}

/* Implement copy_closure.  */

static void *
entry_data_value_copy_closure (const struct value *v)
{
  struct value *target_val = value_computed_closure (v);

  value_incref (target_val);
  return target_val;
}

/* Implement free_closure.  */

static void
entry_data_value_free_closure (struct value *v)
{
  struct value *target_val = value_computed_closure (v);

  value_free (target_val);
}

/* Vector for methods for an entry value reference where the referenced value
   is stored in the caller.  On the first dereference use
   DW_AT_GNU_call_site_data_value in the caller.  */

static const struct lval_funcs entry_data_value_funcs =
{
  NULL,	/* read */
  NULL,	/* write */
  NULL,	/* check_validity */
  NULL,	/* check_any_valid */
  NULL,	/* indirect */
  entry_data_value_coerce_ref,
  NULL,	/* check_synthetic_pointer */
  entry_data_value_copy_closure,
  entry_data_value_free_closure
};

/* Read parameter of TYPE at (callee) FRAME's function entry.  KIND and KIND_U
   are used to match DW_AT_location at the caller's
   DW_TAG_GNU_call_site_parameter.

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
  CORE_ADDR addr;

  parameter = dwarf_expr_reg_to_entry_parameter (frame, kind, kind_u,
						 &caller_per_cu);

  outer_val = dwarf_entry_parameter_to_value (parameter, -1 /* deref_size */,
					      type, caller_frame,
					      caller_per_cu);

  /* Check if DW_AT_GNU_call_site_data_value cannot be used.  If it should be
     used and it is not available do not fall back to OUTER_VAL - dereferencing
     TYPE_CODE_REF with non-entry data value would give current value - not the
     entry value.  */

  if (TYPE_CODE (checked_type) != TYPE_CODE_REF
      || TYPE_TARGET_TYPE (checked_type) == NULL)
    return outer_val;

  target_val = dwarf_entry_parameter_to_value (parameter,
					       TYPE_LENGTH (target_type),
					       target_type, caller_frame,
					       caller_per_cu);

  /* value_as_address dereferences TYPE_CODE_REF.  */
  addr = extract_typed_address (value_contents (outer_val), checked_type);

  /* The target entry value has artificial address of the entry value
     reference.  */
  VALUE_LVAL (target_val) = lval_memory;
  set_value_address (target_val, addr);

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
   DW_TAG_GNU_call_site_parameter.

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
	       _("DWARF-2 expression error: DW_OP_GNU_entry_value is supported "
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
};

/* Allocate a closure for a value formed from separately-described
   PIECES.  */

static struct piece_closure *
allocate_piece_closure (struct dwarf2_per_cu_data *per_cu,
			int n_pieces, struct dwarf_expr_piece *pieces,
			int addr_size)
{
  struct piece_closure *c = XZALLOC (struct piece_closure);
  int i;

  c->refc = 1;
  c->per_cu = per_cu;
  c->n_pieces = n_pieces;
  c->addr_size = addr_size;
  c->pieces = XCALLOC (n_pieces, struct dwarf_expr_piece);

  memcpy (c->pieces, pieces, n_pieces * sizeof (struct dwarf_expr_piece));
  for (i = 0; i < n_pieces; ++i)
    if (c->pieces[i].location == DWARF_VALUE_STACK)
      value_incref (c->pieces[i].v.value);

  return c;
}

/* The lowest-level function to extract bits from a byte buffer.
   SOURCE is the buffer.  It is updated if we read to the end of a
   byte.
   SOURCE_OFFSET_BITS is the offset of the first bit to read.  It is
   updated to reflect the number of bits actually read.
   NBITS is the number of bits we want to read.  It is updated to
   reflect the number of bits actually read.  This function may read
   fewer bits.
   BITS_BIG_ENDIAN is taken directly from gdbarch.
   This function returns the extracted bits.  */

static unsigned int
extract_bits_primitive (const gdb_byte **source,
			unsigned int *source_offset_bits,
			int *nbits, int bits_big_endian)
{
  unsigned int avail, mask, datum;

  gdb_assert (*source_offset_bits < 8);

  avail = 8 - *source_offset_bits;
  if (avail > *nbits)
    avail = *nbits;

  mask = (1 << avail) - 1;
  datum = **source;
  if (bits_big_endian)
    datum >>= 8 - (*source_offset_bits + *nbits);
  else
    datum >>= *source_offset_bits;
  datum &= mask;

  *nbits -= avail;
  *source_offset_bits += avail;
  if (*source_offset_bits >= 8)
    {
      *source_offset_bits -= 8;
      ++*source;
    }

  return datum;
}

/* Extract some bits from a source buffer and move forward in the
   buffer.
   
   SOURCE is the source buffer.  It is updated as bytes are read.
   SOURCE_OFFSET_BITS is the offset into SOURCE.  It is updated as
   bits are read.
   NBITS is the number of bits to read.
   BITS_BIG_ENDIAN is taken directly from gdbarch.
   
   This function returns the bits that were read.  */

static unsigned int
extract_bits (const gdb_byte **source, unsigned int *source_offset_bits,
	      int nbits, int bits_big_endian)
{
  unsigned int datum;

  gdb_assert (nbits > 0 && nbits <= 8);

  datum = extract_bits_primitive (source, source_offset_bits, &nbits,
				  bits_big_endian);
  if (nbits > 0)
    {
      unsigned int more;

      more = extract_bits_primitive (source, source_offset_bits, &nbits,
				     bits_big_endian);
      if (bits_big_endian)
	datum <<= nbits;
      else
	more <<= nbits;
      datum |= more;
    }

  return datum;
}

/* Write some bits into a buffer and move forward in the buffer.
   
   DATUM is the bits to write.  The low-order bits of DATUM are used.
   DEST is the destination buffer.  It is updated as bytes are
   written.
   DEST_OFFSET_BITS is the bit offset in DEST at which writing is
   done.
   NBITS is the number of valid bits in DATUM.
   BITS_BIG_ENDIAN is taken directly from gdbarch.  */

static void
insert_bits (unsigned int datum,
	     gdb_byte *dest, unsigned int dest_offset_bits,
	     int nbits, int bits_big_endian)
{
  unsigned int mask;

  gdb_assert (dest_offset_bits + nbits <= 8);

  mask = (1 << nbits) - 1;
  if (bits_big_endian)
    {
      datum <<= 8 - (dest_offset_bits + nbits);
      mask <<= 8 - (dest_offset_bits + nbits);
    }
  else
    {
      datum <<= dest_offset_bits;
      mask <<= dest_offset_bits;
    }

  gdb_assert ((datum & ~mask) == 0);

  *dest = (*dest & ~mask) | datum;
}

/* Copy bits from a source to a destination.
   
   DEST is where the bits should be written.
   DEST_OFFSET_BITS is the bit offset into DEST.
   SOURCE is the source of bits.
   SOURCE_OFFSET_BITS is the bit offset into SOURCE.
   BIT_COUNT is the number of bits to copy.
   BITS_BIG_ENDIAN is taken directly from gdbarch.  */

static void
copy_bitwise (gdb_byte *dest, unsigned int dest_offset_bits,
	      const gdb_byte *source, unsigned int source_offset_bits,
	      unsigned int bit_count,
	      int bits_big_endian)
{
  unsigned int dest_avail;
  int datum;

  /* Reduce everything to byte-size pieces.  */
  dest += dest_offset_bits / 8;
  dest_offset_bits %= 8;
  source += source_offset_bits / 8;
  source_offset_bits %= 8;

  dest_avail = 8 - dest_offset_bits % 8;

  /* See if we can fill the first destination byte.  */
  if (dest_avail < bit_count)
    {
      datum = extract_bits (&source, &source_offset_bits, dest_avail,
			    bits_big_endian);
      insert_bits (datum, dest, dest_offset_bits, dest_avail, bits_big_endian);
      ++dest;
      dest_offset_bits = 0;
      bit_count -= dest_avail;
    }

  /* Now, either DEST_OFFSET_BITS is byte-aligned, or we have fewer
     than 8 bits remaining.  */
  gdb_assert (dest_offset_bits % 8 == 0 || bit_count < 8);
  for (; bit_count >= 8; bit_count -= 8)
    {
      datum = extract_bits (&source, &source_offset_bits, 8, bits_big_endian);
      *dest++ = (gdb_byte) datum;
    }

  /* Finally, we may have a few leftover bits.  */
  gdb_assert (bit_count <= 8 - dest_offset_bits % 8);
  if (bit_count > 0)
    {
      datum = extract_bits (&source, &source_offset_bits, bit_count,
			    bits_big_endian);
      insert_bits (datum, dest, dest_offset_bits, bit_count, bits_big_endian);
    }
}

static void
read_pieced_value (struct value *v)
{
  int i;
  long offset = 0;
  ULONGEST bits_to_skip;
  gdb_byte *contents;
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (v);
  struct frame_info *frame = frame_find_by_id (VALUE_FRAME_ID (v));
  size_t type_len;
  size_t buffer_size = 0;
  gdb_byte *buffer = NULL;
  struct cleanup *cleanup;
  int bits_big_endian
    = gdbarch_bits_big_endian (get_type_arch (value_type (v)));

  if (value_type (v) != value_enclosing_type (v))
    internal_error (__FILE__, __LINE__,
		    _("Should not be able to create a lazy value with "
		      "an enclosing type"));

  cleanup = make_cleanup (free_current_contents, &buffer);

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
	  buffer = xrealloc (buffer, buffer_size);
	}
      intermediate_buffer = buffer;

      /* Copy from the source to DEST_BUFFER.  */
      switch (p->location)
	{
	case DWARF_VALUE_REGISTER:
	  {
	    struct gdbarch *arch = get_frame_arch (frame);
	    int gdb_regnum = gdbarch_dwarf2_reg_to_regnum (arch, p->v.regno);
	    int reg_offset = source_offset;

	    if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG
		&& this_size < register_size (arch, gdb_regnum))
	      {
		/* Big-endian, and we want less than full size.  */
		reg_offset = register_size (arch, gdb_regnum) - this_size;
		/* We want the lower-order THIS_SIZE_BITS of the bytes
		   we extract from the register.  */
		source_offset_bits += 8 * this_size - this_size_bits;
	      }

	    if (gdb_regnum != -1)
	      {
		int optim, unavail;

		if (!get_frame_register_bytes (frame, gdb_regnum, reg_offset,
					       this_size, buffer,
					       &optim, &unavail))
		  {
		    /* Just so garbage doesn't ever shine through.  */
		    memset (buffer, 0, this_size);

		    if (optim)
		      set_value_optimized_out (v, 1);
		    if (unavail)
		      mark_value_bits_unavailable (v, offset, this_size_bits);
		  }
	      }
	    else
	      {
		error (_("Unable to access DWARF register number %s"),
		       paddress (arch, p->v.regno));
	      }
	  }
	  break;

	case DWARF_VALUE_MEMORY:
	  read_value_memory (v, offset,
			     p->v.mem.in_stack_memory,
			     p->v.mem.addr + source_offset,
			     buffer, this_size);
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
	  set_value_optimized_out (v, 1);
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

  do_cleanups (cleanup);
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
  struct frame_info *frame = frame_find_by_id (VALUE_FRAME_ID (to));
  size_t type_len;
  size_t buffer_size = 0;
  gdb_byte *buffer = NULL;
  struct cleanup *cleanup;
  int bits_big_endian
    = gdbarch_bits_big_endian (get_type_arch (value_type (to)));

  if (frame == NULL)
    {
      set_value_optimized_out (to, 1);
      return;
    }

  cleanup = make_cleanup (free_current_contents, &buffer);

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
	      buffer = xrealloc (buffer, buffer_size);
	    }
	  source_buffer = buffer;
	  need_bitwise = 1;
	}

      switch (p->location)
	{
	case DWARF_VALUE_REGISTER:
	  {
	    struct gdbarch *arch = get_frame_arch (frame);
	    int gdb_regnum = gdbarch_dwarf2_reg_to_regnum (arch, p->v.regno);
	    int reg_offset = dest_offset;

	    if (gdbarch_byte_order (arch) == BFD_ENDIAN_BIG
		&& this_size <= register_size (arch, gdb_regnum))
	      /* Big-endian, and we want less than full size.  */
	      reg_offset = register_size (arch, gdb_regnum) - this_size;

	    if (gdb_regnum != -1)
	      {
		if (need_bitwise)
		  {
		    int optim, unavail;

		    if (!get_frame_register_bytes (frame, gdb_regnum, reg_offset,
						   this_size, buffer,
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
		    copy_bitwise (buffer, dest_offset_bits,
				  contents, source_offset_bits,
				  this_size_bits,
				  bits_big_endian);
		  }

		put_frame_register_bytes (frame, gdb_regnum, reg_offset, 
					  this_size, source_buffer);
	      }
	    else
	      {
		error (_("Unable to write to DWARF register number %s"),
		       paddress (arch, p->v.regno));
	      }
	  }
	  break;
	case DWARF_VALUE_MEMORY:
	  if (need_bitwise)
	    {
	      /* Only the first and last bytes can possibly have any
		 bits reused.  */
	      read_memory (p->v.mem.addr + dest_offset, buffer, 1);
	      read_memory (p->v.mem.addr + dest_offset + this_size - 1,
			   buffer + this_size - 1, 1);
	      copy_bitwise (buffer, dest_offset_bits,
			    contents, source_offset_bits,
			    this_size_bits,
			    bits_big_endian);
	    }

	  write_memory (p->v.mem.addr + dest_offset,
			source_buffer, this_size);
	  break;
	default:
	  set_value_optimized_out (to, 1);
	  break;
	}
      offset += this_size_bits;
    }

  do_cleanups (cleanup);
}

/* A helper function that checks bit validity in a pieced value.
   CHECK_FOR indicates the kind of validity checking.
   DWARF_VALUE_MEMORY means to check whether any bit is valid.
   DWARF_VALUE_OPTIMIZED_OUT means to check whether any bit is
   optimized out.
   DWARF_VALUE_IMPLICIT_POINTER means to check whether the bits are an
   implicit pointer.  */

static int
check_pieced_value_bits (const struct value *value, int bit_offset,
			 int bit_length,
			 enum dwarf_value_location check_for)
{
  struct piece_closure *c
    = (struct piece_closure *) value_computed_closure (value);
  int i;
  int validity = (check_for == DWARF_VALUE_MEMORY
		  || check_for == DWARF_VALUE_IMPLICIT_POINTER);

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

      if (check_for == DWARF_VALUE_IMPLICIT_POINTER)
	{
	  if (p->location != DWARF_VALUE_IMPLICIT_POINTER)
	    return 0;
	}
      else if (p->location == DWARF_VALUE_OPTIMIZED_OUT
	       || p->location == DWARF_VALUE_IMPLICIT_POINTER)
	{
	  if (validity)
	    return 0;
	}
      else
	{
	  if (!validity)
	    return 1;
	}
    }

  return validity;
}

static int
check_pieced_value_validity (const struct value *value, int bit_offset,
			     int bit_length)
{
  return check_pieced_value_bits (value, bit_offset, bit_length,
				  DWARF_VALUE_MEMORY);
}

static int
check_pieced_value_invalid (const struct value *value)
{
  return check_pieced_value_bits (value, 0,
				  8 * TYPE_LENGTH (value_type (value)),
				  DWARF_VALUE_OPTIMIZED_OUT);
}

/* An implementation of an lval_funcs method to see whether a value is
   a synthetic pointer.  */

static int
check_pieced_synthetic_pointer (const struct value *value, int bit_offset,
				int bit_length)
{
  return check_pieced_value_bits (value, bit_offset, bit_length,
				  DWARF_VALUE_IMPLICIT_POINTER);
}

/* A wrapper function for get_frame_address_in_block.  */

static CORE_ADDR
get_frame_address_in_block_wrapper (void *baton)
{
  return get_frame_address_in_block (baton);
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
  int i, bit_offset, bit_length;
  struct dwarf_expr_piece *piece = NULL;
  LONGEST byte_offset;

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
	error (_("Invalid use of DW_OP_GNU_implicit_pointer"));

      piece = p;
      break;
    }

  frame = get_selected_frame (_("No frame selected."));

  /* This is an offset requested by GDB, such as value subscripts.
     However, due to how synthetic pointers are implemented, this is
     always presented to us as a pointer type.  This means we have to
     sign-extend it manually as appropriate.  */
  byte_offset = value_as_address (value);
  if (TYPE_LENGTH (value_type (value)) < sizeof (LONGEST))
    byte_offset = gdb_sign_extend (byte_offset,
				   8 * TYPE_LENGTH (value_type (value)));
  byte_offset += piece->v.ptr.offset;

  gdb_assert (piece);
  baton
    = dwarf2_fetch_die_loc_sect_off (piece->v.ptr.die, c->per_cu,
				     get_frame_address_in_block_wrapper,
				     frame);

  if (baton.data != NULL)
    return dwarf2_evaluate_loc_desc_full (TYPE_TARGET_TYPE (type), frame,
					  baton.data, baton.size, baton.per_cu,
					  byte_offset);

  {
    struct obstack temp_obstack;
    struct cleanup *cleanup;
    const gdb_byte *bytes;
    LONGEST len;
    struct value *result;

    obstack_init (&temp_obstack);
    cleanup = make_cleanup_obstack_free (&temp_obstack);

    bytes = dwarf2_fetch_constant_bytes (piece->v.ptr.die, c->per_cu,
					 &temp_obstack, &len);
    if (bytes == NULL)
      result = allocate_optimized_out_value (TYPE_TARGET_TYPE (type));
    else
      {
	if (byte_offset < 0
	    || byte_offset + TYPE_LENGTH (TYPE_TARGET_TYPE (type)) > len)
	  invalid_synthetic_pointer ();
	bytes += byte_offset;
	result = value_from_contents (TYPE_TARGET_TYPE (type), bytes);
      }

    do_cleanups (cleanup);
    return result;
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
  check_pieced_value_validity,
  check_pieced_value_invalid,
  indirect_pieced_value,
  NULL,	/* coerce_ref */
  check_pieced_synthetic_pointer,
  copy_pieced_value_closure,
  free_pieced_value_closure
};

/* Virtual method table for dwarf2_evaluate_loc_desc_full below.  */

static const struct dwarf_expr_context_funcs dwarf_expr_ctx_funcs =
{
  dwarf_expr_read_addr_from_reg,
  dwarf_expr_get_reg_value,
  dwarf_expr_read_mem,
  dwarf_expr_frame_base,
  dwarf_expr_frame_cfa,
  dwarf_expr_frame_pc,
  dwarf_expr_tls_address,
  dwarf_expr_dwarf_call,
  dwarf_expr_get_base_type,
  dwarf_expr_push_dwarf_reg_entry_value,
  dwarf_expr_get_addr_index
};

/* Evaluate a location description, starting at DATA and with length
   SIZE, to find the current location of variable of TYPE in the
   context of FRAME.  BYTE_OFFSET is applied after the contents are
   computed.  */

static struct value *
dwarf2_evaluate_loc_desc_full (struct type *type, struct frame_info *frame,
			       const gdb_byte *data, size_t size,
			       struct dwarf2_per_cu_data *per_cu,
			       LONGEST byte_offset)
{
  struct value *retval;
  struct dwarf_expr_baton baton;
  struct dwarf_expr_context *ctx;
  struct cleanup *old_chain, *value_chain;
  struct objfile *objfile = dwarf2_per_cu_objfile (per_cu);
  volatile struct gdb_exception ex;

  if (byte_offset < 0)
    invalid_synthetic_pointer ();

  if (size == 0)
    return allocate_optimized_out_value (type);

  baton.frame = frame;
  baton.per_cu = per_cu;

  ctx = new_dwarf_expr_context ();
  old_chain = make_cleanup_free_dwarf_expr_context (ctx);
  value_chain = make_cleanup_value_free_to_mark (value_mark ());

  ctx->gdbarch = get_objfile_arch (objfile);
  ctx->addr_size = dwarf2_per_cu_addr_size (per_cu);
  ctx->ref_addr_size = dwarf2_per_cu_ref_addr_size (per_cu);
  ctx->offset = dwarf2_per_cu_text_offset (per_cu);
  ctx->baton = &baton;
  ctx->funcs = &dwarf_expr_ctx_funcs;

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      dwarf_expr_eval (ctx, data, size);
    }
  if (ex.reason < 0)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  do_cleanups (old_chain);
	  retval = allocate_value (type);
	  mark_value_bytes_unavailable (retval, 0, TYPE_LENGTH (type));
	  return retval;
	}
      else if (ex.error == NO_ENTRY_VALUE_ERROR)
	{
	  if (entry_values_debug)
	    exception_print (gdb_stdout, ex);
	  do_cleanups (old_chain);
	  return allocate_optimized_out_value (type);
	}
      else
	throw_exception (ex);
    }

  if (ctx->num_pieces > 0)
    {
      struct piece_closure *c;
      struct frame_id frame_id = get_frame_id (frame);
      ULONGEST bit_size = 0;
      int i;

      for (i = 0; i < ctx->num_pieces; ++i)
	bit_size += ctx->pieces[i].size;
      if (8 * (byte_offset + TYPE_LENGTH (type)) > bit_size)
	invalid_synthetic_pointer ();

      c = allocate_piece_closure (per_cu, ctx->num_pieces, ctx->pieces,
				  ctx->addr_size);
      /* We must clean up the value chain after creating the piece
	 closure but before allocating the result.  */
      do_cleanups (value_chain);
      retval = allocate_computed_value (type, &pieced_value_funcs, c);
      VALUE_FRAME_ID (retval) = frame_id;
      set_value_offset (retval, byte_offset);
    }
  else
    {
      switch (ctx->location)
	{
	case DWARF_VALUE_REGISTER:
	  {
	    struct gdbarch *arch = get_frame_arch (frame);
	    int dwarf_regnum
	      = longest_to_int (value_as_long (dwarf_expr_fetch (ctx, 0)));
	    int gdb_regnum = gdbarch_dwarf2_reg_to_regnum (arch, dwarf_regnum);

	    if (byte_offset != 0)
	      error (_("cannot use offset on synthetic pointer to register"));
	    do_cleanups (value_chain);
	   if (gdb_regnum == -1)
	      error (_("Unable to access DWARF register number %d"),
		     dwarf_regnum);
	   retval = value_from_register (type, gdb_regnum, frame);
	   if (value_optimized_out (retval))
	     {
	       /* This means the register has undefined value / was
		  not saved.  As we're computing the location of some
		  variable etc. in the program, not a value for
		  inspecting a register ($pc, $sp, etc.), return a
		  generic optimized out value instead, so that we show
		  <optimized out> instead of <not saved>.  */
	       do_cleanups (value_chain);
	       retval = allocate_optimized_out_value (type);
	     }
	  }
	  break;

	case DWARF_VALUE_MEMORY:
	  {
	    CORE_ADDR address = dwarf_expr_fetch_address (ctx, 0);
	    int in_stack_memory = dwarf_expr_fetch_in_stack_memory (ctx, 0);

	    do_cleanups (value_chain);
	    retval = value_at_lazy (type, address + byte_offset);
	    if (in_stack_memory)
	      set_value_stack (retval, 1);
	  }
	  break;

	case DWARF_VALUE_STACK:
	  {
	    struct value *value = dwarf_expr_fetch (ctx, 0);
	    gdb_byte *contents;
	    const gdb_byte *val_bytes;
	    size_t n = TYPE_LENGTH (value_type (value));

	    if (byte_offset + TYPE_LENGTH (type) > n)
	      invalid_synthetic_pointer ();

	    val_bytes = value_contents_all (value);
	    val_bytes += byte_offset;
	    n -= byte_offset;

	    /* Preserve VALUE because we are going to free values back
	       to the mark, but we still need the value contents
	       below.  */
	    value_incref (value);
	    do_cleanups (value_chain);
	    make_cleanup_value_free (value);

	    retval = allocate_value (type);
	    contents = value_contents_raw (retval);
	    if (n > TYPE_LENGTH (type))
	      {
		struct gdbarch *objfile_gdbarch = get_objfile_arch (objfile);

		if (gdbarch_byte_order (objfile_gdbarch) == BFD_ENDIAN_BIG)
		  val_bytes += n - TYPE_LENGTH (type);
		n = TYPE_LENGTH (type);
	      }
	    memcpy (contents, val_bytes, n);
	  }
	  break;

	case DWARF_VALUE_LITERAL:
	  {
	    bfd_byte *contents;
	    const bfd_byte *ldata;
	    size_t n = ctx->len;

	    if (byte_offset + TYPE_LENGTH (type) > n)
	      invalid_synthetic_pointer ();

	    do_cleanups (value_chain);
	    retval = allocate_value (type);
	    contents = value_contents_raw (retval);

	    ldata = ctx->data + byte_offset;
	    n -= byte_offset;

	    if (n > TYPE_LENGTH (type))
	      {
		struct gdbarch *objfile_gdbarch = get_objfile_arch (objfile);

		if (gdbarch_byte_order (objfile_gdbarch) == BFD_ENDIAN_BIG)
		  ldata += n - TYPE_LENGTH (type);
		n = TYPE_LENGTH (type);
	      }
	    memcpy (contents, ldata, n);
	  }
	  break;

	case DWARF_VALUE_OPTIMIZED_OUT:
	  do_cleanups (value_chain);
	  retval = allocate_optimized_out_value (type);
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

  set_value_initialized (retval, ctx->initialized);

  do_cleanups (old_chain);

  return retval;
}

/* The exported interface to dwarf2_evaluate_loc_desc_full; it always
   passes 0 as the byte_offset.  */

struct value *
dwarf2_evaluate_loc_desc (struct type *type, struct frame_info *frame,
			  const gdb_byte *data, size_t size,
			  struct dwarf2_per_cu_data *per_cu)
{
  return dwarf2_evaluate_loc_desc_full (type, frame, data, size, per_cu, 0);
}


/* Helper functions and baton for dwarf2_loc_desc_needs_frame.  */

struct needs_frame_baton
{
  int needs_frame;
  struct dwarf2_per_cu_data *per_cu;
};

/* Reads from registers do require a frame.  */
static CORE_ADDR
needs_frame_read_addr_from_reg (void *baton, int regnum)
{
  struct needs_frame_baton *nf_baton = baton;

  nf_baton->needs_frame = 1;
  return 1;
}

/* struct dwarf_expr_context_funcs' "get_reg_value" callback:
   Reads from registers do require a frame.  */

static struct value *
needs_frame_get_reg_value (void *baton, struct type *type, int regnum)
{
  struct needs_frame_baton *nf_baton = baton;

  nf_baton->needs_frame = 1;
  return value_zero (type, not_lval);
}

/* Reads from memory do not require a frame.  */
static void
needs_frame_read_mem (void *baton, gdb_byte *buf, CORE_ADDR addr, size_t len)
{
  memset (buf, 0, len);
}

/* Frame-relative accesses do require a frame.  */
static void
needs_frame_frame_base (void *baton, const gdb_byte **start, size_t * length)
{
  static gdb_byte lit0 = DW_OP_lit0;
  struct needs_frame_baton *nf_baton = baton;

  *start = &lit0;
  *length = 1;

  nf_baton->needs_frame = 1;
}

/* CFA accesses require a frame.  */

static CORE_ADDR
needs_frame_frame_cfa (void *baton)
{
  struct needs_frame_baton *nf_baton = baton;

  nf_baton->needs_frame = 1;
  return 1;
}

/* Thread-local accesses do require a frame.  */
static CORE_ADDR
needs_frame_tls_address (void *baton, CORE_ADDR offset)
{
  struct needs_frame_baton *nf_baton = baton;

  nf_baton->needs_frame = 1;
  return 1;
}

/* Helper interface of per_cu_dwarf_call for dwarf2_loc_desc_needs_frame.  */

static void
needs_frame_dwarf_call (struct dwarf_expr_context *ctx, cu_offset die_offset)
{
  struct needs_frame_baton *nf_baton = ctx->baton;

  per_cu_dwarf_call (ctx, die_offset, nf_baton->per_cu,
		     ctx->funcs->get_frame_pc, ctx->baton);
}

/* DW_OP_GNU_entry_value accesses require a caller, therefore a frame.  */

static void
needs_dwarf_reg_entry_value (struct dwarf_expr_context *ctx,
			     enum call_site_parameter_kind kind,
			     union call_site_parameter_u kind_u, int deref_size)
{
  struct needs_frame_baton *nf_baton = ctx->baton;

  nf_baton->needs_frame = 1;

  /* The expression may require some stub values on DWARF stack.  */
  dwarf_expr_push_address (ctx, 0, 0);
}

/* DW_OP_GNU_addr_index doesn't require a frame.  */

static CORE_ADDR
needs_get_addr_index (void *baton, unsigned int index)
{
  /* Nothing to do.  */
  return 1;
}

/* Virtual method table for dwarf2_loc_desc_needs_frame below.  */

static const struct dwarf_expr_context_funcs needs_frame_ctx_funcs =
{
  needs_frame_read_addr_from_reg,
  needs_frame_get_reg_value,
  needs_frame_read_mem,
  needs_frame_frame_base,
  needs_frame_frame_cfa,
  needs_frame_frame_cfa,	/* get_frame_pc */
  needs_frame_tls_address,
  needs_frame_dwarf_call,
  NULL,				/* get_base_type */
  needs_dwarf_reg_entry_value,
  needs_get_addr_index
};

/* Return non-zero iff the location expression at DATA (length SIZE)
   requires a frame to evaluate.  */

static int
dwarf2_loc_desc_needs_frame (const gdb_byte *data, size_t size,
			     struct dwarf2_per_cu_data *per_cu)
{
  struct needs_frame_baton baton;
  struct dwarf_expr_context *ctx;
  int in_reg;
  struct cleanup *old_chain;
  struct objfile *objfile = dwarf2_per_cu_objfile (per_cu);

  baton.needs_frame = 0;
  baton.per_cu = per_cu;

  ctx = new_dwarf_expr_context ();
  old_chain = make_cleanup_free_dwarf_expr_context (ctx);
  make_cleanup_value_free_to_mark (value_mark ());

  ctx->gdbarch = get_objfile_arch (objfile);
  ctx->addr_size = dwarf2_per_cu_addr_size (per_cu);
  ctx->ref_addr_size = dwarf2_per_cu_ref_addr_size (per_cu);
  ctx->offset = dwarf2_per_cu_text_offset (per_cu);
  ctx->baton = &baton;
  ctx->funcs = &needs_frame_ctx_funcs;

  dwarf_expr_eval (ctx, data, size);

  in_reg = ctx->location == DWARF_VALUE_REGISTER;

  if (ctx->num_pieces > 0)
    {
      int i;

      /* If the location has several pieces, and any of them are in
         registers, then we will need a frame to fetch them from.  */
      for (i = 0; i < ctx->num_pieces; i++)
        if (ctx->pieces[i].location == DWARF_VALUE_REGISTER)
          in_reg = 1;
    }

  do_cleanups (old_chain);

  return baton.needs_frame || in_reg;
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

/* A helper function to convert a DWARF register to an arch register.
   ARCH is the architecture.
   DWARF_REG is the register.
   This will throw an exception if the DWARF register cannot be
   translated to an architecture register.  */

static int
translate_register (struct gdbarch *arch, int dwarf_reg)
{
  int reg = gdbarch_dwarf2_reg_to_regnum (arch, dwarf_reg);
  if (reg == -1)
    error (_("Unable to access DWARF register number %d"), dwarf_reg);
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
  struct agent_expr *expr = baton;

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
  struct cleanup *cleanups;
  int i, *offsets;
  VEC(int) *dw_labels = NULL, *patches = NULL;
  const gdb_byte * const base = op_ptr;
  const gdb_byte *previous_piece = op_ptr;
  enum bfd_endian byte_order = gdbarch_byte_order (arch);
  ULONGEST bits_collected = 0;
  unsigned int addr_size_bits = 8 * addr_size;
  int bits_big_endian = gdbarch_bits_big_endian (arch);

  offsets = xmalloc ((op_end - op_ptr) * sizeof (int));
  cleanups = make_cleanup (xfree, offsets);

  for (i = 0; i < op_end - op_ptr; ++i)
    offsets[i] = -1;

  make_cleanup (VEC_cleanup (int), &dw_labels);
  make_cleanup (VEC_cleanup (int), &patches);

  /* By default we are making an address.  */
  loc->kind = axs_lvalue_memory;

  while (op_ptr < op_end)
    {
      enum dwarf_location_atom op = *op_ptr;
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
	  loc->u.reg = translate_register (arch, op - DW_OP_reg0);
	  loc->kind = axs_lvalue_register;
	  break;

	case DW_OP_regx:
	  op_ptr = safe_read_uleb128 (op_ptr, op_end, &reg);
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  loc->u.reg = translate_register (arch, reg);
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
	  i = translate_register (arch, op - DW_OP_breg0);
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
	    i = translate_register (arch, reg);
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
	    struct block *b;
	    struct symbol *framefunc;

	    b = block_for_pc (expr->scope);

	    if (!b)
	      error (_("No block found for address"));

	    framefunc = block_linkage_function (b);

	    if (!framefunc)
	      error (_("No function found for block"));

	    dwarf_expr_frame_base_1 (framefunc, expr->scope,
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
	  dwarf2_compile_cfa_to_ax (expr, loc, arch, expr->scope, per_cu);
	  loc->kind = axs_lvalue_memory;
	  break;

	case DW_OP_GNU_push_tls_address:
	  unimplemented (op);
	  break;

	case DW_OP_skip:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  i = ax_goto (expr, aop_goto);
	  VEC_safe_push (int, dw_labels, op_ptr + offset - base);
	  VEC_safe_push (int, patches, i);
	  break;

	case DW_OP_bra:
	  offset = extract_signed_integer (op_ptr, 2, byte_order);
	  op_ptr += 2;
	  /* Zero extend the operand.  */
	  ax_zero_ext (expr, addr_size_bits);
	  i = ax_goto (expr, aop_if_goto);
	  VEC_safe_push (int, dw_labels, op_ptr + offset - base);
	  VEC_safe_push (int, patches, i);
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
	    cu_offset offset;

	    uoffset = extract_unsigned_integer (op_ptr, size, byte_order);
	    op_ptr += size;

	    offset.cu_off = uoffset;
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
  for (i = 0; i < VEC_length (int, patches); ++i)
    {
      int targ = offsets[VEC_index (int, dw_labels, i)];
      if (targ == -1)
	internal_error (__FILE__, __LINE__, _("invalid label"));
      ax_label (expr, VEC_index (int, patches, i), targ);
    }

  do_cleanups (cleanups);
}


/* Return the value of SYMBOL in FRAME using the DWARF-2 expression
   evaluator to calculate the location.  */
static struct value *
locexpr_read_variable (struct symbol *symbol, struct frame_info *frame)
{
  struct dwarf2_locexpr_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
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
  struct dwarf2_locexpr_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);

  return value_of_dwarf_block_entry (SYMBOL_TYPE (symbol), frame, dlbaton->data,
				     dlbaton->size);
}

/* Return non-zero iff we need a frame to evaluate SYMBOL.  */
static int
locexpr_read_needs_frame (struct symbol *symbol)
{
  struct dwarf2_locexpr_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);

  return dwarf2_loc_desc_needs_frame (dlbaton->data, dlbaton->size,
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

  regnum = gdbarch_dwarf2_reg_to_regnum (gdbarch, dwarf_regnum);
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
      struct block *b;
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

      dwarf_expr_frame_base_1 (framefunc, addr, &base_data, &base_size);

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
	   && data[1 + addr_size] == DW_OP_GNU_push_tls_address
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
	   && data[1 + leb128_size] == DW_OP_GNU_push_tls_address
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
      enum dwarf_location_atom op = *data++;
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

	case DW_OP_GNU_deref_type:
	  {
	    int addr_size = *data++;
	    cu_offset offset;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &ul);
	    offset.cu_off = ul;
	    type = dwarf2_get_die_type (offset, per_cu);
	    fprintf_filtered (stream, "<");
	    type_print (type, "", stream, -1);
	    fprintf_filtered (stream, " [0x%s]> %d", phex_nz (offset.cu_off, 0),
			      addr_size);
	  }
	  break;

	case DW_OP_GNU_const_type:
	  {
	    cu_offset type_die;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &ul);
	    type_die.cu_off = ul;
	    type = dwarf2_get_die_type (type_die, per_cu);
	    fprintf_filtered (stream, "<");
	    type_print (type, "", stream, -1);
	    fprintf_filtered (stream, " [0x%s]>", phex_nz (type_die.cu_off, 0));
	  }
	  break;

	case DW_OP_GNU_regval_type:
	  {
	    uint64_t reg;
	    cu_offset type_die;
	    struct type *type;

	    data = safe_read_uleb128 (data, end, &reg);
	    data = safe_read_uleb128 (data, end, &ul);
	    type_die.cu_off = ul;

	    type = dwarf2_get_die_type (type_die, per_cu);
	    fprintf_filtered (stream, "<");
	    type_print (type, "", stream, -1);
	    fprintf_filtered (stream, " [0x%s]> [$%s]",
			      phex_nz (type_die.cu_off, 0),
			      locexpr_regname (arch, reg));
	  }
	  break;

	case DW_OP_GNU_convert:
	case DW_OP_GNU_reinterpret:
	  {
	    cu_offset type_die;

	    data = safe_read_uleb128 (data, end, &ul);
	    type_die.cu_off = ul;

	    if (type_die.cu_off == 0)
	      fprintf_filtered (stream, "<0>");
	    else
	      {
		struct type *type;

		type = dwarf2_get_die_type (type_die, per_cu);
		fprintf_filtered (stream, "<");
		type_print (type, "", stream, -1);
		fprintf_filtered (stream, " [0x%s]>", phex_nz (type_die.cu_off, 0));
	      }
	  }
	  break;

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

      if (!dwarf2_always_disassemble)
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
					       dwarf2_always_disassemble,
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
  struct dwarf2_locexpr_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
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
  struct dwarf2_locexpr_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);

  if (dlbaton->size == 0)
    value->optimized_out = 1;
  else
    dwarf2_compile_expr_to_ax (ax, value, gdbarch, addr_size,
			       dlbaton->data, dlbaton->data + dlbaton->size,
			       dlbaton->per_cu);
}

/* The set of location functions used with the DWARF-2 expression
   evaluator.  */
const struct symbol_computed_ops dwarf2_locexpr_funcs = {
  locexpr_read_variable,
  locexpr_read_variable_at_entry,
  locexpr_read_needs_frame,
  locexpr_describe_location,
  0,	/* location_has_loclist */
  locexpr_tracepoint_var_ref
};


/* Wrapper functions for location lists.  These generally find
   the appropriate location expression and call something above.  */

/* Return the value of SYMBOL in FRAME using the DWARF-2 expression
   evaluator to calculate the location.  */
static struct value *
loclist_read_variable (struct symbol *symbol, struct frame_info *frame)
{
  struct dwarf2_loclist_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
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
  struct dwarf2_loclist_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
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

/* Return non-zero iff we need a frame to evaluate SYMBOL.  */
static int
loclist_read_needs_frame (struct symbol *symbol)
{
  /* If there's a location list, then assume we need to have a frame
     to choose the appropriate location expression.  With tracking of
     global variables this is not necessarily true, but such tracking
     is disabled in GCC at the moment until we figure out how to
     represent it.  */

  return 1;
}

/* Print a natural-language description of SYMBOL to STREAM.  This
   version applies when there is a list of different locations, each
   with a specified address range.  */

static void
loclist_describe_location (struct symbol *symbol, CORE_ADDR addr,
			   struct ui_file *stream)
{
  struct dwarf2_loclist_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
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
  struct dwarf2_loclist_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
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

/* The set of location functions used with the DWARF-2 expression
   evaluator and location lists.  */
const struct symbol_computed_ops dwarf2_loclist_funcs = {
  loclist_read_variable,
  loclist_read_variable_at_entry,
  loclist_read_needs_frame,
  loclist_describe_location,
  1,	/* location_has_loclist */
  loclist_tracepoint_var_ref
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
}
