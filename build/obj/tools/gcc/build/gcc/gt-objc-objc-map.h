/* Type information for objc/objc-map.c.
   Copyright (C) 2004-2020 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* This file is machine generated.  Do not edit.  */

void
gt_ggc_mx_objc_map_private (void *x_p)
{
  struct objc_map_private * const x = (struct objc_map_private *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t l0 = (size_t)(((*x)).number_of_slots);
        size_t l1 = (size_t)(((*x)).number_of_slots);
        if ((*x).slots != NULL) {
          size_t i0;
          for (i0 = 0; i0 != (size_t)(l0); i0++) {
            gt_ggc_m_9tree_node ((*x).slots[i0]);
          }
          ggc_mark ((*x).slots);
        }
        if ((*x).values != NULL) {
          size_t i1;
          for (i1 = 0; i1 != (size_t)(l1); i1++) {
            gt_ggc_m_9tree_node ((*x).values[i1]);
          }
          ggc_mark ((*x).values);
        }
      }
    }
}

void
gt_pch_nx_objc_map_private (void *x_p)
{
  struct objc_map_private * const x = (struct objc_map_private *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16objc_map_private))
    {
      {
        size_t l0 = (size_t)(((*x)).number_of_slots);
        size_t l1 = (size_t)(((*x)).number_of_slots);
        if ((*x).slots != NULL) {
          size_t i0;
          for (i0 = 0; i0 != (size_t)(l0); i0++) {
            gt_pch_n_9tree_node ((*x).slots[i0]);
          }
          gt_pch_note_object ((*x).slots, x, gt_pch_p_16objc_map_private);
        }
        if ((*x).values != NULL) {
          size_t i1;
          for (i1 = 0; i1 != (size_t)(l1); i1++) {
            gt_pch_n_9tree_node ((*x).values[i1]);
          }
          gt_pch_note_object ((*x).values, x, gt_pch_p_16objc_map_private);
        }
      }
    }
}

void
gt_pch_p_16objc_map_private (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct objc_map_private * x ATTRIBUTE_UNUSED = (struct objc_map_private *)x_p;
  {
    size_t l0 = (size_t)(((*x)).number_of_slots);
    size_t l1 = (size_t)(((*x)).number_of_slots);
    if ((*x).slots != NULL) {
      size_t i0;
      for (i0 = 0; i0 != (size_t)(l0) && ((void *)(*x).slots == this_obj); i0++) {
        if ((void *)((*x).slots) == this_obj)
          op (&((*x).slots[i0]), cookie);
      }
      if ((void *)(x) == this_obj)
        op (&((*x).slots), cookie);
    }
    if ((*x).values != NULL) {
      size_t i1;
      for (i1 = 0; i1 != (size_t)(l1) && ((void *)(*x).values == this_obj); i1++) {
        if ((void *)((*x).values) == this_obj)
          op (&((*x).values[i1]), cookie);
      }
      if ((void *)(x) == this_obj)
        op (&((*x).values), cookie);
    }
  }
}
