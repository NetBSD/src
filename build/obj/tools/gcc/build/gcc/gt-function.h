/* Type information for function.c.
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
gt_ggc_mx_temp_slot (void *x_p)
{
  struct temp_slot * const x = (struct temp_slot *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9temp_slot ((*x).next);
      gt_ggc_m_9temp_slot ((*x).prev);
      gt_ggc_m_7rtx_def ((*x).slot);
      gt_ggc_m_9tree_node ((*x).type);
    }
}

void
gt_ggc_mx_initial_value_struct (void *x_p)
{
  struct initial_value_struct * const x = (struct initial_value_struct *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      {
        size_t l0 = (size_t)(((*x)).num_entries);
        if ((*x).entries != NULL) {
          size_t i0;
          for (i0 = 0; i0 != (size_t)(l0); i0++) {
            gt_ggc_m_7rtx_def ((*x).entries[i0].hard_reg);
            gt_ggc_m_7rtx_def ((*x).entries[i0].pseudo);
          }
          ggc_mark ((*x).entries);
        }
      }
    }
}

void
gt_ggc_mx_hash_table_insn_cache_hasher_ (void *x_p)
{
  hash_table<insn_cache_hasher> * const x = (hash_table<insn_cache_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct insn_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct insn_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_temp_slot_address_entry (void *x_p)
{
  struct temp_slot_address_entry * const x = (struct temp_slot_address_entry *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_7rtx_def ((*x).address);
      gt_ggc_m_9temp_slot ((*x).temp_slot);
    }
}

void
gt_ggc_mx (struct temp_slot_address_entry& x_r ATTRIBUTE_UNUSED)
{
  struct temp_slot_address_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_7rtx_def ((*x).address);
  gt_ggc_m_9temp_slot ((*x).temp_slot);
}

void
gt_ggc_mx (struct temp_slot_address_entry *& x)
{
  if (x)
    gt_ggc_mx_temp_slot_address_entry ((void *) x);
}

void
gt_ggc_mx_hash_table_temp_address_hasher_ (void *x_p)
{
  hash_table<temp_address_hasher> * const x = (hash_table<temp_address_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct temp_address_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct temp_address_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_temp_slot (void *x_p)
{
  struct temp_slot * const x = (struct temp_slot *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9temp_slot))
    {
      gt_pch_n_9temp_slot ((*x).next);
      gt_pch_n_9temp_slot ((*x).prev);
      gt_pch_n_7rtx_def ((*x).slot);
      gt_pch_n_9tree_node ((*x).type);
    }
}

void
gt_pch_nx_initial_value_struct (void *x_p)
{
  struct initial_value_struct * const x = (struct initial_value_struct *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_20initial_value_struct))
    {
      {
        size_t l0 = (size_t)(((*x)).num_entries);
        if ((*x).entries != NULL) {
          size_t i0;
          for (i0 = 0; i0 != (size_t)(l0); i0++) {
            gt_pch_n_7rtx_def ((*x).entries[i0].hard_reg);
            gt_pch_n_7rtx_def ((*x).entries[i0].pseudo);
          }
          gt_pch_note_object ((*x).entries, x, gt_pch_p_20initial_value_struct);
        }
      }
    }
}

void
gt_pch_nx_hash_table_insn_cache_hasher_ (void *x_p)
{
  hash_table<insn_cache_hasher> * const x = (hash_table<insn_cache_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_29hash_table_insn_cache_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct insn_cache_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct insn_cache_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_temp_slot_address_entry (void *x_p)
{
  struct temp_slot_address_entry * const x = (struct temp_slot_address_entry *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_23temp_slot_address_entry))
    {
      gt_pch_n_7rtx_def ((*x).address);
      gt_pch_n_9temp_slot ((*x).temp_slot);
    }
}

void
gt_pch_nx (struct temp_slot_address_entry& x_r ATTRIBUTE_UNUSED)
{
  struct temp_slot_address_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_7rtx_def ((*x).address);
  gt_pch_n_9temp_slot ((*x).temp_slot);
}

void
gt_pch_nx (struct temp_slot_address_entry *& x)
{
  if (x)
    gt_pch_nx_temp_slot_address_entry ((void *) x);
}

void
gt_pch_nx_hash_table_temp_address_hasher_ (void *x_p)
{
  hash_table<temp_address_hasher> * const x = (hash_table<temp_address_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_31hash_table_temp_address_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct temp_address_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct temp_address_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_9temp_slot (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct temp_slot * x ATTRIBUTE_UNUSED = (struct temp_slot *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).prev), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).slot), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).type), cookie);
}

void
gt_pch_p_20initial_value_struct (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct initial_value_struct * x ATTRIBUTE_UNUSED = (struct initial_value_struct *)x_p;
  {
    size_t l0 = (size_t)(((*x)).num_entries);
    if ((*x).entries != NULL) {
      size_t i0;
      for (i0 = 0; i0 != (size_t)(l0) && ((void *)(*x).entries == this_obj); i0++) {
        if ((void *)((*x).entries) == this_obj)
          op (&((*x).entries[i0].hard_reg), cookie);
        if ((void *)((*x).entries) == this_obj)
          op (&((*x).entries[i0].pseudo), cookie);
      }
      if ((void *)(x) == this_obj)
        op (&((*x).entries), cookie);
    }
  }
}

void
gt_pch_p_29hash_table_insn_cache_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<insn_cache_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<insn_cache_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct insn_cache_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_23temp_slot_address_entry (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct temp_slot_address_entry * x ATTRIBUTE_UNUSED = (struct temp_slot_address_entry *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).address), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).temp_slot), cookie);
}

void
gt_pch_nx (struct temp_slot_address_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).address), cookie);
    op (&((*x).temp_slot), cookie);
}

void
gt_pch_p_31hash_table_temp_address_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<temp_address_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<temp_address_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct temp_address_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_function_h[] = {
  {
    &temp_slot_address_table,
    1,
    sizeof (temp_slot_address_table),
    &gt_ggc_mx_hash_table_temp_address_hasher_,
    &gt_pch_nx_hash_table_temp_address_hasher_
  },
  {
    &epilogue_insn_hash,
    1,
    sizeof (epilogue_insn_hash),
    &gt_ggc_mx_hash_table_insn_cache_hasher_,
    &gt_pch_nx_hash_table_insn_cache_hasher_
  },
  {
    &prologue_insn_hash,
    1,
    sizeof (prologue_insn_hash),
    &gt_ggc_mx_hash_table_insn_cache_hasher_,
    &gt_pch_nx_hash_table_insn_cache_hasher_
  },
  LAST_GGC_ROOT_TAB
};

void
gt_clear_caches_gt_function_h ()
{
  gt_cleare_cache (epilogue_insn_hash);
  gt_cleare_cache (prologue_insn_hash);
}

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_function_h[] = {
  { &next_block_index, 1, sizeof (next_block_index), NULL, NULL },
  { &funcdef_no, 1, sizeof (funcdef_no), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

