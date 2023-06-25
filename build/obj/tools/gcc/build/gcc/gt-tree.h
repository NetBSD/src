/* Type information for tree.c.
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
gt_ggc_mx_type_hash (void *x_p)
{
  struct type_hash * const x = (struct type_hash *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).type);
    }
}

void
gt_ggc_mx (struct type_hash& x_r ATTRIBUTE_UNUSED)
{
  struct type_hash * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type);
}

void
gt_ggc_mx (struct type_hash *& x)
{
  if (x)
    gt_ggc_mx_type_hash ((void *) x);
}

void
gt_ggc_mx_hash_table_type_cache_hasher_ (void *x_p)
{
  hash_table<type_cache_hasher> * const x = (hash_table<type_cache_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct type_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct type_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_int_cst_hasher_ (void *x_p)
{
  hash_table<int_cst_hasher> * const x = (hash_table<int_cst_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct int_cst_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct int_cst_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_poly_int_cst_hasher_ (void *x_p)
{
  hash_table<poly_int_cst_hasher> * const x = (hash_table<poly_int_cst_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct poly_int_cst_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct poly_int_cst_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_cl_option_hasher_ (void *x_p)
{
  hash_table<cl_option_hasher> * const x = (hash_table<cl_option_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cl_option_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cl_option_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_tree_decl_map_cache_hasher_ (void *x_p)
{
  hash_table<tree_decl_map_cache_hasher> * const x = (hash_table<tree_decl_map_cache_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_decl_map_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_decl_map_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_tree_vec_map_cache_hasher_ (void *x_p)
{
  hash_table<tree_vec_map_cache_hasher> * const x = (hash_table<tree_vec_map_cache_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_vec_map_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_vec_map_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_type_hash (void *x_p)
{
  struct type_hash * const x = (struct type_hash *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9type_hash))
    {
      gt_pch_n_9tree_node ((*x).type);
    }
}

void
gt_pch_nx (struct type_hash& x_r ATTRIBUTE_UNUSED)
{
  struct type_hash * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type);
}

void
gt_pch_nx (struct type_hash *& x)
{
  if (x)
    gt_pch_nx_type_hash ((void *) x);
}

void
gt_pch_nx_hash_table_type_cache_hasher_ (void *x_p)
{
  hash_table<type_cache_hasher> * const x = (hash_table<type_cache_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_29hash_table_type_cache_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct type_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct type_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_int_cst_hasher_ (void *x_p)
{
  hash_table<int_cst_hasher> * const x = (hash_table<int_cst_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26hash_table_int_cst_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct int_cst_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct int_cst_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_poly_int_cst_hasher_ (void *x_p)
{
  hash_table<poly_int_cst_hasher> * const x = (hash_table<poly_int_cst_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_31hash_table_poly_int_cst_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct poly_int_cst_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct poly_int_cst_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_cl_option_hasher_ (void *x_p)
{
  hash_table<cl_option_hasher> * const x = (hash_table<cl_option_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28hash_table_cl_option_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cl_option_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct cl_option_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_tree_decl_map_cache_hasher_ (void *x_p)
{
  hash_table<tree_decl_map_cache_hasher> * const x = (hash_table<tree_decl_map_cache_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_38hash_table_tree_decl_map_cache_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_decl_map_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_decl_map_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_tree_vec_map_cache_hasher_ (void *x_p)
{
  hash_table<tree_vec_map_cache_hasher> * const x = (hash_table<tree_vec_map_cache_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_37hash_table_tree_vec_map_cache_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_vec_map_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_vec_map_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_9type_hash (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct type_hash * x ATTRIBUTE_UNUSED = (struct type_hash *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).type), cookie);
}

void
gt_pch_nx (struct type_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type), cookie);
}

void
gt_pch_p_29hash_table_type_cache_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<type_cache_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<type_cache_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct type_cache_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_26hash_table_int_cst_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<int_cst_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<int_cst_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct int_cst_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_31hash_table_poly_int_cst_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<poly_int_cst_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<poly_int_cst_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct poly_int_cst_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_28hash_table_cl_option_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<cl_option_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<cl_option_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cl_option_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_38hash_table_tree_decl_map_cache_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<tree_decl_map_cache_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<tree_decl_map_cache_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_decl_map_cache_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_37hash_table_tree_vec_map_cache_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<tree_vec_map_cache_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<tree_vec_map_cache_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_vec_map_cache_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_tree_h[] = {
  {
    &gcc_eh_personality_decl,
    1,
    sizeof (gcc_eh_personality_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &nonstandard_boolean_type_cache[0],
    1 * (MAX_BOOL_CACHED_PREC + 1),
    sizeof (nonstandard_boolean_type_cache[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &nonstandard_integer_type_cache[0],
    1 * (2 * MAX_INT_CACHED_PREC + 2),
    sizeof (nonstandard_integer_type_cache[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &debug_args_for_decl,
    1,
    sizeof (debug_args_for_decl),
    &gt_ggc_mx_hash_table_tree_vec_map_cache_hasher_,
    &gt_pch_nx_hash_table_tree_vec_map_cache_hasher_
  },
  {
    &value_expr_for_decl,
    1,
    sizeof (value_expr_for_decl),
    &gt_ggc_mx_hash_table_tree_decl_map_cache_hasher_,
    &gt_pch_nx_hash_table_tree_decl_map_cache_hasher_
  },
  {
    &debug_expr_for_decl,
    1,
    sizeof (debug_expr_for_decl),
    &gt_ggc_mx_hash_table_tree_decl_map_cache_hasher_,
    &gt_pch_nx_hash_table_tree_decl_map_cache_hasher_
  },
  {
    &cl_option_hash_table,
    1,
    sizeof (cl_option_hash_table),
    &gt_ggc_mx_hash_table_cl_option_hasher_,
    &gt_pch_nx_hash_table_cl_option_hasher_
  },
  {
    &cl_target_option_node,
    1,
    sizeof (cl_target_option_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &cl_optimization_node,
    1,
    sizeof (cl_optimization_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &poly_int_cst_hash_table,
    1,
    sizeof (poly_int_cst_hash_table),
    &gt_ggc_mx_hash_table_poly_int_cst_hasher_,
    &gt_pch_nx_hash_table_poly_int_cst_hasher_
  },
  {
    &int_cst_hash_table,
    1,
    sizeof (int_cst_hash_table),
    &gt_ggc_mx_hash_table_int_cst_hasher_,
    &gt_pch_nx_hash_table_int_cst_hasher_
  },
  {
    &int_cst_node,
    1,
    sizeof (int_cst_node),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &type_hash_table,
    1,
    sizeof (type_hash_table),
    &gt_ggc_mx_hash_table_type_cache_hasher_,
    &gt_pch_nx_hash_table_type_cache_hasher_
  },
  LAST_GGC_ROOT_TAB
};

void
gt_clear_caches_gt_tree_h ()
{
  gt_cleare_cache (debug_args_for_decl);
  gt_cleare_cache (value_expr_for_decl);
  gt_cleare_cache (debug_expr_for_decl);
  gt_cleare_cache (cl_option_hash_table);
  gt_cleare_cache (poly_int_cst_hash_table);
  gt_cleare_cache (int_cst_hash_table);
  gt_cleare_cache (type_hash_table);
}

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_tree_h[] = {
  { &anon_cnt, 1, sizeof (anon_cnt), NULL, NULL },
  { &next_debug_decl_uid, 1, sizeof (next_debug_decl_uid), NULL, NULL },
  { &next_type_uid, 1, sizeof (next_type_uid), NULL, NULL },
  { &next_decl_uid, 1, sizeof (next_decl_uid), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

