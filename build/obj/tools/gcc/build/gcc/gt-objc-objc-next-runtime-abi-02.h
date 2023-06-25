/* Type information for objc/objc-next-runtime-abi-02.c.
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
gt_ggc_mx_vec_ident_data_tuple_va_gc_ (void *x_p)
{
  vec<ident_data_tuple,va_gc> * const x = (vec<ident_data_tuple,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ident_data_tuple& x_r ATTRIBUTE_UNUSED)
{
  struct ident_data_tuple * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).ident);
  gt_ggc_m_9tree_node ((*x).data);
}

void
gt_ggc_mx_vec_msgref_entry_va_gc_ (void *x_p)
{
  vec<msgref_entry,va_gc> * const x = (vec<msgref_entry,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct msgref_entry& x_r ATTRIBUTE_UNUSED)
{
  struct msgref_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).func);
  gt_ggc_m_9tree_node ((*x).selname);
  gt_ggc_m_9tree_node ((*x).refdecl);
}

void
gt_ggc_mx_vec_prot_list_entry_va_gc_ (void *x_p)
{
  vec<prot_list_entry,va_gc> * const x = (vec<prot_list_entry,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct prot_list_entry& x_r ATTRIBUTE_UNUSED)
{
  struct prot_list_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).id);
  gt_ggc_m_9tree_node ((*x).refdecl);
}

void
gt_ggc_mx_vec_ivarref_entry_va_gc_ (void *x_p)
{
  vec<ivarref_entry,va_gc> * const x = (vec<ivarref_entry,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct ivarref_entry& x_r ATTRIBUTE_UNUSED)
{
  struct ivarref_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).decl);
  gt_ggc_m_9tree_node ((*x).offset);
}

void
gt_pch_nx_vec_ident_data_tuple_va_gc_ (void *x_p)
{
  vec<ident_data_tuple,va_gc> * const x = (vec<ident_data_tuple,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_27vec_ident_data_tuple_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ident_data_tuple& x_r ATTRIBUTE_UNUSED)
{
  struct ident_data_tuple * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).ident);
  gt_pch_n_9tree_node ((*x).data);
}

void
gt_pch_nx_vec_msgref_entry_va_gc_ (void *x_p)
{
  vec<msgref_entry,va_gc> * const x = (vec<msgref_entry,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_23vec_msgref_entry_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct msgref_entry& x_r ATTRIBUTE_UNUSED)
{
  struct msgref_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).func);
  gt_pch_n_9tree_node ((*x).selname);
  gt_pch_n_9tree_node ((*x).refdecl);
}

void
gt_pch_nx_vec_prot_list_entry_va_gc_ (void *x_p)
{
  vec<prot_list_entry,va_gc> * const x = (vec<prot_list_entry,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_26vec_prot_list_entry_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct prot_list_entry& x_r ATTRIBUTE_UNUSED)
{
  struct prot_list_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).id);
  gt_pch_n_9tree_node ((*x).refdecl);
}

void
gt_pch_nx_vec_ivarref_entry_va_gc_ (void *x_p)
{
  vec<ivarref_entry,va_gc> * const x = (vec<ivarref_entry,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_24vec_ivarref_entry_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct ivarref_entry& x_r ATTRIBUTE_UNUSED)
{
  struct ivarref_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).decl);
  gt_pch_n_9tree_node ((*x).offset);
}

void
gt_pch_p_27vec_ident_data_tuple_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ident_data_tuple,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ident_data_tuple,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ident_data_tuple* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).ident), cookie);
    op (&((*x).data), cookie);
}

void
gt_pch_p_23vec_msgref_entry_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<msgref_entry,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<msgref_entry,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct msgref_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).func), cookie);
    op (&((*x).selname), cookie);
    op (&((*x).refdecl), cookie);
}

void
gt_pch_p_26vec_prot_list_entry_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<prot_list_entry,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<prot_list_entry,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct prot_list_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).id), cookie);
    op (&((*x).refdecl), cookie);
}

void
gt_pch_p_24vec_ivarref_entry_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<ivarref_entry,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<ivarref_entry,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct ivarref_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).decl), cookie);
    op (&((*x).offset), cookie);
}

/* GC roots.  */

static void gt_ggc_ma_extern_names (void *);
static void
gt_ggc_ma_extern_names (ATTRIBUTE_UNUSED void *x_p)
{
  if (extern_names != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(SIZEHASHTABLE); i0++) {
      gt_ggc_m_12hashed_entry (extern_names[i0]);
    }
    ggc_mark (extern_names);
  }
}

static void gt_pch_pa_extern_names
    (void *, void *, gt_pointer_operator, void *);
static void gt_pch_pa_extern_names (ATTRIBUTE_UNUSED void *this_obj,
      ATTRIBUTE_UNUSED void *x_p,
      ATTRIBUTE_UNUSED gt_pointer_operator op,
      ATTRIBUTE_UNUSED void * cookie)
{
  if (extern_names != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(SIZEHASHTABLE) && ((void *)extern_names == this_obj); i0++) {
      if ((void *)(extern_names) == this_obj)
        op (&(extern_names[i0]), cookie);
    }
    if ((void *)(&extern_names) == this_obj)
      op (&(extern_names), cookie);
  }
}

static void gt_pch_na_extern_names (void *);
static void
gt_pch_na_extern_names (ATTRIBUTE_UNUSED void *x_p)
{
  if (extern_names != NULL) {
    size_t i0;
    for (i0 = 0; i0 != (size_t)(SIZEHASHTABLE); i0++) {
      gt_pch_n_12hashed_entry (extern_names[i0]);
    }
    gt_pch_note_object (extern_names, &extern_names, gt_pch_pa_extern_names);
  }
}

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_objc_objc_next_runtime_abi_02_h[] = {
  {
    &objc_eh_personality_decl,
    1,
    sizeof (objc_eh_personality_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &next_v2_EHTYPE_id_decl,
    1,
    sizeof (next_v2_EHTYPE_id_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &next_v2_ehvtable_decl,
    1,
    sizeof (next_v2_ehvtable_decl),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &objc_v2_ehtype_template,
    1,
    sizeof (objc_v2_ehtype_template),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &ehtype_list,
    1,
    sizeof (ehtype_list),
    &gt_ggc_mx_vec_ident_data_tuple_va_gc_,
    &gt_pch_nx_vec_ident_data_tuple_va_gc_
  },
  {
    &ivar_offset_refs,
    1,
    sizeof (ivar_offset_refs),
    &gt_ggc_mx_vec_ivarref_entry_va_gc_,
    &gt_pch_nx_vec_ivarref_entry_va_gc_
  },
  {
    &protlist,
    1,
    sizeof (protlist),
    &gt_ggc_mx_vec_prot_list_entry_va_gc_,
    &gt_pch_nx_vec_prot_list_entry_va_gc_
  },
  {
    &nonlazy_category_list,
    1,
    sizeof (nonlazy_category_list),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &category_list,
    1,
    sizeof (category_list),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &nonlazy_class_list,
    1,
    sizeof (nonlazy_class_list),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &class_list,
    1,
    sizeof (class_list),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  {
    &metaclass_super_refs,
    1,
    sizeof (metaclass_super_refs),
    &gt_ggc_mx_vec_ident_data_tuple_va_gc_,
    &gt_pch_nx_vec_ident_data_tuple_va_gc_
  },
  {
    &class_super_refs,
    1,
    sizeof (class_super_refs),
    &gt_ggc_mx_vec_ident_data_tuple_va_gc_,
    &gt_pch_nx_vec_ident_data_tuple_va_gc_
  },
  {
    &protrefs,
    1,
    sizeof (protrefs),
    &gt_ggc_mx_vec_prot_list_entry_va_gc_,
    &gt_pch_nx_vec_prot_list_entry_va_gc_
  },
  {
    &msgrefs,
    1,
    sizeof (msgrefs),
    &gt_ggc_mx_vec_msgref_entry_va_gc_,
    &gt_pch_nx_vec_msgref_entry_va_gc_
  },
  {
    &classrefs,
    1,
    sizeof (classrefs),
    &gt_ggc_mx_vec_ident_data_tuple_va_gc_,
    &gt_pch_nx_vec_ident_data_tuple_va_gc_
  },
  {
    &extern_names,
    1,
    sizeof (extern_names),
    &gt_ggc_ma_extern_names,
    &gt_pch_na_extern_names
  },
  {
    &objc_v2_global_trees[0],
    1 * (OCTI_V2_MAX),
    sizeof (objc_v2_global_trees[0]),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  LAST_GGC_ROOT_TAB
};

