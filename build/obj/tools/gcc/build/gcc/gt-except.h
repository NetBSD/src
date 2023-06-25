/* Type information for except.c.
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
gt_ggc_mx_call_site_record_d (void *x_p)
{
  struct call_site_record_d * const x = (struct call_site_record_d *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).landing_pad);
    }
}

void
gt_ggc_mx_hash_map_tree_hash_tree_ (void *x_p)
{
  hash_map<tree_hash,tree> * const x = (hash_map<tree_hash,tree> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_hash& x_r ATTRIBUTE_UNUSED)
{
  struct tree_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_call_site_record_d (void *x_p)
{
  struct call_site_record_d * const x = (struct call_site_record_d *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_18call_site_record_d))
    {
      gt_pch_n_7rtx_def ((*x).landing_pad);
    }
}

void
gt_pch_nx_hash_map_tree_hash_tree_ (void *x_p)
{
  hash_map<tree_hash,tree> * const x = (hash_map<tree_hash,tree> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_24hash_map_tree_hash_tree_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_hash& x_r ATTRIBUTE_UNUSED)
{
  struct tree_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_18call_site_record_d (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct call_site_record_d * x ATTRIBUTE_UNUSED = (struct call_site_record_d *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).landing_pad), cookie);
}

void
gt_pch_p_24hash_map_tree_hash_tree_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree_hash,tree> * x ATTRIBUTE_UNUSED = (struct hash_map<tree_hash,tree> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_except_h[] = {
  {
    &sjlj_fc_type_node,
    1,
    sizeof (sjlj_fc_type_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &setjmp_fn,
    1,
    sizeof (setjmp_fn),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &type_to_runtime_map,
    1,
    sizeof (type_to_runtime_map),
    &gt_ggc_mx_hash_map_tree_hash_tree_,
    &gt_pch_nx_hash_map_tree_hash_tree_
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_except_h[] = {
  { &call_site_base, 1, sizeof (call_site_base), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

