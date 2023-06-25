/* Type information for fortran/trans-io.c.
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

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_fortran_trans_io_h[] = {
  {
    &dt_parm,
    1,
    sizeof (dt_parm),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &iocall[0],
    1 * (IOCALL_NUM),
    sizeof (iocall[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &st_parameter_field[0].name,
    1 * ARRAY_SIZE (st_parameter_field),
    sizeof (st_parameter_field[0]),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &st_parameter_field[0].field,
    1 * ARRAY_SIZE (st_parameter_field),
    sizeof (st_parameter_field[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &st_parameter_field[0].field_len,
    1 * ARRAY_SIZE (st_parameter_field),
    sizeof (st_parameter_field[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &st_parameter[0].name,
    1 * ARRAY_SIZE (st_parameter),
    sizeof (st_parameter[0]),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &st_parameter[0].type,
    1 * ARRAY_SIZE (st_parameter),
    sizeof (st_parameter[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_fortran_trans_io_h[] = {
  { &st_parameter_field, 1, sizeof (st_parameter_field), NULL, NULL },
  { &st_parameter, 1, sizeof (st_parameter), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

