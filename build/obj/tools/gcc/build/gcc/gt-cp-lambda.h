/* Type information for cp/lambda.c.
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
gt_ggc_mx_vec_tree_int_va_gc_ (void *x_p)
{
  vec<tree_int,va_gc> * const x = (vec<tree_int,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_int& x_r ATTRIBUTE_UNUSED)
{
  struct tree_int * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).t);
}

void
gt_pch_nx_vec_tree_int_va_gc_ (void *x_p)
{
  vec<tree_int,va_gc> * const x = (vec<tree_int,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_19vec_tree_int_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_int& x_r ATTRIBUTE_UNUSED)
{
  struct tree_int * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).t);
}

void
gt_pch_p_19vec_tree_int_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<tree_int,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<tree_int,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_int* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).t), cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_lambda_h[] = {
  {
    &lambda_scope_stack,
    1,
    sizeof (lambda_scope_stack),
    &gt_ggc_mx_vec_tree_int_va_gc_,
    &gt_pch_nx_vec_tree_int_va_gc_
  },
  {
    &lambda_scope,
    1,
    sizeof (lambda_scope),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &max_id,
    1,
    sizeof (max_id),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ptr_id,
    1,
    sizeof (ptr_id),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_cp_lambda_h[] = {
  { &lambda_count, 1, sizeof (lambda_count), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

