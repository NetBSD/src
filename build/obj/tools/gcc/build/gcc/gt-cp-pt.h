/* Type information for cp/pt.c.
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
gt_ggc_mx_tinst_level (void *x_p)
{
  struct tinst_level * x = (struct tinst_level *)x_p;
  struct tinst_level * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_11tinst_level ((*x).next);
      gt_ggc_m_9tree_node ((*x).tldcl);
      gt_ggc_m_9tree_node ((*x).targs);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_pending_template (void *x_p)
{
  struct pending_template * x = (struct pending_template *)x_p;
  struct pending_template * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_16pending_template ((*x).next);
      gt_ggc_m_11tinst_level ((*x).tinst);
      x = ((*x).next);
    }
}

void
gt_ggc_mx_spec_entry (void *x_p)
{
  struct spec_entry * const x = (struct spec_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).tmpl);
      gt_ggc_m_9tree_node ((*x).args);
      gt_ggc_m_9tree_node ((*x).spec);
    }
}

void
gt_ggc_mx (struct spec_entry& x_r ATTRIBUTE_UNUSED)
{
  struct spec_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).tmpl);
  gt_ggc_m_9tree_node ((*x).args);
  gt_ggc_m_9tree_node ((*x).spec);
}

void
gt_ggc_mx (struct spec_entry *& x)
{
  if (x)
    gt_ggc_mx_spec_entry ((void *) x);
}

void
gt_ggc_mx_hash_table_spec_hasher_ (void *x_p)
{
  hash_table<spec_hasher> * const x = (hash_table<spec_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct spec_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct spec_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_map_tree_tree_pair_p_ (void *x_p)
{
  hash_map<tree,tree_pair_p> * const x = (hash_map<tree,tree_pair_p> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_pair_s *& x)
{
  if (x)
    gt_ggc_mx_tree_pair_s ((void *) x);
}

void
gt_pch_nx_tinst_level (void *x_p)
{
  struct tinst_level * x = (struct tinst_level *)x_p;
  struct tinst_level * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_11tinst_level))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_11tinst_level ((*x).next);
      gt_pch_n_9tree_node ((*x).tldcl);
      gt_pch_n_9tree_node ((*x).targs);
      x = ((*x).next);
    }
}

void
gt_pch_nx_pending_template (void *x_p)
{
  struct pending_template * x = (struct pending_template *)x_p;
  struct pending_template * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_16pending_template))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_16pending_template ((*x).next);
      gt_pch_n_11tinst_level ((*x).tinst);
      x = ((*x).next);
    }
}

void
gt_pch_nx_spec_entry (void *x_p)
{
  struct spec_entry * const x = (struct spec_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10spec_entry))
    {
      gt_pch_n_9tree_node ((*x).tmpl);
      gt_pch_n_9tree_node ((*x).args);
      gt_pch_n_9tree_node ((*x).spec);
    }
}

void
gt_pch_nx (struct spec_entry& x_r ATTRIBUTE_UNUSED)
{
  struct spec_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).tmpl);
  gt_pch_n_9tree_node ((*x).args);
  gt_pch_n_9tree_node ((*x).spec);
}

void
gt_pch_nx (struct spec_entry *& x)
{
  if (x)
    gt_pch_nx_spec_entry ((void *) x);
}

void
gt_pch_nx_hash_table_spec_hasher_ (void *x_p)
{
  hash_table<spec_hasher> * const x = (hash_table<spec_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_23hash_table_spec_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct spec_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct spec_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_map_tree_tree_pair_p_ (void *x_p)
{
  hash_map<tree,tree_pair_p> * const x = (hash_map<tree,tree_pair_p> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26hash_map_tree_tree_pair_p_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_pair_s *& x)
{
  if (x)
    gt_pch_nx_tree_pair_s ((void *) x);
}

void
gt_pch_p_11tinst_level (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tinst_level * x ATTRIBUTE_UNUSED = (struct tinst_level *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).tldcl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).targs), cookie);
}

void
gt_pch_p_16pending_template (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct pending_template * x ATTRIBUTE_UNUSED = (struct pending_template *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).tinst), cookie);
}

void
gt_pch_p_10spec_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct spec_entry * x ATTRIBUTE_UNUSED = (struct spec_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).tmpl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).args), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).spec), cookie);
}

void
gt_pch_nx (struct spec_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).tmpl), cookie);
    op (&((*x).args), cookie);
    op (&((*x).spec), cookie);
}

void
gt_pch_p_23hash_table_spec_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<spec_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<spec_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct spec_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_26hash_map_tree_tree_pair_p_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<tree,tree_pair_p> * x ATTRIBUTE_UNUSED = (struct hash_map<tree,tree_pair_p> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_pt_h[] = {
  {
    &explicit_specifier_map,
    1,
    sizeof (explicit_specifier_map),
    &gt_ggc_mx_hash_map_tree_tree_decl_tree_cache_traits_,
    &gt_pch_nx_hash_map_tree_tree_decl_tree_cache_traits_
  },
  {
    &defarg_inst,
    1,
    sizeof (defarg_inst),
    &gt_ggc_mx_hash_map_tree_tree_decl_tree_cache_traits_,
    &gt_pch_nx_hash_map_tree_tree_decl_tree_cache_traits_
  },
  {
    &last_error_tinst_level,
    1,
    sizeof (last_error_tinst_level),
    &gt_ggc_mx_tinst_level,
    &gt_pch_nx_tinst_level
  },
  {
    &tparm_obj_values,
    1,
    sizeof (tparm_obj_values),
    &gt_ggc_mx_hash_map_tree_tree_,
    &gt_pch_nx_hash_map_tree_tree_
  },
  {
    &canonical_template_parms,
    1,
    sizeof (canonical_template_parms),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &type_specializations,
    1,
    sizeof (type_specializations),
    &gt_ggc_mx_hash_table_spec_hasher_,
    &gt_pch_nx_hash_table_spec_hasher_
  },
  {
    &decl_specializations,
    1,
    sizeof (decl_specializations),
    &gt_ggc_mx_hash_table_spec_hasher_,
    &gt_pch_nx_hash_table_spec_hasher_
  },
  {
    &saved_access_scope,
    1,
    sizeof (saved_access_scope),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &current_tinst_level,
    1,
    sizeof (current_tinst_level),
    &gt_ggc_mx_tinst_level,
    &gt_pch_nx_tinst_level
  },
  {
    &last_pending_template,
    1,
    sizeof (last_pending_template),
    &gt_ggc_mx_pending_template,
    &gt_pch_nx_pending_template
  },
  {
    &pending_templates,
    1,
    sizeof (pending_templates),
    &gt_ggc_mx_pending_template,
    &gt_pch_nx_pending_template
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_cp_pt_h[] = {
  { &dguide_cache, 1, sizeof (dguide_cache), NULL, NULL },
  { &pending_template_freelist_head, 1, sizeof (pending_template_freelist_head), NULL, NULL },
  { &tinst_level_freelist_head, 1, sizeof (tinst_level_freelist_head), NULL, NULL },
  { &tree_list_freelist_head, 1, sizeof (tree_list_freelist_head), NULL, NULL },
  { &defaulted_ttp_cache, 1, sizeof (defaulted_ttp_cache), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

void
gt_clear_caches_gt_cp_pt_h ()
{
  gt_cleare_cache (explicit_specifier_map);
  gt_cleare_cache (defarg_inst);
}

