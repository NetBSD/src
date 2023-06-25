/* Type information for objc/objc-runtime-shared-support.c.
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

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_objc_objc_runtime_shared_support_h[] = {
  {
    &objc_rt_trees[0],
    1 * (OCTI_RT_META_MAX),
    sizeof (objc_rt_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_objc_objc_runtime_shared_support_h[] = {
  { &property_name_attr_idx, 1, sizeof (property_name_attr_idx), NULL, NULL },
  { &meth_var_types_idx, 1, sizeof (meth_var_types_idx), NULL, NULL },
  { &meth_var_names_idx, 1, sizeof (meth_var_names_idx), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

