/* Type information for cp/coroutines.cc.
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
gt_ggc_mx_coroutine_info (void *x_p)
{
  struct coroutine_info * const x = (struct coroutine_info *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_m_9tree_node ((*x).function_decl);
      gt_ggc_m_9tree_node ((*x).actor_decl);
      gt_ggc_m_9tree_node ((*x).destroy_decl);
      gt_ggc_m_9tree_node ((*x).promise_type);
      gt_ggc_m_9tree_node ((*x).handle_type);
      gt_ggc_m_9tree_node ((*x).self_h_proxy);
      gt_ggc_m_9tree_node ((*x).promise_proxy);
      gt_ggc_m_9tree_node ((*x).return_void);
    }
}

void
gt_ggc_mx (struct coroutine_info& x_r ATTRIBUTE_UNUSED)
{
  struct coroutine_info * ATTRIBUTE_UNUSED x = &x_r;
  gt_ggc_m_9tree_node ((*x).function_decl);
  gt_ggc_m_9tree_node ((*x).actor_decl);
  gt_ggc_m_9tree_node ((*x).destroy_decl);
  gt_ggc_m_9tree_node ((*x).promise_type);
  gt_ggc_m_9tree_node ((*x).handle_type);
  gt_ggc_m_9tree_node ((*x).self_h_proxy);
  gt_ggc_m_9tree_node ((*x).promise_proxy);
  gt_ggc_m_9tree_node ((*x).return_void);
}

void
gt_ggc_mx (struct coroutine_info *& x)
{
  if (x)
    gt_ggc_mx_coroutine_info ((void *) x);
}

void
gt_ggc_mx_hash_table_coroutine_info_hasher_ (void *x_p)
{
  hash_table<coroutine_info_hasher> * const x = (hash_table<coroutine_info_hasher> *)x_p;
  if (ggc_test_and_set_mark (x))
    {
      gt_ggc_mx (x);
    }
}

void
gt_ggc_mx (struct coroutine_info_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct coroutine_info_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_nx_coroutine_info (void *x_p)
{
  struct coroutine_info * const x = (struct coroutine_info *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_14coroutine_info))
    {
      gt_pch_n_9tree_node ((*x).function_decl);
      gt_pch_n_9tree_node ((*x).actor_decl);
      gt_pch_n_9tree_node ((*x).destroy_decl);
      gt_pch_n_9tree_node ((*x).promise_type);
      gt_pch_n_9tree_node ((*x).handle_type);
      gt_pch_n_9tree_node ((*x).self_h_proxy);
      gt_pch_n_9tree_node ((*x).promise_proxy);
      gt_pch_n_9tree_node ((*x).return_void);
    }
}

void
gt_pch_nx (struct coroutine_info& x_r ATTRIBUTE_UNUSED)
{
  struct coroutine_info * ATTRIBUTE_UNUSED x = &x_r;
  gt_pch_n_9tree_node ((*x).function_decl);
  gt_pch_n_9tree_node ((*x).actor_decl);
  gt_pch_n_9tree_node ((*x).destroy_decl);
  gt_pch_n_9tree_node ((*x).promise_type);
  gt_pch_n_9tree_node ((*x).handle_type);
  gt_pch_n_9tree_node ((*x).self_h_proxy);
  gt_pch_n_9tree_node ((*x).promise_proxy);
  gt_pch_n_9tree_node ((*x).return_void);
}

void
gt_pch_nx (struct coroutine_info *& x)
{
  if (x)
    gt_pch_nx_coroutine_info ((void *) x);
}

void
gt_pch_nx_hash_table_coroutine_info_hasher_ (void *x_p)
{
  hash_table<coroutine_info_hasher> * const x = (hash_table<coroutine_info_hasher> *)x_p;
  if (gt_pch_note_object (x, x, gt_pch_p_33hash_table_coroutine_info_hasher_))
    {
      gt_pch_nx (x);
    }
}

void
gt_pch_nx (struct coroutine_info_hasher& x_r ATTRIBUTE_UNUSED)
{
  struct coroutine_info_hasher * ATTRIBUTE_UNUSED x = &x_r;
}

void
gt_pch_p_14coroutine_info (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct coroutine_info * x ATTRIBUTE_UNUSED = (struct coroutine_info *)x_p;
  if ((void *)(x) == this_obj)
    op (&((*x).function_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).actor_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).destroy_decl), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).promise_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).handle_type), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).self_h_proxy), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).promise_proxy), cookie);
  if ((void *)(x) == this_obj)
    op (&((*x).return_void), cookie);
}

void
gt_pch_nx (struct coroutine_info* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
    op (&((*x).function_decl), cookie);
    op (&((*x).actor_decl), cookie);
    op (&((*x).destroy_decl), cookie);
    op (&((*x).promise_type), cookie);
    op (&((*x).handle_type), cookie);
    op (&((*x).self_h_proxy), cookie);
    op (&((*x).promise_proxy), cookie);
    op (&((*x).return_void), cookie);
}

void
gt_pch_p_33hash_table_coroutine_info_hasher_ (ATTRIBUTE_UNUSED void *this_obj,
	void *x_p,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
  struct hash_table<coroutine_info_hasher> * x ATTRIBUTE_UNUSED = (struct hash_table<coroutine_info_hasher> *)x_p;
  if ((void *)(x) == this_obj)
    gt_pch_nx (&((*x)), op, cookie);
}

void
gt_pch_nx (struct coroutine_info_hasher* x ATTRIBUTE_UNUSED,
	ATTRIBUTE_UNUSED gt_pointer_operator op,
	ATTRIBUTE_UNUSED void *cookie)
{
}

/* GC roots.  */

EXPORTED_CONST struct ggc_root_tab gt_ggc_r_gt_cp_coroutines_h[] = {
  {
    &to_ramp,
    1,
    sizeof (to_ramp),
    &gt_ggc_mx_hash_map_tree_tree_,
    &gt_pch_nx_hash_map_tree_tree_
  },
  {
    &void_coro_handle_type,
    1,
    sizeof (void_coro_handle_type),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_handle_templ,
    1,
    sizeof (coro_handle_templ),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_traits_templ,
    1,
    sizeof (coro_traits_templ),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_await_resume_identifier,
    1,
    sizeof (coro_await_resume_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_await_suspend_identifier,
    1,
    sizeof (coro_await_suspend_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_await_ready_identifier,
    1,
    sizeof (coro_await_ready_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_unhandled_exception_identifier,
    1,
    sizeof (coro_unhandled_exception_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_gro_on_allocation_fail_identifier,
    1,
    sizeof (coro_gro_on_allocation_fail_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_get_return_object_identifier,
    1,
    sizeof (coro_get_return_object_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_from_address_identifier,
    1,
    sizeof (coro_from_address_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_address_identifier,
    1,
    sizeof (coro_address_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_resume_identifier,
    1,
    sizeof (coro_resume_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_yield_value_identifier,
    1,
    sizeof (coro_yield_value_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_return_value_identifier,
    1,
    sizeof (coro_return_value_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_return_void_identifier,
    1,
    sizeof (coro_return_void_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_final_suspend_identifier,
    1,
    sizeof (coro_final_suspend_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_initial_suspend_identifier,
    1,
    sizeof (coro_initial_suspend_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_await_transform_identifier,
    1,
    sizeof (coro_await_transform_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_promise_type_identifier,
    1,
    sizeof (coro_promise_type_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_handle_identifier,
    1,
    sizeof (coro_handle_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coro_traits_identifier,
    1,
    sizeof (coro_traits_identifier),
    &gt_ggc_mx_tree_node,
    &gt_pch_nx_tree_node
  },
  {
    &coroutine_info_table,
    1,
    sizeof (coroutine_info_table),
    &gt_ggc_mx_hash_table_coroutine_info_hasher_,
    &gt_pch_nx_hash_table_coroutine_info_hasher_
  },
  LAST_GGC_ROOT_TAB
};

