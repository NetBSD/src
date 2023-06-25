/* Type information for cp/decl2.c.
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
gt_ggc_mx_hash_table_mangled_decl_hash_ (void *x_p)
{
  hash_table<mangled_decl_hash> * const x = (hash_table<mangled_decl_hash> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct mangled_decl_hash& x_r ATTRIBUTE_UNUSED)
{
  struct mangled_decl_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_mangled_decl_hash_ (void *x_p)
{
  hash_table<mangled_decl_hash> * const x = (hash_table<mangled_decl_hash> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_29hash_table_mangled_decl_hash_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct mangled_decl_hash& x_r ATTRIBUTE_UNUSED)
{
  struct mangled_decl_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_29hash_table_mangled_decl_hash_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<mangled_decl_hash> * x ATTRIBUTE_UNUSED = (struct hash_table<mangled_decl_hash> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct mangled_decl_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_decl2_h[] = {
  {
    &ssdf_decls,
    1,
    sizeof (ssdf_decls),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &ssdf_decl,
    1,
    sizeof (ssdf_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &priority_decl,
    1,
    sizeof (priority_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &initialize_p_decl,
    1,
    sizeof (initialize_p_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &mangled_decls,
    1,
    sizeof (mangled_decls),
    &gt_ggc_mx_hash_table_mangled_decl_hash_,
    &gt_pch_nx_hash_table_mangled_decl_hash_
  },
  {
    &mangling_aliases,
    1,
    sizeof (mangling_aliases),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &no_linkage_decls,
    1,
    sizeof (no_linkage_decls),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &deferred_fns,
    1,
    sizeof (deferred_fns),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &pending_statics,
    1,
    sizeof (pending_statics),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

