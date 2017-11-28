/* Memory ranges

   Copyright (C) 2010-2016 Free Software Foundation, Inc.

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
#include "memrange.h"

int
mem_ranges_overlap (CORE_ADDR start1, int len1,
		    CORE_ADDR start2, int len2)
{
  ULONGEST h, l;

  l = max (start1, start2);
  h = min (start1 + len1, start2 + len2);
  return (l < h);
}

/* See memrange.h.  */

int
address_in_mem_range (CORE_ADDR address, const struct mem_range *r)
{
  return (r->start <= address
	  && (address - r->start) < r->length);
}

/* qsort comparison function, that compares mem_ranges.  Ranges are
   sorted in ascending START order.  */

static int
compare_mem_ranges (const void *ap, const void *bp)
{
  const struct mem_range *r1 = (const struct mem_range *) ap;
  const struct mem_range *r2 = (const struct mem_range *) bp;

  if (r1->start > r2->start)
    return 1;
  else if (r1->start < r2->start)
    return -1;
  else
    return 0;
}

void
normalize_mem_ranges (VEC(mem_range_s) *ranges)
{
  /* This function must not use any VEC operation on RANGES that
     reallocates the memory block as that invalidates the RANGES
     pointer, which callers expect to remain valid.  */

  if (!VEC_empty (mem_range_s, ranges))
    {
      struct mem_range *ra, *rb;
      int a, b;

      qsort (VEC_address (mem_range_s, ranges),
	     VEC_length (mem_range_s, ranges),
	     sizeof (mem_range_s),
	     compare_mem_ranges);

      a = 0;
      ra = VEC_index (mem_range_s, ranges, a);
      for (b = 1; VEC_iterate (mem_range_s, ranges, b, rb); b++)
	{
	  /* If mem_range B overlaps or is adjacent to mem_range A,
	     merge them.  */
	  if (rb->start <= ra->start + ra->length)
	    {
	      ra->length = max (ra->length,
				(rb->start - ra->start) + rb->length);
	      continue;		/* next b, same a */
	    }
	  a++;			/* next a */
	  ra = VEC_index (mem_range_s, ranges, a);

	  if (a != b)
	    *ra = *rb;
	}
      VEC_truncate (mem_range_s, ranges, a + 1);
    }
}
