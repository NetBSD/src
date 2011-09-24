/* Low level packing and unpacking of values for GDB, the GNU Debugger.

   Copyright (C) 1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994, 1995,
   1996, 1997, 1998, 1999, 2000, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
   2009, 2010, 2011 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "gdb_string.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "demangle.h"
#include "doublest.h"
#include "gdb_assert.h"
#include "regcache.h"
#include "block.h"
#include "dfp.h"
#include "objfiles.h"
#include "valprint.h"
#include "cli/cli-decode.h"
#include "exceptions.h"
#include "python/python.h"
#include <ctype.h>
#include "tracepoint.h"

/* Prototypes for exported functions.  */

void _initialize_values (void);

/* Definition of a user function.  */
struct internal_function
{
  /* The name of the function.  It is a bit odd to have this in the
     function itself -- the user might use a differently-named
     convenience variable to hold the function.  */
  char *name;

  /* The handler.  */
  internal_function_fn handler;

  /* User data for the handler.  */
  void *cookie;
};

/* Defines an [OFFSET, OFFSET + LENGTH) range.  */

struct range
{
  /* Lowest offset in the range.  */
  int offset;

  /* Length of the range.  */
  int length;
};

typedef struct range range_s;

DEF_VEC_O(range_s);

/* Returns true if the ranges defined by [offset1, offset1+len1) and
   [offset2, offset2+len2) overlap.  */

static int
ranges_overlap (int offset1, int len1,
		int offset2, int len2)
{
  ULONGEST h, l;

  l = max (offset1, offset2);
  h = min (offset1 + len1, offset2 + len2);
  return (l < h);
}

/* Returns true if the first argument is strictly less than the
   second, useful for VEC_lower_bound.  We keep ranges sorted by
   offset and coalesce overlapping and contiguous ranges, so this just
   compares the starting offset.  */

static int
range_lessthan (const range_s *r1, const range_s *r2)
{
  return r1->offset < r2->offset;
}

/* Returns true if RANGES contains any range that overlaps [OFFSET,
   OFFSET+LENGTH).  */

static int
ranges_contain (VEC(range_s) *ranges, int offset, int length)
{
  range_s what;
  int i;

  what.offset = offset;
  what.length = length;

  /* We keep ranges sorted by offset and coalesce overlapping and
     contiguous ranges, so to check if a range list contains a given
     range, we can do a binary search for the position the given range
     would be inserted if we only considered the starting OFFSET of
     ranges.  We call that position I.  Since we also have LENGTH to
     care for (this is a range afterall), we need to check if the
     _previous_ range overlaps the I range.  E.g.,

         R
         |---|
       |---|    |---|  |------| ... |--|
       0        1      2            N

       I=1

     In the case above, the binary search would return `I=1', meaning,
     this OFFSET should be inserted at position 1, and the current
     position 1 should be pushed further (and before 2).  But, `0'
     overlaps with R.

     Then we need to check if the I range overlaps the I range itself.
     E.g.,

              R
              |---|
       |---|    |---|  |-------| ... |--|
       0        1      2             N

       I=1
  */

  i = VEC_lower_bound (range_s, ranges, &what, range_lessthan);

  if (i > 0)
    {
      struct range *bef = VEC_index (range_s, ranges, i - 1);

      if (ranges_overlap (bef->offset, bef->length, offset, length))
	return 1;
    }

  if (i < VEC_length (range_s, ranges))
    {
      struct range *r = VEC_index (range_s, ranges, i);

      if (ranges_overlap (r->offset, r->length, offset, length))
	return 1;
    }

  return 0;
}

static struct cmd_list_element *functionlist;

struct value
{
  /* Type of value; either not an lval, or one of the various
     different possible kinds of lval.  */
  enum lval_type lval;

  /* Is it modifiable?  Only relevant if lval != not_lval.  */
  int modifiable;

  /* Location of value (if lval).  */
  union
  {
    /* If lval == lval_memory, this is the address in the inferior.
       If lval == lval_register, this is the byte offset into the
       registers structure.  */
    CORE_ADDR address;

    /* Pointer to internal variable.  */
    struct internalvar *internalvar;

    /* If lval == lval_computed, this is a set of function pointers
       to use to access and describe the value, and a closure pointer
       for them to use.  */
    struct
    {
      struct lval_funcs *funcs; /* Functions to call.  */
      void *closure;            /* Closure for those functions to use.  */
    } computed;
  } location;

  /* Describes offset of a value within lval of a structure in bytes.
     If lval == lval_memory, this is an offset to the address.  If
     lval == lval_register, this is a further offset from
     location.address within the registers structure.  Note also the
     member embedded_offset below.  */
  int offset;

  /* Only used for bitfields; number of bits contained in them.  */
  int bitsize;

  /* Only used for bitfields; position of start of field.  For
     gdbarch_bits_big_endian=0 targets, it is the position of the LSB.  For
     gdbarch_bits_big_endian=1 targets, it is the position of the MSB.  */
  int bitpos;

  /* Only used for bitfields; the containing value.  This allows a
     single read from the target when displaying multiple
     bitfields.  */
  struct value *parent;

  /* Frame register value is relative to.  This will be described in
     the lval enum above as "lval_register".  */
  struct frame_id frame_id;

  /* Type of the value.  */
  struct type *type;

  /* If a value represents a C++ object, then the `type' field gives
     the object's compile-time type.  If the object actually belongs
     to some class derived from `type', perhaps with other base
     classes and additional members, then `type' is just a subobject
     of the real thing, and the full object is probably larger than
     `type' would suggest.

     If `type' is a dynamic class (i.e. one with a vtable), then GDB
     can actually determine the object's run-time type by looking at
     the run-time type information in the vtable.  When this
     information is available, we may elect to read in the entire
     object, for several reasons:

     - When printing the value, the user would probably rather see the
     full object, not just the limited portion apparent from the
     compile-time type.

     - If `type' has virtual base classes, then even printing `type'
     alone may require reaching outside the `type' portion of the
     object to wherever the virtual base class has been stored.

     When we store the entire object, `enclosing_type' is the run-time
     type -- the complete object -- and `embedded_offset' is the
     offset of `type' within that larger type, in bytes.  The
     value_contents() macro takes `embedded_offset' into account, so
     most GDB code continues to see the `type' portion of the value,
     just as the inferior would.

     If `type' is a pointer to an object, then `enclosing_type' is a
     pointer to the object's run-time type, and `pointed_to_offset' is
     the offset in bytes from the full object to the pointed-to object
     -- that is, the value `embedded_offset' would have if we followed
     the pointer and fetched the complete object.  (I don't really see
     the point.  Why not just determine the run-time type when you
     indirect, and avoid the special case?  The contents don't matter
     until you indirect anyway.)

     If we're not doing anything fancy, `enclosing_type' is equal to
     `type', and `embedded_offset' is zero, so everything works
     normally.  */
  struct type *enclosing_type;
  int embedded_offset;
  int pointed_to_offset;

  /* Values are stored in a chain, so that they can be deleted easily
     over calls to the inferior.  Values assigned to internal
     variables, put into the value history or exposed to Python are
     taken off this list.  */
  struct value *next;

  /* Register number if the value is from a register.  */
  short regnum;

  /* If zero, contents of this value are in the contents field.  If
     nonzero, contents are in inferior.  If the lval field is lval_memory,
     the contents are in inferior memory at location.address plus offset.
     The lval field may also be lval_register.

     WARNING: This field is used by the code which handles watchpoints
     (see breakpoint.c) to decide whether a particular value can be
     watched by hardware watchpoints.  If the lazy flag is set for
     some member of a value chain, it is assumed that this member of
     the chain doesn't need to be watched as part of watching the
     value itself.  This is how GDB avoids watching the entire struct
     or array when the user wants to watch a single struct member or
     array element.  If you ever change the way lazy flag is set and
     reset, be sure to consider this use as well!  */
  char lazy;

  /* If nonzero, this is the value of a variable which does not
     actually exist in the program.  */
  char optimized_out;

  /* If value is a variable, is it initialized or not.  */
  int initialized;

  /* If value is from the stack.  If this is set, read_stack will be
     used instead of read_memory to enable extra caching.  */
  int stack;

  /* Actual contents of the value.  Target byte-order.  NULL or not
     valid if lazy is nonzero.  */
  gdb_byte *contents;

  /* Unavailable ranges in CONTENTS.  We mark unavailable ranges,
     rather than available, since the common and default case is for a
     value to be available.  This is filled in at value read time.  */
  VEC(range_s) *unavailable;

  /* The number of references to this value.  When a value is created,
     the value chain holds a reference, so REFERENCE_COUNT is 1.  If
     release_value is called, this value is removed from the chain but
     the caller of release_value now has a reference to this value.
     The caller must arrange for a call to value_free later.  */
  int reference_count;
};

int
value_bytes_available (const struct value *value, int offset, int length)
{
  gdb_assert (!value->lazy);

  return !ranges_contain (value->unavailable, offset, length);
}

int
value_entirely_available (struct value *value)
{
  /* We can only tell whether the whole value is available when we try
     to read it.  */
  if (value->lazy)
    value_fetch_lazy (value);

  if (VEC_empty (range_s, value->unavailable))
    return 1;
  return 0;
}

void
mark_value_bytes_unavailable (struct value *value, int offset, int length)
{
  range_s newr;
  int i;

  /* Insert the range sorted.  If there's overlap or the new range
     would be contiguous with an existing range, merge.  */

  newr.offset = offset;
  newr.length = length;

  /* Do a binary search for the position the given range would be
     inserted if we only considered the starting OFFSET of ranges.
     Call that position I.  Since we also have LENGTH to care for
     (this is a range afterall), we need to check if the _previous_
     range overlaps the I range.  E.g., calling R the new range:

       #1 - overlaps with previous

	   R
	   |-...-|
	 |---|     |---|  |------| ... |--|
	 0         1      2            N

	 I=1

     In the case #1 above, the binary search would return `I=1',
     meaning, this OFFSET should be inserted at position 1, and the
     current position 1 should be pushed further (and become 2).  But,
     note that `0' overlaps with R, so we want to merge them.

     A similar consideration needs to be taken if the new range would
     be contiguous with the previous range:

       #2 - contiguous with previous

	    R
	    |-...-|
	 |--|       |---|  |------| ... |--|
	 0          1      2            N

	 I=1

     If there's no overlap with the previous range, as in:

       #3 - not overlapping and not contiguous

	       R
	       |-...-|
	  |--|         |---|  |------| ... |--|
	  0            1      2            N

	 I=1

     or if I is 0:

       #4 - R is the range with lowest offset

	  R
	 |-...-|
	         |--|       |---|  |------| ... |--|
	         0          1      2            N

	 I=0

     ... we just push the new range to I.

     All the 4 cases above need to consider that the new range may
     also overlap several of the ranges that follow, or that R may be
     contiguous with the following range, and merge.  E.g.,

       #5 - overlapping following ranges

	  R
	 |------------------------|
	         |--|       |---|  |------| ... |--|
	         0          1      2            N

	 I=0

       or:

	    R
	    |-------|
	 |--|       |---|  |------| ... |--|
	 0          1      2            N

	 I=1

  */

  i = VEC_lower_bound (range_s, value->unavailable, &newr, range_lessthan);
  if (i > 0)
    {
      struct range *bef = VEC_index (range_s, value->unavailable, i - 1);

      if (ranges_overlap (bef->offset, bef->length, offset, length))
	{
	  /* #1 */
	  ULONGEST l = min (bef->offset, offset);
	  ULONGEST h = max (bef->offset + bef->length, offset + length);

	  bef->offset = l;
	  bef->length = h - l;
	  i--;
	}
      else if (offset == bef->offset + bef->length)
	{
	  /* #2 */
	  bef->length += length;
	  i--;
	}
      else
	{
	  /* #3 */
	  VEC_safe_insert (range_s, value->unavailable, i, &newr);
	}
    }
  else
    {
      /* #4 */
      VEC_safe_insert (range_s, value->unavailable, i, &newr);
    }

  /* Check whether the ranges following the one we've just added or
     touched can be folded in (#5 above).  */
  if (i + 1 < VEC_length (range_s, value->unavailable))
    {
      struct range *t;
      struct range *r;
      int removed = 0;
      int next = i + 1;

      /* Get the range we just touched.  */
      t = VEC_index (range_s, value->unavailable, i);
      removed = 0;

      i = next;
      for (; VEC_iterate (range_s, value->unavailable, i, r); i++)
	if (r->offset <= t->offset + t->length)
	  {
	    ULONGEST l, h;

	    l = min (t->offset, r->offset);
	    h = max (t->offset + t->length, r->offset + r->length);

	    t->offset = l;
	    t->length = h - l;

	    removed++;
	  }
	else
	  {
	    /* If we couldn't merge this one, we won't be able to
	       merge following ones either, since the ranges are
	       always sorted by OFFSET.  */
	    break;
	  }

      if (removed != 0)
	VEC_block_remove (range_s, value->unavailable, next, removed);
    }
}

/* Find the first range in RANGES that overlaps the range defined by
   OFFSET and LENGTH, starting at element POS in the RANGES vector,
   Returns the index into RANGES where such overlapping range was
   found, or -1 if none was found.  */

static int
find_first_range_overlap (VEC(range_s) *ranges, int pos,
			  int offset, int length)
{
  range_s *r;
  int i;

  for (i = pos; VEC_iterate (range_s, ranges, i, r); i++)
    if (ranges_overlap (r->offset, r->length, offset, length))
      return i;

  return -1;
}

int
value_available_contents_eq (const struct value *val1, int offset1,
			     const struct value *val2, int offset2,
			     int length)
{
  int idx1 = 0, idx2 = 0;

  /* This routine is used by printing routines, where we should
     already have read the value.  Note that we only know whether a
     value chunk is available if we've tried to read it.  */
  gdb_assert (!val1->lazy && !val2->lazy);

  while (length > 0)
    {
      range_s *r1, *r2;
      ULONGEST l1, h1;
      ULONGEST l2, h2;

      idx1 = find_first_range_overlap (val1->unavailable, idx1,
				       offset1, length);
      idx2 = find_first_range_overlap (val2->unavailable, idx2,
				       offset2, length);

      /* The usual case is for both values to be completely available.  */
      if (idx1 == -1 && idx2 == -1)
	return (memcmp (val1->contents + offset1,
			val2->contents + offset2,
			length) == 0);
      /* The contents only match equal if the available set matches as
	 well.  */
      else if (idx1 == -1 || idx2 == -1)
	return 0;

      gdb_assert (idx1 != -1 && idx2 != -1);

      r1 = VEC_index (range_s, val1->unavailable, idx1);
      r2 = VEC_index (range_s, val2->unavailable, idx2);

      /* Get the unavailable windows intersected by the incoming
	 ranges.  The first and last ranges that overlap the argument
	 range may be wider than said incoming arguments ranges.  */
      l1 = max (offset1, r1->offset);
      h1 = min (offset1 + length, r1->offset + r1->length);

      l2 = max (offset2, r2->offset);
      h2 = min (offset2 + length, r2->offset + r2->length);

      /* Make them relative to the respective start offsets, so we can
	 compare them for equality.  */
      l1 -= offset1;
      h1 -= offset1;

      l2 -= offset2;
      h2 -= offset2;

      /* Different availability, no match.  */
      if (l1 != l2 || h1 != h2)
	return 0;

      /* Compare the _available_ contents.  */
      if (memcmp (val1->contents + offset1,
		  val2->contents + offset2,
		  l1) != 0)
	return 0;

      length -= h1;
      offset1 += h1;
      offset2 += h1;
    }

  return 1;
}

/* Prototypes for local functions.  */

static void show_values (char *, int);

static void show_convenience (char *, int);


/* The value-history records all the values printed
   by print commands during this session.  Each chunk
   records 60 consecutive values.  The first chunk on
   the chain records the most recent values.
   The total number of values is in value_history_count.  */

#define VALUE_HISTORY_CHUNK 60

struct value_history_chunk
  {
    struct value_history_chunk *next;
    struct value *values[VALUE_HISTORY_CHUNK];
  };

/* Chain of chunks now in use.  */

static struct value_history_chunk *value_history_chain;

static int value_history_count;	/* Abs number of last entry stored.  */


/* List of all value objects currently allocated
   (except for those released by calls to release_value)
   This is so they can be freed after each command.  */

static struct value *all_values;

/* Allocate a lazy value for type TYPE.  Its actual content is
   "lazily" allocated too: the content field of the return value is
   NULL; it will be allocated when it is fetched from the target.  */

struct value *
allocate_value_lazy (struct type *type)
{
  struct value *val;

  /* Call check_typedef on our type to make sure that, if TYPE
     is a TYPE_CODE_TYPEDEF, its length is set to the length
     of the target type instead of zero.  However, we do not
     replace the typedef type by the target type, because we want
     to keep the typedef in order to be able to set the VAL's type
     description correctly.  */
  check_typedef (type);

  val = (struct value *) xzalloc (sizeof (struct value));
  val->contents = NULL;
  val->next = all_values;
  all_values = val;
  val->type = type;
  val->enclosing_type = type;
  VALUE_LVAL (val) = not_lval;
  val->location.address = 0;
  VALUE_FRAME_ID (val) = null_frame_id;
  val->offset = 0;
  val->bitpos = 0;
  val->bitsize = 0;
  VALUE_REGNUM (val) = -1;
  val->lazy = 1;
  val->optimized_out = 0;
  val->embedded_offset = 0;
  val->pointed_to_offset = 0;
  val->modifiable = 1;
  val->initialized = 1;  /* Default to initialized.  */

  /* Values start out on the all_values chain.  */
  val->reference_count = 1;

  return val;
}

/* Allocate the contents of VAL if it has not been allocated yet.  */

void
allocate_value_contents (struct value *val)
{
  if (!val->contents)
    val->contents = (gdb_byte *) xzalloc (TYPE_LENGTH (val->enclosing_type));
}

/* Allocate a  value  and its contents for type TYPE.  */

struct value *
allocate_value (struct type *type)
{
  struct value *val = allocate_value_lazy (type);

  allocate_value_contents (val);
  val->lazy = 0;
  return val;
}

/* Allocate a  value  that has the correct length
   for COUNT repetitions of type TYPE.  */

struct value *
allocate_repeat_value (struct type *type, int count)
{
  int low_bound = current_language->string_lower_bound;		/* ??? */
  /* FIXME-type-allocation: need a way to free this type when we are
     done with it.  */
  struct type *array_type
    = lookup_array_range_type (type, low_bound, count + low_bound - 1);

  return allocate_value (array_type);
}

struct value *
allocate_computed_value (struct type *type,
                         struct lval_funcs *funcs,
                         void *closure)
{
  struct value *v = allocate_value_lazy (type);

  VALUE_LVAL (v) = lval_computed;
  v->location.computed.funcs = funcs;
  v->location.computed.closure = closure;

  return v;
}

/* Accessor methods.  */

struct value *
value_next (struct value *value)
{
  return value->next;
}

struct type *
value_type (const struct value *value)
{
  return value->type;
}
void
deprecated_set_value_type (struct value *value, struct type *type)
{
  value->type = type;
}

int
value_offset (const struct value *value)
{
  return value->offset;
}
void
set_value_offset (struct value *value, int offset)
{
  value->offset = offset;
}

int
value_bitpos (const struct value *value)
{
  return value->bitpos;
}
void
set_value_bitpos (struct value *value, int bit)
{
  value->bitpos = bit;
}

int
value_bitsize (const struct value *value)
{
  return value->bitsize;
}
void
set_value_bitsize (struct value *value, int bit)
{
  value->bitsize = bit;
}

struct value *
value_parent (struct value *value)
{
  return value->parent;
}

gdb_byte *
value_contents_raw (struct value *value)
{
  allocate_value_contents (value);
  return value->contents + value->embedded_offset;
}

gdb_byte *
value_contents_all_raw (struct value *value)
{
  allocate_value_contents (value);
  return value->contents;
}

struct type *
value_enclosing_type (struct value *value)
{
  return value->enclosing_type;
}

static void
require_not_optimized_out (const struct value *value)
{
  if (value->optimized_out)
    error (_("value has been optimized out"));
}

static void
require_available (const struct value *value)
{
  if (!VEC_empty (range_s, value->unavailable))
    throw_error (NOT_AVAILABLE_ERROR, _("value is not available"));
}

const gdb_byte *
value_contents_for_printing (struct value *value)
{
  if (value->lazy)
    value_fetch_lazy (value);
  return value->contents;
}

const gdb_byte *
value_contents_for_printing_const (const struct value *value)
{
  gdb_assert (!value->lazy);
  return value->contents;
}

const gdb_byte *
value_contents_all (struct value *value)
{
  const gdb_byte *result = value_contents_for_printing (value);
  require_not_optimized_out (value);
  require_available (value);
  return result;
}

/* Copy LENGTH bytes of SRC value's (all) contents
   (value_contents_all) starting at SRC_OFFSET, into DST value's (all)
   contents, starting at DST_OFFSET.  If unavailable contents are
   being copied from SRC, the corresponding DST contents are marked
   unavailable accordingly.  Neither DST nor SRC may be lazy
   values.

   It is assumed the contents of DST in the [DST_OFFSET,
   DST_OFFSET+LENGTH) range are wholly available.  */

void
value_contents_copy_raw (struct value *dst, int dst_offset,
			 struct value *src, int src_offset, int length)
{
  range_s *r;
  int i;

  /* A lazy DST would make that this copy operation useless, since as
     soon as DST's contents were un-lazied (by a later value_contents
     call, say), the contents would be overwritten.  A lazy SRC would
     mean we'd be copying garbage.  */
  gdb_assert (!dst->lazy && !src->lazy);

  /* The overwritten DST range gets unavailability ORed in, not
     replaced.  Make sure to remember to implement replacing if it
     turns out actually necessary.  */
  gdb_assert (value_bytes_available (dst, dst_offset, length));

  /* Copy the data.  */
  memcpy (value_contents_all_raw (dst) + dst_offset,
	  value_contents_all_raw (src) + src_offset,
	  length);

  /* Copy the meta-data, adjusted.  */
  for (i = 0; VEC_iterate (range_s, src->unavailable, i, r); i++)
    {
      ULONGEST h, l;

      l = max (r->offset, src_offset);
      h = min (r->offset + r->length, src_offset + length);

      if (l < h)
	mark_value_bytes_unavailable (dst,
				      dst_offset + (l - src_offset),
				      h - l);
    }
}

/* Copy LENGTH bytes of SRC value's (all) contents
   (value_contents_all) starting at SRC_OFFSET byte, into DST value's
   (all) contents, starting at DST_OFFSET.  If unavailable contents
   are being copied from SRC, the corresponding DST contents are
   marked unavailable accordingly.  DST must not be lazy.  If SRC is
   lazy, it will be fetched now.  If SRC is not valid (is optimized
   out), an error is thrown.

   It is assumed the contents of DST in the [DST_OFFSET,
   DST_OFFSET+LENGTH) range are wholly available.  */

void
value_contents_copy (struct value *dst, int dst_offset,
		     struct value *src, int src_offset, int length)
{
  require_not_optimized_out (src);

  if (src->lazy)
    value_fetch_lazy (src);

  value_contents_copy_raw (dst, dst_offset, src, src_offset, length);
}

int
value_lazy (struct value *value)
{
  return value->lazy;
}

void
set_value_lazy (struct value *value, int val)
{
  value->lazy = val;
}

int
value_stack (struct value *value)
{
  return value->stack;
}

void
set_value_stack (struct value *value, int val)
{
  value->stack = val;
}

const gdb_byte *
value_contents (struct value *value)
{
  const gdb_byte *result = value_contents_writeable (value);
  require_not_optimized_out (value);
  require_available (value);
  return result;
}

gdb_byte *
value_contents_writeable (struct value *value)
{
  if (value->lazy)
    value_fetch_lazy (value);
  return value_contents_raw (value);
}

/* Return non-zero if VAL1 and VAL2 have the same contents.  Note that
   this function is different from value_equal; in C the operator ==
   can return 0 even if the two values being compared are equal.  */

int
value_contents_equal (struct value *val1, struct value *val2)
{
  struct type *type1;
  struct type *type2;
  int len;

  type1 = check_typedef (value_type (val1));
  type2 = check_typedef (value_type (val2));
  len = TYPE_LENGTH (type1);
  if (len != TYPE_LENGTH (type2))
    return 0;

  return (memcmp (value_contents (val1), value_contents (val2), len) == 0);
}

int
value_optimized_out (struct value *value)
{
  return value->optimized_out;
}

void
set_value_optimized_out (struct value *value, int val)
{
  value->optimized_out = val;
}

int
value_entirely_optimized_out (const struct value *value)
{
  if (!value->optimized_out)
    return 0;
  if (value->lval != lval_computed
      || !value->location.computed.funcs->check_any_valid)
    return 1;
  return !value->location.computed.funcs->check_any_valid (value);
}

int
value_bits_valid (const struct value *value, int offset, int length)
{
  if (!value->optimized_out)
    return 1;
  if (value->lval != lval_computed
      || !value->location.computed.funcs->check_validity)
    return 0;
  return value->location.computed.funcs->check_validity (value, offset,
							 length);
}

int
value_bits_synthetic_pointer (const struct value *value,
			      int offset, int length)
{
  if (value->lval != lval_computed
      || !value->location.computed.funcs->check_synthetic_pointer)
    return 0;
  return value->location.computed.funcs->check_synthetic_pointer (value,
								  offset,
								  length);
}

int
value_embedded_offset (struct value *value)
{
  return value->embedded_offset;
}

void
set_value_embedded_offset (struct value *value, int val)
{
  value->embedded_offset = val;
}

int
value_pointed_to_offset (struct value *value)
{
  return value->pointed_to_offset;
}

void
set_value_pointed_to_offset (struct value *value, int val)
{
  value->pointed_to_offset = val;
}

struct lval_funcs *
value_computed_funcs (struct value *v)
{
  gdb_assert (VALUE_LVAL (v) == lval_computed);

  return v->location.computed.funcs;
}

void *
value_computed_closure (const struct value *v)
{
  gdb_assert (v->lval == lval_computed);

  return v->location.computed.closure;
}

enum lval_type *
deprecated_value_lval_hack (struct value *value)
{
  return &value->lval;
}

CORE_ADDR
value_address (const struct value *value)
{
  if (value->lval == lval_internalvar
      || value->lval == lval_internalvar_component)
    return 0;
  return value->location.address + value->offset;
}

CORE_ADDR
value_raw_address (struct value *value)
{
  if (value->lval == lval_internalvar
      || value->lval == lval_internalvar_component)
    return 0;
  return value->location.address;
}

void
set_value_address (struct value *value, CORE_ADDR addr)
{
  gdb_assert (value->lval != lval_internalvar
	      && value->lval != lval_internalvar_component);
  value->location.address = addr;
}

struct internalvar **
deprecated_value_internalvar_hack (struct value *value)
{
  return &value->location.internalvar;
}

struct frame_id *
deprecated_value_frame_id_hack (struct value *value)
{
  return &value->frame_id;
}

short *
deprecated_value_regnum_hack (struct value *value)
{
  return &value->regnum;
}

int
deprecated_value_modifiable (struct value *value)
{
  return value->modifiable;
}
void
deprecated_set_value_modifiable (struct value *value, int modifiable)
{
  value->modifiable = modifiable;
}

/* Return a mark in the value chain.  All values allocated after the
   mark is obtained (except for those released) are subject to being freed
   if a subsequent value_free_to_mark is passed the mark.  */
struct value *
value_mark (void)
{
  return all_values;
}

/* Take a reference to VAL.  VAL will not be deallocated until all
   references are released.  */

void
value_incref (struct value *val)
{
  val->reference_count++;
}

/* Release a reference to VAL, which was acquired with value_incref.
   This function is also called to deallocate values from the value
   chain.  */

void
value_free (struct value *val)
{
  if (val)
    {
      gdb_assert (val->reference_count > 0);
      val->reference_count--;
      if (val->reference_count > 0)
	return;

      /* If there's an associated parent value, drop our reference to
	 it.  */
      if (val->parent != NULL)
	value_free (val->parent);

      if (VALUE_LVAL (val) == lval_computed)
	{
	  struct lval_funcs *funcs = val->location.computed.funcs;

	  if (funcs->free_closure)
	    funcs->free_closure (val);
	}

      xfree (val->contents);
      VEC_free (range_s, val->unavailable);
    }
  xfree (val);
}

/* Free all values allocated since MARK was obtained by value_mark
   (except for those released).  */
void
value_free_to_mark (struct value *mark)
{
  struct value *val;
  struct value *next;

  for (val = all_values; val && val != mark; val = next)
    {
      next = val->next;
      value_free (val);
    }
  all_values = val;
}

/* Free all the values that have been allocated (except for those released).
   Call after each command, successful or not.
   In practice this is called before each command, which is sufficient.  */

void
free_all_values (void)
{
  struct value *val;
  struct value *next;

  for (val = all_values; val; val = next)
    {
      next = val->next;
      value_free (val);
    }

  all_values = 0;
}

/* Frees all the elements in a chain of values.  */

void
free_value_chain (struct value *v)
{
  struct value *next;

  for (; v; v = next)
    {
      next = value_next (v);
      value_free (v);
    }
}

/* Remove VAL from the chain all_values
   so it will not be freed automatically.  */

void
release_value (struct value *val)
{
  struct value *v;

  if (all_values == val)
    {
      all_values = val->next;
      val->next = NULL;
      return;
    }

  for (v = all_values; v; v = v->next)
    {
      if (v->next == val)
	{
	  v->next = val->next;
	  val->next = NULL;
	  break;
	}
    }
}

/* Release all values up to mark  */
struct value *
value_release_to_mark (struct value *mark)
{
  struct value *val;
  struct value *next;

  for (val = next = all_values; next; next = next->next)
    if (next->next == mark)
      {
	all_values = next->next;
	next->next = NULL;
	return val;
      }
  all_values = 0;
  return val;
}

/* Return a copy of the value ARG.
   It contains the same contents, for same memory address,
   but it's a different block of storage.  */

struct value *
value_copy (struct value *arg)
{
  struct type *encl_type = value_enclosing_type (arg);
  struct value *val;

  if (value_lazy (arg))
    val = allocate_value_lazy (encl_type);
  else
    val = allocate_value (encl_type);
  val->type = arg->type;
  VALUE_LVAL (val) = VALUE_LVAL (arg);
  val->location = arg->location;
  val->offset = arg->offset;
  val->bitpos = arg->bitpos;
  val->bitsize = arg->bitsize;
  VALUE_FRAME_ID (val) = VALUE_FRAME_ID (arg);
  VALUE_REGNUM (val) = VALUE_REGNUM (arg);
  val->lazy = arg->lazy;
  val->optimized_out = arg->optimized_out;
  val->embedded_offset = value_embedded_offset (arg);
  val->pointed_to_offset = arg->pointed_to_offset;
  val->modifiable = arg->modifiable;
  if (!value_lazy (val))
    {
      memcpy (value_contents_all_raw (val), value_contents_all_raw (arg),
	      TYPE_LENGTH (value_enclosing_type (arg)));

    }
  val->unavailable = VEC_copy (range_s, arg->unavailable);
  val->parent = arg->parent;
  if (val->parent)
    value_incref (val->parent);
  if (VALUE_LVAL (val) == lval_computed)
    {
      struct lval_funcs *funcs = val->location.computed.funcs;

      if (funcs->copy_closure)
        val->location.computed.closure = funcs->copy_closure (val);
    }
  return val;
}

/* Return a version of ARG that is non-lvalue.  */

struct value *
value_non_lval (struct value *arg)
{
  if (VALUE_LVAL (arg) != not_lval)
    {
      struct type *enc_type = value_enclosing_type (arg);
      struct value *val = allocate_value (enc_type);

      memcpy (value_contents_all_raw (val), value_contents_all (arg),
	      TYPE_LENGTH (enc_type));
      val->type = arg->type;
      set_value_embedded_offset (val, value_embedded_offset (arg));
      set_value_pointed_to_offset (val, value_pointed_to_offset (arg));
      return val;
    }
   return arg;
}

void
set_value_component_location (struct value *component,
			      const struct value *whole)
{
  if (whole->lval == lval_internalvar)
    VALUE_LVAL (component) = lval_internalvar_component;
  else
    VALUE_LVAL (component) = whole->lval;

  component->location = whole->location;
  if (whole->lval == lval_computed)
    {
      struct lval_funcs *funcs = whole->location.computed.funcs;

      if (funcs->copy_closure)
        component->location.computed.closure = funcs->copy_closure (whole);
    }
}


/* Access to the value history.  */

/* Record a new value in the value history.
   Returns the absolute history index of the entry.
   Result of -1 indicates the value was not saved; otherwise it is the
   value history index of this new item.  */

int
record_latest_value (struct value *val)
{
  int i;

  /* We don't want this value to have anything to do with the inferior anymore.
     In particular, "set $1 = 50" should not affect the variable from which
     the value was taken, and fast watchpoints should be able to assume that
     a value on the value history never changes.  */
  if (value_lazy (val))
    value_fetch_lazy (val);
  /* We preserve VALUE_LVAL so that the user can find out where it was fetched
     from.  This is a bit dubious, because then *&$1 does not just return $1
     but the current contents of that location.  c'est la vie...  */
  val->modifiable = 0;
  release_value (val);

  /* Here we treat value_history_count as origin-zero
     and applying to the value being stored now.  */

  i = value_history_count % VALUE_HISTORY_CHUNK;
  if (i == 0)
    {
      struct value_history_chunk *new
	= (struct value_history_chunk *)

      xmalloc (sizeof (struct value_history_chunk));
      memset (new->values, 0, sizeof new->values);
      new->next = value_history_chain;
      value_history_chain = new;
    }

  value_history_chain->values[i] = val;

  /* Now we regard value_history_count as origin-one
     and applying to the value just stored.  */

  return ++value_history_count;
}

/* Return a copy of the value in the history with sequence number NUM.  */

struct value *
access_value_history (int num)
{
  struct value_history_chunk *chunk;
  int i;
  int absnum = num;

  if (absnum <= 0)
    absnum += value_history_count;

  if (absnum <= 0)
    {
      if (num == 0)
	error (_("The history is empty."));
      else if (num == 1)
	error (_("There is only one value in the history."));
      else
	error (_("History does not go back to $$%d."), -num);
    }
  if (absnum > value_history_count)
    error (_("History has not yet reached $%d."), absnum);

  absnum--;

  /* Now absnum is always absolute and origin zero.  */

  chunk = value_history_chain;
  for (i = (value_history_count - 1) / VALUE_HISTORY_CHUNK
	 - absnum / VALUE_HISTORY_CHUNK;
       i > 0; i--)
    chunk = chunk->next;

  return value_copy (chunk->values[absnum % VALUE_HISTORY_CHUNK]);
}

static void
show_values (char *num_exp, int from_tty)
{
  int i;
  struct value *val;
  static int num = 1;

  if (num_exp)
    {
      /* "show values +" should print from the stored position.
         "show values <exp>" should print around value number <exp>.  */
      if (num_exp[0] != '+' || num_exp[1] != '\0')
	num = parse_and_eval_long (num_exp) - 5;
    }
  else
    {
      /* "show values" means print the last 10 values.  */
      num = value_history_count - 9;
    }

  if (num <= 0)
    num = 1;

  for (i = num; i < num + 10 && i <= value_history_count; i++)
    {
      struct value_print_options opts;

      val = access_value_history (i);
      printf_filtered (("$%d = "), i);
      get_user_print_options (&opts);
      value_print (val, gdb_stdout, &opts);
      printf_filtered (("\n"));
    }

  /* The next "show values +" should start after what we just printed.  */
  num += 10;

  /* Hitting just return after this command should do the same thing as
     "show values +".  If num_exp is null, this is unnecessary, since
     "show values +" is not useful after "show values".  */
  if (from_tty && num_exp)
    {
      num_exp[0] = '+';
      num_exp[1] = '\0';
    }
}

/* Internal variables.  These are variables within the debugger
   that hold values assigned by debugger commands.
   The user refers to them with a '$' prefix
   that does not appear in the variable names stored internally.  */

struct internalvar
{
  struct internalvar *next;
  char *name;

  /* We support various different kinds of content of an internal variable.
     enum internalvar_kind specifies the kind, and union internalvar_data
     provides the data associated with this particular kind.  */

  enum internalvar_kind
    {
      /* The internal variable is empty.  */
      INTERNALVAR_VOID,

      /* The value of the internal variable is provided directly as
	 a GDB value object.  */
      INTERNALVAR_VALUE,

      /* A fresh value is computed via a call-back routine on every
	 access to the internal variable.  */
      INTERNALVAR_MAKE_VALUE,

      /* The internal variable holds a GDB internal convenience function.  */
      INTERNALVAR_FUNCTION,

      /* The variable holds an integer value.  */
      INTERNALVAR_INTEGER,

      /* The variable holds a GDB-provided string.  */
      INTERNALVAR_STRING,

    } kind;

  union internalvar_data
    {
      /* A value object used with INTERNALVAR_VALUE.  */
      struct value *value;

      /* The call-back routine used with INTERNALVAR_MAKE_VALUE.  */
      internalvar_make_value make_value;

      /* The internal function used with INTERNALVAR_FUNCTION.  */
      struct
	{
	  struct internal_function *function;
	  /* True if this is the canonical name for the function.  */
	  int canonical;
	} fn;

      /* An integer value used with INTERNALVAR_INTEGER.  */
      struct
        {
	  /* If type is non-NULL, it will be used as the type to generate
	     a value for this internal variable.  If type is NULL, a default
	     integer type for the architecture is used.  */
	  struct type *type;
	  LONGEST val;
        } integer;

      /* A string value used with INTERNALVAR_STRING.  */
      char *string;
    } u;
};

static struct internalvar *internalvars;

/* If the variable does not already exist create it and give it the
   value given.  If no value is given then the default is zero.  */
static void
init_if_undefined_command (char* args, int from_tty)
{
  struct internalvar* intvar;

  /* Parse the expression - this is taken from set_command().  */
  struct expression *expr = parse_expression (args);
  register struct cleanup *old_chain =
    make_cleanup (free_current_contents, &expr);

  /* Validate the expression.
     Was the expression an assignment?
     Or even an expression at all?  */
  if (expr->nelts == 0 || expr->elts[0].opcode != BINOP_ASSIGN)
    error (_("Init-if-undefined requires an assignment expression."));

  /* Extract the variable from the parsed expression.
     In the case of an assign the lvalue will be in elts[1] and elts[2].  */
  if (expr->elts[1].opcode != OP_INTERNALVAR)
    error (_("The first parameter to init-if-undefined "
	     "should be a GDB variable."));
  intvar = expr->elts[2].internalvar;

  /* Only evaluate the expression if the lvalue is void.
     This may still fail if the expresssion is invalid.  */
  if (intvar->kind == INTERNALVAR_VOID)
    evaluate_expression (expr);

  do_cleanups (old_chain);
}


/* Look up an internal variable with name NAME.  NAME should not
   normally include a dollar sign.

   If the specified internal variable does not exist,
   the return value is NULL.  */

struct internalvar *
lookup_only_internalvar (const char *name)
{
  struct internalvar *var;

  for (var = internalvars; var; var = var->next)
    if (strcmp (var->name, name) == 0)
      return var;

  return NULL;
}


/* Create an internal variable with name NAME and with a void value.
   NAME should not normally include a dollar sign.  */

struct internalvar *
create_internalvar (const char *name)
{
  struct internalvar *var;

  var = (struct internalvar *) xmalloc (sizeof (struct internalvar));
  var->name = concat (name, (char *)NULL);
  var->kind = INTERNALVAR_VOID;
  var->next = internalvars;
  internalvars = var;
  return var;
}

/* Create an internal variable with name NAME and register FUN as the
   function that value_of_internalvar uses to create a value whenever
   this variable is referenced.  NAME should not normally include a
   dollar sign.  */

struct internalvar *
create_internalvar_type_lazy (char *name, internalvar_make_value fun)
{
  struct internalvar *var = create_internalvar (name);

  var->kind = INTERNALVAR_MAKE_VALUE;
  var->u.make_value = fun;
  return var;
}

/* Look up an internal variable with name NAME.  NAME should not
   normally include a dollar sign.

   If the specified internal variable does not exist,
   one is created, with a void value.  */

struct internalvar *
lookup_internalvar (const char *name)
{
  struct internalvar *var;

  var = lookup_only_internalvar (name);
  if (var)
    return var;

  return create_internalvar (name);
}

/* Return current value of internal variable VAR.  For variables that
   are not inherently typed, use a value type appropriate for GDBARCH.  */

struct value *
value_of_internalvar (struct gdbarch *gdbarch, struct internalvar *var)
{
  struct value *val;
  struct trace_state_variable *tsv;

  /* If there is a trace state variable of the same name, assume that
     is what we really want to see.  */
  tsv = find_trace_state_variable (var->name);
  if (tsv)
    {
      tsv->value_known = target_get_trace_state_variable_value (tsv->number,
								&(tsv->value));
      if (tsv->value_known)
	val = value_from_longest (builtin_type (gdbarch)->builtin_int64,
				  tsv->value);
      else
	val = allocate_value (builtin_type (gdbarch)->builtin_void);
      return val;
    }

  switch (var->kind)
    {
    case INTERNALVAR_VOID:
      val = allocate_value (builtin_type (gdbarch)->builtin_void);
      break;

    case INTERNALVAR_FUNCTION:
      val = allocate_value (builtin_type (gdbarch)->internal_fn);
      break;

    case INTERNALVAR_INTEGER:
      if (!var->u.integer.type)
	val = value_from_longest (builtin_type (gdbarch)->builtin_int,
				  var->u.integer.val);
      else
	val = value_from_longest (var->u.integer.type, var->u.integer.val);
      break;

    case INTERNALVAR_STRING:
      val = value_cstring (var->u.string, strlen (var->u.string),
			   builtin_type (gdbarch)->builtin_char);
      break;

    case INTERNALVAR_VALUE:
      val = value_copy (var->u.value);
      if (value_lazy (val))
	value_fetch_lazy (val);
      break;

    case INTERNALVAR_MAKE_VALUE:
      val = (*var->u.make_value) (gdbarch, var);
      break;

    default:
      internal_error (__FILE__, __LINE__, _("bad kind"));
    }

  /* Change the VALUE_LVAL to lval_internalvar so that future operations
     on this value go back to affect the original internal variable.

     Do not do this for INTERNALVAR_MAKE_VALUE variables, as those have
     no underlying modifyable state in the internal variable.

     Likewise, if the variable's value is a computed lvalue, we want
     references to it to produce another computed lvalue, where
     references and assignments actually operate through the
     computed value's functions.

     This means that internal variables with computed values
     behave a little differently from other internal variables:
     assignments to them don't just replace the previous value
     altogether.  At the moment, this seems like the behavior we
     want.  */

  if (var->kind != INTERNALVAR_MAKE_VALUE
      && val->lval != lval_computed)
    {
      VALUE_LVAL (val) = lval_internalvar;
      VALUE_INTERNALVAR (val) = var;
    }

  return val;
}

int
get_internalvar_integer (struct internalvar *var, LONGEST *result)
{
  if (var->kind == INTERNALVAR_INTEGER)
    {
      *result = var->u.integer.val;
      return 1;
    }

  if (var->kind == INTERNALVAR_VALUE)
    {
      struct type *type = check_typedef (value_type (var->u.value));

      if (TYPE_CODE (type) == TYPE_CODE_INT)
	{
	  *result = value_as_long (var->u.value);
	  return 1;
	}
    }

  return 0;
}

static int
get_internalvar_function (struct internalvar *var,
			  struct internal_function **result)
{
  switch (var->kind)
    {
    case INTERNALVAR_FUNCTION:
      *result = var->u.fn.function;
      return 1;

    default:
      return 0;
    }
}

void
set_internalvar_component (struct internalvar *var, int offset, int bitpos,
			   int bitsize, struct value *newval)
{
  gdb_byte *addr;

  switch (var->kind)
    {
    case INTERNALVAR_VALUE:
      addr = value_contents_writeable (var->u.value);

      if (bitsize)
	modify_field (value_type (var->u.value), addr + offset,
		      value_as_long (newval), bitpos, bitsize);
      else
	memcpy (addr + offset, value_contents (newval),
		TYPE_LENGTH (value_type (newval)));
      break;

    default:
      /* We can never get a component of any other kind.  */
      internal_error (__FILE__, __LINE__, _("set_internalvar_component"));
    }
}

void
set_internalvar (struct internalvar *var, struct value *val)
{
  enum internalvar_kind new_kind;
  union internalvar_data new_data = { 0 };

  if (var->kind == INTERNALVAR_FUNCTION && var->u.fn.canonical)
    error (_("Cannot overwrite convenience function %s"), var->name);

  /* Prepare new contents.  */
  switch (TYPE_CODE (check_typedef (value_type (val))))
    {
    case TYPE_CODE_VOID:
      new_kind = INTERNALVAR_VOID;
      break;

    case TYPE_CODE_INTERNAL_FUNCTION:
      gdb_assert (VALUE_LVAL (val) == lval_internalvar);
      new_kind = INTERNALVAR_FUNCTION;
      get_internalvar_function (VALUE_INTERNALVAR (val),
				&new_data.fn.function);
      /* Copies created here are never canonical.  */
      break;

    default:
      new_kind = INTERNALVAR_VALUE;
      new_data.value = value_copy (val);
      new_data.value->modifiable = 1;

      /* Force the value to be fetched from the target now, to avoid problems
	 later when this internalvar is referenced and the target is gone or
	 has changed.  */
      if (value_lazy (new_data.value))
       value_fetch_lazy (new_data.value);

      /* Release the value from the value chain to prevent it from being
	 deleted by free_all_values.  From here on this function should not
	 call error () until new_data is installed into the var->u to avoid
	 leaking memory.  */
      release_value (new_data.value);
      break;
    }

  /* Clean up old contents.  */
  clear_internalvar (var);

  /* Switch over.  */
  var->kind = new_kind;
  var->u = new_data;
  /* End code which must not call error().  */
}

void
set_internalvar_integer (struct internalvar *var, LONGEST l)
{
  /* Clean up old contents.  */
  clear_internalvar (var);

  var->kind = INTERNALVAR_INTEGER;
  var->u.integer.type = NULL;
  var->u.integer.val = l;
}

void
set_internalvar_string (struct internalvar *var, const char *string)
{
  /* Clean up old contents.  */
  clear_internalvar (var);

  var->kind = INTERNALVAR_STRING;
  var->u.string = xstrdup (string);
}

static void
set_internalvar_function (struct internalvar *var, struct internal_function *f)
{
  /* Clean up old contents.  */
  clear_internalvar (var);

  var->kind = INTERNALVAR_FUNCTION;
  var->u.fn.function = f;
  var->u.fn.canonical = 1;
  /* Variables installed here are always the canonical version.  */
}

void
clear_internalvar (struct internalvar *var)
{
  /* Clean up old contents.  */
  switch (var->kind)
    {
    case INTERNALVAR_VALUE:
      value_free (var->u.value);
      break;

    case INTERNALVAR_STRING:
      xfree (var->u.string);
      break;

    default:
      break;
    }

  /* Reset to void kind.  */
  var->kind = INTERNALVAR_VOID;
}

char *
internalvar_name (struct internalvar *var)
{
  return var->name;
}

static struct internal_function *
create_internal_function (const char *name,
			  internal_function_fn handler, void *cookie)
{
  struct internal_function *ifn = XNEW (struct internal_function);

  ifn->name = xstrdup (name);
  ifn->handler = handler;
  ifn->cookie = cookie;
  return ifn;
}

char *
value_internal_function_name (struct value *val)
{
  struct internal_function *ifn;
  int result;

  gdb_assert (VALUE_LVAL (val) == lval_internalvar);
  result = get_internalvar_function (VALUE_INTERNALVAR (val), &ifn);
  gdb_assert (result);

  return ifn->name;
}

struct value *
call_internal_function (struct gdbarch *gdbarch,
			const struct language_defn *language,
			struct value *func, int argc, struct value **argv)
{
  struct internal_function *ifn;
  int result;

  gdb_assert (VALUE_LVAL (func) == lval_internalvar);
  result = get_internalvar_function (VALUE_INTERNALVAR (func), &ifn);
  gdb_assert (result);

  return (*ifn->handler) (gdbarch, language, ifn->cookie, argc, argv);
}

/* The 'function' command.  This does nothing -- it is just a
   placeholder to let "help function NAME" work.  This is also used as
   the implementation of the sub-command that is created when
   registering an internal function.  */
static void
function_command (char *command, int from_tty)
{
  /* Do nothing.  */
}

/* Clean up if an internal function's command is destroyed.  */
static void
function_destroyer (struct cmd_list_element *self, void *ignore)
{
  xfree (self->name);
  xfree (self->doc);
}

/* Add a new internal function.  NAME is the name of the function; DOC
   is a documentation string describing the function.  HANDLER is
   called when the function is invoked.  COOKIE is an arbitrary
   pointer which is passed to HANDLER and is intended for "user
   data".  */
void
add_internal_function (const char *name, const char *doc,
		       internal_function_fn handler, void *cookie)
{
  struct cmd_list_element *cmd;
  struct internal_function *ifn;
  struct internalvar *var = lookup_internalvar (name);

  ifn = create_internal_function (name, handler, cookie);
  set_internalvar_function (var, ifn);

  cmd = add_cmd (xstrdup (name), no_class, function_command, (char *) doc,
		 &functionlist);
  cmd->destroyer = function_destroyer;
}

/* Update VALUE before discarding OBJFILE.  COPIED_TYPES is used to
   prevent cycles / duplicates.  */

void
preserve_one_value (struct value *value, struct objfile *objfile,
		    htab_t copied_types)
{
  if (TYPE_OBJFILE (value->type) == objfile)
    value->type = copy_type_recursive (objfile, value->type, copied_types);

  if (TYPE_OBJFILE (value->enclosing_type) == objfile)
    value->enclosing_type = copy_type_recursive (objfile,
						 value->enclosing_type,
						 copied_types);
}

/* Likewise for internal variable VAR.  */

static void
preserve_one_internalvar (struct internalvar *var, struct objfile *objfile,
			  htab_t copied_types)
{
  switch (var->kind)
    {
    case INTERNALVAR_INTEGER:
      if (var->u.integer.type && TYPE_OBJFILE (var->u.integer.type) == objfile)
	var->u.integer.type
	  = copy_type_recursive (objfile, var->u.integer.type, copied_types);
      break;

    case INTERNALVAR_VALUE:
      preserve_one_value (var->u.value, objfile, copied_types);
      break;
    }
}

/* Update the internal variables and value history when OBJFILE is
   discarded; we must copy the types out of the objfile.  New global types
   will be created for every convenience variable which currently points to
   this objfile's types, and the convenience variables will be adjusted to
   use the new global types.  */

void
preserve_values (struct objfile *objfile)
{
  htab_t copied_types;
  struct value_history_chunk *cur;
  struct internalvar *var;
  int i;

  /* Create the hash table.  We allocate on the objfile's obstack, since
     it is soon to be deleted.  */
  copied_types = create_copied_types_hash (objfile);

  for (cur = value_history_chain; cur; cur = cur->next)
    for (i = 0; i < VALUE_HISTORY_CHUNK; i++)
      if (cur->values[i])
	preserve_one_value (cur->values[i], objfile, copied_types);

  for (var = internalvars; var; var = var->next)
    preserve_one_internalvar (var, objfile, copied_types);

  preserve_python_values (objfile, copied_types);

  htab_delete (copied_types);
}

static void
show_convenience (char *ignore, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct internalvar *var;
  int varseen = 0;
  struct value_print_options opts;

  get_user_print_options (&opts);
  for (var = internalvars; var; var = var->next)
    {
      if (!varseen)
	{
	  varseen = 1;
	}
      printf_filtered (("$%s = "), var->name);
      value_print (value_of_internalvar (gdbarch, var), gdb_stdout,
		   &opts);
      printf_filtered (("\n"));
    }
  if (!varseen)
    printf_unfiltered (_("No debugger convenience variables now defined.\n"
			 "Convenience variables have "
			 "names starting with \"$\";\n"
			 "use \"set\" as in \"set "
			 "$foo = 5\" to define them.\n"));
}

/* Extract a value as a C number (either long or double).
   Knows how to convert fixed values to double, or
   floating values to long.
   Does not deallocate the value.  */

LONGEST
value_as_long (struct value *val)
{
  /* This coerces arrays and functions, which is necessary (e.g.
     in disassemble_command).  It also dereferences references, which
     I suspect is the most logical thing to do.  */
  val = coerce_array (val);
  return unpack_long (value_type (val), value_contents (val));
}

DOUBLEST
value_as_double (struct value *val)
{
  DOUBLEST foo;
  int inv;

  foo = unpack_double (value_type (val), value_contents (val), &inv);
  if (inv)
    error (_("Invalid floating value found in program."));
  return foo;
}

/* Extract a value as a C pointer.  Does not deallocate the value.
   Note that val's type may not actually be a pointer; value_as_long
   handles all the cases.  */
CORE_ADDR
value_as_address (struct value *val)
{
  struct gdbarch *gdbarch = get_type_arch (value_type (val));

  /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
     whether we want this to be true eventually.  */
#if 0
  /* gdbarch_addr_bits_remove is wrong if we are being called for a
     non-address (e.g. argument to "signal", "info break", etc.), or
     for pointers to char, in which the low bits *are* significant.  */
  return gdbarch_addr_bits_remove (gdbarch, value_as_long (val));
#else

  /* There are several targets (IA-64, PowerPC, and others) which
     don't represent pointers to functions as simply the address of
     the function's entry point.  For example, on the IA-64, a
     function pointer points to a two-word descriptor, generated by
     the linker, which contains the function's entry point, and the
     value the IA-64 "global pointer" register should have --- to
     support position-independent code.  The linker generates
     descriptors only for those functions whose addresses are taken.

     On such targets, it's difficult for GDB to convert an arbitrary
     function address into a function pointer; it has to either find
     an existing descriptor for that function, or call malloc and
     build its own.  On some targets, it is impossible for GDB to
     build a descriptor at all: the descriptor must contain a jump
     instruction; data memory cannot be executed; and code memory
     cannot be modified.

     Upon entry to this function, if VAL is a value of type `function'
     (that is, TYPE_CODE (VALUE_TYPE (val)) == TYPE_CODE_FUNC), then
     value_address (val) is the address of the function.  This is what
     you'll get if you evaluate an expression like `main'.  The call
     to COERCE_ARRAY below actually does all the usual unary
     conversions, which includes converting values of type `function'
     to `pointer to function'.  This is the challenging conversion
     discussed above.  Then, `unpack_long' will convert that pointer
     back into an address.

     So, suppose the user types `disassemble foo' on an architecture
     with a strange function pointer representation, on which GDB
     cannot build its own descriptors, and suppose further that `foo'
     has no linker-built descriptor.  The address->pointer conversion
     will signal an error and prevent the command from running, even
     though the next step would have been to convert the pointer
     directly back into the same address.

     The following shortcut avoids this whole mess.  If VAL is a
     function, just return its address directly.  */
  if (TYPE_CODE (value_type (val)) == TYPE_CODE_FUNC
      || TYPE_CODE (value_type (val)) == TYPE_CODE_METHOD)
    return value_address (val);

  val = coerce_array (val);

  /* Some architectures (e.g. Harvard), map instruction and data
     addresses onto a single large unified address space.  For
     instance: An architecture may consider a large integer in the
     range 0x10000000 .. 0x1000ffff to already represent a data
     addresses (hence not need a pointer to address conversion) while
     a small integer would still need to be converted integer to
     pointer to address.  Just assume such architectures handle all
     integer conversions in a single function.  */

  /* JimB writes:

     I think INTEGER_TO_ADDRESS is a good idea as proposed --- but we
     must admonish GDB hackers to make sure its behavior matches the
     compiler's, whenever possible.

     In general, I think GDB should evaluate expressions the same way
     the compiler does.  When the user copies an expression out of
     their source code and hands it to a `print' command, they should
     get the same value the compiler would have computed.  Any
     deviation from this rule can cause major confusion and annoyance,
     and needs to be justified carefully.  In other words, GDB doesn't
     really have the freedom to do these conversions in clever and
     useful ways.

     AndrewC pointed out that users aren't complaining about how GDB
     casts integers to pointers; they are complaining that they can't
     take an address from a disassembly listing and give it to `x/i'.
     This is certainly important.

     Adding an architecture method like integer_to_address() certainly
     makes it possible for GDB to "get it right" in all circumstances
     --- the target has complete control over how things get done, so
     people can Do The Right Thing for their target without breaking
     anyone else.  The standard doesn't specify how integers get
     converted to pointers; usually, the ABI doesn't either, but
     ABI-specific code is a more reasonable place to handle it.  */

  if (TYPE_CODE (value_type (val)) != TYPE_CODE_PTR
      && TYPE_CODE (value_type (val)) != TYPE_CODE_REF
      && gdbarch_integer_to_address_p (gdbarch))
    return gdbarch_integer_to_address (gdbarch, value_type (val),
				       value_contents (val));

  return unpack_long (value_type (val), value_contents (val));
#endif
}

/* Unpack raw data (copied from debugee, target byte order) at VALADDR
   as a long, or as a double, assuming the raw data is described
   by type TYPE.  Knows how to convert different sizes of values
   and can convert between fixed and floating point.  We don't assume
   any alignment for the raw data.  Return value is in host byte order.

   If you want functions and arrays to be coerced to pointers, and
   references to be dereferenced, call value_as_long() instead.

   C++: It is assumed that the front-end has taken care of
   all matters concerning pointers to members.  A pointer
   to member which reaches here is considered to be equivalent
   to an INT (or some size).  After all, it is only an offset.  */

LONGEST
unpack_long (struct type *type, const gdb_byte *valaddr)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  enum type_code code = TYPE_CODE (type);
  int len = TYPE_LENGTH (type);
  int nosign = TYPE_UNSIGNED (type);

  switch (code)
    {
    case TYPE_CODE_TYPEDEF:
      return unpack_long (check_typedef (type), valaddr);
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_MEMBERPTR:
      if (nosign)
	return extract_unsigned_integer (valaddr, len, byte_order);
      else
	return extract_signed_integer (valaddr, len, byte_order);

    case TYPE_CODE_FLT:
      return extract_typed_floating (valaddr, type);

    case TYPE_CODE_DECFLOAT:
      /* libdecnumber has a function to convert from decimal to integer, but
	 it doesn't work when the decimal number has a fractional part.  */
      return decimal_to_doublest (valaddr, len, byte_order);

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
         whether we want this to be true eventually.  */
      return extract_typed_address (valaddr, type);

    default:
      error (_("Value can't be converted to integer."));
    }
  return 0;			/* Placate lint.  */
}

/* Return a double value from the specified type and address.
   INVP points to an int which is set to 0 for valid value,
   1 for invalid value (bad float format).  In either case,
   the returned double is OK to use.  Argument is in target
   format, result is in host format.  */

DOUBLEST
unpack_double (struct type *type, const gdb_byte *valaddr, int *invp)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  enum type_code code;
  int len;
  int nosign;

  *invp = 0;			/* Assume valid.  */
  CHECK_TYPEDEF (type);
  code = TYPE_CODE (type);
  len = TYPE_LENGTH (type);
  nosign = TYPE_UNSIGNED (type);
  if (code == TYPE_CODE_FLT)
    {
      /* NOTE: cagney/2002-02-19: There was a test here to see if the
	 floating-point value was valid (using the macro
	 INVALID_FLOAT).  That test/macro have been removed.

	 It turns out that only the VAX defined this macro and then
	 only in a non-portable way.  Fixing the portability problem
	 wouldn't help since the VAX floating-point code is also badly
	 bit-rotten.  The target needs to add definitions for the
	 methods gdbarch_float_format and gdbarch_double_format - these
	 exactly describe the target floating-point format.  The
	 problem here is that the corresponding floatformat_vax_f and
	 floatformat_vax_d values these methods should be set to are
	 also not defined either.  Oops!

         Hopefully someone will add both the missing floatformat
         definitions and the new cases for floatformat_is_valid ().  */

      if (!floatformat_is_valid (floatformat_from_type (type), valaddr))
	{
	  *invp = 1;
	  return 0.0;
	}

      return extract_typed_floating (valaddr, type);
    }
  else if (code == TYPE_CODE_DECFLOAT)
    return decimal_to_doublest (valaddr, len, byte_order);
  else if (nosign)
    {
      /* Unsigned -- be sure we compensate for signed LONGEST.  */
      return (ULONGEST) unpack_long (type, valaddr);
    }
  else
    {
      /* Signed -- we are OK with unpack_long.  */
      return unpack_long (type, valaddr);
    }
}

/* Unpack raw data (copied from debugee, target byte order) at VALADDR
   as a CORE_ADDR, assuming the raw data is described by type TYPE.
   We don't assume any alignment for the raw data.  Return value is in
   host byte order.

   If you want functions and arrays to be coerced to pointers, and
   references to be dereferenced, call value_as_address() instead.

   C++: It is assumed that the front-end has taken care of
   all matters concerning pointers to members.  A pointer
   to member which reaches here is considered to be equivalent
   to an INT (or some size).  After all, it is only an offset.  */

CORE_ADDR
unpack_pointer (struct type *type, const gdb_byte *valaddr)
{
  /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
     whether we want this to be true eventually.  */
  return unpack_long (type, valaddr);
}


/* Get the value of the FIELDNO'th field (which must be static) of
   TYPE.  Return NULL if the field doesn't exist or has been
   optimized out.  */

struct value *
value_static_field (struct type *type, int fieldno)
{
  struct value *retval;

  switch (TYPE_FIELD_LOC_KIND (type, fieldno))
    {
    case FIELD_LOC_KIND_PHYSADDR:
      retval = value_at_lazy (TYPE_FIELD_TYPE (type, fieldno),
			      TYPE_FIELD_STATIC_PHYSADDR (type, fieldno));
      break;
    case FIELD_LOC_KIND_PHYSNAME:
    {
      char *phys_name = TYPE_FIELD_STATIC_PHYSNAME (type, fieldno);
      /* TYPE_FIELD_NAME (type, fieldno); */
      struct symbol *sym = lookup_symbol (phys_name, 0, VAR_DOMAIN, 0);

      if (sym == NULL)
	{
	  /* With some compilers, e.g. HP aCC, static data members are
	     reported as non-debuggable symbols.  */
	  struct minimal_symbol *msym = lookup_minimal_symbol (phys_name,
							       NULL, NULL);

	  if (!msym)
	    return NULL;
	  else
	    {
	      retval = value_at_lazy (TYPE_FIELD_TYPE (type, fieldno),
				      SYMBOL_VALUE_ADDRESS (msym));
	    }
	}
      else
	retval = value_of_variable (sym, NULL);
      break;
    }
    default:
      gdb_assert_not_reached ("unexpected field location kind");
    }

  return retval;
}

/* Change the enclosing type of a value object VAL to NEW_ENCL_TYPE.
   You have to be careful here, since the size of the data area for the value
   is set by the length of the enclosing type.  So if NEW_ENCL_TYPE is bigger
   than the old enclosing type, you have to allocate more space for the
   data.  */

void
set_value_enclosing_type (struct value *val, struct type *new_encl_type)
{
  if (TYPE_LENGTH (new_encl_type) > TYPE_LENGTH (value_enclosing_type (val))) 
    val->contents =
      (gdb_byte *) xrealloc (val->contents, TYPE_LENGTH (new_encl_type));

  val->enclosing_type = new_encl_type;
}

/* Given a value ARG1 (offset by OFFSET bytes)
   of a struct or union type ARG_TYPE,
   extract and return the value of one of its (non-static) fields.
   FIELDNO says which field.  */

struct value *
value_primitive_field (struct value *arg1, int offset,
		       int fieldno, struct type *arg_type)
{
  struct value *v;
  struct type *type;

  CHECK_TYPEDEF (arg_type);
  type = TYPE_FIELD_TYPE (arg_type, fieldno);

  /* Call check_typedef on our type to make sure that, if TYPE
     is a TYPE_CODE_TYPEDEF, its length is set to the length
     of the target type instead of zero.  However, we do not
     replace the typedef type by the target type, because we want
     to keep the typedef in order to be able to print the type
     description correctly.  */
  check_typedef (type);

  /* Handle packed fields */

  if (TYPE_FIELD_BITSIZE (arg_type, fieldno))
    {
      /* Create a new value for the bitfield, with bitpos and bitsize
	 set.  If possible, arrange offset and bitpos so that we can
	 do a single aligned read of the size of the containing type.
	 Otherwise, adjust offset to the byte containing the first
	 bit.  Assume that the address, offset, and embedded offset
	 are sufficiently aligned.  */
      int bitpos = TYPE_FIELD_BITPOS (arg_type, fieldno);
      int container_bitsize = TYPE_LENGTH (type) * 8;

      v = allocate_value_lazy (type);
      v->bitsize = TYPE_FIELD_BITSIZE (arg_type, fieldno);
      if ((bitpos % container_bitsize) + v->bitsize <= container_bitsize
	  && TYPE_LENGTH (type) <= (int) sizeof (LONGEST))
	v->bitpos = bitpos % container_bitsize;
      else
	v->bitpos = bitpos % 8;
      v->offset = (value_embedded_offset (arg1)
		   + offset
		   + (bitpos - v->bitpos) / 8);
      v->parent = arg1;
      value_incref (v->parent);
      if (!value_lazy (arg1))
	value_fetch_lazy (v);
    }
  else if (fieldno < TYPE_N_BASECLASSES (arg_type))
    {
      /* This field is actually a base subobject, so preserve the
	 entire object's contents for later references to virtual
	 bases, etc.  */

      /* Lazy register values with offsets are not supported.  */
      if (VALUE_LVAL (arg1) == lval_register && value_lazy (arg1))
	value_fetch_lazy (arg1);

      if (value_lazy (arg1))
	v = allocate_value_lazy (value_enclosing_type (arg1));
      else
	{
	  v = allocate_value (value_enclosing_type (arg1));
	  value_contents_copy_raw (v, 0, arg1, 0,
				   TYPE_LENGTH (value_enclosing_type (arg1)));
	}
      v->type = type;
      v->offset = value_offset (arg1);
      v->embedded_offset = (offset + value_embedded_offset (arg1)
			    + TYPE_FIELD_BITPOS (arg_type, fieldno) / 8);
    }
  else
    {
      /* Plain old data member */
      offset += TYPE_FIELD_BITPOS (arg_type, fieldno) / 8;

      /* Lazy register values with offsets are not supported.  */
      if (VALUE_LVAL (arg1) == lval_register && value_lazy (arg1))
	value_fetch_lazy (arg1);

      if (value_lazy (arg1))
	v = allocate_value_lazy (type);
      else
	{
	  v = allocate_value (type);
	  value_contents_copy_raw (v, value_embedded_offset (v),
				   arg1, value_embedded_offset (arg1) + offset,
				   TYPE_LENGTH (type));
	}
      v->offset = (value_offset (arg1) + offset
		   + value_embedded_offset (arg1));
    }
  set_value_component_location (v, arg1);
  VALUE_REGNUM (v) = VALUE_REGNUM (arg1);
  VALUE_FRAME_ID (v) = VALUE_FRAME_ID (arg1);
  return v;
}

/* Given a value ARG1 of a struct or union type,
   extract and return the value of one of its (non-static) fields.
   FIELDNO says which field.  */

struct value *
value_field (struct value *arg1, int fieldno)
{
  return value_primitive_field (arg1, 0, fieldno, value_type (arg1));
}

/* Return a non-virtual function as a value.
   F is the list of member functions which contains the desired method.
   J is an index into F which provides the desired method.

   We only use the symbol for its address, so be happy with either a
   full symbol or a minimal symbol.  */

struct value *
value_fn_field (struct value **arg1p, struct fn_field *f,
		int j, struct type *type,
		int offset)
{
  struct value *v;
  struct type *ftype = TYPE_FN_FIELD_TYPE (f, j);
  char *physname = TYPE_FN_FIELD_PHYSNAME (f, j);
  struct symbol *sym;
  struct minimal_symbol *msym;

  sym = lookup_symbol (physname, 0, VAR_DOMAIN, 0);
  if (sym != NULL)
    {
      msym = NULL;
    }
  else
    {
      gdb_assert (sym == NULL);
      msym = lookup_minimal_symbol (physname, NULL, NULL);
      if (msym == NULL)
	return NULL;
    }

  v = allocate_value (ftype);
  if (sym)
    {
      set_value_address (v, BLOCK_START (SYMBOL_BLOCK_VALUE (sym)));
    }
  else
    {
      /* The minimal symbol might point to a function descriptor;
	 resolve it to the actual code address instead.  */
      struct objfile *objfile = msymbol_objfile (msym);
      struct gdbarch *gdbarch = get_objfile_arch (objfile);

      set_value_address (v,
	gdbarch_convert_from_func_ptr_addr
	   (gdbarch, SYMBOL_VALUE_ADDRESS (msym), &current_target));
    }

  if (arg1p)
    {
      if (type != value_type (*arg1p))
	*arg1p = value_ind (value_cast (lookup_pointer_type (type),
					value_addr (*arg1p)));

      /* Move the `this' pointer according to the offset.
         VALUE_OFFSET (*arg1p) += offset; */
    }

  return v;
}



/* Helper function for both unpack_value_bits_as_long and
   unpack_bits_as_long.  See those functions for more details on the
   interface; the only difference is that this function accepts either
   a NULL or a non-NULL ORIGINAL_VALUE.  */

static int
unpack_value_bits_as_long_1 (struct type *field_type, const gdb_byte *valaddr,
			     int embedded_offset, int bitpos, int bitsize,
			     const struct value *original_value,
			     LONGEST *result)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (field_type));
  ULONGEST val;
  ULONGEST valmask;
  int lsbcount;
  int bytes_read;
  int read_offset;

  /* Read the minimum number of bytes required; there may not be
     enough bytes to read an entire ULONGEST.  */
  CHECK_TYPEDEF (field_type);
  if (bitsize)
    bytes_read = ((bitpos % 8) + bitsize + 7) / 8;
  else
    bytes_read = TYPE_LENGTH (field_type);

  read_offset = bitpos / 8;

  if (original_value != NULL
      && !value_bytes_available (original_value, embedded_offset + read_offset,
				 bytes_read))
    return 0;

  val = extract_unsigned_integer (valaddr + embedded_offset + read_offset,
				  bytes_read, byte_order);

  /* Extract bits.  See comment above.  */

  if (gdbarch_bits_big_endian (get_type_arch (field_type)))
    lsbcount = (bytes_read * 8 - bitpos % 8 - bitsize);
  else
    lsbcount = (bitpos % 8);
  val >>= lsbcount;

  /* If the field does not entirely fill a LONGEST, then zero the sign bits.
     If the field is signed, and is negative, then sign extend.  */

  if ((bitsize > 0) && (bitsize < 8 * (int) sizeof (val)))
    {
      valmask = (((ULONGEST) 1) << bitsize) - 1;
      val &= valmask;
      if (!TYPE_UNSIGNED (field_type))
	{
	  if (val & (valmask ^ (valmask >> 1)))
	    {
	      val |= ~valmask;
	    }
	}
    }

  *result = val;
  return 1;
}

/* Unpack a bitfield of the specified FIELD_TYPE, from the object at
   VALADDR + EMBEDDED_OFFSET, and store the result in *RESULT.
   VALADDR points to the contents of ORIGINAL_VALUE, which must not be
   NULL.  The bitfield starts at BITPOS bits and contains BITSIZE
   bits.

   Returns false if the value contents are unavailable, otherwise
   returns true, indicating a valid value has been stored in *RESULT.

   Extracting bits depends on endianness of the machine.  Compute the
   number of least significant bits to discard.  For big endian machines,
   we compute the total number of bits in the anonymous object, subtract
   off the bit count from the MSB of the object to the MSB of the
   bitfield, then the size of the bitfield, which leaves the LSB discard
   count.  For little endian machines, the discard count is simply the
   number of bits from the LSB of the anonymous object to the LSB of the
   bitfield.

   If the field is signed, we also do sign extension.  */

int
unpack_value_bits_as_long (struct type *field_type, const gdb_byte *valaddr,
			   int embedded_offset, int bitpos, int bitsize,
			   const struct value *original_value,
			   LONGEST *result)
{
  gdb_assert (original_value != NULL);

  return unpack_value_bits_as_long_1 (field_type, valaddr, embedded_offset,
				      bitpos, bitsize, original_value, result);

}

/* Unpack a field FIELDNO of the specified TYPE, from the object at
   VALADDR + EMBEDDED_OFFSET.  VALADDR points to the contents of
   ORIGINAL_VALUE.  See unpack_value_bits_as_long for more
   details.  */

static int
unpack_value_field_as_long_1 (struct type *type, const gdb_byte *valaddr,
			      int embedded_offset, int fieldno,
			      const struct value *val, LONGEST *result)
{
  int bitpos = TYPE_FIELD_BITPOS (type, fieldno);
  int bitsize = TYPE_FIELD_BITSIZE (type, fieldno);
  struct type *field_type = TYPE_FIELD_TYPE (type, fieldno);

  return unpack_value_bits_as_long_1 (field_type, valaddr, embedded_offset,
				      bitpos, bitsize, val,
				      result);
}

/* Unpack a field FIELDNO of the specified TYPE, from the object at
   VALADDR + EMBEDDED_OFFSET.  VALADDR points to the contents of
   ORIGINAL_VALUE, which must not be NULL.  See
   unpack_value_bits_as_long for more details.  */

int
unpack_value_field_as_long (struct type *type, const gdb_byte *valaddr,
			    int embedded_offset, int fieldno,
			    const struct value *val, LONGEST *result)
{
  gdb_assert (val != NULL);

  return unpack_value_field_as_long_1 (type, valaddr, embedded_offset,
				       fieldno, val, result);
}

/* Unpack a field FIELDNO of the specified TYPE, from the anonymous
   object at VALADDR.  See unpack_value_bits_as_long for more details.
   This function differs from unpack_value_field_as_long in that it
   operates without a struct value object.  */

LONGEST
unpack_field_as_long (struct type *type, const gdb_byte *valaddr, int fieldno)
{
  LONGEST result;

  unpack_value_field_as_long_1 (type, valaddr, 0, fieldno, NULL, &result);
  return result;
}

/* Return a new value with type TYPE, which is FIELDNO field of the
   object at VALADDR + EMBEDDEDOFFSET.  VALADDR points to the contents
   of VAL.  If the VAL's contents required to extract the bitfield
   from are unavailable, the new value is correspondingly marked as
   unavailable.  */

struct value *
value_field_bitfield (struct type *type, int fieldno,
		      const gdb_byte *valaddr,
		      int embedded_offset, const struct value *val)
{
  LONGEST l;

  if (!unpack_value_field_as_long (type, valaddr, embedded_offset, fieldno,
				   val, &l))
    {
      struct type *field_type = TYPE_FIELD_TYPE (type, fieldno);
      struct value *retval = allocate_value (field_type);
      mark_value_bytes_unavailable (retval, 0, TYPE_LENGTH (field_type));
      return retval;
    }
  else
    {
      return value_from_longest (TYPE_FIELD_TYPE (type, fieldno), l);
    }
}

/* Modify the value of a bitfield.  ADDR points to a block of memory in
   target byte order; the bitfield starts in the byte pointed to.  FIELDVAL
   is the desired value of the field, in host byte order.  BITPOS and BITSIZE
   indicate which bits (in target bit order) comprise the bitfield.
   Requires 0 < BITSIZE <= lbits, 0 <= BITPOS % 8 + BITSIZE <= lbits, and
   0 <= BITPOS, where lbits is the size of a LONGEST in bits.  */

void
modify_field (struct type *type, gdb_byte *addr,
	      LONGEST fieldval, int bitpos, int bitsize)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  ULONGEST oword;
  ULONGEST mask = (ULONGEST) -1 >> (8 * sizeof (ULONGEST) - bitsize);
  int bytesize;

  /* Normalize BITPOS.  */
  addr += bitpos / 8;
  bitpos %= 8;

  /* If a negative fieldval fits in the field in question, chop
     off the sign extension bits.  */
  if ((~fieldval & ~(mask >> 1)) == 0)
    fieldval &= mask;

  /* Warn if value is too big to fit in the field in question.  */
  if (0 != (fieldval & ~mask))
    {
      /* FIXME: would like to include fieldval in the message, but
         we don't have a sprintf_longest.  */
      warning (_("Value does not fit in %d bits."), bitsize);

      /* Truncate it, otherwise adjoining fields may be corrupted.  */
      fieldval &= mask;
    }

  /* Ensure no bytes outside of the modified ones get accessed as it may cause
     false valgrind reports.  */

  bytesize = (bitpos + bitsize + 7) / 8;
  oword = extract_unsigned_integer (addr, bytesize, byte_order);

  /* Shifting for bit field depends on endianness of the target machine.  */
  if (gdbarch_bits_big_endian (get_type_arch (type)))
    bitpos = bytesize * 8 - bitpos - bitsize;

  oword &= ~(mask << bitpos);
  oword |= fieldval << bitpos;

  store_unsigned_integer (addr, bytesize, byte_order, oword);
}

/* Pack NUM into BUF using a target format of TYPE.  */

void
pack_long (gdb_byte *buf, struct type *type, LONGEST num)
{
  enum bfd_endian byte_order = gdbarch_byte_order (get_type_arch (type));
  int len;

  type = check_typedef (type);
  len = TYPE_LENGTH (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_MEMBERPTR:
      store_signed_integer (buf, len, byte_order, num);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_PTR:
      store_typed_address (buf, type, (CORE_ADDR) num);
      break;

    default:
      error (_("Unexpected type (%d) encountered for integer constant."),
	     TYPE_CODE (type));
    }
}


/* Pack NUM into BUF using a target format of TYPE.  */

void
pack_unsigned_long (gdb_byte *buf, struct type *type, ULONGEST num)
{
  int len;
  enum bfd_endian byte_order;

  type = check_typedef (type);
  len = TYPE_LENGTH (type);
  byte_order = gdbarch_byte_order (get_type_arch (type));

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_MEMBERPTR:
      store_unsigned_integer (buf, len, byte_order, num);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_PTR:
      store_typed_address (buf, type, (CORE_ADDR) num);
      break;

    default:
      error (_("Unexpected type (%d) encountered "
	       "for unsigned integer constant."),
	     TYPE_CODE (type));
    }
}


/* Convert C numbers into newly allocated values.  */

struct value *
value_from_longest (struct type *type, LONGEST num)
{
  struct value *val = allocate_value (type);

  pack_long (value_contents_raw (val), type, num);
  return val;
}


/* Convert C unsigned numbers into newly allocated values.  */

struct value *
value_from_ulongest (struct type *type, ULONGEST num)
{
  struct value *val = allocate_value (type);

  pack_unsigned_long (value_contents_raw (val), type, num);

  return val;
}


/* Create a value representing a pointer of type TYPE to the address
   ADDR.  */
struct value *
value_from_pointer (struct type *type, CORE_ADDR addr)
{
  struct value *val = allocate_value (type);

  store_typed_address (value_contents_raw (val), check_typedef (type), addr);
  return val;
}


/* Create a value of type TYPE whose contents come from VALADDR, if it
   is non-null, and whose memory address (in the inferior) is
   ADDRESS.  */

struct value *
value_from_contents_and_address (struct type *type,
				 const gdb_byte *valaddr,
				 CORE_ADDR address)
{
  struct value *v;

  if (valaddr == NULL)
    v = allocate_value_lazy (type);
  else
    {
      v = allocate_value (type);
      memcpy (value_contents_raw (v), valaddr, TYPE_LENGTH (type));
    }
  set_value_address (v, address);
  VALUE_LVAL (v) = lval_memory;
  return v;
}

struct value *
value_from_double (struct type *type, DOUBLEST num)
{
  struct value *val = allocate_value (type);
  struct type *base_type = check_typedef (type);
  enum type_code code = TYPE_CODE (base_type);

  if (code == TYPE_CODE_FLT)
    {
      store_typed_floating (value_contents_raw (val), base_type, num);
    }
  else
    error (_("Unexpected type encountered for floating constant."));

  return val;
}

struct value *
value_from_decfloat (struct type *type, const gdb_byte *dec)
{
  struct value *val = allocate_value (type);

  memcpy (value_contents_raw (val), dec, TYPE_LENGTH (type));
  return val;
}

/* Extract a value from the history file.  Input will be of the form
   $digits or $$digits.  See block comment above 'write_dollar_variable'
   for details.  */

struct value *
value_from_history_ref (char *h, char **endp)
{
  int index, len;

  if (h[0] == '$')
    len = 1;
  else
    return NULL;

  if (h[1] == '$')
    len = 2;

  /* Find length of numeral string.  */
  for (; isdigit (h[len]); len++)
    ;

  /* Make sure numeral string is not part of an identifier.  */
  if (h[len] == '_' || isalpha (h[len]))
    return NULL;

  /* Now collect the index value.  */
  if (h[1] == '$')
    {
      if (len == 2)
	{
	  /* For some bizarre reason, "$$" is equivalent to "$$1", 
	     rather than to "$$0" as it ought to be!  */
	  index = -1;
	  *endp += len;
	}
      else
	index = -strtol (&h[2], endp, 10);
    }
  else
    {
      if (len == 1)
	{
	  /* "$" is equivalent to "$0".  */
	  index = 0;
	  *endp += len;
	}
      else
	index = strtol (&h[1], endp, 10);
    }

  return access_value_history (index);
}

struct value *
coerce_ref (struct value *arg)
{
  struct type *value_type_arg_tmp = check_typedef (value_type (arg));

  if (TYPE_CODE (value_type_arg_tmp) == TYPE_CODE_REF)
    arg = value_at_lazy (TYPE_TARGET_TYPE (value_type_arg_tmp),
			 unpack_pointer (value_type (arg),		
					 value_contents (arg)));
  return arg;
}

struct value *
coerce_array (struct value *arg)
{
  struct type *type;

  arg = coerce_ref (arg);
  type = check_typedef (value_type (arg));

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      if (!TYPE_VECTOR (type) && current_language->c_style_arrays)
	arg = value_coerce_array (arg);
      break;
    case TYPE_CODE_FUNC:
      arg = value_coerce_function (arg);
      break;
    }
  return arg;
}


/* Return true if the function returning the specified type is using
   the convention of returning structures in memory (passing in the
   address as a hidden first parameter).  */

int
using_struct_return (struct gdbarch *gdbarch,
		     struct type *func_type, struct type *value_type)
{
  enum type_code code = TYPE_CODE (value_type);

  if (code == TYPE_CODE_ERROR)
    error (_("Function return type unknown."));

  if (code == TYPE_CODE_VOID)
    /* A void return value is never in memory.  See also corresponding
       code in "print_return_value".  */
    return 0;

  /* Probe the architecture for the return-value convention.  */
  return (gdbarch_return_value (gdbarch, func_type, value_type,
				NULL, NULL, NULL)
	  != RETURN_VALUE_REGISTER_CONVENTION);
}

/* Set the initialized field in a value struct.  */

void
set_value_initialized (struct value *val, int status)
{
  val->initialized = status;
}

/* Return the initialized field in a value struct.  */

int
value_initialized (struct value *val)
{
  return val->initialized;
}

void
_initialize_values (void)
{
  add_cmd ("convenience", no_class, show_convenience, _("\
Debugger convenience (\"$foo\") variables.\n\
These variables are created when you assign them values;\n\
thus, \"print $foo=1\" gives \"$foo\" the value 1.  Values may be any type.\n\
\n\
A few convenience variables are given values automatically:\n\
\"$_\"holds the last address examined with \"x\" or \"info lines\",\n\
\"$__\" holds the contents of the last address examined with \"x\"."),
	   &showlist);

  add_cmd ("values", no_set_class, show_values, _("\
Elements of value history around item number IDX (or last ten)."),
	   &showlist);

  add_com ("init-if-undefined", class_vars, init_if_undefined_command, _("\
Initialize a convenience variable if necessary.\n\
init-if-undefined VARIABLE = EXPRESSION\n\
Set an internal VARIABLE to the result of the EXPRESSION if it does not\n\
exist or does not contain a value.  The EXPRESSION is not evaluated if the\n\
VARIABLE is already initialized."));

  add_prefix_cmd ("function", no_class, function_command, _("\
Placeholder command for showing help on convenience functions."),
		  &functionlist, "function ", 0, &cmdlist);
}
