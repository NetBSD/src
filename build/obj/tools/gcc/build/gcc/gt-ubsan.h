/* Type information for ubsan.c.
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
gt_ggc_mx_tree_type_map (void *x_p)
{
  struct tree_type_map * const x = (struct tree_type_map *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).type.from);
      gt_ggc_m_9tree_node ((*x).decl);
    }
}

void
gt_ggc_mx (struct tree_type_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_type_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).type.from);
  gt_ggc_m_9tree_node ((*x).decl);
}

void
gt_ggc_mx (struct tree_type_map *& x)
{
  if (x)
    gt_ggc_mx_tree_type_map ((void *) x);
}

void
gt_ggc_mx_hash_table_tree_type_map_cache_hasher_ (void *x_p)
{
  hash_table<tree_type_map_cache_hasher> * const x = (hash_table<tree_type_map_cache_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_type_map_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_type_map_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_tree_type_map (void *x_p)
{
  struct tree_type_map * const x = (struct tree_type_map *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_13tree_type_map))
    {
      gt_pch_n_9tree_node ((*x).type.from);
      gt_pch_n_9tree_node ((*x).decl);
    }
}

void
gt_pch_nx (struct tree_type_map& x_r ATTRIBUTE_UNUSED)
{
  struct tree_type_map * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).type.from);
  gt_pch_n_9tree_node ((*x).decl);
}

void
gt_pch_nx (struct tree_type_map *& x)
{
  if (x)
    gt_pch_nx_tree_type_map ((void *) x);
}

void
gt_pch_nx_hash_table_tree_type_map_cache_hasher_ (void *x_p)
{
  hash_table<tree_type_map_cache_hasher> * const x = (hash_table<tree_type_map_cache_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_38hash_table_tree_type_map_cache_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_type_map_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_type_map_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_13tree_type_map (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_type_map * x ATTRIBUTE_UNUSED = (struct tree_type_map *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).type.from), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).decl), cookie);
}

void
gt_pch_nx (struct tree_type_map* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).type.from), cookie);
    op (&((*x).decl), cookie);
}

void
gt_pch_p_38hash_table_tree_type_map_cache_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<tree_type_map_cache_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<tree_type_map_cache_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_type_map_cache_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_ubsan_h[] = {
  {
    &ubsan_vptr_type_cache_decl,
    1,
    sizeof (ubsan_vptr_type_cache_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ubsan_source_location_type,
    1,
    sizeof (ubsan_source_location_type),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ubsan_type_descriptor_type,
    1,
    sizeof (ubsan_type_descriptor_type),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &decl_tree_for_type,
    1,
    sizeof (decl_tree_for_type),
    &gt_ggc_mx_hash_table_tree_type_map_cache_hasher_,
    &gt_pch_nx_hash_table_tree_type_map_cache_hasher_
  },
  LAST_GGC_ROOT_TAB
};

void
gt_clear_caches_gt_ubsan_h ()
{
  gt_cleare_cache (decl_tree_for_type);
}

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_ubsan_h[] = {
  { &ubsan_ids, 1, sizeof (ubsan_ids), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

