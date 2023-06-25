/* Type information for ipa-sra.c.
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
gt_ggc_mx_param_access (void *x_p)
{
  struct param_access * const x = (struct param_access *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).type);
      gt_ggc_m_9tree_node ((*x).alias_ptr_type);
    }
}

void
gt_ggc_mx_vec_param_access__va_gc_ (void *x_p)
{
  vec<param_access*,va_gc> * const x = (vec<param_access*,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct param_access *& x)
{
  if (x)
    gt_ggc_mx_param_access ((void *) x);
}

void
gt_ggc_mx_isra_func_summary (void *x_p)
{
  struct isra_func_summary * const x = (struct isra_func_summary *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_26vec_isra_param_desc_va_gc_ ((*x).m_parameters);
    }
}

void
gt_ggc_mx (struct isra_func_summary& x_r ATTRIBUTE_UNUSED)
{
  struct isra_func_summary * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_26vec_isra_param_desc_va_gc_ ((*x).m_parameters);
}

void
gt_ggc_mx (struct isra_func_summary *& x)
{
  if (x)
    gt_ggc_mx_isra_func_summary ((void *) x);
}

void
gt_ggc_mx_vec_isra_param_desc_va_gc_ (void *x_p)
{
  vec<isra_param_desc,va_gc> * const x = (vec<isra_param_desc,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct isra_param_desc& x_r ATTRIBUTE_UNUSED)
{
  struct isra_param_desc * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_24vec_param_access__va_gc_ ((*x).accesses);
}

void
gt_ggc_mx_ipa_sra_function_summaries (void *x_p)
{
  ipa_sra_function_summaries * const x = (ipa_sra_function_summaries *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_pch_nx_param_access (void *x_p)
{
  struct param_access * const x = (struct param_access *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_12param_access))
    {
      gt_pch_n_9tree_node ((*x).type);
      gt_pch_n_9tree_node ((*x).alias_ptr_type);
    }
}

void
gt_pch_nx_vec_param_access__va_gc_ (void *x_p)
{
  vec<param_access*,va_gc> * const x = (vec<param_access*,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_24vec_param_access__va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct param_access *& x)
{
  if (x)
    gt_pch_nx_param_access ((void *) x);
}

void
gt_pch_nx_isra_func_summary (void *x_p)
{
  struct isra_func_summary * const x = (struct isra_func_summary *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17isra_func_summary))
    {
      gt_pch_n_26vec_isra_param_desc_va_gc_ ((*x).m_parameters);
    }
}

void
gt_pch_nx (struct isra_func_summary& x_r ATTRIBUTE_UNUSED)
{
  struct isra_func_summary * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_26vec_isra_param_desc_va_gc_ ((*x).m_parameters);
}

void
gt_pch_nx (struct isra_func_summary *& x)
{
  if (x)
    gt_pch_nx_isra_func_summary ((void *) x);
}

void
gt_pch_nx_vec_isra_param_desc_va_gc_ (void *x_p)
{
  vec<isra_param_desc,va_gc> * const x = (vec<isra_param_desc,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_isra_param_desc_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct isra_param_desc& x_r ATTRIBUTE_UNUSED)
{
  struct isra_param_desc * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_24vec_param_access__va_gc_ ((*x).accesses);
}

void
gt_pch_nx_ipa_sra_function_summaries (void *x_p)
{
  ipa_sra_function_summaries * const x = (ipa_sra_function_summaries *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26ipa_sra_function_summaries))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_p_12param_access (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct param_access * x ATTRIBUTE_UNUSED = (struct param_access *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).alias_ptr_type), cookie);
}

void
gt_pch_p_24vec_param_access__va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<param_access*,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<param_access*,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_p_17isra_func_summary (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct isra_func_summary * x ATTRIBUTE_UNUSED = (struct isra_func_summary *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).m_parameters), cookie);
}

void
gt_pch_nx (struct isra_func_summary* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).m_parameters), cookie);
}

void
gt_pch_p_26vec_isra_param_desc_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<isra_param_desc,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<isra_param_desc,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct isra_param_desc* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).accesses), cookie);
}

void
gt_pch_p_26ipa_sra_function_summaries (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct ipa_sra_function_summaries * x ATTRIBUTE_UNUSED = (struct ipa_sra_function_summaries *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_ipa_sra_h[] = {
  {
    &func_sums,
    1,
    sizeof (func_sums),
    &gt_ggc_mx_ipa_sra_function_summaries,
    &gt_pch_nx_ipa_sra_function_summaries
  },
  LAST_GGC_ROOT_TAB
};

