/* Low level packing and unpacking of values for GDB, the GNU Debugger.

   Copyright (C) 1986-2023 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "demangle.h"
#include "regcache.h"
#include "block.h"
#include "target-float.h"
#include "objfiles.h"
#include "valprint.h"
#include "cli/cli-decode.h"
#include "extension.h"
#include <ctype.h>
#include "tracepoint.h"
#include "cp-abi.h"
#include "user-regs.h"
#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include "completer.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/array-view.h"
#include "cli/cli-style.h"
#include "expop.h"
#include "inferior.h"
#include "varobj.h"

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
  LONGEST offset;

  /* Length of the range.  */
  LONGEST length;

  /* Returns true if THIS is strictly less than OTHER, useful for
     searching.  We keep ranges sorted by offset and coalesce
     overlapping and contiguous ranges, so this just compares the
     starting offset.  */

  bool operator< (const range &other) const
  {
    return offset < other.offset;
  }

  /* Returns true if THIS is equal to OTHER.  */
  bool operator== (const range &other) const
  {
    return offset == other.offset && length == other.length;
  }
};

/* Returns true if the ranges defined by [offset1, offset1+len1) and
   [offset2, offset2+len2) overlap.  */

static int
ranges_overlap (LONGEST offset1, LONGEST len1,
		LONGEST offset2, LONGEST len2)
{
  ULONGEST h, l;

  l = std::max (offset1, offset2);
  h = std::min (offset1 + len1, offset2 + len2);
  return (l < h);
}

/* Returns true if RANGES contains any range that overlaps [OFFSET,
   OFFSET+LENGTH).  */

static int
ranges_contain (const std::vector<range> &ranges, LONGEST offset,
		LONGEST length)
{
  range what;

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


  auto i = std::lower_bound (ranges.begin (), ranges.end (), what);

  if (i > ranges.begin ())
    {
      const struct range &bef = *(i - 1);

      if (ranges_overlap (bef.offset, bef.length, offset, length))
	return 1;
    }

  if (i < ranges.end ())
    {
      const struct range &r = *i;

      if (ranges_overlap (r.offset, r.length, offset, length))
	return 1;
    }

  return 0;
}

static struct cmd_list_element *functionlist;

/* Note that the fields in this structure are arranged to save a bit
   of memory.  */

struct value
{
  explicit value (struct type *type_)
    : modifiable (1),
      lazy (1),
      initialized (1),
      stack (0),
      is_zero (false),
      type (type_),
      enclosing_type (type_)
  {
  }

  ~value ()
  {
    if (VALUE_LVAL (this) == lval_computed)
      {
	const struct lval_funcs *funcs = location.computed.funcs;

	if (funcs->free_closure)
	  funcs->free_closure (this);
      }
    else if (VALUE_LVAL (this) == lval_xcallable)
      delete location.xm_worker;
  }

  DISABLE_COPY_AND_ASSIGN (value);

  /* Type of value; either not an lval, or one of the various
     different possible kinds of lval.  */
  enum lval_type lval = not_lval;

  /* Is it modifiable?  Only relevant if lval != not_lval.  */
  unsigned int modifiable : 1;

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
  unsigned int lazy : 1;

  /* If value is a variable, is it initialized or not.  */
  unsigned int initialized : 1;

  /* If value is from the stack.  If this is set, read_stack will be
     used instead of read_memory to enable extra caching.  */
  unsigned int stack : 1;

  /* True if this is a zero value, created by 'value_zero'; false
     otherwise.  */
  bool is_zero : 1;

  /* Location of value (if lval).  */
  union
  {
    /* If lval == lval_memory, this is the address in the inferior  */
    CORE_ADDR address;

    /*If lval == lval_register, the value is from a register.  */
    struct
    {
      /* Register number.  */
      int regnum;
      /* Frame ID of "next" frame to which a register value is relative.
	 If the register value is found relative to frame F, then the
	 frame id of F->next will be stored in next_frame_id.  */
      struct frame_id next_frame_id;
    } reg;

    /* Pointer to internal variable.  */
    struct internalvar *internalvar;

    /* Pointer to xmethod worker.  */
    struct xmethod_worker *xm_worker;

    /* If lval == lval_computed, this is a set of function pointers
       to use to access and describe the value, and a closure pointer
       for them to use.  */
    struct
    {
      /* Functions to call.  */
      const struct lval_funcs *funcs;

      /* Closure for those functions to use.  */
      void *closure;
    } computed;
  } location {};

  /* Describes offset of a value within lval of a structure in target
     addressable memory units.  Note also the member embedded_offset
     below.  */
  LONGEST offset = 0;

  /* Only used for bitfields; number of bits contained in them.  */
  LONGEST bitsize = 0;

  /* Only used for bitfields; position of start of field.  For
     little-endian targets, it is the position of the LSB.  For
     big-endian targets, it is the position of the MSB.  */
  LONGEST bitpos = 0;

  /* The number of references to this value.  When a value is created,
     the value chain holds a reference, so REFERENCE_COUNT is 1.  If
     release_value is called, this value is removed from the chain but
     the caller of release_value now has a reference to this value.
     The caller must arrange for a call to value_free later.  */
  int reference_count = 1;

  /* Only used for bitfields; the containing value.  This allows a
     single read from the target when displaying multiple
     bitfields.  */
  value_ref_ptr parent;

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
     offset of `type' within that larger type, in target addressable memory
     units.  The value_contents() macro takes `embedded_offset' into account,
     so most GDB code continues to see the `type' portion of the value, just
     as the inferior would.

     If `type' is a pointer to an object, then `enclosing_type' is a
     pointer to the object's run-time type, and `pointed_to_offset' is
     the offset in target addressable memory units from the full object
     to the pointed-to object -- that is, the value `embedded_offset' would
     have if we followed the pointer and fetched the complete object.
     (I don't really see the point.  Why not just determine the
     run-time type when you indirect, and avoid the special case?  The
     contents don't matter until you indirect anyway.)

     If we're not doing anything fancy, `enclosing_type' is equal to
     `type', and `embedded_offset' is zero, so everything works
     normally.  */
  struct type *enclosing_type;
  LONGEST embedded_offset = 0;
  LONGEST pointed_to_offset = 0;

  /* Actual contents of the value.  Target byte-order.

     May be nullptr if the value is lazy or is entirely optimized out.
     Guaranteed to be non-nullptr otherwise.  */
  gdb::unique_xmalloc_ptr<gdb_byte> contents;

  /* Unavailable ranges in CONTENTS.  We mark unavailable ranges,
     rather than available, since the common and default case is for a
     value to be available.  This is filled in at value read time.
     The unavailable ranges are tracked in bits.  Note that a contents
     bit that has been optimized out doesn't really exist in the
     program, so it can't be marked unavailable either.  */
  std::vector<range> unavailable;

  /* Likewise, but for optimized out contents (a chunk of the value of
     a variable that does not actually exist in the program).  If LVAL
     is lval_register, this is a register ($pc, $sp, etc., never a
     program variable) that has not been saved in the frame.  Not
     saved registers and optimized-out program variables values are
     treated pretty much the same, except not-saved registers have a
     different string representation and related error strings.  */
  std::vector<range> optimized_out;
};

/* See value.h.  */

struct gdbarch *
get_value_arch (const struct value *value)
{
  return value_type (value)->arch ();
}

int
value_bits_available (const struct value *value, LONGEST offset, LONGEST length)
{
  gdb_assert (!value->lazy);

  return !ranges_contain (value->unavailable, offset, length);
}

int
value_bytes_available (const struct value *value,
		       LONGEST offset, LONGEST length)
{
  return value_bits_available (value,
			       offset * TARGET_CHAR_BIT,
			       length * TARGET_CHAR_BIT);
}

int
value_bits_any_optimized_out (const struct value *value, int bit_offset, int bit_length)
{
  gdb_assert (!value->lazy);

  return ranges_contain (value->optimized_out, bit_offset, bit_length);
}

int
value_entirely_available (struct value *value)
{
  /* We can only tell whether the whole value is available when we try
     to read it.  */
  if (value->lazy)
    value_fetch_lazy (value);

  if (value->unavailable.empty ())
    return 1;
  return 0;
}

/* Returns true if VALUE is entirely covered by RANGES.  If the value
   is lazy, it'll be read now.  Note that RANGE is a pointer to
   pointer because reading the value might change *RANGE.  */

static int
value_entirely_covered_by_range_vector (struct value *value,
					const std::vector<range> &ranges)
{
  /* We can only tell whether the whole value is optimized out /
     unavailable when we try to read it.  */
  if (value->lazy)
    value_fetch_lazy (value);

  if (ranges.size () == 1)
    {
      const struct range &t = ranges[0];

      if (t.offset == 0
	  && t.length == (TARGET_CHAR_BIT
			  * value_enclosing_type (value)->length ()))
	return 1;
    }

  return 0;
}

int
value_entirely_unavailable (struct value *value)
{
  return value_entirely_covered_by_range_vector (value, value->unavailable);
}

int
value_entirely_optimized_out (struct value *value)
{
  return value_entirely_covered_by_range_vector (value, value->optimized_out);
}

/* Insert into the vector pointed to by VECTORP the bit range starting of
   OFFSET bits, and extending for the next LENGTH bits.  */

static void
insert_into_bit_range_vector (std::vector<range> *vectorp,
			      LONGEST offset, LONGEST length)
{
  range newr;

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

  auto i = std::lower_bound (vectorp->begin (), vectorp->end (), newr);
  if (i > vectorp->begin ())
    {
      struct range &bef = *(i - 1);

      if (ranges_overlap (bef.offset, bef.length, offset, length))
	{
	  /* #1 */
	  ULONGEST l = std::min (bef.offset, offset);
	  ULONGEST h = std::max (bef.offset + bef.length, offset + length);

	  bef.offset = l;
	  bef.length = h - l;
	  i--;
	}
      else if (offset == bef.offset + bef.length)
	{
	  /* #2 */
	  bef.length += length;
	  i--;
	}
      else
	{
	  /* #3 */
	  i = vectorp->insert (i, newr);
	}
    }
  else
    {
      /* #4 */
      i = vectorp->insert (i, newr);
    }

  /* Check whether the ranges following the one we've just added or
     touched can be folded in (#5 above).  */
  if (i != vectorp->end () && i + 1 < vectorp->end ())
    {
      int removed = 0;
      auto next = i + 1;

      /* Get the range we just touched.  */
      struct range &t = *i;
      removed = 0;

      i = next;
      for (; i < vectorp->end (); i++)
	{
	  struct range &r = *i;
	  if (r.offset <= t.offset + t.length)
	    {
	      ULONGEST l, h;

	      l = std::min (t.offset, r.offset);
	      h = std::max (t.offset + t.length, r.offset + r.length);

	      t.offset = l;
	      t.length = h - l;

	      removed++;
	    }
	  else
	    {
	      /* If we couldn't merge this one, we won't be able to
		 merge following ones either, since the ranges are
		 always sorted by OFFSET.  */
	      break;
	    }
	}

      if (removed != 0)
	vectorp->erase (next, next + removed);
    }
}

void
mark_value_bits_unavailable (struct value *value,
			     LONGEST offset, LONGEST length)
{
  insert_into_bit_range_vector (&value->unavailable, offset, length);
}

void
mark_value_bytes_unavailable (struct value *value,
			      LONGEST offset, LONGEST length)
{
  mark_value_bits_unavailable (value,
			       offset * TARGET_CHAR_BIT,
			       length * TARGET_CHAR_BIT);
}

/* Find the first range in RANGES that overlaps the range defined by
   OFFSET and LENGTH, starting at element POS in the RANGES vector,
   Returns the index into RANGES where such overlapping range was
   found, or -1 if none was found.  */

static int
find_first_range_overlap (const std::vector<range> *ranges, int pos,
			  LONGEST offset, LONGEST length)
{
  int i;

  for (i = pos; i < ranges->size (); i++)
    {
      const range &r = (*ranges)[i];
      if (ranges_overlap (r.offset, r.length, offset, length))
	return i;
    }

  return -1;
}

/* Compare LENGTH_BITS of memory at PTR1 + OFFSET1_BITS with the memory at
   PTR2 + OFFSET2_BITS.  Return 0 if the memory is the same, otherwise
   return non-zero.

   It must always be the case that:
     OFFSET1_BITS % TARGET_CHAR_BIT == OFFSET2_BITS % TARGET_CHAR_BIT

   It is assumed that memory can be accessed from:
     PTR + (OFFSET_BITS / TARGET_CHAR_BIT)
   to:
     PTR + ((OFFSET_BITS + LENGTH_BITS + TARGET_CHAR_BIT - 1)
	    / TARGET_CHAR_BIT)  */
static int
memcmp_with_bit_offsets (const gdb_byte *ptr1, size_t offset1_bits,
			 const gdb_byte *ptr2, size_t offset2_bits,
			 size_t length_bits)
{
  gdb_assert (offset1_bits % TARGET_CHAR_BIT
	      == offset2_bits % TARGET_CHAR_BIT);

  if (offset1_bits % TARGET_CHAR_BIT != 0)
    {
      size_t bits;
      gdb_byte mask, b1, b2;

      /* The offset from the base pointers PTR1 and PTR2 is not a complete
	 number of bytes.  A number of bits up to either the next exact
	 byte boundary, or LENGTH_BITS (which ever is sooner) will be
	 compared.  */
      bits = TARGET_CHAR_BIT - offset1_bits % TARGET_CHAR_BIT;
      gdb_assert (bits < sizeof (mask) * TARGET_CHAR_BIT);
      mask = (1 << bits) - 1;

      if (length_bits < bits)
	{
	  mask &= ~(gdb_byte) ((1 << (bits - length_bits)) - 1);
	  bits = length_bits;
	}

      /* Now load the two bytes and mask off the bits we care about.  */
      b1 = *(ptr1 + offset1_bits / TARGET_CHAR_BIT) & mask;
      b2 = *(ptr2 + offset2_bits / TARGET_CHAR_BIT) & mask;

      if (b1 != b2)
	return 1;

      /* Now update the length and offsets to take account of the bits
	 we've just compared.  */
      length_bits -= bits;
      offset1_bits += bits;
      offset2_bits += bits;
    }

  if (length_bits % TARGET_CHAR_BIT != 0)
    {
      size_t bits;
      size_t o1, o2;
      gdb_byte mask, b1, b2;

      /* The length is not an exact number of bytes.  After the previous
	 IF.. block then the offsets are byte aligned, or the
	 length is zero (in which case this code is not reached).  Compare
	 a number of bits at the end of the region, starting from an exact
	 byte boundary.  */
      bits = length_bits % TARGET_CHAR_BIT;
      o1 = offset1_bits + length_bits - bits;
      o2 = offset2_bits + length_bits - bits;

      gdb_assert (bits < sizeof (mask) * TARGET_CHAR_BIT);
      mask = ((1 << bits) - 1) << (TARGET_CHAR_BIT - bits);

      gdb_assert (o1 % TARGET_CHAR_BIT == 0);
      gdb_assert (o2 % TARGET_CHAR_BIT == 0);

      b1 = *(ptr1 + o1 / TARGET_CHAR_BIT) & mask;
      b2 = *(ptr2 + o2 / TARGET_CHAR_BIT) & mask;

      if (b1 != b2)
	return 1;

      length_bits -= bits;
    }

  if (length_bits > 0)
    {
      /* We've now taken care of any stray "bits" at the start, or end of
	 the region to compare, the remainder can be covered with a simple
	 memcmp.  */
      gdb_assert (offset1_bits % TARGET_CHAR_BIT == 0);
      gdb_assert (offset2_bits % TARGET_CHAR_BIT == 0);
      gdb_assert (length_bits % TARGET_CHAR_BIT == 0);

      return memcmp (ptr1 + offset1_bits / TARGET_CHAR_BIT,
		     ptr2 + offset2_bits / TARGET_CHAR_BIT,
		     length_bits / TARGET_CHAR_BIT);
    }

  /* Length is zero, regions match.  */
  return 0;
}

/* Helper struct for find_first_range_overlap_and_match and
   value_contents_bits_eq.  Keep track of which slot of a given ranges
   vector have we last looked at.  */

struct ranges_and_idx
{
  /* The ranges.  */
  const std::vector<range> *ranges;

  /* The range we've last found in RANGES.  Given ranges are sorted,
     we can start the next lookup here.  */
  int idx;
};

/* Helper function for value_contents_bits_eq.  Compare LENGTH bits of
   RP1's ranges starting at OFFSET1 bits with LENGTH bits of RP2's
   ranges starting at OFFSET2 bits.  Return true if the ranges match
   and fill in *L and *H with the overlapping window relative to
   (both) OFFSET1 or OFFSET2.  */

static int
find_first_range_overlap_and_match (struct ranges_and_idx *rp1,
				    struct ranges_and_idx *rp2,
				    LONGEST offset1, LONGEST offset2,
				    LONGEST length, ULONGEST *l, ULONGEST *h)
{
  rp1->idx = find_first_range_overlap (rp1->ranges, rp1->idx,
				       offset1, length);
  rp2->idx = find_first_range_overlap (rp2->ranges, rp2->idx,
				       offset2, length);

  if (rp1->idx == -1 && rp2->idx == -1)
    {
      *l = length;
      *h = length;
      return 1;
    }
  else if (rp1->idx == -1 || rp2->idx == -1)
    return 0;
  else
    {
      const range *r1, *r2;
      ULONGEST l1, h1;
      ULONGEST l2, h2;

      r1 = &(*rp1->ranges)[rp1->idx];
      r2 = &(*rp2->ranges)[rp2->idx];

      /* Get the unavailable windows intersected by the incoming
	 ranges.  The first and last ranges that overlap the argument
	 range may be wider than said incoming arguments ranges.  */
      l1 = std::max (offset1, r1->offset);
      h1 = std::min (offset1 + length, r1->offset + r1->length);

      l2 = std::max (offset2, r2->offset);
      h2 = std::min (offset2 + length, offset2 + r2->length);

      /* Make them relative to the respective start offsets, so we can
	 compare them for equality.  */
      l1 -= offset1;
      h1 -= offset1;

      l2 -= offset2;
      h2 -= offset2;

      /* Different ranges, no match.  */
      if (l1 != l2 || h1 != h2)
	return 0;

      *h = h1;
      *l = l1;
      return 1;
    }
}

/* Helper function for value_contents_eq.  The only difference is that
   this function is bit rather than byte based.

   Compare LENGTH bits of VAL1's contents starting at OFFSET1 bits
   with LENGTH bits of VAL2's contents starting at OFFSET2 bits.
   Return true if the available bits match.  */

static bool
value_contents_bits_eq (const struct value *val1, int offset1,
			const struct value *val2, int offset2,
			int length)
{
  /* Each array element corresponds to a ranges source (unavailable,
     optimized out).  '1' is for VAL1, '2' for VAL2.  */
  struct ranges_and_idx rp1[2], rp2[2];

  /* See function description in value.h.  */
  gdb_assert (!val1->lazy && !val2->lazy);

  /* We shouldn't be trying to compare past the end of the values.  */
  gdb_assert (offset1 + length
	      <= val1->enclosing_type->length () * TARGET_CHAR_BIT);
  gdb_assert (offset2 + length
	      <= val2->enclosing_type->length () * TARGET_CHAR_BIT);

  memset (&rp1, 0, sizeof (rp1));
  memset (&rp2, 0, sizeof (rp2));
  rp1[0].ranges = &val1->unavailable;
  rp2[0].ranges = &val2->unavailable;
  rp1[1].ranges = &val1->optimized_out;
  rp2[1].ranges = &val2->optimized_out;

  while (length > 0)
    {
      ULONGEST l = 0, h = 0; /* init for gcc -Wall */
      int i;

      for (i = 0; i < 2; i++)
	{
	  ULONGEST l_tmp, h_tmp;

	  /* The contents only match equal if the invalid/unavailable
	     contents ranges match as well.  */
	  if (!find_first_range_overlap_and_match (&rp1[i], &rp2[i],
						   offset1, offset2, length,
						   &l_tmp, &h_tmp))
	    return false;

	  /* We're interested in the lowest/first range found.  */
	  if (i == 0 || l_tmp < l)
	    {
	      l = l_tmp;
	      h = h_tmp;
	    }
	}

      /* Compare the available/valid contents.  */
      if (memcmp_with_bit_offsets (val1->contents.get (), offset1,
				   val2->contents.get (), offset2, l) != 0)
	return false;

      length -= h;
      offset1 += h;
      offset2 += h;
    }

  return true;
}

bool
value_contents_eq (const struct value *val1, LONGEST offset1,
		   const struct value *val2, LONGEST offset2,
		   LONGEST length)
{
  return value_contents_bits_eq (val1, offset1 * TARGET_CHAR_BIT,
				 val2, offset2 * TARGET_CHAR_BIT,
				 length * TARGET_CHAR_BIT);
}

/* See value.h.  */

bool
value_contents_eq (const struct value *val1, const struct value *val2)
{
  ULONGEST len1 = check_typedef (value_enclosing_type (val1))->length ();
  ULONGEST len2 = check_typedef (value_enclosing_type (val2))->length ();
  if (len1 != len2)
    return false;
  return value_contents_eq (val1, 0, val2, 0, len1);
}

/* The value-history records all the values printed by print commands
   during this session.  */

static std::vector<value_ref_ptr> value_history;


/* List of all value objects currently allocated
   (except for those released by calls to release_value)
   This is so they can be freed after each command.  */

static std::vector<value_ref_ptr> all_values;

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

  val = new struct value (type);

  /* Values start out on the all_values chain.  */
  all_values.emplace_back (val);

  return val;
}

/* The maximum size, in bytes, that GDB will try to allocate for a value.
   The initial value of 64k was not selected for any specific reason, it is
   just a reasonable starting point.  */

static int max_value_size = 65536; /* 64k bytes */

/* It is critical that the MAX_VALUE_SIZE is at least as big as the size of
   LONGEST, otherwise GDB will not be able to parse integer values from the
   CLI; for example if the MAX_VALUE_SIZE could be set to 1 then GDB would
   be unable to parse "set max-value-size 2".

   As we want a consistent GDB experience across hosts with different sizes
   of LONGEST, this arbitrary minimum value was selected, so long as this
   is bigger than LONGEST on all GDB supported hosts we're fine.  */

#define MIN_VALUE_FOR_MAX_VALUE_SIZE 16
gdb_static_assert (sizeof (LONGEST) <= MIN_VALUE_FOR_MAX_VALUE_SIZE);

/* Implement the "set max-value-size" command.  */

static void
set_max_value_size (const char *args, int from_tty,
		    struct cmd_list_element *c)
{
  gdb_assert (max_value_size == -1 || max_value_size >= 0);

  if (max_value_size > -1 && max_value_size < MIN_VALUE_FOR_MAX_VALUE_SIZE)
    {
      max_value_size = MIN_VALUE_FOR_MAX_VALUE_SIZE;
      error (_("max-value-size set too low, increasing to %d bytes"),
	     max_value_size);
    }
}

/* Implement the "show max-value-size" command.  */

static void
show_max_value_size (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  if (max_value_size == -1)
    gdb_printf (file, _("Maximum value size is unlimited.\n"));
  else
    gdb_printf (file, _("Maximum value size is %d bytes.\n"),
		max_value_size);
}

/* Called before we attempt to allocate or reallocate a buffer for the
   contents of a value.  TYPE is the type of the value for which we are
   allocating the buffer.  If the buffer is too large (based on the user
   controllable setting) then throw an error.  If this function returns
   then we should attempt to allocate the buffer.  */

static void
check_type_length_before_alloc (const struct type *type)
{
  ULONGEST length = type->length ();

  if (max_value_size > -1 && length > max_value_size)
    {
      if (type->name () != NULL)
	error (_("value of type `%s' requires %s bytes, which is more "
		 "than max-value-size"), type->name (), pulongest (length));
      else
	error (_("value requires %s bytes, which is more than "
		 "max-value-size"), pulongest (length));
    }
}

/* Allocate the contents of VAL if it has not been allocated yet.  */

static void
allocate_value_contents (struct value *val)
{
  if (!val->contents)
    {
      check_type_length_before_alloc (val->enclosing_type);
      val->contents.reset
	((gdb_byte *) xzalloc (val->enclosing_type->length ()));
    }
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
  /* Despite the fact that we are really creating an array of TYPE here, we
     use the string lower bound as the array lower bound.  This seems to
     work fine for now.  */
  int low_bound = current_language->string_lower_bound ();
  /* FIXME-type-allocation: need a way to free this type when we are
     done with it.  */
  struct type *array_type
    = lookup_array_range_type (type, low_bound, count + low_bound - 1);

  return allocate_value (array_type);
}

struct value *
allocate_computed_value (struct type *type,
			 const struct lval_funcs *funcs,
			 void *closure)
{
  struct value *v = allocate_value_lazy (type);

  VALUE_LVAL (v) = lval_computed;
  v->location.computed.funcs = funcs;
  v->location.computed.closure = closure;

  return v;
}

/* Allocate NOT_LVAL value for type TYPE being OPTIMIZED_OUT.  */

struct value *
allocate_optimized_out_value (struct type *type)
{
  struct value *retval = allocate_value_lazy (type);

  mark_value_bytes_optimized_out (retval, 0, type->length ());
  set_value_lazy (retval, 0);
  return retval;
}

/* Accessor methods.  */

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

LONGEST
value_offset (const struct value *value)
{
  return value->offset;
}
void
set_value_offset (struct value *value, LONGEST offset)
{
  value->offset = offset;
}

LONGEST
value_bitpos (const struct value *value)
{
  return value->bitpos;
}
void
set_value_bitpos (struct value *value, LONGEST bit)
{
  value->bitpos = bit;
}

LONGEST
value_bitsize (const struct value *value)
{
  return value->bitsize;
}
void
set_value_bitsize (struct value *value, LONGEST bit)
{
  value->bitsize = bit;
}

struct value *
value_parent (const struct value *value)
{
  return value->parent.get ();
}

/* See value.h.  */

void
set_value_parent (struct value *value, struct value *parent)
{
  value->parent = value_ref_ptr::new_reference (parent);
}

gdb::array_view<gdb_byte>
value_contents_raw (struct value *value)
{
  struct gdbarch *arch = get_value_arch (value);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  allocate_value_contents (value);

  ULONGEST length = value_type (value)->length ();
  return gdb::make_array_view
    (value->contents.get () + value->embedded_offset * unit_size, length);
}

gdb::array_view<gdb_byte>
value_contents_all_raw (struct value *value)
{
  allocate_value_contents (value);

  ULONGEST length = value_enclosing_type (value)->length ();
  return gdb::make_array_view (value->contents.get (), length);
}

struct type *
value_enclosing_type (const struct value *value)
{
  return value->enclosing_type;
}

/* Look at value.h for description.  */

struct type *
value_actual_type (struct value *value, int resolve_simple_types,
		   int *real_type_found)
{
  struct value_print_options opts;
  struct type *result;

  get_user_print_options (&opts);

  if (real_type_found)
    *real_type_found = 0;
  result = value_type (value);
  if (opts.objectprint)
    {
      /* If result's target type is TYPE_CODE_STRUCT, proceed to
	 fetch its rtti type.  */
      if (result->is_pointer_or_reference ()
	  && (check_typedef (result->target_type ())->code ()
	      == TYPE_CODE_STRUCT)
	  && !value_optimized_out (value))
	{
	  struct type *real_type;

	  real_type = value_rtti_indirect_type (value, NULL, NULL, NULL);
	  if (real_type)
	    {
	      if (real_type_found)
		*real_type_found = 1;
	      result = real_type;
	    }
	}
      else if (resolve_simple_types)
	{
	  if (real_type_found)
	    *real_type_found = 1;
	  result = value_enclosing_type (value);
	}
    }

  return result;
}

void
error_value_optimized_out (void)
{
  throw_error (OPTIMIZED_OUT_ERROR, _("value has been optimized out"));
}

static void
require_not_optimized_out (const struct value *value)
{
  if (!value->optimized_out.empty ())
    {
      if (value->lval == lval_register)
	throw_error (OPTIMIZED_OUT_ERROR,
		     _("register has not been saved in frame"));
      else
	error_value_optimized_out ();
    }
}

static void
require_available (const struct value *value)
{
  if (!value->unavailable.empty ())
    throw_error (NOT_AVAILABLE_ERROR, _("value is not available"));
}

gdb::array_view<const gdb_byte>
value_contents_for_printing (struct value *value)
{
  if (value->lazy)
    value_fetch_lazy (value);

  ULONGEST length = value_enclosing_type (value)->length ();
  return gdb::make_array_view (value->contents.get (), length);
}

gdb::array_view<const gdb_byte>
value_contents_for_printing_const (const struct value *value)
{
  gdb_assert (!value->lazy);

  ULONGEST length = value_enclosing_type (value)->length ();
  return gdb::make_array_view (value->contents.get (), length);
}

gdb::array_view<const gdb_byte>
value_contents_all (struct value *value)
{
  gdb::array_view<const gdb_byte> result = value_contents_for_printing (value);
  require_not_optimized_out (value);
  require_available (value);
  return result;
}

/* Copy ranges in SRC_RANGE that overlap [SRC_BIT_OFFSET,
   SRC_BIT_OFFSET+BIT_LENGTH) ranges into *DST_RANGE, adjusted.  */

static void
ranges_copy_adjusted (std::vector<range> *dst_range, int dst_bit_offset,
		      const std::vector<range> &src_range, int src_bit_offset,
		      int bit_length)
{
  for (const range &r : src_range)
    {
      ULONGEST h, l;

      l = std::max (r.offset, (LONGEST) src_bit_offset);
      h = std::min (r.offset + r.length,
		    (LONGEST) src_bit_offset + bit_length);

      if (l < h)
	insert_into_bit_range_vector (dst_range,
				      dst_bit_offset + (l - src_bit_offset),
				      h - l);
    }
}

/* Copy the ranges metadata in SRC that overlaps [SRC_BIT_OFFSET,
   SRC_BIT_OFFSET+BIT_LENGTH) into DST, adjusted.  */

static void
value_ranges_copy_adjusted (struct value *dst, int dst_bit_offset,
			    const struct value *src, int src_bit_offset,
			    int bit_length)
{
  ranges_copy_adjusted (&dst->unavailable, dst_bit_offset,
			src->unavailable, src_bit_offset,
			bit_length);
  ranges_copy_adjusted (&dst->optimized_out, dst_bit_offset,
			src->optimized_out, src_bit_offset,
			bit_length);
}

/* Copy LENGTH target addressable memory units of SRC value's (all) contents
   (value_contents_all) starting at SRC_OFFSET, into DST value's (all)
   contents, starting at DST_OFFSET.  If unavailable contents are
   being copied from SRC, the corresponding DST contents are marked
   unavailable accordingly.  Neither DST nor SRC may be lazy
   values.

   It is assumed the contents of DST in the [DST_OFFSET,
   DST_OFFSET+LENGTH) range are wholly available.  */

static void
value_contents_copy_raw (struct value *dst, LONGEST dst_offset,
			 struct value *src, LONGEST src_offset, LONGEST length)
{
  LONGEST src_bit_offset, dst_bit_offset, bit_length;
  struct gdbarch *arch = get_value_arch (src);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  /* A lazy DST would make that this copy operation useless, since as
     soon as DST's contents were un-lazied (by a later value_contents
     call, say), the contents would be overwritten.  A lazy SRC would
     mean we'd be copying garbage.  */
  gdb_assert (!dst->lazy && !src->lazy);

  /* The overwritten DST range gets unavailability ORed in, not
     replaced.  Make sure to remember to implement replacing if it
     turns out actually necessary.  */
  gdb_assert (value_bytes_available (dst, dst_offset, length));
  gdb_assert (!value_bits_any_optimized_out (dst,
					     TARGET_CHAR_BIT * dst_offset,
					     TARGET_CHAR_BIT * length));

  /* Copy the data.  */
  gdb::array_view<gdb_byte> dst_contents
    = value_contents_all_raw (dst).slice (dst_offset * unit_size,
					  length * unit_size);
  gdb::array_view<const gdb_byte> src_contents
    = value_contents_all_raw (src).slice (src_offset * unit_size,
					  length * unit_size);
  copy (src_contents, dst_contents);

  /* Copy the meta-data, adjusted.  */
  src_bit_offset = src_offset * unit_size * HOST_CHAR_BIT;
  dst_bit_offset = dst_offset * unit_size * HOST_CHAR_BIT;
  bit_length = length * unit_size * HOST_CHAR_BIT;

  value_ranges_copy_adjusted (dst, dst_bit_offset,
			      src, src_bit_offset,
			      bit_length);
}

/* A helper for value_from_component_bitsize that copies bits from SRC
   to DEST.  */

static void
value_contents_copy_raw_bitwise (struct value *dst, LONGEST dst_bit_offset,
				 struct value *src, LONGEST src_bit_offset,
				 LONGEST bit_length)
{
  /* A lazy DST would make that this copy operation useless, since as
     soon as DST's contents were un-lazied (by a later value_contents
     call, say), the contents would be overwritten.  A lazy SRC would
     mean we'd be copying garbage.  */
  gdb_assert (!dst->lazy && !src->lazy);

  /* The overwritten DST range gets unavailability ORed in, not
     replaced.  Make sure to remember to implement replacing if it
     turns out actually necessary.  */
  LONGEST dst_offset = dst_bit_offset / TARGET_CHAR_BIT;
  LONGEST length = bit_length / TARGET_CHAR_BIT;
  gdb_assert (value_bytes_available (dst, dst_offset, length));
  gdb_assert (!value_bits_any_optimized_out (dst, dst_bit_offset,
					     bit_length));

  /* Copy the data.  */
  gdb::array_view<gdb_byte> dst_contents = value_contents_all_raw (dst);
  gdb::array_view<const gdb_byte> src_contents = value_contents_all_raw (src);
  copy_bitwise (dst_contents.data (), dst_bit_offset,
		src_contents.data (), src_bit_offset,
		bit_length,
		type_byte_order (value_type (src)) == BFD_ENDIAN_BIG);

  /* Copy the meta-data.  */
  value_ranges_copy_adjusted (dst, dst_bit_offset,
			      src, src_bit_offset,
			      bit_length);
}

/* Copy LENGTH bytes of SRC value's (all) contents
   (value_contents_all) starting at SRC_OFFSET byte, into DST value's
   (all) contents, starting at DST_OFFSET.  If unavailable contents
   are being copied from SRC, the corresponding DST contents are
   marked unavailable accordingly.  DST must not be lazy.  If SRC is
   lazy, it will be fetched now.

   It is assumed the contents of DST in the [DST_OFFSET,
   DST_OFFSET+LENGTH) range are wholly available.  */

void
value_contents_copy (struct value *dst, LONGEST dst_offset,
		     struct value *src, LONGEST src_offset, LONGEST length)
{
  if (src->lazy)
    value_fetch_lazy (src);

  value_contents_copy_raw (dst, dst_offset, src, src_offset, length);
}

int
value_lazy (const struct value *value)
{
  return value->lazy;
}

void
set_value_lazy (struct value *value, int val)
{
  value->lazy = val;
}

int
value_stack (const struct value *value)
{
  return value->stack;
}

void
set_value_stack (struct value *value, int val)
{
  value->stack = val;
}

gdb::array_view<const gdb_byte>
value_contents (struct value *value)
{
  gdb::array_view<const gdb_byte> result = value_contents_writeable (value);
  require_not_optimized_out (value);
  require_available (value);
  return result;
}

gdb::array_view<gdb_byte>
value_contents_writeable (struct value *value)
{
  if (value->lazy)
    value_fetch_lazy (value);
  return value_contents_raw (value);
}

int
value_optimized_out (struct value *value)
{
  if (value->lazy)
    {
      /* See if we can compute the result without fetching the
	 value.  */
      if (VALUE_LVAL (value) == lval_memory)
	return false;
      else if (VALUE_LVAL (value) == lval_computed)
	{
	  const struct lval_funcs *funcs = value->location.computed.funcs;

	  if (funcs->is_optimized_out != nullptr)
	    return funcs->is_optimized_out (value);
	}

      /* Fall back to fetching.  */
      try
	{
	  value_fetch_lazy (value);
	}
      catch (const gdb_exception_error &ex)
	{
	  switch (ex.error)
	    {
	    case MEMORY_ERROR:
	    case OPTIMIZED_OUT_ERROR:
	    case NOT_AVAILABLE_ERROR:
	      /* These can normally happen when we try to access an
		 optimized out or unavailable register, either in a
		 physical register or spilled to memory.  */
	      break;
	    default:
	      throw;
	    }
	}
    }

  return !value->optimized_out.empty ();
}

/* Mark contents of VALUE as optimized out, starting at OFFSET bytes, and
   the following LENGTH bytes.  */

void
mark_value_bytes_optimized_out (struct value *value, int offset, int length)
{
  mark_value_bits_optimized_out (value,
				 offset * TARGET_CHAR_BIT,
				 length * TARGET_CHAR_BIT);
}

/* See value.h.  */

void
mark_value_bits_optimized_out (struct value *value,
			       LONGEST offset, LONGEST length)
{
  insert_into_bit_range_vector (&value->optimized_out, offset, length);
}

int
value_bits_synthetic_pointer (const struct value *value,
			      LONGEST offset, LONGEST length)
{
  if (value->lval != lval_computed
      || !value->location.computed.funcs->check_synthetic_pointer)
    return 0;
  return value->location.computed.funcs->check_synthetic_pointer (value,
								  offset,
								  length);
}

LONGEST
value_embedded_offset (const struct value *value)
{
  return value->embedded_offset;
}

void
set_value_embedded_offset (struct value *value, LONGEST val)
{
  value->embedded_offset = val;
}

LONGEST
value_pointed_to_offset (const struct value *value)
{
  return value->pointed_to_offset;
}

void
set_value_pointed_to_offset (struct value *value, LONGEST val)
{
  value->pointed_to_offset = val;
}

const struct lval_funcs *
value_computed_funcs (const struct value *v)
{
  gdb_assert (value_lval_const (v) == lval_computed);

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

enum lval_type
value_lval_const (const struct value *value)
{
  return value->lval;
}

CORE_ADDR
value_address (const struct value *value)
{
  if (value->lval != lval_memory)
    return 0;
  if (value->parent != NULL)
    return value_address (value->parent.get ()) + value->offset;
  if (NULL != TYPE_DATA_LOCATION (value_type (value)))
    {
      gdb_assert (PROP_CONST == TYPE_DATA_LOCATION_KIND (value_type (value)));
      return TYPE_DATA_LOCATION_ADDR (value_type (value));
    }

  return value->location.address + value->offset;
}

CORE_ADDR
value_raw_address (const struct value *value)
{
  if (value->lval != lval_memory)
    return 0;
  return value->location.address;
}

void
set_value_address (struct value *value, CORE_ADDR addr)
{
  gdb_assert (value->lval == lval_memory);
  value->location.address = addr;
}

struct internalvar **
deprecated_value_internalvar_hack (struct value *value)
{
  return &value->location.internalvar;
}

struct frame_id *
deprecated_value_next_frame_id_hack (struct value *value)
{
  gdb_assert (value->lval == lval_register);
  return &value->location.reg.next_frame_id;
}

int *
deprecated_value_regnum_hack (struct value *value)
{
  gdb_assert (value->lval == lval_register);
  return &value->location.reg.regnum;
}

int
deprecated_value_modifiable (const struct value *value)
{
  return value->modifiable;
}

/* Return a mark in the value chain.  All values allocated after the
   mark is obtained (except for those released) are subject to being freed
   if a subsequent value_free_to_mark is passed the mark.  */
struct value *
value_mark (void)
{
  if (all_values.empty ())
    return nullptr;
  return all_values.back ().get ();
}

/* See value.h.  */

void
value_incref (struct value *val)
{
  val->reference_count++;
}

/* Release a reference to VAL, which was acquired with value_incref.
   This function is also called to deallocate values from the value
   chain.  */

void
value_decref (struct value *val)
{
  if (val != nullptr)
    {
      gdb_assert (val->reference_count > 0);
      val->reference_count--;
      if (val->reference_count == 0)
	delete val;
    }
}

/* Free all values allocated since MARK was obtained by value_mark
   (except for those released).  */
void
value_free_to_mark (const struct value *mark)
{
  auto iter = std::find (all_values.begin (), all_values.end (), mark);
  if (iter == all_values.end ())
    all_values.clear ();
  else
    all_values.erase (iter + 1, all_values.end ());
}

/* Remove VAL from the chain all_values
   so it will not be freed automatically.  */

value_ref_ptr
release_value (struct value *val)
{
  if (val == nullptr)
    return value_ref_ptr ();

  std::vector<value_ref_ptr>::reverse_iterator iter;
  for (iter = all_values.rbegin (); iter != all_values.rend (); ++iter)
    {
      if (*iter == val)
	{
	  value_ref_ptr result = *iter;
	  all_values.erase (iter.base () - 1);
	  return result;
	}
    }

  /* We must always return an owned reference.  Normally this happens
     because we transfer the reference from the value chain, but in
     this case the value was not on the chain.  */
  return value_ref_ptr::new_reference (val);
}

/* See value.h.  */

std::vector<value_ref_ptr>
value_release_to_mark (const struct value *mark)
{
  std::vector<value_ref_ptr> result;

  auto iter = std::find (all_values.begin (), all_values.end (), mark);
  if (iter == all_values.end ())
    std::swap (result, all_values);
  else
    {
      std::move (iter + 1, all_values.end (), std::back_inserter (result));
      all_values.erase (iter + 1, all_values.end ());
    }
  std::reverse (result.begin (), result.end ());
  return result;
}

/* Return a copy of the value ARG.
   It contains the same contents, for same memory address,
   but it's a different block of storage.  */

struct value *
value_copy (const value *arg)
{
  struct type *encl_type = value_enclosing_type (arg);
  struct value *val;

  if (value_lazy (arg))
    val = allocate_value_lazy (encl_type);
  else
    val = allocate_value (encl_type);
  val->type = arg->type;
  VALUE_LVAL (val) = arg->lval;
  val->location = arg->location;
  val->offset = arg->offset;
  val->bitpos = arg->bitpos;
  val->bitsize = arg->bitsize;
  val->lazy = arg->lazy;
  val->embedded_offset = value_embedded_offset (arg);
  val->pointed_to_offset = arg->pointed_to_offset;
  val->modifiable = arg->modifiable;
  val->stack = arg->stack;
  val->is_zero = arg->is_zero;
  val->initialized = arg->initialized;
  val->unavailable = arg->unavailable;
  val->optimized_out = arg->optimized_out;

  if (!value_lazy (val) && !value_entirely_optimized_out (val))
    {
      gdb_assert (arg->contents != nullptr);
      ULONGEST length = value_enclosing_type (arg)->length ();
      const auto &arg_view
	= gdb::make_array_view (arg->contents.get (), length);
      copy (arg_view, value_contents_all_raw (val));
    }

  val->parent = arg->parent;
  if (VALUE_LVAL (val) == lval_computed)
    {
      const struct lval_funcs *funcs = val->location.computed.funcs;

      if (funcs->copy_closure)
	val->location.computed.closure = funcs->copy_closure (val);
    }
  return val;
}

/* Return a "const" and/or "volatile" qualified version of the value V.
   If CNST is true, then the returned value will be qualified with
   "const".
   if VOLTL is true, then the returned value will be qualified with
   "volatile".  */

struct value *
make_cv_value (int cnst, int voltl, struct value *v)
{
  struct type *val_type = value_type (v);
  struct type *enclosing_type = value_enclosing_type (v);
  struct value *cv_val = value_copy (v);

  deprecated_set_value_type (cv_val,
			     make_cv_type (cnst, voltl, val_type, NULL));
  set_value_enclosing_type (cv_val,
			    make_cv_type (cnst, voltl, enclosing_type, NULL));

  return cv_val;
}

/* Return a version of ARG that is non-lvalue.  */

struct value *
value_non_lval (struct value *arg)
{
  if (VALUE_LVAL (arg) != not_lval)
    {
      struct type *enc_type = value_enclosing_type (arg);
      struct value *val = allocate_value (enc_type);

      copy (value_contents_all (arg), value_contents_all_raw (val));
      val->type = arg->type;
      set_value_embedded_offset (val, value_embedded_offset (arg));
      set_value_pointed_to_offset (val, value_pointed_to_offset (arg));
      return val;
    }
   return arg;
}

/* Write contents of V at ADDR and set its lval type to be LVAL_MEMORY.  */

void
value_force_lval (struct value *v, CORE_ADDR addr)
{
  gdb_assert (VALUE_LVAL (v) == not_lval);

  write_memory (addr, value_contents_raw (v).data (), value_type (v)->length ());
  v->lval = lval_memory;
  v->location.address = addr;
}

void
set_value_component_location (struct value *component,
			      const struct value *whole)
{
  struct type *type;

  gdb_assert (whole->lval != lval_xcallable);

  if (whole->lval == lval_internalvar)
    VALUE_LVAL (component) = lval_internalvar_component;
  else
    VALUE_LVAL (component) = whole->lval;

  component->location = whole->location;
  if (whole->lval == lval_computed)
    {
      const struct lval_funcs *funcs = whole->location.computed.funcs;

      if (funcs->copy_closure)
	component->location.computed.closure = funcs->copy_closure (whole);
    }

  /* If the WHOLE value has a dynamically resolved location property then
     update the address of the COMPONENT.  */
  type = value_type (whole);
  if (NULL != TYPE_DATA_LOCATION (type)
      && TYPE_DATA_LOCATION_KIND (type) == PROP_CONST)
    set_value_address (component, TYPE_DATA_LOCATION_ADDR (type));

  /* Similarly, if the COMPONENT value has a dynamically resolved location
     property then update its address.  */
  type = value_type (component);
  if (NULL != TYPE_DATA_LOCATION (type)
      && TYPE_DATA_LOCATION_KIND (type) == PROP_CONST)
    {
      /* If the COMPONENT has a dynamic location, and is an
	 lval_internalvar_component, then we change it to a lval_memory.

	 Usually a component of an internalvar is created non-lazy, and has
	 its content immediately copied from the parent internalvar.
	 However, for components with a dynamic location, the content of
	 the component is not contained within the parent, but is instead
	 accessed indirectly.  Further, the component will be created as a
	 lazy value.

	 By changing the type of the component to lval_memory we ensure
	 that value_fetch_lazy can successfully load the component.

         This solution isn't ideal, but a real fix would require values to
         carry around both the parent value contents, and the contents of
         any dynamic fields within the parent.  This is a substantial
         change to how values work in GDB.  */
      if (VALUE_LVAL (component) == lval_internalvar_component)
	{
	  gdb_assert (value_lazy (component));
	  VALUE_LVAL (component) = lval_memory;
	}
      else
	gdb_assert (VALUE_LVAL (component) == lval_memory);
      set_value_address (component, TYPE_DATA_LOCATION_ADDR (type));
    }
}

/* Access to the value history.  */

/* Record a new value in the value history.
   Returns the absolute history index of the entry.  */

int
record_latest_value (struct value *val)
{
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

  value_history.push_back (release_value (val));

  return value_history.size ();
}

/* Return a copy of the value in the history with sequence number NUM.  */

struct value *
access_value_history (int num)
{
  int absnum = num;

  if (absnum <= 0)
    absnum += value_history.size ();

  if (absnum <= 0)
    {
      if (num == 0)
	error (_("The history is empty."));
      else if (num == 1)
	error (_("There is only one value in the history."));
      else
	error (_("History does not go back to $$%d."), -num);
    }
  if (absnum > value_history.size ())
    error (_("History has not yet reached $%d."), absnum);

  absnum--;

  return value_copy (value_history[absnum].get ());
}

/* See value.h.  */

ULONGEST
value_history_count ()
{
  return value_history.size ();
}

static void
show_values (const char *num_exp, int from_tty)
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
      num = value_history.size () - 9;
    }

  if (num <= 0)
    num = 1;

  for (i = num; i < num + 10 && i <= value_history.size (); i++)
    {
      struct value_print_options opts;

      val = access_value_history (i);
      gdb_printf (("$%d = "), i);
      get_user_print_options (&opts);
      value_print (val, gdb_stdout, &opts);
      gdb_printf (("\n"));
    }

  /* The next "show values +" should start after what we just printed.  */
  num += 10;

  /* Hitting just return after this command should do the same thing as
     "show values +".  If num_exp is null, this is unnecessary, since
     "show values +" is not useful after "show values".  */
  if (from_tty && num_exp)
    set_repeat_arguments ("+");
}

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
};

union internalvar_data
{
  /* A value object used with INTERNALVAR_VALUE.  */
  struct value *value;

  /* The call-back routine used with INTERNALVAR_MAKE_VALUE.  */
  struct
  {
    /* The functions to call.  */
    const struct internalvar_funcs *functions;

    /* The function's user-data.  */
    void *data;
  } make_value;

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
};

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

  enum internalvar_kind kind;

  union internalvar_data u;
};

static struct internalvar *internalvars;

/* If the variable does not already exist create it and give it the
   value given.  If no value is given then the default is zero.  */
static void
init_if_undefined_command (const char* args, int from_tty)
{
  struct internalvar *intvar = nullptr;

  /* Parse the expression - this is taken from set_command().  */
  expression_up expr = parse_expression (args);

  /* Validate the expression.
     Was the expression an assignment?
     Or even an expression at all?  */
  if (expr->first_opcode () != BINOP_ASSIGN)
    error (_("Init-if-undefined requires an assignment expression."));

  /* Extract the variable from the parsed expression.  */
  expr::assign_operation *assign
    = dynamic_cast<expr::assign_operation *> (expr->op.get ());
  if (assign != nullptr)
    {
      expr::operation *lhs = assign->get_lhs ();
      expr::internalvar_operation *ivarop
	= dynamic_cast<expr::internalvar_operation *> (lhs);
      if (ivarop != nullptr)
	intvar = ivarop->get_internalvar ();
    }

  if (intvar == nullptr)
    error (_("The first parameter to init-if-undefined "
	     "should be a GDB variable."));

  /* Only evaluate the expression if the lvalue is void.
     This may still fail if the expression is invalid.  */
  if (intvar->kind == INTERNALVAR_VOID)
    evaluate_expression (expr.get ());
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

/* Complete NAME by comparing it to the names of internal
   variables.  */

void
complete_internalvar (completion_tracker &tracker, const char *name)
{
  struct internalvar *var;
  int len;

  len = strlen (name);

  for (var = internalvars; var; var = var->next)
    if (strncmp (var->name, name, len) == 0)
      tracker.add_completion (make_unique_xstrdup (var->name));
}

/* Create an internal variable with name NAME and with a void value.
   NAME should not normally include a dollar sign.  */

struct internalvar *
create_internalvar (const char *name)
{
  struct internalvar *var = XNEW (struct internalvar);

  var->name = xstrdup (name);
  var->kind = INTERNALVAR_VOID;
  var->next = internalvars;
  internalvars = var;
  return var;
}

/* Create an internal variable with name NAME and register FUN as the
   function that value_of_internalvar uses to create a value whenever
   this variable is referenced.  NAME should not normally include a
   dollar sign.  DATA is passed uninterpreted to FUN when it is
   called.  CLEANUP, if not NULL, is called when the internal variable
   is destroyed.  It is passed DATA as its only argument.  */

struct internalvar *
create_internalvar_type_lazy (const char *name,
			      const struct internalvar_funcs *funcs,
			      void *data)
{
  struct internalvar *var = create_internalvar (name);

  var->kind = INTERNALVAR_MAKE_VALUE;
  var->u.make_value.functions = funcs;
  var->u.make_value.data = data;
  return var;
}

/* See documentation in value.h.  */

int
compile_internalvar_to_ax (struct internalvar *var,
			   struct agent_expr *expr,
			   struct axs_value *value)
{
  if (var->kind != INTERNALVAR_MAKE_VALUE
      || var->u.make_value.functions->compile_to_ax == NULL)
    return 0;

  var->u.make_value.functions->compile_to_ax (var, expr, value,
					      var->u.make_value.data);
  return 1;
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
      val = (*var->u.make_value.functions->make_value) (gdbarch, var,
							var->u.make_value.data);
      break;

    default:
      internal_error (_("bad kind"));
    }

  /* Change the VALUE_LVAL to lval_internalvar so that future operations
     on this value go back to affect the original internal variable.

     Do not do this for INTERNALVAR_MAKE_VALUE variables, as those have
     no underlying modifiable state in the internal variable.

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

      if (type->code () == TYPE_CODE_INT)
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
set_internalvar_component (struct internalvar *var,
			   LONGEST offset, LONGEST bitpos,
			   LONGEST bitsize, struct value *newval)
{
  gdb_byte *addr;
  struct gdbarch *arch;
  int unit_size;

  switch (var->kind)
    {
    case INTERNALVAR_VALUE:
      addr = value_contents_writeable (var->u.value).data ();
      arch = get_value_arch (var->u.value);
      unit_size = gdbarch_addressable_memory_unit_size (arch);

      if (bitsize)
	modify_field (value_type (var->u.value), addr + offset,
		      value_as_long (newval), bitpos, bitsize);
      else
	memcpy (addr + offset * unit_size, value_contents (newval).data (),
		value_type (newval)->length ());
      break;

    default:
      /* We can never get a component of any other kind.  */
      internal_error (_("set_internalvar_component"));
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
  switch (check_typedef (value_type (val))->code ())
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
      struct value *copy = value_copy (val);
      copy->modifiable = 1;

      /* Force the value to be fetched from the target now, to avoid problems
	 later when this internalvar is referenced and the target is gone or
	 has changed.  */
      if (value_lazy (copy))
	value_fetch_lazy (copy);

      /* Release the value from the value chain to prevent it from being
	 deleted by free_all_values.  From here on this function should not
	 call error () until new_data is installed into the var->u to avoid
	 leaking memory.  */
      new_data.value = release_value (copy).release ();

      /* Internal variables which are created from values with a dynamic
	 location don't need the location property of the origin anymore.
	 The resolved dynamic location is used prior then any other address
	 when accessing the value.
	 If we keep it, we would still refer to the origin value.
	 Remove the location property in case it exist.  */
      value_type (new_data.value)->remove_dyn_prop (DYN_PROP_DATA_LOCATION);

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
      value_decref (var->u.value);
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

const char *
internalvar_name (const struct internalvar *var)
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

const char *
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
function_command (const char *command, int from_tty)
{
  /* Do nothing.  */
}

/* Helper function that does the work for add_internal_function.  */

static struct cmd_list_element *
do_add_internal_function (const char *name, const char *doc,
			  internal_function_fn handler, void *cookie)
{
  struct internal_function *ifn;
  struct internalvar *var = lookup_internalvar (name);

  ifn = create_internal_function (name, handler, cookie);
  set_internalvar_function (var, ifn);

  return add_cmd (name, no_class, function_command, doc, &functionlist);
}

/* See value.h.  */

void
add_internal_function (const char *name, const char *doc,
		       internal_function_fn handler, void *cookie)
{
  do_add_internal_function (name, doc, handler, cookie);
}

/* See value.h.  */

void
add_internal_function (gdb::unique_xmalloc_ptr<char> &&name,
		       gdb::unique_xmalloc_ptr<char> &&doc,
		       internal_function_fn handler, void *cookie)
{
  struct cmd_list_element *cmd
    = do_add_internal_function (name.get (), doc.get (), handler, cookie);
  doc.release ();
  cmd->doc_allocated = 1;
  name.release ();
  cmd->name_allocated = 1;
}

/* Update VALUE before discarding OBJFILE.  COPIED_TYPES is used to
   prevent cycles / duplicates.  */

void
preserve_one_value (struct value *value, struct objfile *objfile,
		    htab_t copied_types)
{
  if (value->type->objfile_owner () == objfile)
    value->type = copy_type_recursive (value->type, copied_types);

  if (value->enclosing_type->objfile_owner () == objfile)
    value->enclosing_type = copy_type_recursive (value->enclosing_type,
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
      if (var->u.integer.type
	  && var->u.integer.type->objfile_owner () == objfile)
	var->u.integer.type
	  = copy_type_recursive (var->u.integer.type, copied_types);
      break;

    case INTERNALVAR_VALUE:
      preserve_one_value (var->u.value, objfile, copied_types);
      break;
    }
}

/* Make sure that all types and values referenced by VAROBJ are updated before
   OBJFILE is discarded.  COPIED_TYPES is used to prevent cycles and
   duplicates.  */

static void
preserve_one_varobj (struct varobj *varobj, struct objfile *objfile,
		     htab_t copied_types)
{
  if (varobj->type->is_objfile_owned ()
      && varobj->type->objfile_owner () == objfile)
    {
      varobj->type
	= copy_type_recursive (varobj->type, copied_types);
    }

  if (varobj->value != nullptr)
    preserve_one_value (varobj->value.get (), objfile, copied_types);
}

/* Update the internal variables and value history when OBJFILE is
   discarded; we must copy the types out of the objfile.  New global types
   will be created for every convenience variable which currently points to
   this objfile's types, and the convenience variables will be adjusted to
   use the new global types.  */

void
preserve_values (struct objfile *objfile)
{
  struct internalvar *var;

  /* Create the hash table.  We allocate on the objfile's obstack, since
     it is soon to be deleted.  */
  htab_up copied_types = create_copied_types_hash ();

  for (const value_ref_ptr &item : value_history)
    preserve_one_value (item.get (), objfile, copied_types.get ());

  for (var = internalvars; var; var = var->next)
    preserve_one_internalvar (var, objfile, copied_types.get ());

  /* For the remaining varobj, check that none has type owned by OBJFILE.  */
  all_root_varobjs ([&copied_types, objfile] (struct varobj *varobj)
    {
      preserve_one_varobj (varobj, objfile,
			   copied_types.get ());
    });

  preserve_ext_lang_values (objfile, copied_types.get ());
}

static void
show_convenience (const char *ignore, int from_tty)
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
      gdb_printf (("$%s = "), var->name);

      try
	{
	  struct value *val;

	  val = value_of_internalvar (gdbarch, var);
	  value_print (val, gdb_stdout, &opts);
	}
      catch (const gdb_exception_error &ex)
	{
	  fprintf_styled (gdb_stdout, metadata_style.style (),
			  _("<error: %s>"), ex.what ());
	}

      gdb_printf (("\n"));
    }
  if (!varseen)
    {
      /* This text does not mention convenience functions on purpose.
	 The user can't create them except via Python, and if Python support
	 is installed this message will never be printed ($_streq will
	 exist).  */
      gdb_printf (_("No debugger convenience variables now defined.\n"
		    "Convenience variables have "
		    "names starting with \"$\";\n"
		    "use \"set\" as in \"set "
		    "$foo = 5\" to define them.\n"));
    }
}


/* See value.h.  */

struct value *
value_from_xmethod (xmethod_worker_up &&worker)
{
  struct value *v;

  v = allocate_value (builtin_type (target_gdbarch ())->xmethod);
  v->lval = lval_xcallable;
  v->location.xm_worker = worker.release ();
  v->modifiable = 0;

  return v;
}

/* Return the type of the result of TYPE_CODE_XMETHOD value METHOD.  */

struct type *
result_type_of_xmethod (struct value *method, gdb::array_view<value *> argv)
{
  gdb_assert (value_type (method)->code () == TYPE_CODE_XMETHOD
	      && method->lval == lval_xcallable && !argv.empty ());

  return method->location.xm_worker->get_result_type (argv[0], argv.slice (1));
}

/* Call the xmethod corresponding to the TYPE_CODE_XMETHOD value METHOD.  */

struct value *
call_xmethod (struct value *method, gdb::array_view<value *> argv)
{
  gdb_assert (value_type (method)->code () == TYPE_CODE_XMETHOD
	      && method->lval == lval_xcallable && !argv.empty ());

  return method->location.xm_worker->invoke (argv[0], argv.slice (1));
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
  return unpack_long (value_type (val), value_contents (val).data ());
}

/* Extract a value as a C pointer.  Does not deallocate the value.
   Note that val's type may not actually be a pointer; value_as_long
   handles all the cases.  */
CORE_ADDR
value_as_address (struct value *val)
{
  struct gdbarch *gdbarch = value_type (val)->arch ();

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
  if (value_type (val)->code () == TYPE_CODE_FUNC
      || value_type (val)->code () == TYPE_CODE_METHOD)
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

  if (!value_type (val)->is_pointer_or_reference ()
      && gdbarch_integer_to_address_p (gdbarch))
    return gdbarch_integer_to_address (gdbarch, value_type (val),
				       value_contents (val).data ());

  return unpack_long (value_type (val), value_contents (val).data ());
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
  if (is_fixed_point_type (type))
    type = type->fixed_point_type_base_type ();

  enum bfd_endian byte_order = type_byte_order (type);
  enum type_code code = type->code ();
  int len = type->length ();
  int nosign = type->is_unsigned ();

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
      {
	LONGEST result;

	if (type->bit_size_differs_p ())
	  {
	    unsigned bit_off = type->bit_offset ();
	    unsigned bit_size = type->bit_size ();
	    if (bit_size == 0)
	      {
		/* unpack_bits_as_long doesn't handle this case the
		   way we'd like, so handle it here.  */
		result = 0;
	      }
	    else
	      result = unpack_bits_as_long (type, valaddr, bit_off, bit_size);
	  }
	else
	  {
	    if (nosign)
	      result = extract_unsigned_integer (valaddr, len, byte_order);
	    else
	      result = extract_signed_integer (valaddr, len, byte_order);
	  }
	if (code == TYPE_CODE_RANGE)
	  result += type->bounds ()->bias;
	return result;
      }

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      return target_float_to_longest (valaddr, type);

    case TYPE_CODE_FIXED_POINT:
      {
	gdb_mpq vq;
	vq.read_fixed_point (gdb::make_array_view (valaddr, len),
			     byte_order, nosign,
			     type->fixed_point_scaling_factor ());

	gdb_mpz vz;
	mpz_tdiv_q (vz.val, mpq_numref (vq.val), mpq_denref (vq.val));
	return vz.as_integer<LONGEST> ();
      }

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
	 whether we want this to be true eventually.  */
      return extract_typed_address (valaddr, type);

    default:
      error (_("Value can't be converted to integer."));
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

bool
is_floating_value (struct value *val)
{
  struct type *type = check_typedef (value_type (val));

  if (is_floating_type (type))
    {
      if (!target_float_is_valid (value_contents (val).data (), type))
	error (_("Invalid floating value found in program."));
      return true;
    }

  return false;
}


/* Get the value of the FIELDNO'th field (which must be static) of
   TYPE.  */

struct value *
value_static_field (struct type *type, int fieldno)
{
  struct value *retval;

  switch (type->field (fieldno).loc_kind ())
    {
    case FIELD_LOC_KIND_PHYSADDR:
      retval = value_at_lazy (type->field (fieldno).type (),
			      type->field (fieldno).loc_physaddr ());
      break;
    case FIELD_LOC_KIND_PHYSNAME:
    {
      const char *phys_name = type->field (fieldno).loc_physname ();
      /* type->field (fieldno).name (); */
      struct block_symbol sym = lookup_symbol (phys_name, 0, VAR_DOMAIN, 0);

      if (sym.symbol == NULL)
	{
	  /* With some compilers, e.g. HP aCC, static data members are
	     reported as non-debuggable symbols.  */
	  struct bound_minimal_symbol msym
	    = lookup_minimal_symbol (phys_name, NULL, NULL);
	  struct type *field_type = type->field (fieldno).type ();

	  if (!msym.minsym)
	    retval = allocate_optimized_out_value (field_type);
	  else
	    retval = value_at_lazy (field_type, msym.value_address ());
	}
      else
	retval = value_of_variable (sym.symbol, sym.block);
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
  if (new_encl_type->length () > value_enclosing_type (val)->length ())
    {
      check_type_length_before_alloc (new_encl_type);
      val->contents
	.reset ((gdb_byte *) xrealloc (val->contents.release (),
				       new_encl_type->length ()));
    }

  val->enclosing_type = new_encl_type;
}

/* Given a value ARG1 (offset by OFFSET bytes)
   of a struct or union type ARG_TYPE,
   extract and return the value of one of its (non-static) fields.
   FIELDNO says which field.  */

struct value *
value_primitive_field (struct value *arg1, LONGEST offset,
		       int fieldno, struct type *arg_type)
{
  struct value *v;
  struct type *type;
  struct gdbarch *arch = get_value_arch (arg1);
  int unit_size = gdbarch_addressable_memory_unit_size (arch);

  arg_type = check_typedef (arg_type);
  type = arg_type->field (fieldno).type ();

  /* Call check_typedef on our type to make sure that, if TYPE
     is a TYPE_CODE_TYPEDEF, its length is set to the length
     of the target type instead of zero.  However, we do not
     replace the typedef type by the target type, because we want
     to keep the typedef in order to be able to print the type
     description correctly.  */
  check_typedef (type);

  if (TYPE_FIELD_BITSIZE (arg_type, fieldno))
    {
      /* Handle packed fields.

	 Create a new value for the bitfield, with bitpos and bitsize
	 set.  If possible, arrange offset and bitpos so that we can
	 do a single aligned read of the size of the containing type.
	 Otherwise, adjust offset to the byte containing the first
	 bit.  Assume that the address, offset, and embedded offset
	 are sufficiently aligned.  */

      LONGEST bitpos = arg_type->field (fieldno).loc_bitpos ();
      LONGEST container_bitsize = type->length () * 8;

      v = allocate_value_lazy (type);
      v->bitsize = TYPE_FIELD_BITSIZE (arg_type, fieldno);
      if ((bitpos % container_bitsize) + v->bitsize <= container_bitsize
	  && type->length () <= (int) sizeof (LONGEST))
	v->bitpos = bitpos % container_bitsize;
      else
	v->bitpos = bitpos % 8;
      v->offset = (value_embedded_offset (arg1)
		   + offset
		   + (bitpos - v->bitpos) / 8);
      set_value_parent (v, arg1);
      if (!value_lazy (arg1))
	value_fetch_lazy (v);
    }
  else if (fieldno < TYPE_N_BASECLASSES (arg_type))
    {
      /* This field is actually a base subobject, so preserve the
	 entire object's contents for later references to virtual
	 bases, etc.  */
      LONGEST boffset;

      /* Lazy register values with offsets are not supported.  */
      if (VALUE_LVAL (arg1) == lval_register && value_lazy (arg1))
	value_fetch_lazy (arg1);

      /* We special case virtual inheritance here because this
	 requires access to the contents, which we would rather avoid
	 for references to ordinary fields of unavailable values.  */
      if (BASETYPE_VIA_VIRTUAL (arg_type, fieldno))
	boffset = baseclass_offset (arg_type, fieldno,
				    value_contents (arg1).data (),
				    value_embedded_offset (arg1),
				    value_address (arg1),
				    arg1);
      else
	boffset = arg_type->field (fieldno).loc_bitpos () / 8;

      if (value_lazy (arg1))
	v = allocate_value_lazy (value_enclosing_type (arg1));
      else
	{
	  v = allocate_value (value_enclosing_type (arg1));
	  value_contents_copy_raw (v, 0, arg1, 0,
				   value_enclosing_type (arg1)->length ());
	}
      v->type = type;
      v->offset = value_offset (arg1);
      v->embedded_offset = offset + value_embedded_offset (arg1) + boffset;
    }
  else if (NULL != TYPE_DATA_LOCATION (type))
    {
      /* Field is a dynamic data member.  */

      gdb_assert (0 == offset);
      /* We expect an already resolved data location.  */
      gdb_assert (PROP_CONST == TYPE_DATA_LOCATION_KIND (type));
      /* For dynamic data types defer memory allocation
	 until we actual access the value.  */
      v = allocate_value_lazy (type);
    }
  else
    {
      /* Plain old data member */
      offset += (arg_type->field (fieldno).loc_bitpos ()
		 / (HOST_CHAR_BIT * unit_size));

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
				   type_length_units (type));
	}
      v->offset = (value_offset (arg1) + offset
		   + value_embedded_offset (arg1));
    }
  set_value_component_location (v, arg1);
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
		LONGEST offset)
{
  struct value *v;
  struct type *ftype = TYPE_FN_FIELD_TYPE (f, j);
  const char *physname = TYPE_FN_FIELD_PHYSNAME (f, j);
  struct symbol *sym;
  struct bound_minimal_symbol msym;

  sym = lookup_symbol (physname, 0, VAR_DOMAIN, 0).symbol;
  if (sym == nullptr)
    {
      msym = lookup_bound_minimal_symbol (physname);
      if (msym.minsym == NULL)
	return NULL;
    }

  v = allocate_value (ftype);
  VALUE_LVAL (v) = lval_memory;
  if (sym)
    {
      set_value_address (v, sym->value_block ()->entry_pc ());
    }
  else
    {
      /* The minimal symbol might point to a function descriptor;
	 resolve it to the actual code address instead.  */
      struct objfile *objfile = msym.objfile;
      struct gdbarch *gdbarch = objfile->arch ();

      set_value_address (v,
	gdbarch_convert_from_func_ptr_addr
	   (gdbarch, msym.value_address (),
	    current_inferior ()->top_target ()));
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



/* See value.h.  */

LONGEST
unpack_bits_as_long (struct type *field_type, const gdb_byte *valaddr,
		     LONGEST bitpos, LONGEST bitsize)
{
  enum bfd_endian byte_order = type_byte_order (field_type);
  ULONGEST val;
  ULONGEST valmask;
  int lsbcount;
  LONGEST bytes_read;
  LONGEST read_offset;

  /* Read the minimum number of bytes required; there may not be
     enough bytes to read an entire ULONGEST.  */
  field_type = check_typedef (field_type);
  if (bitsize)
    bytes_read = ((bitpos % 8) + bitsize + 7) / 8;
  else
    {
      bytes_read = field_type->length ();
      bitsize = 8 * bytes_read;
    }

  read_offset = bitpos / 8;

  val = extract_unsigned_integer (valaddr + read_offset,
				  bytes_read, byte_order);

  /* Extract bits.  See comment above.  */

  if (byte_order == BFD_ENDIAN_BIG)
    lsbcount = (bytes_read * 8 - bitpos % 8 - bitsize);
  else
    lsbcount = (bitpos % 8);
  val >>= lsbcount;

  /* If the field does not entirely fill a LONGEST, then zero the sign bits.
     If the field is signed, and is negative, then sign extend.  */

  if (bitsize < 8 * (int) sizeof (val))
    {
      valmask = (((ULONGEST) 1) << bitsize) - 1;
      val &= valmask;
      if (!field_type->is_unsigned ())
	{
	  if (val & (valmask ^ (valmask >> 1)))
	    {
	      val |= ~valmask;
	    }
	}
    }

  return val;
}

/* Unpack a field FIELDNO of the specified TYPE, from the object at
   VALADDR + EMBEDDED_OFFSET.  VALADDR points to the contents of
   ORIGINAL_VALUE, which must not be NULL.  See
   unpack_value_bits_as_long for more details.  */

int
unpack_value_field_as_long (struct type *type, const gdb_byte *valaddr,
			    LONGEST embedded_offset, int fieldno,
			    const struct value *val, LONGEST *result)
{
  int bitpos = type->field (fieldno).loc_bitpos ();
  int bitsize = TYPE_FIELD_BITSIZE (type, fieldno);
  struct type *field_type = type->field (fieldno).type ();
  int bit_offset;

  gdb_assert (val != NULL);

  bit_offset = embedded_offset * TARGET_CHAR_BIT + bitpos;
  if (value_bits_any_optimized_out (val, bit_offset, bitsize)
      || !value_bits_available (val, bit_offset, bitsize))
    return 0;

  *result = unpack_bits_as_long (field_type, valaddr + embedded_offset,
				 bitpos, bitsize);
  return 1;
}

/* Unpack a field FIELDNO of the specified TYPE, from the anonymous
   object at VALADDR.  See unpack_bits_as_long for more details.  */

LONGEST
unpack_field_as_long (struct type *type, const gdb_byte *valaddr, int fieldno)
{
  int bitpos = type->field (fieldno).loc_bitpos ();
  int bitsize = TYPE_FIELD_BITSIZE (type, fieldno);
  struct type *field_type = type->field (fieldno).type ();

  return unpack_bits_as_long (field_type, valaddr, bitpos, bitsize);
}

/* Unpack a bitfield of BITSIZE bits found at BITPOS in the object at
   VALADDR + EMBEDDEDOFFSET that has the type of DEST_VAL and store
   the contents in DEST_VAL, zero or sign extending if the type of
   DEST_VAL is wider than BITSIZE.  VALADDR points to the contents of
   VAL.  If the VAL's contents required to extract the bitfield from
   are unavailable/optimized out, DEST_VAL is correspondingly
   marked unavailable/optimized out.  */

void
unpack_value_bitfield (struct value *dest_val,
		       LONGEST bitpos, LONGEST bitsize,
		       const gdb_byte *valaddr, LONGEST embedded_offset,
		       const struct value *val)
{
  enum bfd_endian byte_order;
  int src_bit_offset;
  int dst_bit_offset;
  struct type *field_type = value_type (dest_val);

  byte_order = type_byte_order (field_type);

  /* First, unpack and sign extend the bitfield as if it was wholly
     valid.  Optimized out/unavailable bits are read as zero, but
     that's OK, as they'll end up marked below.  If the VAL is
     wholly-invalid we may have skipped allocating its contents,
     though.  See allocate_optimized_out_value.  */
  if (valaddr != NULL)
    {
      LONGEST num;

      num = unpack_bits_as_long (field_type, valaddr + embedded_offset,
				 bitpos, bitsize);
      store_signed_integer (value_contents_raw (dest_val).data (),
			    field_type->length (), byte_order, num);
    }

  /* Now copy the optimized out / unavailability ranges to the right
     bits.  */
  src_bit_offset = embedded_offset * TARGET_CHAR_BIT + bitpos;
  if (byte_order == BFD_ENDIAN_BIG)
    dst_bit_offset = field_type->length () * TARGET_CHAR_BIT - bitsize;
  else
    dst_bit_offset = 0;
  value_ranges_copy_adjusted (dest_val, dst_bit_offset,
			      val, src_bit_offset, bitsize);
}

/* Return a new value with type TYPE, which is FIELDNO field of the
   object at VALADDR + EMBEDDEDOFFSET.  VALADDR points to the contents
   of VAL.  If the VAL's contents required to extract the bitfield
   from are unavailable/optimized out, the new value is
   correspondingly marked unavailable/optimized out.  */

struct value *
value_field_bitfield (struct type *type, int fieldno,
		      const gdb_byte *valaddr,
		      LONGEST embedded_offset, const struct value *val)
{
  int bitpos = type->field (fieldno).loc_bitpos ();
  int bitsize = TYPE_FIELD_BITSIZE (type, fieldno);
  struct value *res_val = allocate_value (type->field (fieldno).type ());

  unpack_value_bitfield (res_val, bitpos, bitsize,
			 valaddr, embedded_offset, val);

  return res_val;
}

/* Modify the value of a bitfield.  ADDR points to a block of memory in
   target byte order; the bitfield starts in the byte pointed to.  FIELDVAL
   is the desired value of the field, in host byte order.  BITPOS and BITSIZE
   indicate which bits (in target bit order) comprise the bitfield.
   Requires 0 < BITSIZE <= lbits, 0 <= BITPOS % 8 + BITSIZE <= lbits, and
   0 <= BITPOS, where lbits is the size of a LONGEST in bits.  */

void
modify_field (struct type *type, gdb_byte *addr,
	      LONGEST fieldval, LONGEST bitpos, LONGEST bitsize)
{
  enum bfd_endian byte_order = type_byte_order (type);
  ULONGEST oword;
  ULONGEST mask = (ULONGEST) -1 >> (8 * sizeof (ULONGEST) - bitsize);
  LONGEST bytesize;

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
      warning (_("Value does not fit in %s bits."), plongest (bitsize));

      /* Truncate it, otherwise adjoining fields may be corrupted.  */
      fieldval &= mask;
    }

  /* Ensure no bytes outside of the modified ones get accessed as it may cause
     false valgrind reports.  */

  bytesize = (bitpos + bitsize + 7) / 8;
  oword = extract_unsigned_integer (addr, bytesize, byte_order);

  /* Shifting for bit field depends on endianness of the target machine.  */
  if (byte_order == BFD_ENDIAN_BIG)
    bitpos = bytesize * 8 - bitpos - bitsize;

  oword &= ~(mask << bitpos);
  oword |= fieldval << bitpos;

  store_unsigned_integer (addr, bytesize, byte_order, oword);
}

/* Pack NUM into BUF using a target format of TYPE.  */

void
pack_long (gdb_byte *buf, struct type *type, LONGEST num)
{
  enum bfd_endian byte_order = type_byte_order (type);
  LONGEST len;

  type = check_typedef (type);
  len = type->length ();

  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      num -= type->bounds ()->bias;
      /* Fall through.  */
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_MEMBERPTR:
      if (type->bit_size_differs_p ())
	{
	  unsigned bit_off = type->bit_offset ();
	  unsigned bit_size = type->bit_size ();
	  num &= ((ULONGEST) 1 << bit_size) - 1;
	  num <<= bit_off;
	}
      store_signed_integer (buf, len, byte_order, num);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_PTR:
      store_typed_address (buf, type, (CORE_ADDR) num);
      break;

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      target_float_from_longest (buf, type, num);
      break;

    default:
      error (_("Unexpected type (%d) encountered for integer constant."),
	     type->code ());
    }
}


/* Pack NUM into BUF using a target format of TYPE.  */

static void
pack_unsigned_long (gdb_byte *buf, struct type *type, ULONGEST num)
{
  LONGEST len;
  enum bfd_endian byte_order;

  type = check_typedef (type);
  len = type->length ();
  byte_order = type_byte_order (type);

  switch (type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_MEMBERPTR:
      if (type->bit_size_differs_p ())
	{
	  unsigned bit_off = type->bit_offset ();
	  unsigned bit_size = type->bit_size ();
	  num &= ((ULONGEST) 1 << bit_size) - 1;
	  num <<= bit_off;
	}
      store_unsigned_integer (buf, len, byte_order, num);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_PTR:
      store_typed_address (buf, type, (CORE_ADDR) num);
      break;

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      target_float_from_ulongest (buf, type, num);
      break;

    default:
      error (_("Unexpected type (%d) encountered "
	       "for unsigned integer constant."),
	     type->code ());
    }
}


/* Create a value of type TYPE that is zero, and return it.  */

struct value *
value_zero (struct type *type, enum lval_type lv)
{
  struct value *val = allocate_value_lazy (type);

  VALUE_LVAL (val) = (lv == lval_computed ? not_lval : lv);
  val->is_zero = true;
  return val;
}

/* Convert C numbers into newly allocated values.  */

struct value *
value_from_longest (struct type *type, LONGEST num)
{
  struct value *val = allocate_value (type);

  pack_long (value_contents_raw (val).data (), type, num);
  return val;
}


/* Convert C unsigned numbers into newly allocated values.  */

struct value *
value_from_ulongest (struct type *type, ULONGEST num)
{
  struct value *val = allocate_value (type);

  pack_unsigned_long (value_contents_raw (val).data (), type, num);

  return val;
}


/* Create a value representing a pointer of type TYPE to the address
   ADDR.  */

struct value *
value_from_pointer (struct type *type, CORE_ADDR addr)
{
  struct value *val = allocate_value (type);

  store_typed_address (value_contents_raw (val).data (),
		       check_typedef (type), addr);
  return val;
}

/* Create and return a value object of TYPE containing the value D.  The
   TYPE must be of TYPE_CODE_FLT, and must be large enough to hold D once
   it is converted to target format.  */

struct value *
value_from_host_double (struct type *type, double d)
{
  struct value *value = allocate_value (type);
  gdb_assert (type->code () == TYPE_CODE_FLT);
  target_float_from_host_double (value_contents_raw (value).data (),
				 value_type (value), d);
  return value;
}

/* Create a value of type TYPE whose contents come from VALADDR, if it
   is non-null, and whose memory address (in the inferior) is
   ADDRESS.  The type of the created value may differ from the passed
   type TYPE.  Make sure to retrieve values new type after this call.
   Note that TYPE is not passed through resolve_dynamic_type; this is
   a special API intended for use only by Ada.  */

struct value *
value_from_contents_and_address_unresolved (struct type *type,
					    const gdb_byte *valaddr,
					    CORE_ADDR address)
{
  struct value *v;

  if (valaddr == NULL)
    v = allocate_value_lazy (type);
  else
    v = value_from_contents (type, valaddr);
  VALUE_LVAL (v) = lval_memory;
  set_value_address (v, address);
  return v;
}

/* Create a value of type TYPE whose contents come from VALADDR, if it
   is non-null, and whose memory address (in the inferior) is
   ADDRESS.  The type of the created value may differ from the passed
   type TYPE.  Make sure to retrieve values new type after this call.  */

struct value *
value_from_contents_and_address (struct type *type,
				 const gdb_byte *valaddr,
				 CORE_ADDR address)
{
  gdb::array_view<const gdb_byte> view;
  if (valaddr != nullptr)
    view = gdb::make_array_view (valaddr, type->length ());
  struct type *resolved_type = resolve_dynamic_type (type, view, address);
  struct type *resolved_type_no_typedef = check_typedef (resolved_type);
  struct value *v;

  if (valaddr == NULL)
    v = allocate_value_lazy (resolved_type);
  else
    v = value_from_contents (resolved_type, valaddr);
  if (TYPE_DATA_LOCATION (resolved_type_no_typedef) != NULL
      && TYPE_DATA_LOCATION_KIND (resolved_type_no_typedef) == PROP_CONST)
    address = TYPE_DATA_LOCATION_ADDR (resolved_type_no_typedef);
  VALUE_LVAL (v) = lval_memory;
  set_value_address (v, address);
  return v;
}

/* Create a value of type TYPE holding the contents CONTENTS.
   The new value is `not_lval'.  */

struct value *
value_from_contents (struct type *type, const gdb_byte *contents)
{
  struct value *result;

  result = allocate_value (type);
  memcpy (value_contents_raw (result).data (), contents, type->length ());
  return result;
}

/* Extract a value from the history file.  Input will be of the form
   $digits or $$digits.  See block comment above 'write_dollar_variable'
   for details.  */

struct value *
value_from_history_ref (const char *h, const char **endp)
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
	{
	  char *local_end;

	  index = -strtol (&h[2], &local_end, 10);
	  *endp = local_end;
	}
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
	{
	  char *local_end;

	  index = strtol (&h[1], &local_end, 10);
	  *endp = local_end;
	}
    }

  return access_value_history (index);
}

/* Get the component value (offset by OFFSET bytes) of a struct or
   union WHOLE.  Component's type is TYPE.  */

struct value *
value_from_component (struct value *whole, struct type *type, LONGEST offset)
{
  struct value *v;

  if (VALUE_LVAL (whole) == lval_memory && value_lazy (whole))
    v = allocate_value_lazy (type);
  else
    {
      v = allocate_value (type);
      value_contents_copy (v, value_embedded_offset (v),
			   whole, value_embedded_offset (whole) + offset,
			   type_length_units (type));
    }
  v->offset = value_offset (whole) + offset + value_embedded_offset (whole);
  set_value_component_location (v, whole);

  return v;
}

/* See value.h.  */

struct value *
value_from_component_bitsize (struct value *whole, struct type *type,
			      LONGEST bit_offset, LONGEST bit_length)
{
  gdb_assert (!value_lazy (whole));

  /* Preserve lvalue-ness if possible.  This is needed to avoid
     array-printing failures (including crashes) when printing Ada
     arrays in programs compiled with -fgnat-encodings=all.  */
  if ((bit_offset % TARGET_CHAR_BIT) == 0
      && (bit_length % TARGET_CHAR_BIT) == 0
      && bit_length == TARGET_CHAR_BIT * type->length ())
    return value_from_component (whole, type, bit_offset / TARGET_CHAR_BIT);

  struct value *v = allocate_value (type);

  LONGEST dst_offset = TARGET_CHAR_BIT * value_embedded_offset (v);
  if (is_scalar_type (type) && type_byte_order (type) == BFD_ENDIAN_BIG)
    dst_offset += TARGET_CHAR_BIT * type->length () - bit_length;

  value_contents_copy_raw_bitwise (v, dst_offset,
				   whole,
				   TARGET_CHAR_BIT
				   * value_embedded_offset (whole)
				   + bit_offset,
				   bit_length);
  return v;
}

struct value *
coerce_ref_if_computed (const struct value *arg)
{
  const struct lval_funcs *funcs;

  if (!TYPE_IS_REFERENCE (check_typedef (value_type (arg))))
    return NULL;

  if (value_lval_const (arg) != lval_computed)
    return NULL;

  funcs = value_computed_funcs (arg);
  if (funcs->coerce_ref == NULL)
    return NULL;

  return funcs->coerce_ref (arg);
}

/* Look at value.h for description.  */

struct value *
readjust_indirect_value_type (struct value *value, struct type *enc_type,
			      const struct type *original_type,
			      struct value *original_value,
			      CORE_ADDR original_value_address)
{
  gdb_assert (original_type->is_pointer_or_reference ());

  struct type *original_target_type = original_type->target_type ();
  gdb::array_view<const gdb_byte> view;
  struct type *resolved_original_target_type
    = resolve_dynamic_type (original_target_type, view,
			    original_value_address);

  /* Re-adjust type.  */
  deprecated_set_value_type (value, resolved_original_target_type);

  /* Add embedding info.  */
  set_value_enclosing_type (value, enc_type);
  set_value_embedded_offset (value, value_pointed_to_offset (original_value));

  /* We may be pointing to an object of some derived type.  */
  return value_full_object (value, NULL, 0, 0, 0);
}

struct value *
coerce_ref (struct value *arg)
{
  struct type *value_type_arg_tmp = check_typedef (value_type (arg));
  struct value *retval;
  struct type *enc_type;

  retval = coerce_ref_if_computed (arg);
  if (retval)
    return retval;

  if (!TYPE_IS_REFERENCE (value_type_arg_tmp))
    return arg;

  enc_type = check_typedef (value_enclosing_type (arg));
  enc_type = enc_type->target_type ();

  CORE_ADDR addr = unpack_pointer (value_type (arg), value_contents (arg).data ());
  retval = value_at_lazy (enc_type, addr);
  enc_type = value_type (retval);
  return readjust_indirect_value_type (retval, enc_type, value_type_arg_tmp,
				       arg, addr);
}

struct value *
coerce_array (struct value *arg)
{
  struct type *type;

  arg = coerce_ref (arg);
  type = check_typedef (value_type (arg));

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      if (!type->is_vector () && current_language->c_style_arrays_p ())
	arg = value_coerce_array (arg);
      break;
    case TYPE_CODE_FUNC:
      arg = value_coerce_function (arg);
      break;
    }
  return arg;
}


/* Return the return value convention that will be used for the
   specified type.  */

enum return_value_convention
struct_return_convention (struct gdbarch *gdbarch,
			  struct value *function, struct type *value_type)
{
  enum type_code code = value_type->code ();

  if (code == TYPE_CODE_ERROR)
    error (_("Function return type unknown."));

  /* Probe the architecture for the return-value convention.  */
  return gdbarch_return_value (gdbarch, function, value_type,
			       NULL, NULL, NULL);
}

/* Return true if the function returning the specified type is using
   the convention of returning structures in memory (passing in the
   address as a hidden first parameter).  */

int
using_struct_return (struct gdbarch *gdbarch,
		     struct value *function, struct type *value_type)
{
  if (value_type->code () == TYPE_CODE_VOID)
    /* A void return value is never in memory.  See also corresponding
       code in "print_return_value".  */
    return 0;

  return (struct_return_convention (gdbarch, function, value_type)
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
value_initialized (const struct value *val)
{
  return val->initialized;
}

/* Helper for value_fetch_lazy when the value is a bitfield.  */

static void
value_fetch_lazy_bitfield (struct value *val)
{
  gdb_assert (value_bitsize (val) != 0);

  /* To read a lazy bitfield, read the entire enclosing value.  This
     prevents reading the same block of (possibly volatile) memory once
     per bitfield.  It would be even better to read only the containing
     word, but we have no way to record that just specific bits of a
     value have been fetched.  */
  struct value *parent = value_parent (val);

  if (value_lazy (parent))
    value_fetch_lazy (parent);

  unpack_value_bitfield (val, value_bitpos (val), value_bitsize (val),
			 value_contents_for_printing (parent).data (),
			 value_offset (val), parent);
}

/* Helper for value_fetch_lazy when the value is in memory.  */

static void
value_fetch_lazy_memory (struct value *val)
{
  gdb_assert (VALUE_LVAL (val) == lval_memory);

  CORE_ADDR addr = value_address (val);
  struct type *type = check_typedef (value_enclosing_type (val));

  if (type->length ())
      read_value_memory (val, 0, value_stack (val),
			 addr, value_contents_all_raw (val).data (),
			 type_length_units (type));
}

/* Helper for value_fetch_lazy when the value is in a register.  */

static void
value_fetch_lazy_register (struct value *val)
{
  frame_info_ptr next_frame;
  int regnum;
  struct type *type = check_typedef (value_type (val));
  struct value *new_val = val, *mark = value_mark ();

  /* Offsets are not supported here; lazy register values must
     refer to the entire register.  */
  gdb_assert (value_offset (val) == 0);

  while (VALUE_LVAL (new_val) == lval_register && value_lazy (new_val))
    {
      struct frame_id next_frame_id = VALUE_NEXT_FRAME_ID (new_val);

      next_frame = frame_find_by_id (next_frame_id);
      regnum = VALUE_REGNUM (new_val);

      gdb_assert (next_frame != NULL);

      /* Convertible register routines are used for multi-register
	 values and for interpretation in different types
	 (e.g. float or int from a double register).  Lazy
	 register values should have the register's natural type,
	 so they do not apply.  */
      gdb_assert (!gdbarch_convert_register_p (get_frame_arch (next_frame),
					       regnum, type));

      /* FRAME was obtained, above, via VALUE_NEXT_FRAME_ID.
	 Since a "->next" operation was performed when setting
	 this field, we do not need to perform a "next" operation
	 again when unwinding the register.  That's why
	 frame_unwind_register_value() is called here instead of
	 get_frame_register_value().  */
      new_val = frame_unwind_register_value (next_frame, regnum);

      /* If we get another lazy lval_register value, it means the
	 register is found by reading it from NEXT_FRAME's next frame.
	 frame_unwind_register_value should never return a value with
	 the frame id pointing to NEXT_FRAME.  If it does, it means we
	 either have two consecutive frames with the same frame id
	 in the frame chain, or some code is trying to unwind
	 behind get_prev_frame's back (e.g., a frame unwind
	 sniffer trying to unwind), bypassing its validations.  In
	 any case, it should always be an internal error to end up
	 in this situation.  */
      if (VALUE_LVAL (new_val) == lval_register
	  && value_lazy (new_val)
	  && VALUE_NEXT_FRAME_ID (new_val) == next_frame_id)
	internal_error (_("infinite loop while fetching a register"));
    }

  /* If it's still lazy (for instance, a saved register on the
     stack), fetch it.  */
  if (value_lazy (new_val))
    value_fetch_lazy (new_val);

  /* Copy the contents and the unavailability/optimized-out
     meta-data from NEW_VAL to VAL.  */
  set_value_lazy (val, 0);
  value_contents_copy (val, value_embedded_offset (val),
		       new_val, value_embedded_offset (new_val),
		       type_length_units (type));

  if (frame_debug)
    {
      struct gdbarch *gdbarch;
      frame_info_ptr frame;
      frame = frame_find_by_id (VALUE_NEXT_FRAME_ID (val));
      frame = get_prev_frame_always (frame);
      regnum = VALUE_REGNUM (val);
      gdbarch = get_frame_arch (frame);

      string_file debug_file;
      gdb_printf (&debug_file,
		  "(frame=%d, regnum=%d(%s), ...) ",
		  frame_relative_level (frame), regnum,
		  user_reg_map_regnum_to_name (gdbarch, regnum));

      gdb_printf (&debug_file, "->");
      if (value_optimized_out (new_val))
	{
	  gdb_printf (&debug_file, " ");
	  val_print_optimized_out (new_val, &debug_file);
	}
      else
	{
	  int i;
	  gdb::array_view<const gdb_byte> buf = value_contents (new_val);

	  if (VALUE_LVAL (new_val) == lval_register)
	    gdb_printf (&debug_file, " register=%d",
			VALUE_REGNUM (new_val));
	  else if (VALUE_LVAL (new_val) == lval_memory)
	    gdb_printf (&debug_file, " address=%s",
			paddress (gdbarch,
				  value_address (new_val)));
	  else
	    gdb_printf (&debug_file, " computed");

	  gdb_printf (&debug_file, " bytes=");
	  gdb_printf (&debug_file, "[");
	  for (i = 0; i < register_size (gdbarch, regnum); i++)
	    gdb_printf (&debug_file, "%02x", buf[i]);
	  gdb_printf (&debug_file, "]");
	}

      frame_debug_printf ("%s", debug_file.c_str ());
    }

  /* Dispose of the intermediate values.  This prevents
     watchpoints from trying to watch the saved frame pointer.  */
  value_free_to_mark (mark);
}

/* Load the actual content of a lazy value.  Fetch the data from the
   user's process and clear the lazy flag to indicate that the data in
   the buffer is valid.

   If the value is zero-length, we avoid calling read_memory, which
   would abort.  We mark the value as fetched anyway -- all 0 bytes of
   it.  */

void
value_fetch_lazy (struct value *val)
{
  gdb_assert (value_lazy (val));
  allocate_value_contents (val);
  /* A value is either lazy, or fully fetched.  The
     availability/validity is only established as we try to fetch a
     value.  */
  gdb_assert (val->optimized_out.empty ());
  gdb_assert (val->unavailable.empty ());
  if (val->is_zero)
    {
      /* Nothing.  */
    }
  else if (value_bitsize (val))
    value_fetch_lazy_bitfield (val);
  else if (VALUE_LVAL (val) == lval_memory)
    value_fetch_lazy_memory (val);
  else if (VALUE_LVAL (val) == lval_register)
    value_fetch_lazy_register (val);
  else if (VALUE_LVAL (val) == lval_computed
	   && value_computed_funcs (val)->read != NULL)
    value_computed_funcs (val)->read (val);
  else
    internal_error (_("Unexpected lazy value type."));

  set_value_lazy (val, 0);
}

/* Implementation of the convenience function $_isvoid.  */

static struct value *
isvoid_internal_fn (struct gdbarch *gdbarch,
		    const struct language_defn *language,
		    void *cookie, int argc, struct value **argv)
{
  int ret;

  if (argc != 1)
    error (_("You must provide one argument for $_isvoid."));

  ret = value_type (argv[0])->code () == TYPE_CODE_VOID;

  return value_from_longest (builtin_type (gdbarch)->builtin_int, ret);
}

/* Implementation of the convenience function $_creal.  Extracts the
   real part from a complex number.  */

static struct value *
creal_internal_fn (struct gdbarch *gdbarch,
		   const struct language_defn *language,
		   void *cookie, int argc, struct value **argv)
{
  if (argc != 1)
    error (_("You must provide one argument for $_creal."));

  value *cval = argv[0];
  type *ctype = check_typedef (value_type (cval));
  if (ctype->code () != TYPE_CODE_COMPLEX)
    error (_("expected a complex number"));
  return value_real_part (cval);
}

/* Implementation of the convenience function $_cimag.  Extracts the
   imaginary part from a complex number.  */

static struct value *
cimag_internal_fn (struct gdbarch *gdbarch,
		   const struct language_defn *language,
		   void *cookie, int argc,
		   struct value **argv)
{
  if (argc != 1)
    error (_("You must provide one argument for $_cimag."));

  value *cval = argv[0];
  type *ctype = check_typedef (value_type (cval));
  if (ctype->code () != TYPE_CODE_COMPLEX)
    error (_("expected a complex number"));
  return value_imaginary_part (cval);
}

#if GDB_SELF_TEST
namespace selftests
{

/* Test the ranges_contain function.  */

static void
test_ranges_contain ()
{
  std::vector<range> ranges;
  range r;

  /* [10, 14] */
  r.offset = 10;
  r.length = 5;
  ranges.push_back (r);

  /* [20, 24] */
  r.offset = 20;
  r.length = 5;
  ranges.push_back (r);

  /* [2, 6] */
  SELF_CHECK (!ranges_contain (ranges, 2, 5));
  /* [9, 13] */
  SELF_CHECK (ranges_contain (ranges, 9, 5));
  /* [10, 11] */
  SELF_CHECK (ranges_contain (ranges, 10, 2));
  /* [10, 14] */
  SELF_CHECK (ranges_contain (ranges, 10, 5));
  /* [13, 18] */
  SELF_CHECK (ranges_contain (ranges, 13, 6));
  /* [14, 18] */
  SELF_CHECK (ranges_contain (ranges, 14, 5));
  /* [15, 18] */
  SELF_CHECK (!ranges_contain (ranges, 15, 4));
  /* [16, 19] */
  SELF_CHECK (!ranges_contain (ranges, 16, 4));
  /* [16, 21] */
  SELF_CHECK (ranges_contain (ranges, 16, 6));
  /* [21, 21] */
  SELF_CHECK (ranges_contain (ranges, 21, 1));
  /* [21, 25] */
  SELF_CHECK (ranges_contain (ranges, 21, 5));
  /* [26, 28] */
  SELF_CHECK (!ranges_contain (ranges, 26, 3));
}

/* Check that RANGES contains the same ranges as EXPECTED.  */

static bool
check_ranges_vector (gdb::array_view<const range> ranges,
		     gdb::array_view<const range> expected)
{
  return ranges == expected;
}

/* Test the insert_into_bit_range_vector function.  */

static void
test_insert_into_bit_range_vector ()
{
  std::vector<range> ranges;

  /* [10, 14] */
  {
    insert_into_bit_range_vector (&ranges, 10, 5);
    static const range expected[] = {
      {10, 5}
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [10, 14] */
  {
    insert_into_bit_range_vector (&ranges, 11, 4);
    static const range expected = {10, 5};
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [10, 14] [20, 24] */
  {
    insert_into_bit_range_vector (&ranges, 20, 5);
    static const range expected[] = {
      {10, 5},
      {20, 5},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [10, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 17, 5);
    static const range expected[] = {
      {10, 5},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 8] [10, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 2, 7);
    static const range expected[] = {
      {2, 7},
      {10, 5},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 9, 1);
    static const range expected[] = {
      {2, 13},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 9, 1);
    static const range expected[] = {
      {2, 13},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 33] */
  {
    insert_into_bit_range_vector (&ranges, 4, 30);
    static const range expected = {2, 32};
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }
}

static void
test_value_copy ()
{
  type *type = builtin_type (current_inferior ()->gdbarch)->builtin_int;

  /* Verify that we can copy an entirely optimized out value, that may not have
     its contents allocated.  */
  value_ref_ptr val = release_value (allocate_optimized_out_value (type));
  value_ref_ptr copy = release_value (value_copy (val.get ()));

  SELF_CHECK (value_entirely_optimized_out (val.get ()));
  SELF_CHECK (value_entirely_optimized_out (copy.get ()));
}

} /* namespace selftests */
#endif /* GDB_SELF_TEST */

void _initialize_values ();
void
_initialize_values ()
{
  cmd_list_element *show_convenience_cmd
    = add_cmd ("convenience", no_class, show_convenience, _("\
Debugger convenience (\"$foo\") variables and functions.\n\
Convenience variables are created when you assign them values;\n\
thus, \"set $foo=1\" gives \"$foo\" the value 1.  Values may be any type.\n\
\n\
A few convenience variables are given values automatically:\n\
\"$_\"holds the last address examined with \"x\" or \"info lines\",\n\
\"$__\" holds the contents of the last address examined with \"x\"."
#ifdef HAVE_PYTHON
"\n\n\
Convenience functions are defined via the Python API."
#endif
	   ), &showlist);
  add_alias_cmd ("conv", show_convenience_cmd, no_class, 1, &showlist);

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
		  &functionlist, 0, &cmdlist);

  add_internal_function ("_isvoid", _("\
Check whether an expression is void.\n\
Usage: $_isvoid (expression)\n\
Return 1 if the expression is void, zero otherwise."),
			 isvoid_internal_fn, NULL);

  add_internal_function ("_creal", _("\
Extract the real part of a complex number.\n\
Usage: $_creal (expression)\n\
Return the real part of a complex number, the type depends on the\n\
type of a complex number."),
			 creal_internal_fn, NULL);

  add_internal_function ("_cimag", _("\
Extract the imaginary part of a complex number.\n\
Usage: $_cimag (expression)\n\
Return the imaginary part of a complex number, the type depends on the\n\
type of a complex number."),
			 cimag_internal_fn, NULL);

  add_setshow_zuinteger_unlimited_cmd ("max-value-size",
				       class_support, &max_value_size, _("\
Set maximum sized value gdb will load from the inferior."), _("\
Show maximum sized value gdb will load from the inferior."), _("\
Use this to control the maximum size, in bytes, of a value that gdb\n\
will load from the inferior.  Setting this value to 'unlimited'\n\
disables checking.\n\
Setting this does not invalidate already allocated values, it only\n\
prevents future values, larger than this size, from being allocated."),
			    set_max_value_size,
			    show_max_value_size,
			    &setlist, &showlist);
  set_show_commands vsize_limit
    = add_setshow_zuinteger_unlimited_cmd ("varsize-limit", class_support,
					   &max_value_size, _("\
Set the maximum number of bytes allowed in a variable-size object."), _("\
Show the maximum number of bytes allowed in a variable-size object."), _("\
Attempts to access an object whose size is not a compile-time constant\n\
and exceeds this limit will cause an error."),
					   NULL, NULL, &setlist, &showlist);
  deprecate_cmd (vsize_limit.set, "set max-value-size");

#if GDB_SELF_TEST
  selftests::register_test ("ranges_contain", selftests::test_ranges_contain);
  selftests::register_test ("insert_into_bit_range_vector",
			    selftests::test_insert_into_bit_range_vector);
  selftests::register_test ("value_copy", selftests::test_value_copy);
#endif
}

/* See value.h.  */

void
finalize_values ()
{
  all_values.clear ();
}
