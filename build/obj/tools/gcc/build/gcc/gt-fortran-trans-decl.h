/* Type information for fortran/trans-decl.c.
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
gt_ggc_mx_hash_table_module_hasher_ (void *x_p)
{
  hash_table<module_hasher> * const x = (hash_table<module_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct module_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct module_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_module_hasher_ (void *x_p)
{
  hash_table<module_hasher> * const x = (hash_table<module_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_25hash_table_module_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct module_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct module_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_25hash_table_module_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<module_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<module_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct module_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_fortran_trans_decl_h[] = {
  {
    &module_htab,
    1,
    sizeof (module_htab),
    &gt_ggc_mx_hash_table_module_hasher_,
    &gt_pch_nx_hash_table_module_hasher_
  },
  {
    &saved_local_decls,
    1,
    sizeof (saved_local_decls),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &saved_parent_function_decls,
    1,
    sizeof (saved_parent_function_decls),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &saved_function_decls,
    1,
    sizeof (saved_function_decls),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &parent_fake_result_decl,
    1,
    sizeof (parent_fake_result_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &current_fake_result_decl,
    1,
    sizeof (current_fake_result_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

