/* DWARF 2 location expression support for GDB.

   Copyright (C) 2003, 2005, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

#include "dwarf2.h"
#include "dwarf2expr.h"
#include "dwarf2loc.h"
#include "dwarf2-frame.h"

#include "gdb_string.h"
#include "gdb_assert.h"

extern int dwarf2_always_disassemble;

static void
dwarf_expr_frame_base_1 (struct symbol *framefunc, CORE_ADDR pc,
			 const gdb_byte **start, size_t *length);

static struct value *
dwarf2_evaluate_loc_desc_full (struct type *type, struct frame_info *frame,
			       const gdb_byte *data, unsigned short size,
			       struct dwarf2_per_cu_data *per_cu,
			       LONGEST byte_offset);

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
  CORE_ADDR low, high;
  const gdb_byte *loc_ptr, *buf_end;
  int length;
  struct objfile *objfile = dwarf2_per_cu_objfile (baton->per_cu);
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int addr_size = dwarf2_per_cu_addr_size (baton->per_cu);
  int signed_addr_p = bfd_get_sign_extend_vma (objfile->obfd);
  CORE_ADDR base_mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));
  /* Adjust base_address for relocatable objects.  */
  CORE_ADDR base_offset = dwarf2_per_cu_text_offset (baton->per_cu);
  CORE_ADDR base_address = baton->base_address + base_offset;

  loc_ptr = baton->data;
  buf_end = baton->data + baton->size;

  while (1)
    {
      if (buf_end - loc_ptr < 2 * addr_size)
	error (_("dwarf2_find_location_expression: "
		 "Corrupted DWARF expression."));

      if (signed_addr_p)
	low = extract_signed_integer (loc_ptr, addr_size, byte_order);
      else
	low = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
      loc_ptr += addr_size;

      if (signed_addr_p)
	high = extract_signed_integer (loc_ptr, addr_size, byte_order);
      else
	high = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
      loc_ptr += addr_size;

      /* A base-address-selection entry.  */
      if ((low & base_mask) == base_mask)
	{
	  base_address = high + base_offset;
	  continue;
	}

      /* An end-of-list entry.  */
      if (low == 0 && high == 0)
	return NULL;

      /* Otherwise, a location expression entry.  */
      low += base_address;
      high += base_address;

      length = extract_unsigned_integer (loc_ptr, 2, byte_order);
      loc_ptr += 2;

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
dwarf_expr_read_reg (void *baton, int dwarf_regnum)
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

  /* Use block_linkage_function, which returns a real (not inlined)
     function, instead of get_frame_function, which may return an
     inlined function.  */
  framefunc = block_linkage_function (get_frame_block (debaton->frame, NULL));

  /* If we found a frame-relative symbol then it was certainly within
     some function associated with a frame. If we can't find the frame,
     something has gone wrong.  */
  gdb_assert (framefunc != NULL);

  dwarf_expr_frame_base_1 (framefunc,
			   get_frame_address_in_block (debaton->frame),
			   start, length);
}

static void
dwarf_expr_frame_base_1 (struct symbol *framefunc, CORE_ADDR pc,
			 const gdb_byte **start, size_t *length)
{
  if (SYMBOL_LOCATION_BATON (framefunc) == NULL)
    *start = NULL;
  else if (SYMBOL_COMPUTED_OPS (framefunc) == &dwarf2_loclist_funcs)
    {
      struct dwarf2_loclist_baton *symbaton;

      symbaton = SYMBOL_LOCATION_BATON (framefunc);
      *start = dwarf2_find_location_expression (symbaton, length, pc);
    }
  else
    {
      struct dwarf2_locexpr_baton *symbaton;

      symbaton = SYMBOL_LOCATION_BATON (framefunc);
      if (symbaton != NULL)
	{
	  *length = symbaton->size;
	  *start = symbaton->data;
	}
      else
	*start = NULL;
    }

  if (*start == NULL)
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
per_cu_dwarf_call (struct dwarf_expr_context *ctx, size_t die_offset,
		   struct dwarf2_per_cu_data *per_cu,
		   CORE_ADDR (*get_frame_pc) (void *baton),
		   void *baton)
{
  struct dwarf2_locexpr_baton block;

  block = dwarf2_fetch_die_location_block (die_offset, per_cu,
					   get_frame_pc, baton);

  /* DW_OP_call_ref is currently not supported.  */
  gdb_assert (block.per_cu == per_cu);

  dwarf_expr_eval (ctx, block.data, block.size);
}

/* Helper interface of per_cu_dwarf_call for dwarf2_evaluate_loc_desc.  */

static void
dwarf_expr_dwarf_call (struct dwarf_expr_context *ctx, size_t die_offset)
{
  struct dwarf_expr_baton *debaton = ctx->baton;

  per_cu_dwarf_call (ctx, die_offset, debaton->per_cu,
		     ctx->get_frame_pc, ctx->baton);
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

  c->refc = 1;
  c->per_cu = per_cu;
  c->n_pieces = n_pieces;
  c->addr_size = addr_size;
  c->pieces = XCALLOC (n_pieces, struct dwarf_expr_piece);

  memcpy (c->pieces, pieces, n_pieces * sizeof (struct dwarf_expr_piece));

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
  char *buffer = NULL;
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
      if (this_size_bits > type_len - offset)
	this_size_bits = type_len - offset;
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
	    int gdb_regnum = gdbarch_dwarf2_reg_to_regnum (arch, p->v.value);
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
		      mark_value_bytes_unavailable (v, offset, this_size);
		  }
	      }
	    else
	      {
		error (_("Unable to access DWARF register number %s"),
		       paddress (arch, p->v.value));
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
	    struct gdbarch *gdbarch = get_type_arch (value_type (v));
	    size_t n = this_size;

	    if (n > c->addr_size - source_offset)
	      n = (c->addr_size >= source_offset
		   ? c->addr_size - source_offset
		   : 0);
	    if (n == 0)
	      {
		/* Nothing.  */
	      }
	    else if (source_offset == 0)
	      store_unsigned_integer (buffer, n,
				      gdbarch_byte_order (gdbarch),
				      p->v.value);
	    else
	      {
		gdb_byte bytes[sizeof (ULONGEST)];

		store_unsigned_integer (bytes, n + source_offset,
					gdbarch_byte_order (gdbarch),
					p->v.value);
		memcpy (buffer, bytes + source_offset, n);
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
  char *buffer = NULL;
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
	    int gdb_regnum = gdbarch_dwarf2_reg_to_regnum (arch, p->v.value);
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
			  error (_("Can't do read-modify-write to "
				   "update bitfield; containing word has been "
				   "optimized out"));
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
		       paddress (arch, p->v.value));
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
  struct value *result;
  LONGEST byte_offset;

  type = value_type (value);
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
  byte_offset = value_as_address (value);

  gdb_assert (piece);
  baton = dwarf2_fetch_die_location_block (piece->v.ptr.die, c->per_cu,
					   get_frame_address_in_block_wrapper,
					   frame);

  result = dwarf2_evaluate_loc_desc_full (TYPE_TARGET_TYPE (type), frame,
					  baton.data, baton.size, baton.per_cu,
					  byte_offset);

  return result;
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
      xfree (c->pieces);
      xfree (c);
    }
}

/* Functions for accessing a variable described by DW_OP_piece.  */
static struct lval_funcs pieced_value_funcs = {
  read_pieced_value,
  write_pieced_value,
  check_pieced_value_validity,
  check_pieced_value_invalid,
  indirect_pieced_value,
  check_pieced_synthetic_pointer,
  copy_pieced_value_closure,
  free_pieced_value_closure
};

/* Helper function which throws an error if a synthetic pointer is
   invalid.  */

static void
invalid_synthetic_pointer (void)
{
  error (_("access outside bounds of object "
	   "referenced via synthetic pointer"));
}

/* Evaluate a location description, starting at DATA and with length
   SIZE, to find the current location of variable of TYPE in the
   context of FRAME.  BYTE_OFFSET is applied after the contents are
   computed.  */

static struct value *
dwarf2_evaluate_loc_desc_full (struct type *type, struct frame_info *frame,
			       const gdb_byte *data, unsigned short size,
			       struct dwarf2_per_cu_data *per_cu,
			       LONGEST byte_offset)
{
  struct value *retval;
  struct dwarf_expr_baton baton;
  struct dwarf_expr_context *ctx;
  struct cleanup *old_chain;
  struct objfile *objfile = dwarf2_per_cu_objfile (per_cu);
  volatile struct gdb_exception ex;

  if (byte_offset < 0)
    invalid_synthetic_pointer ();

  if (size == 0)
    {
      retval = allocate_value (type);
      VALUE_LVAL (retval) = not_lval;
      set_value_optimized_out (retval, 1);
      return retval;
    }

  baton.frame = frame;
  baton.per_cu = per_cu;

  ctx = new_dwarf_expr_context ();
  old_chain = make_cleanup_free_dwarf_expr_context (ctx);

  ctx->gdbarch = get_objfile_arch (objfile);
  ctx->addr_size = dwarf2_per_cu_addr_size (per_cu);
  ctx->offset = dwarf2_per_cu_text_offset (per_cu);
  ctx->baton = &baton;
  ctx->read_reg = dwarf_expr_read_reg;
  ctx->read_mem = dwarf_expr_read_mem;
  ctx->get_frame_base = dwarf_expr_frame_base;
  ctx->get_frame_cfa = dwarf_expr_frame_cfa;
  ctx->get_frame_pc = dwarf_expr_frame_pc;
  ctx->get_tls_address = dwarf_expr_tls_address;
  ctx->dwarf_call = dwarf_expr_dwarf_call;

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      dwarf_expr_eval (ctx, data, size);
    }
  if (ex.reason < 0)
    {
      if (ex.error == NOT_AVAILABLE_ERROR)
	{
	  retval = allocate_value (type);
	  mark_value_bytes_unavailable (retval, 0, TYPE_LENGTH (type));
	  return retval;
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
	    ULONGEST dwarf_regnum = dwarf_expr_fetch (ctx, 0);
	    int gdb_regnum = gdbarch_dwarf2_reg_to_regnum (arch, dwarf_regnum);

	    if (byte_offset != 0)
	      error (_("cannot use offset on synthetic pointer to register"));
	    if (gdb_regnum != -1)
	      retval = value_from_register (type, gdb_regnum, frame);
	    else
	      error (_("Unable to access DWARF register number %s"),
		     paddress (arch, dwarf_regnum));
	  }
	  break;

	case DWARF_VALUE_MEMORY:
	  {
	    CORE_ADDR address = dwarf_expr_fetch_address (ctx, 0);
	    int in_stack_memory = dwarf_expr_fetch_in_stack_memory (ctx, 0);

	    retval = allocate_value_lazy (type);
	    VALUE_LVAL (retval) = lval_memory;
	    if (in_stack_memory)
	      set_value_stack (retval, 1);
	    set_value_address (retval, address + byte_offset);
	  }
	  break;

	case DWARF_VALUE_STACK:
	  {
	    ULONGEST value = dwarf_expr_fetch (ctx, 0);
	    bfd_byte *contents, *tem;
	    size_t n = ctx->addr_size;

	    if (byte_offset + TYPE_LENGTH (type) > n)
	      invalid_synthetic_pointer ();

	    tem = alloca (n);
	    store_unsigned_integer (tem, n,
				    gdbarch_byte_order (ctx->gdbarch),
				    value);

	    tem += byte_offset;
	    n -= byte_offset;

	    retval = allocate_value (type);
	    contents = value_contents_raw (retval);
	    if (n > TYPE_LENGTH (type))
	      n = TYPE_LENGTH (type);
	    memcpy (contents, tem, n);
	  }
	  break;

	case DWARF_VALUE_LITERAL:
	  {
	    bfd_byte *contents;
	    const bfd_byte *ldata;
	    size_t n = ctx->len;

	    if (byte_offset + TYPE_LENGTH (type) > n)
	      invalid_synthetic_pointer ();

	    retval = allocate_value (type);
	    contents = value_contents_raw (retval);

	    ldata = ctx->data + byte_offset;
	    n -= byte_offset;

	    if (n > TYPE_LENGTH (type))
	      n = TYPE_LENGTH (type);
	    memcpy (contents, ldata, n);
	  }
	  break;

	case DWARF_VALUE_OPTIMIZED_OUT:
	  retval = allocate_value (type);
	  VALUE_LVAL (retval) = not_lval;
	  set_value_optimized_out (retval, 1);
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
			  const gdb_byte *data, unsigned short size,
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
needs_frame_read_reg (void *baton, int regnum)
{
  struct needs_frame_baton *nf_baton = baton;

  nf_baton->needs_frame = 1;
  return 1;
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
needs_frame_dwarf_call (struct dwarf_expr_context *ctx, size_t die_offset)
{
  struct needs_frame_baton *nf_baton = ctx->baton;

  per_cu_dwarf_call (ctx, die_offset, nf_baton->per_cu,
		     ctx->get_frame_pc, ctx->baton);
}

/* Return non-zero iff the location expression at DATA (length SIZE)
   requires a frame to evaluate.  */

static int
dwarf2_loc_desc_needs_frame (const gdb_byte *data, unsigned short size,
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

  ctx->gdbarch = get_objfile_arch (objfile);
  ctx->addr_size = dwarf2_per_cu_addr_size (per_cu);
  ctx->offset = dwarf2_per_cu_text_offset (per_cu);
  ctx->baton = &baton;
  ctx->read_reg = needs_frame_read_reg;
  ctx->read_mem = needs_frame_read_mem;
  ctx->get_frame_base = needs_frame_frame_base;
  ctx->get_frame_cfa = needs_frame_frame_cfa;
  ctx->get_frame_pc = needs_frame_frame_cfa;
  ctx->get_tls_address = needs_frame_tls_address;
  ctx->dwarf_call = needs_frame_dwarf_call;

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
  const char *name = dwarf_stack_op_name (op);

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

  gdb_assert (nbits > 0 && nbits <= sizeof (LONGEST));

  if (trace_kludge)
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
      ULONGEST uoffset, reg;
      LONGEST offset;
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
	  op_ptr = read_uleb128 (op_ptr, op_end, &uoffset);
	  ax_const_l (expr, uoffset);
	  break;
	case DW_OP_consts:
	  op_ptr = read_sleb128 (op_ptr, op_end, &offset);
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
	  op_ptr = read_uleb128 (op_ptr, op_end, &reg);
	  dwarf_expr_require_composition (op_ptr, op_end, "DW_OP_regx");
	  loc->u.reg = translate_register (arch, reg);
	  loc->kind = axs_lvalue_register;
	  break;

	case DW_OP_implicit_value:
	  {
	    ULONGEST len;

	    op_ptr = read_uleb128 (op_ptr, op_end, &len);
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
	  op_ptr = read_sleb128 (op_ptr, op_end, &offset);
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
	    op_ptr = read_uleb128 (op_ptr, op_end, &reg);
	    op_ptr = read_sleb128 (op_ptr, op_end, &offset);
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
	    unsigned int before_stack_len;
	    struct block *b;
	    struct symbol *framefunc;
	    LONGEST base_offset = 0;

	    b = block_for_pc (expr->scope);

	    if (!b)
	      error (_("No block found for address"));

	    framefunc = block_linkage_function (b);

	    if (!framefunc)
	      error (_("No function found for block"));

	    dwarf_expr_frame_base_1 (framefunc, expr->scope,
				     &datastart, &datalen);

	    op_ptr = read_sleb128 (op_ptr, op_end, &offset);
	    dwarf2_compile_expr_to_ax (expr, loc, arch, addr_size, datastart,
				       datastart + datalen, per_cu);

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

	    switch (size)
	      {
	      case 8:
		ax_simple (expr, aop_ref8);
		break;
	      case 16:
		ax_simple (expr, aop_ref16);
		break;
	      case 32:
		ax_simple (expr, aop_ref32);
		break;
	      case 64:
		ax_simple (expr, aop_ref64);
		break;
	      default:
		/* Note that dwarf_stack_op_name will never return
		   NULL here.  */
		error (_("Unsupported size %d in %s"),
		       size, dwarf_stack_op_name (op));
	      }
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
	  op_ptr = read_uleb128 (op_ptr, op_end, &reg);
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
	    ULONGEST size, offset;

	    if (op_ptr - 1 == previous_piece)
	      error (_("Cannot translate empty pieces to agent expressions"));
	    previous_piece = op_ptr - 1;

            op_ptr = read_uleb128 (op_ptr, op_end, &size);
	    if (op == DW_OP_piece)
	      {
		size *= 8;
		offset = 0;
	      }
	    else
	      op_ptr = read_uleb128 (op_ptr, op_end, &offset);

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

	    block = dwarf2_fetch_die_location_block (uoffset, per_cu,
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

/* Nicely describe a single piece of a location, returning an updated
   position in the bytecode sequence.  This function cannot recognize
   all locations; if a location is not recognized, it simply returns
   DATA.  */

static const gdb_byte *
locexpr_describe_location_piece (struct symbol *symbol, struct ui_file *stream,
				 CORE_ADDR addr, struct objfile *objfile,
				 const gdb_byte *data, const gdb_byte *end,
				 unsigned int addr_size)
{
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  int regno;

  if (data[0] >= DW_OP_reg0 && data[0] <= DW_OP_reg31)
    {
      regno = gdbarch_dwarf2_reg_to_regnum (gdbarch, data[0] - DW_OP_reg0);
      fprintf_filtered (stream, _("a variable in $%s"),
			gdbarch_register_name (gdbarch, regno));
      data += 1;
    }
  else if (data[0] == DW_OP_regx)
    {
      ULONGEST reg;

      data = read_uleb128 (data + 1, end, &reg);
      regno = gdbarch_dwarf2_reg_to_regnum (gdbarch, reg);
      fprintf_filtered (stream, _("a variable in $%s"),
			gdbarch_register_name (gdbarch, regno));
    }
  else if (data[0] == DW_OP_fbreg)
    {
      struct block *b;
      struct symbol *framefunc;
      int frame_reg = 0;
      LONGEST frame_offset;
      const gdb_byte *base_data, *new_data, *save_data = data;
      size_t base_size;
      LONGEST base_offset = 0;

      new_data = read_sleb128 (data + 1, end, &frame_offset);
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
	  buf_end = read_sleb128 (base_data + 1,
				  base_data + base_size, &base_offset);
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

      regno = gdbarch_dwarf2_reg_to_regnum (gdbarch, frame_reg);

      fprintf_filtered (stream,
			_("a variable at frame base reg $%s offset %s+%s"),
			gdbarch_register_name (gdbarch, regno),
			plongest (base_offset), plongest (frame_offset));
    }
  else if (data[0] >= DW_OP_breg0 && data[0] <= DW_OP_breg31
	   && piece_end_p (data, end))
    {
      LONGEST offset;

      regno = gdbarch_dwarf2_reg_to_regnum (gdbarch, data[0] - DW_OP_breg0);

      data = read_sleb128 (data + 1, end, &offset);

      fprintf_filtered (stream,
			_("a variable at offset %s from base reg $%s"),
			plongest (offset),
			gdbarch_register_name (gdbarch, regno));
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
			phex_nz (offset, addr_size), objfile->name);

      data += 1 + addr_size + 1;
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
   will keep going until it reaches the end of the expression.  */

static const gdb_byte *
disassemble_dwarf_expression (struct ui_file *stream,
			      struct gdbarch *arch, unsigned int addr_size,
			      int offset_size,
			      const gdb_byte *data, const gdb_byte *end,
			      int all)
{
  const gdb_byte *start = data;

  fprintf_filtered (stream, _("a complex DWARF expression:\n"));

  while (data < end
	 && (all
	     || (data[0] != DW_OP_piece && data[0] != DW_OP_bit_piece)))
    {
      enum dwarf_location_atom op = *data++;
      ULONGEST ul;
      LONGEST l;
      const char *name;

      name = dwarf_stack_op_name (op);

      if (!name)
	error (_("Unrecognized DWARF opcode 0x%02x at %ld"),
	       op, (long) (data - start));
      fprintf_filtered (stream, "  % 4ld: %s", (long) (data - start), name);

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
	  data = read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s", pulongest (ul));
	  break;
	case DW_OP_consts:
	  data = read_sleb128 (data, end, &l);
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
			    gdbarch_register_name (arch, op - DW_OP_reg0));
	  break;

	case DW_OP_regx:
	  data = read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s [$%s]", pulongest (ul),
			    gdbarch_register_name (arch, (int) ul));
	  break;

	case DW_OP_implicit_value:
	  data = read_uleb128 (data, end, &ul);
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
	  data = read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " %s [$%s]", plongest (l),
			    gdbarch_register_name (arch, op - DW_OP_breg0));
	  break;

	case DW_OP_bregx:
	  data = read_uleb128 (data, end, &ul);
	  data = read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " register %s [$%s] offset %s",
			    pulongest (ul),
			    gdbarch_register_name (arch, (int) ul),
			    plongest (l));
	  break;

	case DW_OP_fbreg:
	  data = read_sleb128 (data, end, &l);
	  fprintf_filtered (stream, " %s", plongest (l));
	  break;

	case DW_OP_xderef_size:
	case DW_OP_deref_size:
	case DW_OP_pick:
	  fprintf_filtered (stream, " %d", *data);
	  ++data;
	  break;

	case DW_OP_plus_uconst:
	  data = read_uleb128 (data, end, &ul);
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
	  data = read_uleb128 (data, end, &ul);
	  fprintf_filtered (stream, " %s (bytes)", pulongest (ul));
	  break;

	case DW_OP_bit_piece:
	  {
	    ULONGEST offset;

	    data = read_uleb128 (data, end, &ul);
	    data = read_uleb128 (data, end, &offset);
	    fprintf_filtered (stream, " size %s offset %s (bits)",
			      pulongest (ul), pulongest (offset));
	  }
	  break;

	case DW_OP_GNU_implicit_pointer:
	  {
	    ul = extract_unsigned_integer (data, offset_size,
					   gdbarch_byte_order (arch));
	    data += offset_size;

	    data = read_sleb128 (data, end, &l);

	    fprintf_filtered (stream, " DIE %s offset %s",
			      phex_nz (ul, offset_size),
			      plongest (l));
	  }
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
			     const gdb_byte *data, int size,
			     struct objfile *objfile, unsigned int addr_size,
			     int offset_size)
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
						  addr, objfile,
						  data, end, addr_size);
	  /* If we printed anything, or if we have an empty piece,
	     then don't disassemble.  */
	  if (data != here
	      || data[0] == DW_OP_piece
	      || data[0] == DW_OP_bit_piece)
	    disassemble = 0;
	}
      if (disassemble)
	data = disassemble_dwarf_expression (stream,
					     get_objfile_arch (objfile),
					     addr_size, offset_size, data, end,
					     dwarf2_always_disassemble);

      if (data < end)
	{
	  int empty = data == here;
	      
	  if (disassemble)
	    fprintf_filtered (stream, "   ");
	  if (data[0] == DW_OP_piece)
	    {
	      ULONGEST bytes;

	      data = read_uleb128 (data + 1, end, &bytes);

	      if (empty)
		fprintf_filtered (stream, _("an empty %s-byte piece"),
				  pulongest (bytes));
	      else
		fprintf_filtered (stream, _(" [%s-byte piece]"),
				  pulongest (bytes));
	    }
	  else if (data[0] == DW_OP_bit_piece)
	    {
	      ULONGEST bits, offset;

	      data = read_uleb128 (data + 1, end, &bits);
	      data = read_uleb128 (data, end, &offset);

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
			       objfile, addr_size, offset_size);
}

/* Describe the location of SYMBOL as an agent value in VALUE, generating
   any necessary bytecode in AX.  */

static void
locexpr_tracepoint_var_ref (struct symbol *symbol, struct gdbarch *gdbarch,
			    struct agent_expr *ax, struct axs_value *value)
{
  struct dwarf2_locexpr_baton *dlbaton = SYMBOL_LOCATION_BATON (symbol);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);

  if (dlbaton->data == NULL || dlbaton->size == 0)
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
  locexpr_read_needs_frame,
  locexpr_describe_location,
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
  if (data == NULL)
    {
      val = allocate_value (SYMBOL_TYPE (symbol));
      VALUE_LVAL (val) = not_lval;
      set_value_optimized_out (val, 1);
    }
  else
    val = dwarf2_evaluate_loc_desc (SYMBOL_TYPE (symbol), frame, data, size,
				    dlbaton->per_cu);

  return val;
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
  CORE_ADDR low, high;
  const gdb_byte *loc_ptr, *buf_end;
  int length, first = 1;
  struct objfile *objfile = dwarf2_per_cu_objfile (dlbaton->per_cu);
  struct gdbarch *gdbarch = get_objfile_arch (objfile);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  unsigned int addr_size = dwarf2_per_cu_addr_size (dlbaton->per_cu);
  int offset_size = dwarf2_per_cu_offset_size (dlbaton->per_cu);
  int signed_addr_p = bfd_get_sign_extend_vma (objfile->obfd);
  CORE_ADDR base_mask = ~(~(CORE_ADDR)1 << (addr_size * 8 - 1));
  /* Adjust base_address for relocatable objects.  */
  CORE_ADDR base_offset = dwarf2_per_cu_text_offset (dlbaton->per_cu);
  CORE_ADDR base_address = dlbaton->base_address + base_offset;

  loc_ptr = dlbaton->data;
  buf_end = dlbaton->data + dlbaton->size;

  fprintf_filtered (stream, _("multi-location:\n"));

  /* Iterate through locations until we run out.  */
  while (1)
    {
      if (buf_end - loc_ptr < 2 * addr_size)
	error (_("Corrupted DWARF expression for symbol \"%s\"."),
	       SYMBOL_PRINT_NAME (symbol));

      if (signed_addr_p)
	low = extract_signed_integer (loc_ptr, addr_size, byte_order);
      else
	low = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
      loc_ptr += addr_size;

      if (signed_addr_p)
	high = extract_signed_integer (loc_ptr, addr_size, byte_order);
      else
	high = extract_unsigned_integer (loc_ptr, addr_size, byte_order);
      loc_ptr += addr_size;

      /* A base-address-selection entry.  */
      if ((low & base_mask) == base_mask)
	{
	  base_address = high + base_offset;
	  fprintf_filtered (stream, _("  Base address %s"),
			    paddress (gdbarch, base_address));
	  continue;
	}

      /* An end-of-list entry.  */
      if (low == 0 && high == 0)
	break;

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
				   objfile, addr_size, offset_size);

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
  if (data == NULL || size == 0)
    value->optimized_out = 1;
  else
    dwarf2_compile_expr_to_ax (ax, value, gdbarch, addr_size, data, data + size,
			       dlbaton->per_cu);
}

/* The set of location functions used with the DWARF-2 expression
   evaluator and location lists.  */
const struct symbol_computed_ops dwarf2_loclist_funcs = {
  loclist_read_variable,
  loclist_read_needs_frame,
  loclist_describe_location,
  loclist_tracepoint_var_ref
};
