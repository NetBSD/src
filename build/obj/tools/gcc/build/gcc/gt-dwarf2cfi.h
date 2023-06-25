/* Type information for dwarf2cfi.c.
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
gt_ggc_mx_dw_cfa_location (void *x_p)
{
  struct dw_cfa_location * const x = (struct dw_cfa_location *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_dw_cfi_row (void *x_p)
{
  struct dw_cfi_row * const x = (struct dw_cfi_row *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_11dw_cfi_node ((*x).cfa_cfi);
      gt_ggc_m_21vec_dw_cfi_ref_va_gc_ ((*x).reg_save);
    }
}

void
gt_ggc_mx_reg_saved_in_data (void *x_p)
{
  struct reg_saved_in_data * const x = (struct reg_saved_in_data *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).orig_reg);
      gt_ggc_m_7rtx_def ((*x).saved_in_reg);
    }
}

void
gt_pch_nx_dw_cfa_location (void *x_p)
{
  struct dw_cfa_location * const x = (struct dw_cfa_location *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_15dw_cfa_location))
    {
    }
}

void
gt_pch_nx_dw_cfi_row (void *x_p)
{
  struct dw_cfi_row * const x = (struct dw_cfi_row *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10dw_cfi_row))
    {
      gt_pch_n_11dw_cfi_node ((*x).cfa_cfi);
      gt_pch_n_21vec_dw_cfi_ref_va_gc_ ((*x).reg_save);
    }
}

void
gt_pch_nx_reg_saved_in_data (void *x_p)
{
  struct reg_saved_in_data * const x = (struct reg_saved_in_data *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17reg_saved_in_data))
    {
      gt_pch_n_7rtx_def ((*x).orig_reg);
      gt_pch_n_7rtx_def ((*x).saved_in_reg);
    }
}

void
gt_pch_p_15dw_cfa_location (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct dw_cfa_location * x ATTRIBUTE_UNUSED = (struct dw_cfa_location *)x_p;
}

void
gt_pch_p_10dw_cfi_row (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct dw_cfi_row * x ATTRIBUTE_UNUSED = (struct dw_cfi_row *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).cfa_cfi), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).reg_save), cookie);
}

void
gt_pch_p_17reg_saved_in_data (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct reg_saved_in_data * x ATTRIBUTE_UNUSED = (struct reg_saved_in_data *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).orig_reg), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).saved_in_reg), cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_dwarf2cfi_h[] = {
  {
    &cie_return_save,
    1,
    sizeof (cie_return_save),
    &gt_ggc_mx_reg_saved_in_data,
    &gt_pch_nx_reg_saved_in_data
  },
  {
    &cie_cfi_row,
    1,
    sizeof (cie_cfi_row),
    &gt_ggc_mx_dw_cfi_row,
    &gt_pch_nx_dw_cfi_row
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_dwarf2cfi_h[] = {
  { &saved_do_cfi_asm, 1, sizeof (saved_do_cfi_asm), NULL, NULL },
  { &dwarf2out_cfi_label_num, 1, sizeof (dwarf2out_cfi_label_num), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

