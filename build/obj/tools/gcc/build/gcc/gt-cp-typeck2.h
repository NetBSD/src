/* Type information for cp/typeck2.c.
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
gt_ggc_mx_pending_abstract_type (void *x_p)
{
  struct pending_abstract_type * x = (struct pending_abstract_type *)x_p;
  struct pending_abstract_type * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_9tree_node ((*x).decl);
      gt_ggc_m_9tree_node ((*x).type);
      gt_ggc_m_21pending_abstract_type ((*x).next);
      x = ((*x).next);
    }
}

void
gt_ggc_mx (struct pending_abstract_type& x_r ATTRIBUTE_UNUSED)
{
  struct pending_abstract_type * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).decl);
  gt_ggc_m_9tree_node ((*x).type);
  gt_ggc_m_21pending_abstract_type ((*x).next);
}

void
gt_ggc_mx (struct pending_abstract_type *& x)
{
  if (x)
    gt_ggc_mx_pending_abstract_type ((void *) x);
}

void
gt_ggc_mx_hash_table_abstract_type_hasher_ (void *x_p)
{
  hash_table<abstract_type_hasher> * const x = (hash_table<abstract_type_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct abstract_type_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct abstract_type_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_pending_abstract_type (void *x_p)
{
  struct pending_abstract_type * x = (struct pending_abstract_type *)x_p;
  struct pending_abstract_type * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_21pending_abstract_type))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_9tree_node ((*x).decl);
      gt_pch_n_9tree_node ((*x).type);
      gt_pch_n_21pending_abstract_type ((*x).next);
      x = ((*x).next);
    }
}

void
gt_pch_nx (struct pending_abstract_type& x_r ATTRIBUTE_UNUSED)
{
  struct pending_abstract_type * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).decl);
  gt_pch_n_9tree_node ((*x).type);
  gt_pch_n_21pending_abstract_type ((*x).next);
}

void
gt_pch_nx (struct pending_abstract_type *& x)
{
  if (x)
    gt_pch_nx_pending_abstract_type ((void *) x);
}

void
gt_pch_nx_hash_table_abstract_type_hasher_ (void *x_p)
{
  hash_table<abstract_type_hasher> * const x = (hash_table<abstract_type_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_32hash_table_abstract_type_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct abstract_type_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct abstract_type_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_21pending_abstract_type (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct pending_abstract_type * x ATTRIBUTE_UNUSED = (struct pending_abstract_type *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_nx (struct pending_abstract_type* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).decl), cookie);
    op (&((*x).type), cookie);
    op (&((*x).next), cookie);
}

void
gt_pch_p_32hash_table_abstract_type_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<abstract_type_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<abstract_type_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct abstract_type_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_typeck2_h[] = {
  {
    &abstract_pending_vars,
    1,
    sizeof (abstract_pending_vars),
    &gt_ggc_mx_hash_table_abstract_type_hasher_,
    &gt_pch_nx_hash_table_abstract_type_hasher_
  },
  LAST_GGC_ROOT_TAB
};

