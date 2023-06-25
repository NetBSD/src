/* Type information for tree-ssa-address.c.
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
gt_ggc_mx_vec_mem_addr_template_va_gc_ (void *x_p)
{
  vec<mem_addr_template,va_gc> * const x = (vec<mem_addr_template,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct mem_addr_template& x_r ATTRIBUTE_UNUSED)
{
  struct mem_addr_template * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_7rtx_def ((*x).ref);
}

void
gt_pch_nx_vec_mem_addr_template_va_gc_ (void *x_p)
{
  vec<mem_addr_template,va_gc> * const x = (vec<mem_addr_template,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_28vec_mem_addr_template_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct mem_addr_template& x_r ATTRIBUTE_UNUSED)
{
  struct mem_addr_template * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_7rtx_def ((*x).ref);
}

void
gt_pch_p_28vec_mem_addr_template_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<mem_addr_template,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<mem_addr_template,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct mem_addr_template* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).ref), cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_tree_ssa_address_h[] = {
  {
    &mem_addr_template_list,
    1,
    sizeof (mem_addr_template_list),
    &gt_ggc_mx_vec_mem_addr_template_va_gc_,
    &gt_pch_nx_vec_mem_addr_template_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

