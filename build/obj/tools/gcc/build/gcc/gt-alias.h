/* Type information for alias.c.
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
gt_ggc_mx_hash_map_alias_set_hash_int_ (void *x_p)
{
  hash_map<alias_set_hash,int> * const x = (hash_map<alias_set_hash,int> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct alias_set_hash& x_r ATTRIBUTE_UNUSED)
{
  struct alias_set_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_alias_set_entry (void *x_p)
{
  struct alias_set_entry * const x = (struct alias_set_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_28hash_map_alias_set_hash_int_ ((*x).children);
    }
}

void
gt_ggc_mx_vec_alias_set_entry__va_gc_ (void *x_p)
{
  vec<alias_set_entry*,va_gc> * const x = (vec<alias_set_entry*,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct alias_set_entry *& x)
{
  if (x)
    gt_ggc_mx_alias_set_entry ((void *) x);
}

void
gt_pch_nx_hash_map_alias_set_hash_int_ (void *x_p)
{
  hash_map<alias_set_hash,int> * const x = (hash_map<alias_set_hash,int> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28hash_map_alias_set_hash_int_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct alias_set_hash& x_r ATTRIBUTE_UNUSED)
{
  struct alias_set_hash * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_alias_set_entry (void *x_p)
{
  struct alias_set_entry * const x = (struct alias_set_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15alias_set_entry))
    {
      gt_pch_n_28hash_map_alias_set_hash_int_ ((*x).children);
    }
}

void
gt_pch_nx_vec_alias_set_entry__va_gc_ (void *x_p)
{
  vec<alias_set_entry*,va_gc> * const x = (vec<alias_set_entry*,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27vec_alias_set_entry__va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct alias_set_entry *& x)
{
  if (x)
    gt_pch_nx_alias_set_entry ((void *) x);
}

void
gt_pch_p_28hash_map_alias_set_hash_int_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_map<alias_set_hash,int> * x ATTRIBUTE_UNUSED = (struct hash_map<alias_set_hash,int> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct alias_set_hash* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_15alias_set_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct alias_set_entry * x ATTRIBUTE_UNUSED = (struct alias_set_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).children), cookie);
}

void
gt_pch_p_27vec_alias_set_entry__va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<alias_set_entry*,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<alias_set_entry*,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_alias_h[] = {
  {
    &alias_sets,
    1,
    sizeof (alias_sets),
    &gt_ggc_mx_vec_alias_set_entry__va_gc_,
    &gt_pch_nx_vec_alias_set_entry__va_gc_
  },
  {
    &reg_known_value,
    1,
    sizeof (reg_known_value),
    &gt_ggc_mx_vec_rtx_va_gc_,
    &gt_pch_nx_vec_rtx_va_gc_
  },
  {
    &arg_base_value,
    1,
    sizeof (arg_base_value),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &reg_base_value,
    1,
    sizeof (reg_base_value),
    &gt_ggc_mx_vec_rtx_va_gc_,
    &gt_pch_nx_vec_rtx_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_alias_h[] = {
  { &old_reg_base_value, 1, sizeof (old_reg_base_value), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_alias_h[] = {
  { &frame_set, 1, sizeof (frame_set), NULL, NULL },
  { &varargs_set, 1, sizeof (varargs_set), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

