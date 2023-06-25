/* Type information for omp-expand.c.
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
gt_ggc_mx_grid_launch_attributes_trees (void *x_p)
{
  struct grid_launch_attributes_trees * const x = (struct grid_launch_attributes_trees *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).kernel_dim_array_type);
      gt_ggc_m_9tree_node ((*x).kernel_lattrs_dimnum_decl);
      gt_ggc_m_9tree_node ((*x).kernel_lattrs_grid_decl);
      gt_ggc_m_9tree_node ((*x).kernel_lattrs_group_decl);
      gt_ggc_m_9tree_node ((*x).kernel_launch_attributes_type);
    }
}

void
gt_pch_nx_grid_launch_attributes_trees (void *x_p)
{
  struct grid_launch_attributes_trees * const x = (struct grid_launch_attributes_trees *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28grid_launch_attributes_trees))
    {
      gt_pch_n_9tree_node ((*x).kernel_dim_array_type);
      gt_pch_n_9tree_node ((*x).kernel_lattrs_dimnum_decl);
      gt_pch_n_9tree_node ((*x).kernel_lattrs_grid_decl);
      gt_pch_n_9tree_node ((*x).kernel_lattrs_group_decl);
      gt_pch_n_9tree_node ((*x).kernel_launch_attributes_type);
    }
}

void
gt_pch_p_28grid_launch_attributes_trees (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct grid_launch_attributes_trees * x ATTRIBUTE_UNUSED = (struct grid_launch_attributes_trees *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).kernel_dim_array_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).kernel_lattrs_dimnum_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).kernel_lattrs_grid_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).kernel_lattrs_group_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).kernel_launch_attributes_type), cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_omp_expand_h[] = {
  {
    &grid_attr_trees,
    1,
    sizeof (grid_attr_trees),
    &gt_ggc_mx_grid_launch_attributes_trees,
    &gt_pch_nx_grid_launch_attributes_trees
  },
  LAST_GGC_ROOT_TAB
};

