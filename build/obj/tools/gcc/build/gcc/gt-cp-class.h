/* Type information for cp/class.c.
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
gt_ggc_mx_hash_map_tree_int_ (void *x_p)
{
  hash_map<tree,int> * const x = (hash_map<tree,int> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_pch_nx_hash_map_tree_int_ (void *x_p)
{
  hash_map<tree,int> * const x = (hash_map<tree,int> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_18hash_map_tree_int_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_p_18hash_map_tree_int_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree,int> * x ATTRIBUTE_UNUSED = (struct hash_map<tree,int> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_class_h[] = {
  {
    &dvirt_fn,
    1,
    sizeof (dvirt_fn),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &abort_fndecl_addr,
    1,
    sizeof (abort_fndecl_addr),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &sizeof_biggest_empty_class,
    1,
    sizeof (sizeof_biggest_empty_class),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_cp_class_h[] = {
  { &enum_to_min_precision, 1, sizeof (enum_to_min_precision), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

