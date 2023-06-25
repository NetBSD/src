/* Type information for ipa-devirt.c.
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
gt_ggc_mx_odr_type_d (void *x_p)
{
  struct odr_type_d * const x = (struct odr_type_d *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).type);
      gt_ggc_m_15vec_tree_va_gc_ ((*x).types);
    }
}

void
gt_ggc_mx_vec_odr_type_va_gc_ (void *x_p)
{
  vec<odr_type,va_gc> * const x = (vec<odr_type,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct odr_type_d *& x)
{
  if (x)
    gt_ggc_mx_odr_type_d ((void *) x);
}

void
gt_pch_nx_odr_type_d (void *x_p)
{
  struct odr_type_d * const x = (struct odr_type_d *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10odr_type_d))
    {
      gt_pch_n_9tree_node ((*x).type);
      gt_pch_n_15vec_tree_va_gc_ ((*x).types);
    }
}

void
gt_pch_nx_vec_odr_type_va_gc_ (void *x_p)
{
  vec<odr_type,va_gc> * const x = (vec<odr_type,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_19vec_odr_type_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct odr_type_d *& x)
{
  if (x)
    gt_pch_nx_odr_type_d ((void *) x);
}

void
gt_pch_p_10odr_type_d (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct odr_type_d * x ATTRIBUTE_UNUSED = (struct odr_type_d *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).types), cookie);
}

void
gt_pch_p_19vec_odr_type_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<odr_type,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<odr_type,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_ipa_devirt_h[] = {
  {
    &odr_enums,
    1,
    sizeof (odr_enums),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &odr_types_ptr,
    1,
    sizeof (odr_types_ptr),
    &gt_ggc_mx_vec_odr_type_va_gc_,
    &gt_pch_nx_vec_odr_type_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

