/* Type information for objc/objc-act.c.
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
gt_ggc_mx_hashed_entry (void *x_p)
{
  struct hashed_entry * const x = (struct hashed_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_16hashed_attribute ((*x).list);
      gt_ggc_m_12hashed_entry ((*x).next);
      gt_ggc_m_9tree_node ((*x).key);
    }
}

void
gt_ggc_mx_hashed_attribute (void *x_p)
{
  struct hashed_attribute * const x = (struct hashed_attribute *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_16hashed_attribute ((*x).next);
      gt_ggc_m_9tree_node ((*x).value);
    }
}

void
gt_ggc_mx_imp_entry (void *x_p)
{
  struct imp_entry * const x = (struct imp_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9imp_entry ((*x).next);
      gt_ggc_m_9tree_node ((*x).imp_context);
      gt_ggc_m_9tree_node ((*x).imp_template);
      gt_ggc_m_9tree_node ((*x).class_decl);
      gt_ggc_m_9tree_node ((*x).meta_decl);
    }
}

void
gt_ggc_mx_string_descriptor (void *x_p)
{
  struct string_descriptor * const x = (struct string_descriptor *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).literal);
      gt_ggc_m_9tree_node ((*x).constructor);
    }
}

void
gt_ggc_mx (struct string_descriptor& x_r ATTRIBUTE_UNUSED)
{
  struct string_descriptor * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).literal);
  gt_ggc_m_9tree_node ((*x).constructor);
}

void
gt_ggc_mx (struct string_descriptor *& x)
{
  if (x)
    gt_ggc_mx_string_descriptor ((void *) x);
}

void
gt_ggc_mx_hash_table_objc_string_hasher_ (void *x_p)
{
  hash_table<objc_string_hasher> * const x = (hash_table<objc_string_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct objc_string_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct objc_string_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hashed_entry (void *x_p)
{
  struct hashed_entry * const x = (struct hashed_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12hashed_entry))
    {
      gt_pch_n_16hashed_attribute ((*x).list);
      gt_pch_n_12hashed_entry ((*x).next);
      gt_pch_n_9tree_node ((*x).key);
    }
}

void
gt_pch_nx_hashed_attribute (void *x_p)
{
  struct hashed_attribute * const x = (struct hashed_attribute *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_16hashed_attribute))
    {
      gt_pch_n_16hashed_attribute ((*x).next);
      gt_pch_n_9tree_node ((*x).value);
    }
}

void
gt_pch_nx_imp_entry (void *x_p)
{
  struct imp_entry * const x = (struct imp_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9imp_entry))
    {
      gt_pch_n_9imp_entry ((*x).next);
      gt_pch_n_9tree_node ((*x).imp_context);
      gt_pch_n_9tree_node ((*x).imp_template);
      gt_pch_n_9tree_node ((*x).class_decl);
      gt_pch_n_9tree_node ((*x).meta_decl);
    }
}

void
gt_pch_nx_string_descriptor (void *x_p)
{
  struct string_descriptor * const x = (struct string_descriptor *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17string_descriptor))
    {
      gt_pch_n_9tree_node ((*x).literal);
      gt_pch_n_9tree_node ((*x).constructor);
    }
}

void
gt_pch_nx (struct string_descriptor& x_r ATTRIBUTE_UNUSED)
{
  struct string_descriptor * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).literal);
  gt_pch_n_9tree_node ((*x).constructor);
}

void
gt_pch_nx (struct string_descriptor *& x)
{
  if (x)
    gt_pch_nx_string_descriptor ((void *) x);
}

void
gt_pch_nx_hash_table_objc_string_hasher_ (void *x_p)
{
  hash_table<objc_string_hasher> * const x = (hash_table<objc_string_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_30hash_table_objc_string_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct objc_string_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct objc_string_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_12hashed_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hashed_entry * x ATTRIBUTE_UNUSED = (struct hashed_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).list), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).key), cookie);
}

void
gt_pch_p_16hashed_attribute (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hashed_attribute * x ATTRIBUTE_UNUSED = (struct hashed_attribute *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).value), cookie);
}

void
gt_pch_p_9imp_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct imp_entry * x ATTRIBUTE_UNUSED = (struct imp_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).imp_context), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).imp_template), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).class_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).meta_decl), cookie);
}

void
gt_pch_p_17string_descriptor (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct string_descriptor * x ATTRIBUTE_UNUSED = (struct string_descriptor *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).literal), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).constructor), cookie);
}

void
gt_pch_nx (struct string_descriptor* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).literal), cookie);
    op (&((*x).constructor), cookie);
}

void
gt_pch_p_30hash_table_objc_string_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<objc_string_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<objc_string_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct objc_string_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_objc_objc_act_h[] = {
  {
    &objc_parmlist,
    1,
    sizeof (objc_parmlist),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &interface_map,
    1,
    sizeof (interface_map),
    &gt_ggc_mx_objc_map_private,
    &gt_pch_nx_objc_map_private
  },
  {
    &string_htab,
    1,
    sizeof (string_htab),
    &gt_ggc_mx_hash_table_objc_string_hasher_,
    &gt_pch_nx_hash_table_objc_string_hasher_
  },
  {
    &alias_name_map,
    1,
    sizeof (alias_name_map),
    &gt_ggc_mx_objc_map_private,
    &gt_pch_nx_objc_map_private
  },
  {
    &class_name_map,
    1,
    sizeof (class_name_map),
    &gt_ggc_mx_objc_map_private,
    &gt_pch_nx_objc_map_private
  },
  {
    &class_method_map,
    1,
    sizeof (class_method_map),
    &gt_ggc_mx_objc_map_private,
    &gt_pch_nx_objc_map_private
  },
  {
    &instance_method_map,
    1,
    sizeof (instance_method_map),
    &gt_ggc_mx_objc_map_private,
    &gt_pch_nx_objc_map_private
  },
  {
    &objc_global_trees[0],
    1 * (OCTI_MAX),
    sizeof (objc_global_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &imp_list,
    1,
    sizeof (imp_list),
    &gt_ggc_mx_imp_entry,
    &gt_pch_nx_imp_entry
  },
  {
    &local_variables_to_volatilize,
    1,
    sizeof (local_variables_to_volatilize),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_objc_objc_act_h[] = {
  { &string_layout_checked, 1, sizeof (string_layout_checked), NULL, NULL },
  { &objc_ivar_visibility, 1, sizeof (objc_ivar_visibility), NULL, NULL },
  { &cat_count, 1, sizeof (cat_count), NULL, NULL },
  { &imp_count, 1, sizeof (imp_count), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

