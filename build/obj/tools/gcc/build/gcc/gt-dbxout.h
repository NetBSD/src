/* Type information for dbxout.c.
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
gt_ggc_mx_typeinfo (void *x_p)
{
  struct typeinfo * const x = (struct typeinfo *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_pch_nx_typeinfo (void *x_p)
{
  struct typeinfo * const x = (struct typeinfo *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8typeinfo))
    {
    }
}

void
gt_pch_p_8typeinfo (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct typeinfo * x ATTRIBUTE_UNUSED = (struct typeinfo *)x_p;
}

/* GC roots.  */

static void gt_ggc_ma_typevec (void *);
static void
gt_ggc_ma_typevec (ATTRIBUTE_UNUSED void *x_p)
{
  if (typevec != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(typevec_len); i0++) {
    }
    ggc_mark (typevec);
  }
}

static void gt_pch_pa_typevec
    (void *, void *, gt_pointer_operator, void *);
static void gt_pch_pa_typevec (ATTRIBUTE_UNUSED void *this_obj,
      ATTRIBUTE_UNUSED void *x_p,
      ATTRIBUTE_UNUSED gt_pointer_operator op,
      ATTRIBUTE_UNUSED void * cookie)
{
  if (typevec != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(typevec_len) && ((void *)typevec == this_obj); i0++) {
    }
    if ((void *)(&typevec) == this_obj)
      op (&(typevec), cookie);
  }
}

static void gt_pch_na_typevec (void *);
static void
gt_pch_na_typevec (ATTRIBUTE_UNUSED void *x_p)
{
  if (typevec != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(typevec_len); i0++) {
    }
    gt_pch_note_object (typevec, &typevec, gt_pch_pa_typevec);
  }
}

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_dbxout_h[] = {
  {
    &lastfile,
    1,
    sizeof (lastfile),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &preinit_symbols,
    1,
    sizeof (preinit_symbols),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &typevec,
    1,
    sizeof (typevec),
    &gt_ggc_ma_typevec,
    &gt_pch_na_typevec
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_dbxout_h[] = {
  { &lastfile_is_base, 1, sizeof (lastfile_is_base), NULL, NULL },
  { &lastlineno, 1, sizeof (lastlineno), NULL, NULL },
  { &source_label_number, 1, sizeof (source_label_number), NULL, NULL },
  { &dbxout_source_line_counter, 1, sizeof (dbxout_source_line_counter), NULL, NULL },
  { &scope_labelno, 1, sizeof (scope_labelno), NULL, NULL },
  { &next_file_number, 1, sizeof (next_file_number), NULL, NULL },
  { &next_type_number, 1, sizeof (next_type_number), NULL, NULL },
  { &typevec_len, 1, sizeof (typevec_len), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

