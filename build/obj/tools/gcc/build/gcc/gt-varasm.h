/* Type information for varasm.c.
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
gt_ggc_mx_constant_descriptor_rtx (void *x_p)
{
  struct constant_descriptor_rtx * x = (struct constant_descriptor_rtx *)x_p;
  struct constant_descriptor_rtx * xlimit = x;
  while (ggc_test_and_set_mark (xlimit))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_ggc_m_23constant_descriptor_rtx ((*x).next);
      gt_ggc_m_7rtx_def ((*x).mem);
      gt_ggc_m_7rtx_def ((*x).sym);
      gt_ggc_m_7rtx_def ((*x).constant);
      x = ((*x).next);
    }
}

void
gt_ggc_mx (struct constant_descriptor_rtx& x_r ATTRIBUTE_UNUSED)
{
  struct constant_descriptor_rtx * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_23constant_descriptor_rtx ((*x).next);
  gt_ggc_m_7rtx_def ((*x).mem);
  gt_ggc_m_7rtx_def ((*x).sym);
  gt_ggc_m_7rtx_def ((*x).constant);
}

void
gt_ggc_mx (struct constant_descriptor_rtx *& x)
{
  if (x)
    gt_ggc_mx_constant_descriptor_rtx ((void *) x);
}

void
gt_ggc_mx_rtx_constant_pool (void *x_p)
{
  struct rtx_constant_pool * const x = (struct rtx_constant_pool *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_23constant_descriptor_rtx ((*x).first);
      gt_ggc_m_23constant_descriptor_rtx ((*x).last);
      gt_ggc_m_33hash_table_const_rtx_desc_hasher_ ((*x).const_rtx_htab);
    }
}

void
gt_ggc_mx_hash_table_section_hasher_ (void *x_p)
{
  hash_table<section_hasher> * const x = (hash_table<section_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct section_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct section_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_object_block_hasher_ (void *x_p)
{
  hash_table<object_block_hasher> * const x = (hash_table<object_block_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct object_block_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct object_block_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_tree_descriptor_hasher_ (void *x_p)
{
  hash_table<tree_descriptor_hasher> * const x = (hash_table<tree_descriptor_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tree_descriptor_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_descriptor_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_const_rtx_desc_hasher_ (void *x_p)
{
  hash_table<const_rtx_desc_hasher> * const x = (hash_table<const_rtx_desc_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct const_rtx_desc_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct const_rtx_desc_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_ggc_mx_hash_table_tm_clone_hasher_ (void *x_p)
{
  hash_table<tm_clone_hasher> * const x = (hash_table<tm_clone_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct tm_clone_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tm_clone_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_constant_descriptor_rtx (void *x_p)
{
  struct constant_descriptor_rtx * x = (struct constant_descriptor_rtx *)x_p;
  struct constant_descriptor_rtx * xlimit = x;
  while (gt_pch_note_object (xlimit, xlimit, gt_pch_p_23constant_descriptor_rtx))
   xlimit = ((*xlimit).next);
  while (x != xlimit)
    {
      gt_pch_n_23constant_descriptor_rtx ((*x).next);
      gt_pch_n_7rtx_def ((*x).mem);
      gt_pch_n_7rtx_def ((*x).sym);
      gt_pch_n_7rtx_def ((*x).constant);
      x = ((*x).next);
    }
}

void
gt_pch_nx (struct constant_descriptor_rtx& x_r ATTRIBUTE_UNUSED)
{
  struct constant_descriptor_rtx * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_23constant_descriptor_rtx ((*x).next);
  gt_pch_n_7rtx_def ((*x).mem);
  gt_pch_n_7rtx_def ((*x).sym);
  gt_pch_n_7rtx_def ((*x).constant);
}

void
gt_pch_nx (struct constant_descriptor_rtx *& x)
{
  if (x)
    gt_pch_nx_constant_descriptor_rtx ((void *) x);
}

void
gt_pch_nx_rtx_constant_pool (void *x_p)
{
  struct rtx_constant_pool * const x = (struct rtx_constant_pool *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17rtx_constant_pool))
    {
      gt_pch_n_23constant_descriptor_rtx ((*x).first);
      gt_pch_n_23constant_descriptor_rtx ((*x).last);
      gt_pch_n_33hash_table_const_rtx_desc_hasher_ ((*x).const_rtx_htab);
    }
}

void
gt_pch_nx_hash_table_section_hasher_ (void *x_p)
{
  hash_table<section_hasher> * const x = (hash_table<section_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26hash_table_section_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct section_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct section_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_object_block_hasher_ (void *x_p)
{
  hash_table<object_block_hasher> * const x = (hash_table<object_block_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_31hash_table_object_block_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct object_block_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct object_block_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_tree_descriptor_hasher_ (void *x_p)
{
  hash_table<tree_descriptor_hasher> * const x = (hash_table<tree_descriptor_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_34hash_table_tree_descriptor_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tree_descriptor_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tree_descriptor_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_const_rtx_desc_hasher_ (void *x_p)
{
  hash_table<const_rtx_desc_hasher> * const x = (hash_table<const_rtx_desc_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_33hash_table_const_rtx_desc_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct const_rtx_desc_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct const_rtx_desc_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_hash_table_tm_clone_hasher_ (void *x_p)
{
  hash_table<tm_clone_hasher> * const x = (hash_table<tm_clone_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27hash_table_tm_clone_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct tm_clone_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct tm_clone_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_23constant_descriptor_rtx (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct constant_descriptor_rtx * x ATTRIBUTE_UNUSED = (struct constant_descriptor_rtx *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).mem), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).sym), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).constant), cookie);
}

void
gt_pch_nx (struct constant_descriptor_rtx* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).next), cookie);
    op (&((*x).mem), cookie);
    op (&((*x).sym), cookie);
    op (&((*x).constant), cookie);
}

void
gt_pch_p_17rtx_constant_pool (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct rtx_constant_pool * x ATTRIBUTE_UNUSED = (struct rtx_constant_pool *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).first), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).last), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).const_rtx_htab), cookie);
}

void
gt_pch_p_26hash_table_section_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<section_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<section_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct section_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_31hash_table_object_block_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<object_block_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<object_block_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct object_block_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_34hash_table_tree_descriptor_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<tree_descriptor_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<tree_descriptor_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tree_descriptor_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_33hash_table_const_rtx_desc_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<const_rtx_desc_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<const_rtx_desc_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct const_rtx_desc_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

void
gt_pch_p_27hash_table_tm_clone_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<tm_clone_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<tm_clone_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct tm_clone_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_varasm_h[] = {
  {
    &elf_fini_array_section,
    1,
    sizeof (elf_fini_array_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &elf_init_array_section,
    1,
    sizeof (elf_init_array_section),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &tm_clone_hash,
    1,
    sizeof (tm_clone_hash),
    &gt_ggc_mx_hash_table_tm_clone_hasher_,
    &gt_pch_nx_hash_table_tm_clone_hasher_
  },
  {
    &weakref_targets,
    1,
    sizeof (weakref_targets),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &const_desc_htab,
    1,
    sizeof (const_desc_htab),
    &gt_ggc_mx_hash_table_tree_descriptor_hasher_,
    &gt_pch_nx_hash_table_tree_descriptor_hasher_
  },
  {
    &initial_trampoline,
    1,
    sizeof (initial_trampoline),
    &gt_ggc_mx_rtx_def,
    &gt_pch_nx_rtx_def
  },
  {
    &weak_decls,
    1,
    sizeof (weak_decls),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &pending_assemble_externals,
    1,
    sizeof (pending_assemble_externals),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &shared_constant_pool,
    1,
    sizeof (shared_constant_pool),
    &gt_ggc_mx_rtx_constant_pool,
    &gt_pch_nx_rtx_constant_pool
  },
  {
    &object_block_htab,
    1,
    sizeof (object_block_htab),
    &gt_ggc_mx_hash_table_object_block_hasher_,
    &gt_pch_nx_hash_table_object_block_hasher_
  },
  {
    &section_htab,
    1,
    sizeof (section_htab),
    &gt_ggc_mx_hash_table_section_hasher_,
    &gt_pch_nx_hash_table_section_hasher_
  },
  {
    &unnamed_sections,
    1,
    sizeof (unnamed_sections),
    &gt_ggc_mx_section,
    &gt_pch_nx_section
  },
  {
    &weak_global_object_name,
    1,
    sizeof (weak_global_object_name),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  {
    &first_global_object_name,
    1,
    sizeof (first_global_object_name),
    (gt_pointer_walker) &gt_ggc_m_S,
    (gt_pointer_walker) &gt_pch_n_S
  },
  LAST_GGC_ROOT_TAB
};

void
gt_clear_caches_gt_varasm_h ()
{
  gt_cleare_cache (tm_clone_hash);
}

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_varasm_h[] = {
  { &anchor_labelno, 1, sizeof (anchor_labelno), NULL, NULL },
  { &const_labelno, 1, sizeof (const_labelno), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

