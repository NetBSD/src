/* Type information for cp/parser.c.
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
gt_ggc_mx_cp_token_cache (void *x_p)
{
  struct cp_token_cache * const x = (struct cp_token_cache *)x_p;
  if (ggc_test_and_set_mark (x))
    {
    }
}

void
gt_ggc_mx_vec_deferred_access_check_va_gc_ (void *x_p)
{
  vec<deferred_access_check,va_gc> * const x = (vec<deferred_access_check,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct deferred_access_check& x_r ATTRIBUTE_UNUSED)
{
  struct deferred_access_check * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).binfo);
  gt_ggc_m_9tree_node ((*x).decl);
  gt_ggc_m_9tree_node ((*x).diag_decl);
}

void
gt_ggc_mx_tree_check (void *x_p)
{
  struct tree_check * const x = (struct tree_check *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).value);
      gt_ggc_m_32vec_deferred_access_check_va_gc_ ((*x).checks);
      gt_ggc_m_9tree_node ((*x).qualifying_scope);
    }
}

void
gt_ggc_mx_vec_cp_token_va_gc_ (void *x_p)
{
  vec<cp_token,va_gc> * const x = (vec<cp_token,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cp_token& x_r ATTRIBUTE_UNUSED)
{
  struct cp_token * ATTRIBUTE_UNUSED x = &x_r;
  switch ((int) (((*x)).tree_check_p))
    {
    case true:
      gt_ggc_m_10tree_check ((*x).u.tree_check_value);
      break;
    case false:
      gt_ggc_m_9tree_node ((*x).u.value);
      break;
    default:
      break;
    }
}

void
gt_ggc_mx_cp_lexer (void *x_p)
{
  struct cp_lexer * const x = (struct cp_lexer *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_19vec_cp_token_va_gc_ ((*x).buffer);
      gt_ggc_m_8cp_lexer ((*x).next);
    }
}

void
gt_ggc_mx_vec_cp_default_arg_entry_va_gc_ (void *x_p)
{
  vec<cp_default_arg_entry,va_gc> * const x = (vec<cp_default_arg_entry,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cp_default_arg_entry& x_r ATTRIBUTE_UNUSED)
{
  struct cp_default_arg_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).class_type);
  gt_ggc_m_9tree_node ((*x).decl);
}

void
gt_ggc_mx_cp_parser_context (void *x_p)
{
  struct cp_parser_context * const x = (struct cp_parser_context *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).object_type);
      gt_ggc_m_17cp_parser_context ((*x).next);
    }
}

void
gt_ggc_mx_vec_cp_unparsed_functions_entry_va_gc_ (void *x_p)
{
  vec<cp_unparsed_functions_entry,va_gc> * const x = (vec<cp_unparsed_functions_entry,va_gc> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct cp_unparsed_functions_entry& x_r ATTRIBUTE_UNUSED)
{
  struct cp_unparsed_functions_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_31vec_cp_default_arg_entry_va_gc_ ((*x).funs_with_default_args);
  gt_ggc_m_15vec_tree_va_gc_ ((*x).funs_with_definitions);
  gt_ggc_m_15vec_tree_va_gc_ ((*x).nsdmis);
  gt_ggc_m_15vec_tree_va_gc_ ((*x).noexcepts);
}

void
gt_ggc_mx_cp_parser (void *x_p)
{
  struct cp_parser * const x = (struct cp_parser *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_8cp_lexer ((*x).lexer);
      gt_ggc_m_9tree_node ((*x).scope);
      gt_ggc_m_9tree_node ((*x).object_scope);
      gt_ggc_m_9tree_node ((*x).qualifying_scope);
      gt_ggc_m_17cp_parser_context ((*x).context);
      gt_ggc_m_S ((*x).type_definition_forbidden_message);
      gt_ggc_m_S ((*x).type_definition_forbidden_message_arg);
      gt_ggc_m_38vec_cp_unparsed_functions_entry_va_gc_ ((*x).unparsed_queues);
      gt_ggc_m_9tree_node ((*x).implicit_template_parms);
      gt_ggc_m_16cp_binding_level ((*x).implicit_template_scope);
    }
}

void
gt_pch_nx_cp_token_cache (void *x_p)
{
  struct cp_token_cache * const x = (struct cp_token_cache *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14cp_token_cache))
    {
    }
}

void
gt_pch_nx_vec_deferred_access_check_va_gc_ (void *x_p)
{
  vec<deferred_access_check,va_gc> * const x = (vec<deferred_access_check,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_32vec_deferred_access_check_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct deferred_access_check& x_r ATTRIBUTE_UNUSED)
{
  struct deferred_access_check * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).binfo);
  gt_pch_n_9tree_node ((*x).decl);
  gt_pch_n_9tree_node ((*x).diag_decl);
}

void
gt_pch_nx_tree_check (void *x_p)
{
  struct tree_check * const x = (struct tree_check *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_10tree_check))
    {
      gt_pch_n_9tree_node ((*x).value);
      gt_pch_n_32vec_deferred_access_check_va_gc_ ((*x).checks);
      gt_pch_n_9tree_node ((*x).qualifying_scope);
    }
}

void
gt_pch_nx_vec_cp_token_va_gc_ (void *x_p)
{
  vec<cp_token,va_gc> * const x = (vec<cp_token,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_19vec_cp_token_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cp_token& x_r ATTRIBUTE_UNUSED)
{
  struct cp_token * ATTRIBUTE_UNUSED x = &x_r;
  switch ((int) (((*x)).tree_check_p))
    {
    case true:
      gt_pch_n_10tree_check ((*x).u.tree_check_value);
      break;
    case false:
      gt_pch_n_9tree_node ((*x).u.value);
      break;
    default:
      break;
    }
}

void
gt_pch_nx_cp_lexer (void *x_p)
{
  struct cp_lexer * const x = (struct cp_lexer *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_8cp_lexer))
    {
      gt_pch_n_19vec_cp_token_va_gc_ ((*x).buffer);
      gt_pch_n_8cp_lexer ((*x).next);
    }
}

void
gt_pch_nx_vec_cp_default_arg_entry_va_gc_ (void *x_p)
{
  vec<cp_default_arg_entry,va_gc> * const x = (vec<cp_default_arg_entry,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_31vec_cp_default_arg_entry_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cp_default_arg_entry& x_r ATTRIBUTE_UNUSED)
{
  struct cp_default_arg_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).class_type);
  gt_pch_n_9tree_node ((*x).decl);
}

void
gt_pch_nx_cp_parser_context (void *x_p)
{
  struct cp_parser_context * const x = (struct cp_parser_context *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_17cp_parser_context))
    {
      gt_pch_n_9tree_node ((*x).object_type);
      gt_pch_n_17cp_parser_context ((*x).next);
    }
}

void
gt_pch_nx_vec_cp_unparsed_functions_entry_va_gc_ (void *x_p)
{
  vec<cp_unparsed_functions_entry,va_gc> * const x = (vec<cp_unparsed_functions_entry,va_gc> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_38vec_cp_unparsed_functions_entry_va_gc_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct cp_unparsed_functions_entry& x_r ATTRIBUTE_UNUSED)
{
  struct cp_unparsed_functions_entry * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_31vec_cp_default_arg_entry_va_gc_ ((*x).funs_with_default_args);
  gt_pch_n_15vec_tree_va_gc_ ((*x).funs_with_definitions);
  gt_pch_n_15vec_tree_va_gc_ ((*x).nsdmis);
  gt_pch_n_15vec_tree_va_gc_ ((*x).noexcepts);
}

void
gt_pch_nx_cp_parser (void *x_p)
{
  struct cp_parser * const x = (struct cp_parser *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_9cp_parser))
    {
      gt_pch_n_8cp_lexer ((*x).lexer);
      gt_pch_n_9tree_node ((*x).scope);
      gt_pch_n_9tree_node ((*x).object_scope);
      gt_pch_n_9tree_node ((*x).qualifying_scope);
      gt_pch_n_17cp_parser_context ((*x).context);
      gt_pch_n_S ((*x).type_definition_forbidden_message);
      gt_pch_n_S ((*x).type_definition_forbidden_message_arg);
      gt_pch_n_38vec_cp_unparsed_functions_entry_va_gc_ ((*x).unparsed_queues);
      gt_pch_n_9tree_node ((*x).implicit_template_parms);
      gt_pch_n_16cp_binding_level ((*x).implicit_template_scope);
    }
}

void
gt_pch_p_14cp_token_cache (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cp_token_cache * x ATTRIBUTE_UNUSED = (struct cp_token_cache *)x_p;
}

void
gt_pch_p_32vec_deferred_access_check_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<deferred_access_check,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<deferred_access_check,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct deferred_access_check* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).binfo), cookie);
    op (&((*x).decl), cookie);
    op (&((*x).diag_decl), cookie);
}

void
gt_pch_p_10tree_check (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct tree_check * x ATTRIBUTE_UNUSED = (struct tree_check *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).value), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).checks), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).qualifying_scope), cookie);
}

void
gt_pch_p_19vec_cp_token_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<cp_token,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<cp_token,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cp_token* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  switch ((int) (((*x)).tree_check_p))
    {
    case true:
        op (&((*x).u.tree_check_value), cookie);
      break;
    case false:
        op (&((*x).u.value), cookie);
      break;
    default:
      break;
    }
}

void
gt_pch_p_8cp_lexer (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cp_lexer * x ATTRIBUTE_UNUSED = (struct cp_lexer *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).buffer), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_31vec_cp_default_arg_entry_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<cp_default_arg_entry,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<cp_default_arg_entry,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cp_default_arg_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).class_type), cookie);
    op (&((*x).decl), cookie);
}

void
gt_pch_p_17cp_parser_context (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cp_parser_context * x ATTRIBUTE_UNUSED = (struct cp_parser_context *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).object_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).next), cookie);
}

void
gt_pch_p_38vec_cp_unparsed_functions_entry_va_gc_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct vec<cp_unparsed_functions_entry,va_gc> * x ATTRIBUTE_UNUSED = (struct vec<cp_unparsed_functions_entry,va_gc> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct cp_unparsed_functions_entry* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).funs_with_default_args), cookie);
    op (&((*x).funs_with_definitions), cookie);
    op (&((*x).nsdmis), cookie);
    op (&((*x).noexcepts), cookie);
}

void
gt_pch_p_9cp_parser (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct cp_parser * x ATTRIBUTE_UNUSED = (struct cp_parser *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).lexer), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).scope), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).object_scope), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).qualifying_scope), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).context), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).type_definition_forbidden_message), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).type_definition_forbidden_message_arg), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).unparsed_queues), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).implicit_template_parms), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).implicit_template_scope), cookie);
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_parser_h[] = {
  {
    &the_parser,
    1,
    sizeof (the_parser),
    &gt_ggc_mx_cp_parser,
    &gt_pch_nx_cp_parser
  },
  {
    &cp_parser_decl_specs_attrs,
    1,
    sizeof (cp_parser_decl_specs_attrs),
    &gt_ggc_mx_vec_tree_va_gc_,
    &gt_pch_nx_vec_tree_va_gc_
  },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_ggc_rd_gt_cp_parser_h[] = {
  { &cp_parser_context_free_list, 1, sizeof (cp_parser_context_free_list), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

EXPORTED_CONST struct ggc_root_tab gt_pch_rs_gt_cp_parser_h[] = {
  { &generic_parm_count, 1, sizeof (generic_parm_count), NULL, NULL },
  LAST_GGC_ROOT_TAB
};

