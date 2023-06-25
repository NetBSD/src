/* Type information for cp/logic.cc.
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
gt_ggc_mx_subsumption_entry (void *x_p)
{
  struct subsumption_entry * const x = (struct subsumption_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).lhs);
      gt_ggc_m_9tree_node ((*x).rhs);
    }
}

void
gt_ggc_mx (struct subsumption_entry& x_r ATTRIBUTE_UNUSED)
{
  struct subsumption_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).lhs);
  gt_ggc_m_9tree_node ((*x).rhs);
}

void
gt_ggc_mx (struct subsumption_entry *& x)
{
  if (x)
    gt_ggc_mx_subsumption_entry ((void *) x);
}

void
gt_ggc_mx_hash_table_subsumption_hasher_ (void *x_p)
{
  hash_table<subsumption_hasher> * const x = (hash_table<subsumption_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct subsumption_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct subsumption_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_subsumption_entry (void *x_p)
{
  struct subsumption_entry * const x = (struct subsumption_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17subsumption_entry))
    {
      gt_pch_n_9tree_node ((*x).lhs);
      gt_pch_n_9tree_node ((*x).rhs);
    }
}

void
gt_pch_nx (struct subsumption_entry& x_r ATTRIBUTE_UNUSED)
{
  struct subsumption_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).lhs);
  gt_pch_n_9tree_node ((*x).rhs);
}

void
gt_pch_nx (struct subsumption_entry *& x)
{
  if (x)
    gt_pch_nx_subsumption_entry ((void *) x);
}

void
gt_pch_nx_hash_table_subsumption_hasher_ (void *x_p)
{
  hash_table<subsumption_hasher> * const x = (hash_table<subsumption_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_30hash_table_subsumption_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct subsumption_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct subsumption_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_17subsumption_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct subsumption_entry * x ATTRIBUTE_UNUSED = (struct subsumption_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).lhs), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).rhs), cookie);
}

void
gt_pch_nx (struct subsumption_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).lhs), cookie);
    op (&((*x).rhs), cookie);
}

void
gt_pch_p_30hash_table_subsumption_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<subsumption_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<subsumption_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct subsumption_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_cp_logic_h[] = {
  { &subsumption_cache, 1, sizeof (subsumption_cache), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

