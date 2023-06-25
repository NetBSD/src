/* Type information for cp/except.c.
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
gt_ggc_mx_vec_pending_noexcept_va_gc_ (void *x_p)
{
  vec<pending_noexcept,va_gc> * const x = (vec<pending_noexcept,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct pending_noexcept& x_r ATTRIBUTE_UNUSED)
{
  struct pending_noexcept * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).fn);
}

void
gt_pch_nx_vec_pending_noexcept_va_gc_ (void *x_p)
{
  vec<pending_noexcept,va_gc> * const x = (vec<pending_noexcept,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27vec_pending_noexcept_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct pending_noexcept& x_r ATTRIBUTE_UNUSED)
{
  struct pending_noexcept * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).fn);
}

void
gt_pch_p_27vec_pending_noexcept_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<pending_noexcept,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<pending_noexcept,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct pending_noexcept* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).fn), cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_except_h[] = {
  {
    &pending_noexcept_checks,
    1,
    sizeof (pending_noexcept_checks),
    &gt_ggc_mx_vec_pending_noexcept_va_gc_,
    &gt_pch_nx_vec_pending_noexcept_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

