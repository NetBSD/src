/* Type information for c-family/c-cppbuiltin.c.
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

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_c_family_c_cppbuiltin_h[] = {
  {
    &lazy_hex_fp_values[0].hex_str,
    1 * (LAZY_HEX_FP_VALUES_CNT),
    sizeof (lazy_hex_fp_values[0]),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &lazy_hex_fp_values[0].fp_suffix,
    1 * (LAZY_HEX_FP_VALUES_CNT),
    sizeof (lazy_hex_fp_values[0]),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_c_family_c_cppbuiltin_h[] = {
  { &lazy_hex_fp_value_count, 1, sizeof (lazy_hex_fp_value_count), NULL, NULL },
  { &lazy_hex_fp_values, 1, sizeof (lazy_hex_fp_values), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

