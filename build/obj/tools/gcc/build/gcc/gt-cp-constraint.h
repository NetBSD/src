/* Type information for cp/constraint.cc.
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
gt_ggc_mx_sat_entry (void *x_p)
{
  struct sat_entry * const x = (struct sat_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).constr);
      gt_ggc_m_9tree_node ((*x).args);
      gt_ggc_m_9tree_node ((*x).result);
    }
}

void
gt_ggc_mx (struct sat_entry& x_r ATTRIBUTE_UNUSED)
{
  struct sat_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).constr);
  gt_ggc_m_9tree_node ((*x).args);
  gt_ggc_m_9tree_node ((*x).result);
}

void
gt_ggc_mx (struct sat_entry *& x)
{
  if (x)
    gt_ggc_mx_sat_entry ((void *) x);
}

void
gt_ggc_mx_hash_table_sat_hasher_ (void *x_p)
{
  hash_table<sat_hasher> * const x = (hash_table<sat_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct sat_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct sat_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_sat_entry (void *x_p)
{
  struct sat_entry * const x = (struct sat_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9sat_entry))
    {
      gt_pch_n_9tree_node ((*x).constr);
      gt_pch_n_9tree_node ((*x).args);
      gt_pch_n_9tree_node ((*x).result);
    }
}

void
gt_pch_nx (struct sat_entry& x_r ATTRIBUTE_UNUSED)
{
  struct sat_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).constr);
  gt_pch_n_9tree_node ((*x).args);
  gt_pch_n_9tree_node ((*x).result);
}

void
gt_pch_nx (struct sat_entry *& x)
{
  if (x)
    gt_pch_nx_sat_entry ((void *) x);
}

void
gt_pch_nx_hash_table_sat_hasher_ (void *x_p)
{
  hash_table<sat_hasher> * const x = (hash_table<sat_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_22hash_table_sat_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct sat_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct sat_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_9sat_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct sat_entry * x ATTRIBUTE_UNUSED = (struct sat_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).constr), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).args), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).result), cookie);
}

void
gt_pch_nx (struct sat_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).constr), cookie);
    op (&((*x).args), cookie);
    op (&((*x).result), cookie);
}

void
gt_pch_p_22hash_table_sat_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<sat_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<sat_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct sat_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_constraint_h[] = {
  {
    &decl_constraints,
    1,
    sizeof (decl_constraints),
    &gt_ggc_mx_hash_map_tree_tree_decl_tree_cache_traits_,
    &gt_pch_nx_hash_map_tree_tree_decl_tree_cache_traits_
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_cp_constraint_h[] = {
  { &decl_satisfied_cache, 1, sizeof (decl_satisfied_cache), NULL, NULL },
  { &sat_cache, 1, sizeof (sat_cache), NULL, NULL },
  { &normalized_map, 1, sizeof (normalized_map), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

void
gt_clear_caches_gt_cp_constraint_h ()
{
  gt_cleare_cache (decl_constraints);
}

