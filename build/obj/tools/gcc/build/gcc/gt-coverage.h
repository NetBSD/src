/* Type information for coverage.c.
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
gt_ggc_mx_coverage_data (void *x_p)
{
  struct coverage_data * x = (struct coverage_data *)x_p;
  struct coverage_data * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_13coverage_data ((*x).next);
      gt_ggc_m_9tree_node ((*x).fn_decl);
      {
        size_t i0;
        size_t l0 = (size_t)(GCOV_COUNTERS);
        for (i0 = 0; i0 != l0; i0++) {
          gt_ggc_m_9tree_node ((*x).ctr_vars[i0]);
        }
      }
      x = ((*x).next);
    }
}

void
gt_pch_nx_coverage_data (void *x_p)
{
  struct coverage_data * x = (struct coverage_data *)x_p;
  struct coverage_data * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_13coverage_data))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_13coverage_data ((*x).next);
      gt_pch_n_9tree_node ((*x).fn_decl);
      {
        size_t i0;
        size_t l0 = (size_t)(GCOV_COUNTERS);
        for (i0 = 0; i0 != l0; i0++) {
          gt_pch_n_9tree_node ((*x).ctr_vars[i0]);
        }
      }
      x = ((*x).next);
    }
}

void
gt_pch_p_13coverage_data (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct coverage_data * x ATTRIBUTE_UNUSED = (struct coverage_data *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).fn_decl), cookie);
  {
    size_t i0;
    size_t l0 = (size_t)(GCOV_COUNTERS);
    for (i0 = 0; i0 != l0; i0++) {
      if ((void *)(x) == this_obj)
        op (&((*x).ctr_vars[i0]), cookie);
    }
  }
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_coverage_h[] = {
  {
    &gcov_fn_info_ptr_type,
    1,
    sizeof (gcov_fn_info_ptr_type),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &gcov_fn_info_type,
    1,
    sizeof (gcov_fn_info_type),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &gcov_info_var,
    1,
    sizeof (gcov_info_var),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &fn_v_ctrs[0],
    1 * (GCOV_COUNTERS),
    sizeof (fn_v_ctrs[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &functions_head,
    1,
    sizeof (functions_head),
    &gt_ggc_mx_coverage_data,
    &gt_pch_nx_coverage_data
  },
  LAST_GGC_ROOT_TAB
};

